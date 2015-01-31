// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/container.h"

#include <Windows.h>
#include <memory>
#include <string>

#include <winc_types.h>
#include "core/policy.h"
#include "core/desktop.h"
#include "core/sid.h"
#include "core/job_object.h"
#include "core/util.h"

using std::unique_ptr;
using std::wstring;
using winc::Sid;

namespace winc {

Container::Container() =default;
Container::~Container() =default;

ResultCode Container::Spawn(const wchar_t *exe_path,
                            SpawnOptions *options OPTIONAL,
                            IoHandles *io_handles OPTIONAL,
                            TargetProcess **out_process) {
  if (!policy_)
    policy_.reset(CreateDefaultPolicy());
  HANDLE restricted_token;
  ResultCode rc = policy_->CreateRestrictedToken(&restricted_token);
  if (rc != WINC_OK)
    return rc;
  unique_handle restricted_token_holder(restricted_token);

  Desktop *desktop;
  rc = policy_->CreateTargetDesktop(&desktop);
  if (rc != WINC_OK)
    return rc;
  unique_ptr<Desktop> desktop_holder(desktop);

  JobObject *job_object;
  rc = policy_->CreateTargetJobObject(&job_object);
  if (rc != WINC_OK)
    return rc;
  unique_ptr<JobObject> job_object_holder(job_object);

  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_FORCEOFFFEEDBACK;
  if (io_handles) {
    si.dwFlags   |= STARTF_USESTDHANDLES;
    si.hStdInput  = io_handles->stdin_handle;
    si.hStdOutput = io_handles->stdout_handle;
    si.hStdError  = io_handles->stderr_handle;
  }
  wstring desktop_name;
  if (!desktop->IsDefaultDesktop()) {
    rc = desktop->GetFullName(&desktop_name);
    if (rc != WINC_OK)
      return rc;
    si.lpDesktop = const_cast<wchar_t *>(desktop_name.c_str());
  }

  PROCESS_INFORMATION pi;
  BOOL success = ::CreateProcessAsUserW(restricted_token,
    exe_path,
    options ? options->command_line : NULL,
    NULL, NULL, FALSE,
    CREATE_BREAKAWAY_FROM_JOB | CREATE_SUSPENDED,
    NULL, NULL, &si, &pi);
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

Policy *Container::CreateDefaultPolicy() {
  Policy *policy = new Policy;
  policy->UseAlternateDesktop();
  policy->DisableMaxPrivilege();
  Sid logon_sid;
  policy->GetSidLogonSession(&logon_sid);
  policy->RestrictSid(logon_sid);
  policy->RestrictSid(WinBuiltinUsersSid);
  policy->RestrictSid(WinWorldSid);
  policy->SetJobObjectUILimit(JOB_OBJECT_UILIMIT_HANDLES
                            | JOB_OBJECT_UILIMIT_READCLIPBOARD
                            | JOB_OBJECT_UILIMIT_WRITECLIPBOARD
                            | JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS
                            | JOB_OBJECT_UILIMIT_DISPLAYSETTINGS
                            | JOB_OBJECT_UILIMIT_GLOBALATOMS
                            | JOB_OBJECT_UILIMIT_DESKTOP
                            | JOB_OBJECT_UILIMIT_EXITWINDOWS);
  return policy;
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
