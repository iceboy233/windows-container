// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_LOGON_H_
#define WINC_CORE_LOGON_H_

#include <Windows.h>
#include <Aclapi.h>
#include <string>

#include <winc_types.h>
#include "core/sid.h"

namespace winc {

class Logon {
public:
  Logon()
    : is_sid_cached_(false)
    {}

  virtual ~Logon() = default;
  virtual HANDLE GetToken() const = 0;
  virtual ResultCode GrantAccess(HANDLE object, SE_OBJECT_TYPE object_type,
                                 DWORD allowed_access) const = 0;
  ResultCode GetGroupSid(Sid *out_sid) const;

private:
  ResultCode InitSidCache() const;

private:
  mutable bool is_sid_cached_;
  mutable Sid sid_cache_;
};

class LogonWithOwnedToken : public Logon {
public:
  LogonWithOwnedToken()
    : token_(NULL)
    {}

  virtual ~LogonWithOwnedToken() override;

  virtual HANDLE GetToken() const override {
    return token_;
  }

protected:
  void set_token(HANDLE token) {
    token_ = token;
  }

private:
  HANDLE token_;

private:
  LogonWithOwnedToken(const LogonWithOwnedToken &) = delete;
  void operator=(const LogonWithOwnedToken &) = delete;
};

class CurrentLogon : public LogonWithOwnedToken {
public:
  ResultCode Init(DWORD access);

  virtual ResultCode GrantAccess(HANDLE object, SE_OBJECT_TYPE object_type,
                                 DWORD allowed_access) const override {
    return WINC_OK;
  }
};

class UserLogon : public LogonWithOwnedToken {
public:
  ResultCode Init(const std::wstring &username,
                  const std::wstring &password);
  virtual ResultCode GrantAccess(HANDLE object, SE_OBJECT_TYPE object_type,
                                 DWORD allowed_access) const override;
};

}

#endif
