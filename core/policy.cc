// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/policy.h"

#include <malloc.h>
#include <memory>
#include <vector>

#include "core/desktop.h"
#include "core/job_object.h"

using std::make_unique;
using std::vector;

namespace winc {

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

ResultCode Policy::GetSidLogonSession(Sid *out_sid) {
  // TODO(iceboy): Cache the SID if possible

  HANDLE token;
  if (!CreateEffectiveToken(&token))
    return WINC_ERROR_TOKEN;

  DWORD size;
  if (!::GetTokenInformation(token, TokenGroups, NULL, 0, &size) &&
      ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    ::CloseHandle(token);
    return WINC_ERROR_TOKEN;
  }

  TOKEN_GROUPS *info = reinterpret_cast<TOKEN_GROUPS *>(_malloca(size));
  if (::GetTokenInformation(token, TokenGroups, info, size, &size)) {
    for (unsigned int i = 0; i < info->GroupCount; ++i) {
      if (info->Groups[i].Attributes & SE_GROUP_LOGON_ID) {
        out_sid->Init(reinterpret_cast<SID *>(info->Groups[i].Sid));
        _freea(info);
        ::CloseHandle(token);
        return WINC_OK;
      }
    }
  }

  _freea(info);
  ::CloseHandle(token);
  return WINC_ERROR_TOKEN;
}

ResultCode Policy::CreateRestrictedToken(HANDLE *out_token) {
  // TODO(iceboy): Cache the token if possible

  HANDLE effective_token;
  if (!CreateEffectiveToken(&effective_token))
    return WINC_ERROR_TOKEN;

  vector<SID_AND_ATTRIBUTES> sids_to_restrict(restricted_sids_.size());
  for (unsigned int i = 0; i < restricted_sids_.size(); ++i) {
    sids_to_restrict[i].Sid = restricted_sids_[i].data();
    sids_to_restrict[i].Attributes = 0;
  }

  HANDLE restricted_token;
  BOOL success = ::CreateRestrictedToken(effective_token,
    disable_max_privilege_ ? DISABLE_MAX_PRIVILEGE : 0,
    0, NULL,
    0, NULL,
    sids_to_restrict.size(), sids_to_restrict.data(),
    &restricted_token);

  ::CloseHandle(effective_token);

  if (!success)
    return WINC_ERROR_TOKEN;

  *out_token = restricted_token;
  return WINC_OK;
}

ResultCode Policy::CreateTargetDesktop(Desktop **out_desktop) {
  if (!use_alternate_desktop_) {
    *out_desktop = new DefaultDesktop;
  } else {
    auto desktop = make_unique<AlternateDesktop>();
    Sid logon_sid;
    ResultCode rc = GetSidLogonSession(&logon_sid);
    rc = desktop->Init();
    if (rc != WINC_OK)
      return rc;
    *out_desktop = desktop.release();
  }
  return WINC_OK;
}

ResultCode Policy::CreateTargetJobObject(JobObject **out_job) {
  JobObject *job = new JobObject;
  ResultCode rc = job->Init();
  if (rc != WINC_OK)
    return rc;

  // TODO(iceboy): Configure the job object
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit;
  job->GetLimit(&limit);
  limit.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  job->SetLimit(limit);

  *out_job = job;
  return WINC_OK;
}

bool Policy::CreateEffectiveToken(HANDLE *out_token) {
  // TODO(iceboy): Use the logon token if present

  if (!::OpenProcessToken(::GetCurrentProcess(),
    TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
    out_token))
    return false;
  return true;
}

}
