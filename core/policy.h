// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_POLICY_H_
#define WINC_CORE_POLICY_H_

#include <Windows.h>
#include <memory>
#include <vector>

#include "core/desktop.h"
#include "core/logon.h"
#include "core/util.h"

namespace winc {

class Container;
class JobObject;
class Sid;

class Policy {
public:
  Policy()
    : use_desktop_(false)
    , job_basic_limit_(0)
    , job_ui_limit_(0)
    {}

public:
  ResultCode GetLogon(Logon **out_logon);
  void SetLogon(std::unique_ptr<Logon> &logon);
  void AddRestrictSid(const Sid &sid);
  void RemoveRestrictSid(const Sid &sid);

public:
  const std::vector<Sid> &restricted_sids() {
    return restricted_sids_;
  }

  bool use_desktop() {
    return use_desktop_;
  }

  void set_use_desktop(bool use) {
    use_desktop_ = use;
  }

  DWORD job_basic_limit() {
    return job_basic_limit_;
  }

  void set_job_basic_limit(DWORD basic_limit) {
    job_basic_limit_ = basic_limit;
  }

  DWORD job_ui_limit() {
    return job_ui_limit_;
  }

  void set_job_ui_limit(DWORD ui_limit) {
    job_ui_limit_ = ui_limit;
  }

private:
  friend class Container;
  // Get a restricted token, returns borrow reference
  ResultCode GetRestrictedToken(HANDLE *out_token);
  // Get a desktop, returns borrow reference
  ResultCode GetDesktop(Desktop **out_desktop);
  // Make a job object, returns new reference
  ResultCode MakeJobObject(JobObject **out_job);

private:
  unique_handle restricted_token_;
  bool use_desktop_;
  DWORD job_basic_limit_;
  DWORD job_ui_limit_;
  std::unique_ptr<DefaultDesktop> default_desktop_;
  std::unique_ptr<AlternateDesktop> alternate_desktop_;
  std::unique_ptr<Logon> logon_;
  std::vector<Sid> restricted_sids_;
};

}

#endif
