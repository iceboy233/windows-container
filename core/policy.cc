// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <winc/policy.h>

#include <Windows.h>
#include <algorithm>
#include <memory>
#include <vector>

#include <winc/desktop.h>
#include <winc/logon.h>
#include "core/job_object.h"

using std::make_shared;
using std::make_unique;
using std::remove;
using std::shared_ptr;
using std::vector;

namespace winc {

ResultCode Policy::GetLogon(shared_ptr<Logon> *out_logon) {
  if (!logon_) {
    auto logon = make_shared<CurrentLogon>();
    ResultCode rc = logon->Init(SECURITY_MANDATORY_LOW_RID);
    if (rc != WINC_OK)
      return rc;
    logon_ = move(logon);
  }
  *out_logon = logon_;
  return WINC_OK;
}

void Policy::SetLogon(const shared_ptr<Logon> &logon) {
  logon_ = logon;
  restricted_token_.reset();
  alternate_desktop_.reset();
}

void Policy::AddRestrictSid(const Sid &sid) {
  restricted_sids_.push_back(sid);
  restricted_token_.reset();
}

void Policy::RemoveRestrictSid(const Sid &sid) {
  restricted_sids_.erase(remove(restricted_sids_.begin(),
                                restricted_sids_.end(), sid),
                         restricted_sids_.end());
  restricted_token_.reset();
}

ResultCode Policy::GetRestrictedToken(HANDLE *out_token) {
  if (!restricted_token_) {
    shared_ptr<Logon> logon;
    ResultCode rc = GetLogon(&logon);
    if (rc != WINC_OK)
      return rc;
    vector<SID_AND_ATTRIBUTES> sids_to_restrict(restricted_sids_.size());
    for (unsigned int i = 0; i < restricted_sids_.size(); ++i) {
      sids_to_restrict[i].Sid = restricted_sids_[i].data();
      sids_to_restrict[i].Attributes = 0;
    }
    HANDLE restricted_token;
    rc = logon->FilterToken(sids_to_restrict.data(),
                            static_cast<DWORD>(sids_to_restrict.size()),
                            &restricted_token);
    if (rc != WINC_OK)
      return rc;
    restricted_token_.reset(restricted_token);
  }
  *out_token = restricted_token_.get();
  return WINC_OK;
}

ResultCode Policy::GetDesktop(Desktop **out_desktop) {
  if (!use_desktop_) {
    if (!default_desktop_) {
      default_desktop_.reset(new DefaultDesktop);
    }
    *out_desktop = default_desktop_.get();
  } else {
    if (!alternate_desktop_) {
      shared_ptr<Logon> logon;
      ResultCode rc = GetLogon(&logon);
      if (rc != WINC_OK)
        return rc;
      auto d = make_unique<AlternateDesktop>();
      rc = d->Init(DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW |
                   DESKTOP_WRITEOBJECTS | DESKTOP_SWITCHDESKTOP |
                   READ_CONTROL | WRITE_DAC | WRITE_OWNER);
      if (rc != WINC_OK)
        return rc;
      rc = logon->GrantAccess(d->GetDesktopHandle(), SE_WINDOW_OBJECT,
                               GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
      if (rc != WINC_OK)
        return rc;
      rc = logon->GrantAccess(d->GetWinstaHandle(), SE_WINDOW_OBJECT,
                               GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
      if (rc != WINC_OK)
        return rc;
      alternate_desktop_ = move(d);
    }
    *out_desktop = alternate_desktop_.get();
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
    limit.BasicLimitInformation.LimitFlags = job_basic_limit_;
    rc = job->SetBasicLimit(limit);
    if (rc != WINC_OK)
      return rc;
  }
  {
    JOBOBJECT_BASIC_UI_RESTRICTIONS limit = {};
    limit.UIRestrictionsClass = job_ui_limit_;
    rc = job->SetUILimit(limit);
    if (rc != WINC_OK)
      return rc;
  }

  *out_job = job;
  return WINC_OK;
}

}
