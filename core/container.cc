// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <winc/container.h>

#include <Windows.h>
#include <memory>

#include <winc_types.h>
#include <winc/desktop.h>
#include <winc/logon.h>
#include <winc/policy.h>
#include <winc/target.h>
#include <winc/util.h>
#include "core/ntnative.h"
#include "core/job_object.h"

using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;

namespace winc {

ResultCode Container::Spawn(const wchar_t *exe_path,
                            Target *target,
                            SpawnOptions *options) {
  Policy *policy;
  ResultCode rc = GetPolicy(&policy);
  if (rc != WINC_OK)
    return rc;

  HANDLE restricted_token;
  rc = policy->GetRestrictedToken(&restricted_token);
  if (rc != WINC_OK)
    return rc;

  STARTUPINFOEXW si = {};
  si.StartupInfo.cb = sizeof(si);
  si.StartupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK;

  Desktop *desktop;
  rc = policy->GetDesktop(&desktop);
  if (rc != WINC_OK)
    return rc;
  if (!desktop->IsDefaultDesktop()) {
    const wchar_t *desktop_name;
    rc = desktop->GetFullName(&desktop_name);
    if (rc != WINC_OK)
      return rc;
    si.StartupInfo.lpDesktop = const_cast<wchar_t *>(desktop_name);
  }

  JobObject *job_object;
  rc = policy->MakeJobObject(&job_object);
  if (rc != WINC_OK)
    return rc;
  unique_ptr<JobObject> job_object_holder(job_object);

  ProcThreadAttributeList attribute_list;
  HANDLE inherit_list[3];
  SIZE_T inherit_count = 0;
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
    if (options->stdin_handle) {
      si.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
      si.StartupInfo.hStdInput = options->stdin_handle;
      inherit_list[inherit_count++] = options->stdin_handle;
    }
    if (options->stdout_handle) {
      si.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
      si.StartupInfo.hStdOutput = options->stdout_handle;
      inherit_list[inherit_count++] = options->stdout_handle;
    }
    if (options->stdin_handle) {
      si.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
      si.StartupInfo.hStdError = options->stderr_handle;
      inherit_list[inherit_count++] = options->stderr_handle;
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
  }

  PROCESS_INFORMATION pi;
  BOOL success = ::CreateProcessAsUserW(restricted_token,
    exe_path,
    options ? options->command_line : NULL,
    NULL, NULL, inherit_count ? TRUE : FALSE,
    CREATE_BREAKAWAY_FROM_JOB | CREATE_SUSPENDED,
    NULL,
    options ? options->current_directory : NULL,
    &si.StartupInfo, &pi);
  if (!success) {
    if (::GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
      return WINC_PRIVILEGE_NOT_HELD;
    return WINC_ERROR_SPAWN;
  }

  // Assign the process to the job object as soon as possible
  rc = job_object->AssignProcess(pi.hProcess);
  unique_handle process_holder(pi.hProcess);
  unique_handle thread_holder(pi.hThread);
  if (rc != WINC_OK) {
    ::TerminateProcess(pi.hProcess, 1);
    return rc;
  }

  // Disable hard error of the target process
  NTSTATUS status;
  ULONG default_hard_error_mode;
  status = ::NtQueryInformationProcess(pi.hProcess,
                                       ProcessDefaultHardErrorMode,
                                       &default_hard_error_mode,
                                       sizeof(default_hard_error_mode),
                                       NULL);
  if (!NT_SUCCESS(status))
    return WINC_ERROR_SPAWN;
  default_hard_error_mode &= ~1;
  status = ::NtSetInformationProcess(pi.hProcess,
                                     ProcessDefaultHardErrorMode,
                                     &default_hard_error_mode,
                                     sizeof(default_hard_error_mode));
  if (!NT_SUCCESS(status))
    return WINC_ERROR_SPAWN;

  target->Assign(pi.dwProcessId, job_object_holder,
                 process_holder, thread_holder);
  return WINC_OK;
}

ResultCode Container::GetPolicy(Policy **out_policy) {
  if (!policy_) {
    auto policy = make_unique<Policy>();
    shared_ptr<Logon> logon;
    ResultCode rc = policy->GetLogon(&logon);
    if (rc != WINC_OK)
      return rc;
    Sid *logon_sid;
    rc = logon->GetGroupSid(&logon_sid);
    if (rc != WINC_OK)
      return rc;
    policy->AddRestrictSid(*logon_sid);
    Sid builtin_user_sid;
    rc = builtin_user_sid.Init(WinBuiltinUsersSid);
    if (rc != WINC_OK)
      return rc;
    policy->AddRestrictSid(builtin_user_sid);
    Sid world_sid;
    rc = world_sid.Init(WinWorldSid);
    if (rc != WINC_OK)
      return rc;
    policy->AddRestrictSid(world_sid);
    policy->set_job_basic_limit(JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION
                              | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE);
    policy->set_job_ui_limit(JOB_OBJECT_UILIMIT_HANDLES
                           | JOB_OBJECT_UILIMIT_READCLIPBOARD
                           | JOB_OBJECT_UILIMIT_WRITECLIPBOARD
                           | JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS
                           | JOB_OBJECT_UILIMIT_DISPLAYSETTINGS
                           | JOB_OBJECT_UILIMIT_GLOBALATOMS
                           | JOB_OBJECT_UILIMIT_DESKTOP
                           | JOB_OBJECT_UILIMIT_EXITWINDOWS);
    policy_ = move(policy);
  }
  *out_policy = policy_.get();
  return WINC_OK;
}

}
