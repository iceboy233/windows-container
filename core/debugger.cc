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

#include <winc_types.h>

using std::unordered_map;
using winc::Debugger;
using winc::ResultCode;
using winc::WINC_OK;
using winc::WINC_ERROR_DEBUGGER;

namespace {

struct DebuggerSharedResource {
  DebuggerSharedResource()
    : debug_object(NULL)
    {}

  ~DebuggerSharedResource() {
    if (debug_object)
      ::CloseHandle(debug_object);
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

  HANDLE debug_object;
  unordered_map<HANDLE, Debugger *> client_map;
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

ResultCode Debugger::Init(HANDLE process) {
  DebuggerSharedResource *sr;
  ResultCode rc = InitSharedResource(&sr);
  if (rc != WINC_OK)
    return rc;

  // TODO(iceboy): Not implemented
  return WINC_OK;
}

}
