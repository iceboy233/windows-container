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
class Logon;

class Policy {
public:
  explicit Policy(std::unique_ptr<Logon> &logon)
    : restricted_token_(NULL)
    , use_alternate_desktop_(false)
    , disable_max_privilege_(false)
    , job_object_basic_limit_(0)
    , job_object_ui_limit_(0)
    , logon_(move(logon))
    {}

  ~Policy();

  void UseAlternateDesktop() {
    use_alternate_desktop_ = true;
  }

  void DisableMaxPrivilege() {
    disable_max_privilege_ = true;
  }

  ResultCode RestrictSid(WELL_KNOWN_SID_TYPE type);
  void RestrictSid(const Sid &sid);

  void SetJobObjectBasicLimit(DWORD basic_limit) {
    job_object_basic_limit_ |= basic_limit;
  }

  void SetJobObjectUILimit(DWORD ui_limit) {
    job_object_ui_limit_ |= ui_limit;
  }

  ResultCode GetRestrictedToken(HANDLE *out_token);
  ResultCode MakeDesktop(Desktop **out_desktop);
  ResultCode MakeJobObject(JobObject **out_job);

private:
  ResultCode InitRestrictedToken();

private:
  HANDLE restricted_token_;
  bool use_alternate_desktop_;
  bool disable_max_privilege_;
  DWORD job_object_basic_limit_;
  DWORD job_object_ui_limit_;
  std::unique_ptr<Logon> logon_;
  std::vector<Sid> restricted_sids_;
};

}

#endif
