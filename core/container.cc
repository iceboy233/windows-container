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

  STARTUPINFOW si = {0};
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

  // TODO(iceboy): Not implemented
  ::ResumeThread(pi.hThread);
  ::WaitForSingleObject(pi.hProcess, INFINITE);
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
  return policy;
}

}
