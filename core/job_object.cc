// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/job_object.h"

namespace winc {

JobObject::~JobObject() {
  ::CloseHandle(job_);
}

ResultCode JobObject::Init() {
  HANDLE job = ::CreateJobObjectW(NULL, NULL);
  if (!job)
    return WINC_ERROR_JOB_OBJECT;
  job_ = job;
  return WINC_OK;
}

ResultCode JobObject::AssignProcess(HANDLE process) {
  if (!::AssignProcessToJobObject(job_, process))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

ResultCode JobObject::GetBasicLimit(JOBOBJECT_EXTENDED_LIMIT_INFORMATION *limit) {
  if (!::QueryInformationJobObject(job_, JobObjectExtendedLimitInformation,
      limit, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION), NULL))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

ResultCode JobObject::SetBasicLimit(
    const JOBOBJECT_EXTENDED_LIMIT_INFORMATION &limit) {
  if (!::SetInformationJobObject(job_, JobObjectExtendedLimitInformation,
      const_cast<JOBOBJECT_EXTENDED_LIMIT_INFORMATION *>(&limit),
      sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

ResultCode JobObject::GetUILimit(JOBOBJECT_BASIC_UI_RESTRICTIONS *limit) {
  if (!::QueryInformationJobObject(job_, JobObjectBasicUIRestrictions,
      limit, sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS), NULL))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}


ResultCode JobObject::SetUILimit(
    const JOBOBJECT_BASIC_UI_RESTRICTIONS &ui_limit) {
  if (!::SetInformationJobObject(job_, JobObjectBasicUIRestrictions,
      const_cast<JOBOBJECT_BASIC_UI_RESTRICTIONS *>(&ui_limit),
      sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS)))
    return WINC_ERROR_JOB_OBJECT;
  return WINC_OK;
}

}
