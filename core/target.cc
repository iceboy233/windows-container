// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/target.h"

#include <Windows.h>
#include <Psapi.h>
#include <memory>
#include <utility>

#include "core/job_object.h"

using std::move;
using std::unique_ptr;

namespace winc {

Target::Target()
  : listen_(false)
  , listening_(false)
  {}

Target::Target(bool listen)
  : listen_(listen)
  , listening_(false)
  {}

Target::~Target() {
  if (listening_)
    JobObject::DeassociateCompletionPort(this);
}

ResultCode Target::Assign(DWORD process_id,
                          unique_ptr<JobObject> &job_object,
                          unique_handle &process_handle,
                          unique_handle &thread_handle) {
  process_id_ = process_id;
  job_object_ = move(job_object);
  process_handle_ = move(process_handle);
  thread_handle_ = move(thread_handle);
  if (listen_) {
    ResultCode rc = job_object_->AssociateCompletionPort(this);
    if (rc != WINC_OK)
      return rc;
    listening_ = true;
  }
  return WINC_OK;
}

ResultCode Target::Start() {
  DWORD ret = ::ResumeThread(thread_handle_.get());
  if (ret == static_cast<DWORD>(-1)) {
    return WINC_ERROR_TARGET;
  }
  thread_handle_.reset();
  return WINC_OK;
}

ResultCode Target::WaitForProcess() {
  if (::WaitForSingleObject(process_handle_.get(), INFINITE) == WAIT_FAILED)
    return WINC_ERROR_TARGET;
  return WINC_OK;
}

ResultCode Target::GetJobTime(ULONG64 *out_time) {
  JOBOBJECT_BASIC_ACCOUNTING_INFORMATION info;
  ResultCode rc = job_object_->GetAccountInfo(&info);
  if (rc != WINC_OK)
    return WINC_ERROR_TARGET;
  *out_time = info.TotalKernelTime.QuadPart + info.TotalUserTime.QuadPart;
  return WINC_OK;
}

ResultCode Target::GetProcessTime(ULONG64 *out_time) {
  ULONG64 creation_time, exit_time, kernel_time, user_time;
  if (!::GetProcessTimes(process_handle_.get(),
                         reinterpret_cast<LPFILETIME>(&creation_time),
                         reinterpret_cast<LPFILETIME>(&exit_time),
                         reinterpret_cast<LPFILETIME>(&kernel_time),
                         reinterpret_cast<LPFILETIME>(&user_time)))
    return WINC_ERROR_TARGET;
  *out_time = kernel_time + user_time;
  return WINC_OK;
}

ResultCode Target::GetProcessCycle(ULONG64 *out_cycle) {
  if (!::QueryProcessCycleTime(process_handle_.get(), out_cycle))
    return WINC_ERROR_TARGET;
  return WINC_OK;
}

ResultCode Target::GetProcessPeakMemory(SIZE_T *out_size) {
  PROCESS_MEMORY_COUNTERS pmc;
  if (!::GetProcessMemoryInfo(process_handle_.get(), &pmc, sizeof(pmc)))
    return WINC_ERROR_TARGET;
  *out_size = pmc.PeakPagefileUsage;
  return WINC_OK;
}

ResultCode Target::GetJobPeakMemory(SIZE_T *out_size) {
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit;
  ResultCode rc = job_object_->GetBasicLimit(&limit);
  if (rc != WINC_OK)
    return WINC_ERROR_TARGET;
  *out_size = limit.PeakJobMemoryUsed;
  return WINC_OK;
}

ResultCode Target::GetProcessExitCode(DWORD *out_code) {
  if (!::GetExitCodeProcess(process_handle_.get(), out_code))
    return WINC_ERROR_TARGET;
  return WINC_OK;
}

}
