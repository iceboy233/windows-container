// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/container.h"

#include <Windows.h>
#include <string>

#include <winc_types.h>
#include "core/policy.h"
#include "core/desktop.h"
#include "core/sid.h"

using std::wstring;
using winc::Sid;

namespace winc {

Container::Container() =default;
Container::~Container() =default;

ResultCode Container::Spawn(const wchar_t *exe_path,
                            wchar_t *command_line) {
  if (!policy_)
    CreateDefaultPolicy();
  HANDLE restricted_token;
  ResultCode rc = policy_->CreateRestrictedToken(&restricted_token);
  if (rc != WINC_OK)
    return rc;
  Desktop *desktop;
  rc = policy_->CreateTargetDesktop(&desktop);
  if (rc != WINC_OK)
    return rc;

  STARTUPINFOW si = {0};
  si.cb = sizeof(si);
  wstring desktop_name;
  if (!desktop->IsDefaultDesktop()) {
    rc = desktop->GetFullName(&desktop_name);
    if (rc != WINC_OK) {
      delete desktop;
      return rc;
    }
    si.lpDesktop = const_cast<wchar_t *>(desktop_name.c_str());
  }

  PROCESS_INFORMATION pi;
  BOOL success = ::CreateProcessAsUserW(restricted_token,
    exe_path, command_line, NULL, NULL, FALSE,
    STARTF_FORCEOFFFEEDBACK | CREATE_BREAKAWAY_FROM_JOB,
    NULL, NULL, &si, &pi);

  ::CloseHandle(restricted_token);
  if (!success) {
    delete desktop;
    return WINC_ERROR_SPAWN;
  }

  // TODO(iceboy): Not implemented
  ::CloseHandle(pi.hThread);
  ::WaitForSingleObject(pi.hProcess, INFINITE);
  ::CloseHandle(pi.hProcess);
  delete desktop;
  return WINC_OK;
}

void Container::CreateDefaultPolicy() {
  Policy *policy = new Policy;
  policy->UseAlternateDesktop();
  policy->DisableMaxPrivilege();
  Sid logon_sid;
  policy->GetSidLogonSession(&logon_sid);
  policy->RestrictSid(logon_sid);
  policy->RestrictSid(WinBuiltinUsersSid);
  policy->RestrictSid(WinWorldSid);
  policy_.reset(policy);
}

}
