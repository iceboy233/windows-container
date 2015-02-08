// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/logon.h"

#include <Windows.h>
#include <vector>

#include "core/sid.h"

using namespace std;

namespace winc {

ResultCode Logon::GetGroupSid(Sid **out_sid) const {
  if (!is_sid_cached_) {
    ResultCode rc = InitSidCache();
    if (rc != WINC_OK)
      return rc;
  }

  *out_sid = &sid_cache_;
  return WINC_OK;
}

ResultCode Logon::FilterToken(const SID_AND_ATTRIBUTES *sids,
                              DWORD sids_count,
                              HANDLE *out_token) const {
  HANDLE new_token;
  if (!::CreateRestrictedToken(token_.get(), DISABLE_MAX_PRIVILEGE,
                               0, NULL, 0, NULL, sids_count,
                               const_cast<SID_AND_ATTRIBUTES *>(sids),
                               &new_token))
    return WINC_ERROR_LOGON;
  *out_token = new_token;
  return WINC_OK;
}

ResultCode Logon::InitSidCache() const {
  DWORD size;
  if (!::GetTokenInformation(token_.get(), TokenGroups, NULL, 0, &size) &&
      ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return WINC_ERROR_LOGON;
  }

  vector<BYTE> buffer(size);
  TOKEN_GROUPS *info = reinterpret_cast<TOKEN_GROUPS *>(buffer.data());
  if (::GetTokenInformation(token_.get(), TokenGroups, info, size, &size)) {
    for (unsigned int i = 0; i < info->GroupCount; ++i) {
      if (info->Groups[i].Attributes & SE_GROUP_LOGON_ID) {
        sid_cache_.Init(info->Groups[i].Sid);
        is_sid_cached_ = true;
        return WINC_OK;
      }
    }
  }

  return WINC_ERROR_LOGON;
}

ResultCode LogonWithIntegrity::FilterToken(const SID_AND_ATTRIBUTES *sids,
                                           DWORD sids_count,
                                           HANDLE *out_token) const {
  HANDLE new_token;
  ResultCode rc = Logon::FilterToken(sids, sids_count, &new_token);
  if (rc != WINC_OK)
    return rc;

  // Set the integrity level of the new token
  SID_IDENTIFIER_AUTHORITY authority = SECURITY_MANDATORY_LABEL_AUTHORITY;
  Sid sid;
  rc = sid.Init(&authority, integrity_level_);
  if (rc != WINC_OK) {
    ::CloseHandle(new_token);
    return rc;
  }

  TOKEN_MANDATORY_LABEL tml;
  tml.Label.Sid = sid.data();
  tml.Label.Attributes = SE_GROUP_INTEGRITY;
  if (!::SetTokenInformation(new_token, TokenIntegrityLevel,
                             &tml, sizeof(tml) + sid.GetLength())) {
    ::CloseHandle(new_token);
    return rc;
  }

  *out_token = new_token;
  return WINC_OK;
}

ResultCode LogonWithIntegrity::GrantAccess(HANDLE object,
                                           SE_OBJECT_TYPE object_type,
                                           DWORD allowed_access) const {
  // Set the integrity level of the object
  SID_IDENTIFIER_AUTHORITY authority = SECURITY_MANDATORY_LABEL_AUTHORITY;
  Sid sid;
  ResultCode rc = sid.Init(&authority, integrity_level_);
  if (rc != WINC_OK)
    return rc;
  vector<BYTE> sacl_buffer(sizeof(ACL) +
                           FIELD_OFFSET(SYSTEM_MANDATORY_LABEL_ACE, SidStart) +
                           sid.GetLength());
  ACL *acl = reinterpret_cast<ACL *>(sacl_buffer.data());
  if (!::InitializeAcl(acl, static_cast<DWORD>(sacl_buffer.size()),
                       ACL_REVISION))
    return WINC_ERROR_LOGON;
  acl->AceCount = 1;
  SYSTEM_MANDATORY_LABEL_ACE *ace =
    reinterpret_cast<SYSTEM_MANDATORY_LABEL_ACE *>(
      sacl_buffer.data() + sizeof(ACL));
  ace->Header.AceType = SYSTEM_MANDATORY_LABEL_ACE_TYPE;
  ace->Header.AceSize = static_cast<WORD>(
    FIELD_OFFSET(SYSTEM_MANDATORY_LABEL_ACE, SidStart) + sid.GetLength());
  ace->Mask = SYSTEM_MANDATORY_LABEL_NO_WRITE_UP;
  if (!::CopySid(sid.GetLength(), &ace->SidStart, sid.data()))
    return WINC_ERROR_LOGON;
  DWORD ret = ::SetSecurityInfo(object, object_type,
                                LABEL_SECURITY_INFORMATION,
                                NULL, NULL, NULL,
                                reinterpret_cast<PACL>(sacl_buffer.data()));
  if (ret != ERROR_SUCCESS)
    return WINC_ERROR_LOGON;
  return Logon::GrantAccess(object, object_type, allowed_access);
}

ResultCode CurrentLogon::Init(DWORD access, DWORD integrity_level) {
  HANDLE token;
  if (!::OpenProcessToken(::GetCurrentProcess(), access, &token))
    return WINC_ERROR_LOGON;
  set_token(token);
  LogonWithIntegrity::Init(integrity_level);
  return WINC_OK;
}

ResultCode UserLogon::Init(const std::wstring &username,
                           const std::wstring &password,
                           DWORD integrity_level) {
  HANDLE token;
  if (!::LogonUserW(username.c_str(), L".", password.c_str(),
                    LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT,
                    &token))
    return WINC_ERROR_LOGON;
  set_token(token);
  LogonWithIntegrity::Init(integrity_level);
  return WINC_OK;
}

ResultCode UserLogon::GrantAccess(HANDLE object, SE_OBJECT_TYPE object_type,
                                  DWORD allowed_access) const {
  Sid *sid;
  ResultCode rc = GetGroupSid(&sid);
  if (rc != WINC_OK)
    return rc;

  PACL old_dacl, new_dacl;
  PSECURITY_DESCRIPTOR sd;
  if (::GetSecurityInfo(object, object_type, DACL_SECURITY_INFORMATION,
                        NULL, NULL, &old_dacl, NULL, &sd) != ERROR_SUCCESS)
    return WINC_ERROR_LOGON;

  EXPLICIT_ACCESSW ea;
  ea.grfAccessPermissions = allowed_access;
  ea.grfAccessMode = GRANT_ACCESS;
  ea.grfInheritance = 0;
  ea.Trustee.pMultipleTrustee = NULL;
  ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
  ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
  ea.Trustee.ptstrName = reinterpret_cast<LPWSTR>(sid->data());
  if (::SetEntriesInAclW(1, &ea, old_dacl, &new_dacl) != ERROR_SUCCESS) {
    ::LocalFree(sd);
    return WINC_ERROR_LOGON;
  }

  DWORD ret = ::SetSecurityInfo(object, object_type,
                                DACL_SECURITY_INFORMATION,
                                NULL, NULL, new_dacl, NULL);
  ::LocalFree(new_dacl);
  ::LocalFree(sd);
  if (ret != ERROR_SUCCESS)
    return WINC_ERROR_LOGON;
  return LogonWithIntegrity::GrantAccess(object, object_type, allowed_access);
}

}
