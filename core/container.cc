// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/container.h"

#include <Windows.h>
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

using std::make_unique;
using std::move;
using std::unique_ptr;
using std::vector;
using std::wstring;
using winc::Sid;

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

  Desktop *desktop;
  rc = policy_->MakeDesktop(&desktop);
  if (rc != WINC_OK)
    return rc;
  unique_ptr<Desktop> desktop_holder(desktop);

  JobObject *job_object;
  rc = policy_->MakeJobObject(&job_object);
  if (rc != WINC_OK)
    return rc;
  unique_ptr<JobObject> job_object_holder(job_object);

  STARTUPINFOEXW si = {};
  si.StartupInfo.cb = sizeof(si);
  si.StartupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK;
  wstring desktop_name;
  if (!desktop->IsDefaultDesktop()) {
    rc = desktop->GetFullName(&desktop_name);
    if (rc != WINC_OK)
      return rc;
    si.StartupInfo.lpDesktop = const_cast<wchar_t *>(desktop_name.c_str());
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
  unique_handle process_holder(pi.hProcess);
  unique_handle thread_holder(pi.hThread);

  rc = job_object->AssignProcess(pi.hProcess);
  if (rc != WINC_OK) {
    ::TerminateProcess(pi.hProcess, 1);
    return rc;
  }

  *out_process = new TargetProcess(desktop_holder, job_object_holder,
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
  policy->UseAlternateDesktop();
  policy->DisableMaxPrivilege();
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

TargetProcess::TargetProcess(unique_ptr<Desktop> &desktop,
                             unique_ptr<JobObject> &job_object,
                             DWORD process_id, DWORD thread_id,
                             unique_handle &process_handle,
                             unique_handle &thread_handle)
  : desktop_(move(desktop))
  , job_object_(move(job_object))
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

}
