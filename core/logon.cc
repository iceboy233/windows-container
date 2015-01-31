// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/logon.h"

#include <vector>

using namespace std;

namespace winc {

ResultCode Logon::GetGroupSid(Sid *out_sid) const {
  if (!is_sid_cached_) {
    ResultCode rc = InitSidCache();
    if (rc != WINC_OK)
      return rc;
  }

  out_sid->Init(sid_cache_.data());
  return WINC_OK;
}

ResultCode Logon::InitSidCache() const {
  HANDLE token = GetToken();
  DWORD size;
  if (!::GetTokenInformation(token, TokenGroups, NULL, 0, &size) &&
      ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return WINC_ERROR_TOKEN;
  }

  vector<BYTE> buffer(size);
  TOKEN_GROUPS *info = reinterpret_cast<TOKEN_GROUPS *>(buffer.data());
  if (::GetTokenInformation(token, TokenGroups, info, size, &size)) {
    for (unsigned int i = 0; i < info->GroupCount; ++i) {
      if (info->Groups[i].Attributes & SE_GROUP_LOGON_ID) {
        sid_cache_.Init(reinterpret_cast<SID *>(info->Groups[i].Sid));
        is_sid_cached_ = true;
        return WINC_OK;
      }
    }
  }

  return WINC_ERROR_TOKEN;
}

CurrentLogon::CurrentLogon()
  : token_(NULL)
  {}

CurrentLogon::~CurrentLogon() {
  if (token_ != NULL)
    ::CloseHandle(token_);
}

ResultCode CurrentLogon::Init(DWORD access) {
  HANDLE token;
  if (!::OpenProcessToken(::GetCurrentProcess(), access, &token))
    return WINC_ERROR_TOKEN;
  token_ = token;
  return WINC_OK;
}

}
