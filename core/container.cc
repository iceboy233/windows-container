// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/container.h"

#include <Windows.h>
#include <memory>

#include "core/policy.h"

using namespace std;

namespace winc {

Container::Container() {
}

Container::~Container() {
}

ResultCode Container::Spawn(const wchar_t *exe_path,
                            wchar_t *command_line) {
  if (!policy_)
    CreateDefaultPolicy();
  HANDLE restricted_token;
  ResultCode rc = policy_->CreateRestrictedToken(&restricted_token);
  if (rc != WINC_OK)
    return rc;

  STARTUPINFOW si = {0};
  PROCESS_INFORMATION pi;
  BOOL success = ::CreateProcessAsUserW(restricted_token,
    exe_path, command_line,
    NULL, NULL, FALSE, STARTF_FORCEOFFFEEDBACK, NULL, NULL, &si, &pi);

  ::CloseHandle(restricted_token);
  if (!success)
    return WINC_SPAWN_ERROR;

  // TODO(iceboy): Not implemented
  ::CloseHandle(pi.hProcess);
  ::CloseHandle(pi.hThread);
  return WINC_OK;
}

void Container::CreateDefaultPolicy() {
  auto policy = make_unique<Policy>();
  policy->DisableMaxPrivilege();
  policy->RestrictSidLogonSession();
  policy->RestrictSid(WinBuiltinUsersSid);
  policy->RestrictSid(WinWorldSid);
  policy_ = move(policy);
}

}
