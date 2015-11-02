// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_LOGON_H_
#define WINC_CORE_LOGON_H_

#include <Windows.h>
#include <Aclapi.h>
#include <string>

#include <winc_types.h>
#include <winc/sid.h>
#include <winc/util.h>

namespace winc {

class Logon {
protected:
  Logon()
    : is_user_sid_cached_(false)
    , is_group_sid_cached_(false)
    {}

public:
  virtual ~Logon() {}
  ResultCode GetUserSid(Sid **out_sid) const;
  ResultCode GetGroupSid(Sid **out_sid) const;
  virtual ResultCode FilterToken(const SID_AND_ATTRIBUTES *sids,
                                 DWORD sids_count,
                                 HANDLE *out_token) const;
  virtual ResultCode GrantAccess(HANDLE object, SE_OBJECT_TYPE object_type,
                                 DWORD allowed_access) const {
    return WINC_OK;
  }

protected:
  void set_token(HANDLE token) {
    token_.reset(token);
  }

private:
  ResultCode InitUserSidCache() const;
  ResultCode InitGroupSidCache() const;

private:
  unique_handle token_;
  mutable bool is_user_sid_cached_;
  mutable bool is_group_sid_cached_;
  mutable Sid user_sid_cache_;
  mutable Sid group_sid_cache_;

private:
  Logon(const Logon &) = delete;
  void operator=(const Logon &) = delete;
};

class LogonWithIntegrity : public Logon {
protected:
  LogonWithIntegrity() = default;
  void Init(DWORD integrity_level) {
    integrity_level_ = integrity_level;
  }

public:
  virtual ResultCode FilterToken(const SID_AND_ATTRIBUTES *sids,
                                 DWORD sids_count,
                                 HANDLE *out_token) const override;
  virtual ResultCode GrantAccess(HANDLE object, SE_OBJECT_TYPE object_type,
                                 DWORD allowed_access) const override;
private:
  DWORD integrity_level_;
};

class CurrentLogon : public LogonWithIntegrity {
public:
  ResultCode Init(DWORD integrity_level);
};

class UserLogon : public LogonWithIntegrity {
public:
  ResultCode Init(const std::wstring &username,
                  const std::wstring &password,
                  DWORD integrity_level);
  virtual ResultCode GrantAccess(HANDLE object, SE_OBJECT_TYPE object_type,
                                 DWORD allowed_access) const override;
};

}

#endif
