// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/container.h"

#include <Windows.h>
#include <Psapi.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <winc_types.h>
#include "core/policy.h"
#include "core/desktop.h"
#include "core/sid.h"
#include "core/job_object.h"
#include "core/util.h"
#include "core/logon.h"
#include "core/debugger.h"

using std::make_unique;
using std::move;
using std::unique_ptr;
using std::vector;
using std::wstring;

namespace winc {

Container::Container() =default;
Container::~Container() =default;

ResultCode Container::Spawn(const wchar_t *exe_path,
                            SpawnOptions *options OPTIONAL,
                            IoHandles *io_handles OPTIONAL,
                            TargetProcess **out_process) {
  if (!policy_) {
    Policy *policy;
    ResultCode rc = CreateDefaultPolicy(&policy);
    if (rc != WINC_OK)
      return rc;
    policy_.reset(policy);
  }
  HANDLE restricted_token;
  ResultCode rc = policy_->GetRestrictedToken(&restricted_token);
  if (rc != WINC_OK)
    return rc;

  JobObject *job_object;
  rc = policy_->MakeJobObject(&job_object);
  if (rc != WINC_OK)
    return rc;
  unique_ptr<JobObject> job_object_holder(job_object);
  if (options) {
    if (options->processor_affinity ||
        options->memory_limit ||
        options->active_process_limit) {
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit;
      rc = job_object->GetBasicLimit(&limit);
      if (rc != WINC_OK)
        return rc;
      if (options->processor_affinity) {
        limit.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
        limit.BasicLimitInformation.Affinity = options->processor_affinity;
      }
      if (options->memory_limit) {
        limit.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
        limit.JobMemoryLimit = options->memory_limit;
      }
      if (options->active_process_limit) {
        limit.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
        limit.BasicLimitInformation.ActiveProcessLimit = options->active_process_limit;
      }
      rc = job_object->SetBasicLimit(limit);
      if (rc != WINC_OK)
        return rc;
    }
  }

  STARTUPINFOEXW si = {};
  si.StartupInfo.cb = sizeof(si);
  si.StartupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK;
  const Desktop &desktop = policy_->desktop();
  if (!desktop.IsDefaultDesktop()) {
    const wchar_t *desktop_name;
    rc = desktop.GetFullName(&desktop_name);
    if (rc != WINC_OK)
      return rc;
    si.StartupInfo.lpDesktop = const_cast<wchar_t *>(desktop_name);
  }

  ProcThreadAttributeList attribute_list;
  HANDLE inherit_list[3];
  SIZE_T inherit_count = 0;
  if (io_handles) {
    si.StartupInfo.dwFlags   |= STARTF_USESTDHANDLES;
    si.StartupInfo.hStdInput  = io_handles->stdin_handle;
    si.StartupInfo.hStdOutput = io_handles->stdout_handle;
    si.StartupInfo.hStdError  = io_handles->stderr_handle;

    if (io_handles->stdin_handle)
      inherit_list[inherit_count++] = io_handles->stdin_handle;
    if (io_handles->stdout_handle)
      inherit_list[inherit_count++] = io_handles->stdout_handle;
    if (io_handles->stderr_handle)
      inherit_list[inherit_count++] = io_handles->stderr_handle;
  }

  if (inherit_count) {
    rc = attribute_list.Init(1, 0);
    if (rc != WINC_OK)
      return rc;
    rc = attribute_list.Update(0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                          inherit_list, inherit_count * sizeof(HANDLE));
    if (rc != WINC_OK)
      return rc;
    si.lpAttributeList = attribute_list.data();
  }

  PROCESS_INFORMATION pi;
  BOOL success = ::CreateProcessAsUserW(restricted_token,
    exe_path,
    options ? options->command_line : NULL,
    NULL, NULL, inherit_count ? TRUE : FALSE,
    CREATE_BREAKAWAY_FROM_JOB | CREATE_SUSPENDED,
    NULL, NULL, &si.StartupInfo, &pi);
  if (!success)
    return WINC_ERROR_SPAWN;

  // Assign the process to the job object as soon as possible
  rc = job_object->AssignProcess(pi.hProcess);
  unique_handle process_holder(pi.hProcess);
  unique_handle thread_holder(pi.hThread);
  if (rc != WINC_OK) {
    ::TerminateProcess(pi.hProcess, 1);
    return rc;
  }

  Debugger *debugger = new Debugger;
  rc = debugger->Init(pi.hProcess);
  if (rc != WINC_OK) {
    ::TerminateProcess(pi.hProcess, 1);
    return rc;
  }
  unique_ptr<Debugger> debugger_holder(debugger);

  *out_process = new TargetProcess(job_object_holder, debugger_holder,
                                   pi.dwProcessId, pi.dwThreadId,
                                   process_holder, thread_holder);
  return WINC_OK;
}

ResultCode Container::CreateDefaultPolicy(Policy **out_policy) {
  CurrentLogon *logon = new CurrentLogon;
  ResultCode rc = logon->Init(TOKEN_QUERY | TOKEN_DUPLICATE |
                              TOKEN_ASSIGN_PRIMARY);
  Sid logon_sid;
  rc = logon->GetGroupSid(&logon_sid);
  if (rc != WINC_OK) {
    delete logon;
    return rc;
  }

  Policy *policy = new Policy(unique_ptr<Logon>(logon));
  rc = policy->UseAlternateDesktop();
  if (rc != WINC_OK)
    return rc;
  policy->RestrictSid(logon_sid);
  policy->RestrictSid(WinBuiltinUsersSid);
  policy->RestrictSid(WinWorldSid);
  policy->SetJobObjectBasicLimit(JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION
                               | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE);
  policy->SetJobObjectUILimit(JOB_OBJECT_UILIMIT_HANDLES
                            | JOB_OBJECT_UILIMIT_READCLIPBOARD
                            | JOB_OBJECT_UILIMIT_WRITECLIPBOARD
                            | JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS
                            | JOB_OBJECT_UILIMIT_DISPLAYSETTINGS
                            | JOB_OBJECT_UILIMIT_GLOBALATOMS
                            | JOB_OBJECT_UILIMIT_DESKTOP
                            | JOB_OBJECT_UILIMIT_EXITWINDOWS);
  *out_policy = policy;
  return WINC_OK;
}

TargetProcess::TargetProcess(unique_ptr<JobObject> &job_object,
                             unique_ptr<Debugger> &debugger,
                             DWORD process_id, DWORD thread_id,
                             unique_handle &process_handle,
                             unique_handle &thread_handle)
  : job_object_(move(job_object))
  , debugger_(move(debugger))
  , process_id_(process_id)
  , thread_id_(thread_id)
  , process_handle_(move(process_handle))
  , thread_handle_(move(thread_handle))
  {}

TargetProcess::~TargetProcess() =default;

ResultCode TargetProcess::Run() {
  if (::ResumeThread(thread_handle_.get()) == static_cast<DWORD>(-1))
    return WINC_ERROR_RUN;
  if (::WaitForSingleObject(process_handle_.get(), INFINITE) == WAIT_FAILED)
    return WINC_ERROR_RUN;
  return WINC_OK;
}

ResultCode TargetProcess::GetJobTime(ULONG64 *out_time) {
  JOBOBJECT_BASIC_ACCOUNTING_INFORMATION info;
  ResultCode rc = job_object_->GetAccountInfo(&info);
  if (rc != WINC_OK)
    return WINC_ERROR_QUERY;
  *out_time = info.TotalKernelTime.QuadPart + info.TotalUserTime.QuadPart;
  return WINC_OK;
}

ResultCode TargetProcess::GetProcessTime(ULONG64 *out_time) {
  ULONG64 creation_time, exit_time, kernel_time, user_time;
  if (!::GetProcessTimes(process_handle_.get(),
                         reinterpret_cast<LPFILETIME>(&creation_time),
                         reinterpret_cast<LPFILETIME>(&exit_time),
                         reinterpret_cast<LPFILETIME>(&kernel_time),
                         reinterpret_cast<LPFILETIME>(&user_time)))
    return WINC_ERROR_QUERY;
  *out_time = kernel_time + user_time;
  return WINC_OK;
}

ResultCode TargetProcess::GetProcessCycle(ULONG64 *out_cycle) {
  if (!::QueryProcessCycleTime(process_handle_.get(), out_cycle))
    return WINC_ERROR_QUERY;
  return WINC_OK;
}

ResultCode TargetProcess::GetProcessPeakMemory(SIZE_T *out_size) {
  PROCESS_MEMORY_COUNTERS pmc;
  if (!::GetProcessMemoryInfo(process_handle_.get(), &pmc, sizeof(pmc)))
    return WINC_ERROR_QUERY;
  *out_size = pmc.PeakPagefileUsage;
  return WINC_OK;
}

ResultCode TargetProcess::GetJobPeakMemory(SIZE_T *out_size) {
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit;
  ResultCode rc = job_object_->GetBasicLimit(&limit);
  if (rc != WINC_OK)
    return WINC_ERROR_QUERY;
  *out_size = limit.PeakJobMemoryUsed;
  return WINC_OK;
}

}
