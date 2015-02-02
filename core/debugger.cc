// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/debugger.h"

extern "C" {
  #include <dbgkfuncs.h>
  #include <dbgktypes.h>
}
#include <Windows.h>
#include <unordered_map>
#include <utility>

#include <winc_types.h>

using std::make_pair;
using std::unordered_map;
using winc::Debugger;
using winc::DebuggerThread;
using winc::ResultCode;
using winc::WINC_OK;
using winc::WINC_ERROR_DEBUGGER;

namespace {

struct DebuggerSharedResource {
  DebuggerSharedResource()
    : debug_object(NULL)
    , thread_created(false) {
    ::InitializeCriticalSection(&thread_crit_sec);
    ::InitializeSRWLock(&client_lock);
  }

  ~DebuggerSharedResource() {
    if (debug_object)
      ::CloseHandle(debug_object);
    ::DeleteCriticalSection(&thread_crit_sec);
  }

  ResultCode Init() {
    HANDLE dbgobj;
    NTSTATUS status = ::NtCreateDebugObject(&dbgobj, DEBUG_OBJECT_ALL_ACCESS,
                                            NULL, DBGK_KILL_PROCESS_ON_EXIT);
    if (!NT_SUCCESS(status))
      return WINC_ERROR_DEBUGGER;
    debug_object = dbgobj;
    return WINC_OK;
  }

  ResultCode GuardDebuggerThread() {
    if (!thread_created) {
      ::EnterCriticalSection(&thread_crit_sec);
      if (!thread_created) {
        HANDLE thread = ::CreateThread(NULL, 0, DebuggerThread,
                                       this, 0, NULL);
        if (!thread) {
          ::LeaveCriticalSection(&thread_crit_sec);
          return WINC_ERROR_DEBUGGER;
        }
        ::CloseHandle(thread);
        thread_created = true;
      }
      ::LeaveCriticalSection(&thread_crit_sec);
    }
    return WINC_OK;
  }

  HANDLE debug_object;
  volatile bool thread_created;
  CRITICAL_SECTION thread_crit_sec;
  unordered_map<DWORD, Debugger *> client_map;
  SRWLOCK client_lock;
};

DebuggerSharedResource *g_shared = nullptr;

ResultCode InitSharedResource(DebuggerSharedResource **out_sr) {
  DebuggerSharedResource *sr = g_shared;
  if (sr != nullptr) {
    *out_sr = sr;
    return WINC_OK;
  }

  // Create a new shared resource, and then set the global pointer
  // by interlocked operation
  sr = new DebuggerSharedResource;
  ResultCode rc = sr->Init();
  if (rc != WINC_OK)
    return rc;
  PVOID original = ::InterlockedCompareExchangePointer(
      reinterpret_cast<PVOID *>(&g_shared), sr, nullptr);
  if (original) {
    delete sr;
    *out_sr = reinterpret_cast<DebuggerSharedResource *>(original);
  } else {
    *out_sr = sr;
  }
  return WINC_OK;
}

}

namespace winc {

DWORD WINAPI DebuggerThread(PVOID param) {
  DebuggerSharedResource *sr =
    reinterpret_cast<DebuggerSharedResource *>(param);
  DBGUI_WAIT_STATE_CHANGE wsc;

  while (true) {
    NTSTATUS status = ::NtWaitForDebugEvent(sr->debug_object, FALSE,
                                            NULL, &wsc);
    if (!NT_SUCCESS(status)) {
      if (status == STATUS_DEBUGGER_INACTIVE)
        return 0;
      return 0xDEADBEEF;
    }

    NTSTATUS cont_status = DBG_CONTINUE;
    switch (wsc.NewState) {
    case DbgCreateThreadStateChange: {
      auto &info = wsc.StateInfo.CreateThread;
      ::CloseHandle(info.HandleToThread);
      break;
    }
    case DbgCreateProcessStateChange: {
      auto &info = wsc.StateInfo.CreateProcessInfo;
      ::CloseHandle(info.HandleToProcess);
      ::CloseHandle(info.HandleToThread);
      ::CloseHandle(info.NewProcess.FileHandle);
      break;
    }
    case DbgExitProcessStateChange: {
      auto &info = wsc.StateInfo.ExitProcess;
      DWORD process_id = reinterpret_cast<DWORD>(
          wsc.AppClientId.UniqueProcess);
      ::AcquireSRWLockShared(&sr->client_lock);
      auto client_iter = sr->client_map.find(process_id);
      if (client_iter != sr->client_map.end()) {
        client_iter->second->HandleExitProcess(info.ExitStatus);
        ::ReleaseSRWLockShared(&sr->client_lock);
        ::AcquireSRWLockExclusive(&sr->client_lock);
        sr->client_map.erase(process_id);
        ::ReleaseSRWLockExclusive(&sr->client_lock);
      } else {
        ::ReleaseSRWLockShared(&sr->client_lock);
      }
      break;
    }
    case DbgExceptionStateChange:
    case DbgBreakpointStateChange:
    case DbgSingleStepStateChange: {
      auto &info = wsc.StateInfo.Exception;
      if (!info.FirstChance) {
        DWORD process_id = reinterpret_cast<DWORD>(
            wsc.AppClientId.UniqueProcess);
        ::AcquireSRWLockShared(&sr->client_lock);
        auto client_iter = sr->client_map.find(process_id);
        if (client_iter != sr->client_map.end()) {
          client_iter->second->HandleException(info.ExceptionRecord);
        }
        ::ReleaseSRWLockShared(&sr->client_lock);
      }
      cont_status = DBG_EXCEPTION_NOT_HANDLED;
      break;
    }
    case DbgLoadDllStateChange: {
      auto &info = wsc.StateInfo.LoadDll;
      ::CloseHandle(info.FileHandle);
      break;
    }
    }
    ::NtDebugContinue(sr->debug_object, &wsc.AppClientId, cont_status);
  }
}

ResultCode Debugger::Init(HANDLE process, DWORD process_id) {
  DebuggerSharedResource *sr;
  ResultCode rc = InitSharedResource(&sr);
  if (rc != WINC_OK)
    return rc;
  rc = sr->GuardDebuggerThread();
  if (rc != WINC_OK)
    return rc;
  // Add self to the client map
  ::AcquireSRWLockExclusive(&sr->client_lock);
  sr->client_map.insert(make_pair(process_id, this));
  ::ReleaseSRWLockExclusive(&sr->client_lock);
  NTSTATUS status = ::NtDebugActiveProcess(process, sr->debug_object);
  if (!NT_SUCCESS(status)) {
    ::AcquireSRWLockExclusive(&sr->client_lock);
    sr->client_map.erase(process_id);
    ::ReleaseSRWLockExclusive(&sr->client_lock);
    return WINC_ERROR_DEBUGGER;
  }
  this->process_id_ = process_id;
  return WINC_OK;
}

Debugger::~Debugger() {
  // Remove self from the client map
  if (process_id_) {
    DebuggerSharedResource *sr = g_shared;
    ::AcquireSRWLockShared(&sr->client_lock);
    if (sr->client_map.find(process_id_) != sr->client_map.end()) {
      ::ReleaseSRWLockShared(&sr->client_lock);
      ::AcquireSRWLockExclusive(&sr->client_lock);
      sr->client_map.erase(process_id_);
      ::ReleaseSRWLockExclusive(&sr->client_lock);
    } else {
      ::ReleaseSRWLockShared(&sr->client_lock);
    }
  }
}

void Debugger::HandleExitProcess(NTSTATUS exit_status) {
  // TODO(iceboy): Not implemented
  printf("OnExitProcess %d\n", exit_status);
}

void Debugger::HandleException(const EXCEPTION_RECORD &exception_record) {
  // TODO(iceboy): Not implemented
  printf("OnException %X\n", exception_record.ExceptionCode);
}

}
