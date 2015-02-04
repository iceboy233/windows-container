// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/job_object.h"

#include "core/container.h"
#include "core/target.h"

namespace winc {

class JobObjectSharedResource {
public:
  JobObjectSharedResource()
    : completion_port(NULL)
    , thread_created(false) {
    ::InitializeCriticalSection(&thread_crit_sec);
  }

  ~JobObjectSharedResource() {
    if (completion_port)
      ::CloseHandle(completion_port);
    ::DeleteCriticalSection(&thread_crit_sec);
  }

  ResultCode Init() {
    HANDLE port = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                           NULL, 0, 1);
    if (!port)
      return WINC_ERROR_COMPLETION_PORT;
    completion_port = port;
    return WINC_OK;
  }

  ResultCode GuardCompletionPortThread() {
    if (!thread_created) {
      ::EnterCriticalSection(&thread_crit_sec);
      if (!thread_created) {
        HANDLE thread = ::CreateThread(NULL, 0, JobObject::MessageThread,
                                       this, 0, NULL);
        if (!thread) {
          ::LeaveCriticalSection(&thread_crit_sec);
          return WINC_ERROR_COMPLETION_PORT;
        }
        ::CloseHandle(thread);
        thread_created = true;
      }
      ::LeaveCriticalSection(&thread_crit_sec);
    }
    return WINC_OK;
  }

  HANDLE completion_port;
  volatile bool thread_created;
  CRITICAL_SECTION thread_crit_sec;
};

JobObjectSharedResource *g_shared = nullptr;

ResultCode InitJobObjectSharedResource(JobObjectSharedResource **out_sr) {
  JobObjectSharedResource *sr = g_shared;
  if (sr != nullptr) {
    *out_sr = sr;
    return WINC_OK;
  }

  // Create a new shared resource, and then set the global pointer
  // by interlocked operation
  sr = new JobObjectSharedResource;
  ResultCode rc = sr->Init();
  if (rc != WINC_OK)
    return rc;
  PVOID original = ::InterlockedCompareExchangePointer(
      reinterpret_cast<PVOID *>(&g_shared), sr, nullptr);
  if (original) {
    delete sr;
    *out_sr = reinterpret_cast<JobObjectSharedResource *>(original);
  } else {
    *out_sr = sr;
  }
  return WINC_OK;
}

JobObject::~JobObject() {
  if (job_)
    ::CloseHandle(job_);
}

ResultCode JobObject::Init() {
  HANDLE job = ::CreateJobObjectW(NULL, NULL);
  if (!job)
    return WINC_ERROR_JOB_OBJECT;
  job_ = job;
  return WINC_OK;
}

ResultCode JobObject::AssignProcess(HANDLE process) {
  if (!::AssignProcessToJobObject(job_, process))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

ResultCode JobObject::GetBasicLimit(JOBOBJECT_EXTENDED_LIMIT_INFORMATION *limit) {
  if (!::QueryInformationJobObject(job_, JobObjectExtendedLimitInformation,
      limit, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION), NULL))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

ResultCode JobObject::SetBasicLimit(
    const JOBOBJECT_EXTENDED_LIMIT_INFORMATION &limit) {
  if (!::SetInformationJobObject(job_, JobObjectExtendedLimitInformation,
      const_cast<JOBOBJECT_EXTENDED_LIMIT_INFORMATION *>(&limit),
      sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

ResultCode JobObject::GetUILimit(JOBOBJECT_BASIC_UI_RESTRICTIONS *limit) {
  if (!::QueryInformationJobObject(job_, JobObjectBasicUIRestrictions,
      limit, sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS), NULL))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}


ResultCode JobObject::SetUILimit(
    const JOBOBJECT_BASIC_UI_RESTRICTIONS &ui_limit) {
  if (!::SetInformationJobObject(job_, JobObjectBasicUIRestrictions,
      const_cast<JOBOBJECT_BASIC_UI_RESTRICTIONS *>(&ui_limit),
      sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS)))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

ResultCode JobObject::GetAccountInfo(
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION *info) {
  if (!::QueryInformationJobObject(job_, JobObjectBasicAccountingInformation,
      info, sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION), NULL))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

ResultCode JobObject::AssociateCompletionPort(Target *target) {
  JobObjectSharedResource *sr;
  ResultCode rc = InitJobObjectSharedResource(&sr);
  if (rc != WINC_OK)
    return rc;
  rc = sr->GuardCompletionPortThread();
  if (rc != WINC_OK)
    return rc;

  JOBOBJECT_ASSOCIATE_COMPLETION_PORT port;
  port.CompletionKey = target;
  port.CompletionPort = sr->completion_port;
  if (!::SetInformationJobObject(job_,
                                 JobObjectAssociateCompletionPortInformation,
                                 &port, sizeof(port)))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

DWORD WINAPI JobObject::MessageThread(PVOID param) {
  const ULONG ENTRY_PER_CALL = 16;
  JobObjectSharedResource *sr =
    reinterpret_cast<JobObjectSharedResource *>(param);
  OVERLAPPED_ENTRY entries[ENTRY_PER_CALL];
  ULONG actual_count;
  while (::GetQueuedCompletionStatusEx(sr->completion_port, entries,
                                       ENTRY_PER_CALL, &actual_count,
                                       INFINITE, FALSE)) {
    for (OVERLAPPED_ENTRY *entry = entries;
         entry != entries + actual_count; ++entry) {
      Target *target =
          reinterpret_cast<Target *>(entry->lpCompletionKey);
      DWORD message_id = entry->dwNumberOfBytesTransferred;
      DWORD process_id;
      switch (message_id) {
      case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT:
        target->OnActiveProcessLimit();
        break;
      case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
        target->OnExitAll();
        break;
      case JOB_OBJECT_MSG_NEW_PROCESS:
        process_id = reinterpret_cast<DWORD>(entry->lpOverlapped);
        target->OnNewProcess(process_id);
        break;
      case JOB_OBJECT_MSG_EXIT_PROCESS:
      case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
        process_id = reinterpret_cast<DWORD>(entry->lpOverlapped);
        target->OnExitProcess(process_id);
        break;
      case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
        process_id = reinterpret_cast<DWORD>(entry->lpOverlapped);
        target->OnMemoryLimit(process_id);
        break;
      }
    }
  }
  return 0xDEADBEEF;
}

}
