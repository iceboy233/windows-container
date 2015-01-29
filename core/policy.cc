// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/policy.h"

#include <malloc.h>
#include <memory>
#include <vector>

using std::make_unique;
using std::unique_ptr;
using std::vector;

namespace {

bool GetSidLogonSession(HANDLE token, winc::Sid *sid) {
  DWORD size;
  if (!::GetTokenInformation(token, TokenGroups, NULL, 0, &size) &&
      ::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    return false;

  TOKEN_GROUPS *info = reinterpret_cast<TOKEN_GROUPS *>(_malloca(size));
  if (::GetTokenInformation(token, TokenGroups, info, size, &size)) {
    for (unsigned int i = 0; i < info->GroupCount; ++i) {
      if (info->Groups[i].Attributes & SE_GROUP_LOGON_ID) {
        sid->Init(reinterpret_cast<SID *>(info->Groups[i].Sid));
        _freea(info);
        return true;
      }
    }
  }

  _freea(info);
  return false;
}

}

namespace winc {

ResultCode Policy::RestrictSid(WELL_KNOWN_SID_TYPE type) {
  restricted_sids_.push_back(Sid());
  ResultCode rc = restricted_sids_.back().Init(type);
  if (rc != WINC_OK)
    restricted_sids_.pop_back();
  return rc;
}

void Policy::RestrictSid(const Sid &sid) {
  restricted_sids_.push_back(sid);
}

ResultCode Policy::CreateRestrictedToken(HANDLE *out_token) {
  // TODO(iceboy): Cache the token if possible
  // TODO(iceboy): Use the logon token if present

  HANDLE effective_token;
  if (!::OpenProcessToken(::GetCurrentProcess(),
    TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
    &effective_token))
    return WINC_ERROR_TOKEN;

  if (restrict_sid_logon_session_) {
    restricted_sids_.push_back(Sid());
    if (!GetSidLogonSession(effective_token, &restricted_sids_.back())) {
      restricted_sids_.pop_back();
      ::CloseHandle(effective_token);
      return WINC_ERROR_TOKEN;
    }
  }

  vector<SID_AND_ATTRIBUTES> sids_to_restrict(restricted_sids_.size());
  for (unsigned int i = 0; i < restricted_sids_.size(); ++i) {
    sids_to_restrict[i].Sid = restricted_sids_[i].data();
    sids_to_restrict[i].Attributes = 0;
  }

  HANDLE restricted_token;
  BOOL success = ::CreateRestrictedToken(effective_token,
    disable_max_privilege_ ? DISABLE_MAX_PRIVILEGE : 0,
    0, NULL,
    0, NULL,
    sids_to_restrict.size(), sids_to_restrict.data(),
    &restricted_token);

  if (restrict_sid_logon_session_)
    restricted_sids_.pop_back();
  ::CloseHandle(effective_token);

  if (!success)
    return WINC_ERROR_TOKEN;

  *out_token = restricted_token;
  return WINC_OK;
}

ResultCode Policy::CreateTargetDesktop(unique_ptr<Desktop> *out_desktop) {
  if (!use_alternate_desktop_) {
    *out_desktop = make_unique<DefaultDesktop>();
  } else {
    auto desktop = make_unique<AlternateDesktop>();
    ResultCode rc = desktop->Init();
    if (rc != WINC_OK)
      return rc;
    *out_desktop = move(desktop);
  }
  return WINC_OK;
}

}
