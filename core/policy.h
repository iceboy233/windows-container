// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_POLICY_H_
#define WINC_CORE_POLICY_H_

#include <Windows.h>
#include <memory>
#include <vector>

#include "core/sid.h"

namespace winc {

class Desktop;
class JobObject;

class Policy {
public:
  Policy()
    : use_alternate_desktop_(false)
    , disable_max_privilege_(false)
    , job_object_basic_limit_(0)
    , job_object_ui_limit_(0)
    {}

  void UseAlternateDesktop() {
    use_alternate_desktop_ = true;
  }

  void DisableMaxPrivilege() {
    disable_max_privilege_ = true;
  }

  ResultCode GetSidLogonSession(Sid *out_sid);

  ResultCode RestrictSid(WELL_KNOWN_SID_TYPE type);
  void RestrictSid(const Sid &sid);

  void SetJobObjectBasicLimit(DWORD basic_limit) {
    job_object_basic_limit_ |= basic_limit;
  }

  void SetJobObjectUILimit(DWORD ui_limit) {
    job_object_ui_limit_ |= ui_limit;
  }

  ResultCode CreateRestrictedToken(HANDLE *out_token);
  ResultCode CreateTargetDesktop(Desktop **out_desktop);
  ResultCode CreateTargetJobObject(JobObject **out_job);

private:
  bool CreateEffectiveToken(HANDLE *out_token);

private:
  bool use_alternate_desktop_;
  bool disable_max_privilege_;
  DWORD job_object_basic_limit_;
  DWORD job_object_ui_limit_;
  std::vector<Sid> restricted_sids_;
};

}

#endif
