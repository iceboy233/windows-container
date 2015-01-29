// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/container.h"

#include <Windows.h>
#include <memory>
#include <string>
#include <utility>

#include <winc_types.h>
#include "core/policy.h"
#include "core/desktop.h"
#include "core/sid.h"

using std::make_unique;
using std::move;
using std::wstring;
using std::unique_ptr;
using winc::ResultCode;
using winc::Sid;

namespace {

ResultCode AdjustUserObjectAccessControl(HANDLE user_object,
  const Sid &sid, ACCESS_MASK access) {
  return winc::WINC_OK;
}

}

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
  unique_ptr<Desktop> desktop;
  rc = policy_->CreateTargetDesktop(&desktop);
  if (rc != WINC_OK)
    return rc;

  // 

  STARTUPINFOW si = {0};
  si.cb = sizeof(si);
  wstring desktop_name;
  if (!desktop->IsDefaultDesktop()) {
    rc = desktop->GetFullName(&desktop_name);
    if (rc != WINC_OK)
      return rc;
    si.lpDesktop = const_cast<wchar_t *>(desktop_name.c_str());
  }

  PROCESS_INFORMATION pi;
  BOOL success = ::CreateProcessAsUserW(restricted_token,
    exe_path, command_line,
    NULL, NULL, FALSE, STARTF_FORCEOFFFEEDBACK, NULL, NULL, &si, &pi);

  ::CloseHandle(restricted_token);
  if (!success)
    return WINC_ERROR_SPAWN;

  // TODO(iceboy): Not implemented
  ::CloseHandle(pi.hProcess);
  ::CloseHandle(pi.hThread);
  return WINC_OK;
}

void Container::CreateDefaultPolicy() {
  auto policy = make_unique<Policy>();
  policy->UseAlternateDesktop();
  policy->DisableMaxPrivilege();
  policy->RestrictSidLogonSession();
  policy->RestrictSid(WinBuiltinUsersSid);
  policy->RestrictSid(WinWorldSid);
  policy_ = move(policy);
}

}
