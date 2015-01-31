// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/policy.h"

#include <malloc.h>
#include <memory>
#include <vector>

#include "core/desktop.h"
#include "core/job_object.h"
#include "core/logon.h"

using std::make_unique;
using std::vector;

namespace winc {

Policy::~Policy() {
  if (restricted_token_ != NULL)
    ::CloseHandle(restricted_token_);
}

ResultCode Policy::RestrictSid(WELL_KNOWN_SID_TYPE type) {
  restricted_sids_.push_back(Sid());
  ResultCode rc = restricted_sids_.back().Init(type);
  if (rc != WINC_OK)
    restricted_sids_.pop_back();
  return rc;
}

void Policy::RestrictSid(const Sid &sid) {
  restricted_sids_.push_back(sid);
}

ResultCode Policy::InitRestrictedToken() {
  vector<SID_AND_ATTRIBUTES> sids_to_restrict(restricted_sids_.size());
  for (unsigned int i = 0; i < restricted_sids_.size(); ++i) {
    sids_to_restrict[i].Sid = restricted_sids_[i].data();
    sids_to_restrict[i].Attributes = 0;
  }

  HANDLE restricted_token;
  BOOL success = ::CreateRestrictedToken(logon_->GetToken(),
    disable_max_privilege_ ? DISABLE_MAX_PRIVILEGE : 0,
    0, NULL,
    0, NULL,
    sids_to_restrict.size(), sids_to_restrict.data(),
    &restricted_token);

  if (!success)
    return WINC_ERROR_TOKEN;

  restricted_token_ = restricted_token;
  return WINC_OK;
}

ResultCode Policy::GetRestrictedToken(HANDLE *out_token) {
  if (restricted_token_ == NULL) {
    ResultCode rc = InitRestrictedToken();
    if (rc != WINC_OK)
      return rc;
  }

  *out_token = restricted_token_;
  return WINC_OK;
}

ResultCode Policy::MakeDesktop(Desktop **out_desktop) {
  if (!use_alternate_desktop_) {
    *out_desktop = new DefaultDesktop;
  } else {
    auto desktop = make_unique<AlternateDesktop>();
    ResultCode rc = desktop->Init(
        DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_WRITEOBJECTS |
        DESKTOP_SWITCHDESKTOP | READ_CONTROL | WRITE_DAC);
    if (rc != WINC_OK)
      return rc;
    *out_desktop = desktop.release();
  }
  return WINC_OK;
}

ResultCode Policy::MakeJobObject(JobObject **out_job) {
  JobObject *job = new JobObject;
  ResultCode rc = job->Init();
  if (rc != WINC_OK)
    return rc;

  {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit = {};
    limit.BasicLimitInformation.LimitFlags = job_object_basic_limit_;
    rc = job->SetBasicLimit(limit);
    if (rc != WINC_OK)
      return rc;
  }
  {
    JOBOBJECT_BASIC_UI_RESTRICTIONS limit = {};
    limit.UIRestrictionsClass = job_object_ui_limit_;
    rc = job->SetUILimit(limit);
    if (rc != WINC_OK)
      return rc;
  }

  *out_job = job;
  return WINC_OK;
}

}
