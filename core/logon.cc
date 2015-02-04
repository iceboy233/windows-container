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

LogonWithOwnedToken::~LogonWithOwnedToken() {
  if (token_ != NULL)
    ::CloseHandle(token_);
}

ResultCode CurrentLogon::Init(DWORD access) {
  HANDLE token;
  if (!::OpenProcessToken(::GetCurrentProcess(), access, &token))
    return WINC_ERROR_TOKEN;
  set_token(token);
  return WINC_OK;
}

ResultCode UserLogon::Init(const std::wstring &username,
                           const std::wstring &password) {
  HANDLE token;
  if (!::LogonUserW(username.c_str(), L".", password.c_str(),
                    LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT,
                    &token))
    return WINC_ERROR_TOKEN;
  set_token(token);
  return WINC_OK;
}

ResultCode UserLogon::GrantAccess(HANDLE object, SE_OBJECT_TYPE object_type,
                                  DWORD allowed_access) const {
  Sid sid;
  ResultCode rc = GetGroupSid(&sid);
  if (rc != WINC_OK)
    return rc;

  PACL old_dacl, new_dacl;
  PSECURITY_DESCRIPTOR sd;
  if (::GetSecurityInfo(object, object_type, DACL_SECURITY_INFORMATION,
                        NULL, NULL, &old_dacl, NULL, &sd) != ERROR_SUCCESS)
    return WINC_ERROR_TOKEN;

  EXPLICIT_ACCESSW ea;
  ea.grfAccessPermissions = allowed_access;
  ea.grfAccessMode = GRANT_ACCESS;
  ea.grfInheritance = 0;
  ea.Trustee.pMultipleTrustee = NULL;
  ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
  ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
  ea.Trustee.ptstrName = reinterpret_cast<LPWSTR>(sid.data());
  if (::SetEntriesInAclW(1, &ea, old_dacl, &new_dacl) != ERROR_SUCCESS) {
    ::LocalFree(sd);
    return WINC_ERROR_TOKEN;
  }

  DWORD ret = ::SetSecurityInfo(object, object_type,
                                DACL_SECURITY_INFORMATION,
                                NULL, NULL, new_dacl, NULL);
  ::LocalFree(new_dacl);
  ::LocalFree(sd);
  if (ret != ERROR_SUCCESS)
    return WINC_ERROR_TOKEN;
  return WINC_OK;
}

}
