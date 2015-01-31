// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_LOGON_H_
#define WINC_CORE_LOGON_H_

#include <winc_types.h>
#include "core/sid.h"

namespace winc {

class Logon {
public:
  Logon()
    : is_sid_cached_(false)
    {}

  virtual ~Logon() =default;
  virtual HANDLE GetToken() const =0;
  ResultCode GetGroupSid(Sid *out_sid) const;

private:
  ResultCode InitSidCache() const;

private:
  mutable bool is_sid_cached_;
  mutable Sid sid_cache_;
};

class CurrentLogon : public Logon {
public:
  CurrentLogon();
  virtual ~CurrentLogon();

  ResultCode Init(DWORD access);

  virtual HANDLE GetToken() const {
    return token_;
  }

private:
  HANDLE token_;

private:
  CurrentLogon(const CurrentLogon &) =delete;
  void operator=(const CurrentLogon &) =delete;
};

}

#endif
