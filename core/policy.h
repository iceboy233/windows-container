// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_POLICY_H_
#define WINC_CORE_POLICY_H_

#include <Windows.h>
#include <vector>

#include "core/sid.h"

namespace winc {

class Policy {
public:
  Policy()
    : disable_max_privilege_(false)
    , restrict_sid_logon_session_(false)
    {}

  void DisableMaxPrivilege() {
    disable_max_privilege_ = true;
  }

  void RestrictSidLogonSession() {
    restrict_sid_logon_session_ = true;
  }

  ResultCode RestrictSid(WELL_KNOWN_SID_TYPE type) {
    restricted_sids_.push_back(Sid());
    ResultCode rc = restricted_sids_.back().Init(type);
    if (rc != WINC_OK)
      restricted_sids_.pop_back();
    return rc;
  }

  void RestrictSid(const Sid &sid) {
    restricted_sids_.push_back(sid);
  }

  ResultCode CreateRestrictedToken(HANDLE *out_token);

private:
  bool disable_max_privilege_;
  bool restrict_sid_logon_session_;
  std::vector<Sid> restricted_sids_;
};

}

#endif
