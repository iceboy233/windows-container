// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_POLICY_H_
#define WINC_CORE_POLICY_H_

#include <Windows.h>
#include <memory>
#include <vector>

#include "core/desktop.h"
#include "core/sid.h"

namespace winc {

class JobObject;
class Logon;

class Policy {
public:
  explicit Policy(std::unique_ptr<Logon> &logon)
    : restricted_token_(NULL)
    , desktop_(new DefaultDesktop)
    , job_object_basic_limit_(0)
    , job_object_ui_limit_(0)
    , logon_(move(logon))
    {}

  ~Policy();

  ResultCode UseAlternateDesktop();
  const Desktop &desktop() const {
    return *desktop_.get();
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
  ResultCode MakeJobObject(JobObject **out_job);

private:
  ResultCode InitRestrictedToken();
  ResultCode InitDesktop();

private:
  HANDLE restricted_token_;
  std::unique_ptr<Desktop> desktop_;
  DWORD job_object_basic_limit_;
  DWORD job_object_ui_limit_;
  std::unique_ptr<Logon> logon_;
  std::vector<Sid> restricted_sids_;
};

}

#endif
