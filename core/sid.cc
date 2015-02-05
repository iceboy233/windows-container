// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/sid.h"

#include <Windows.h>

namespace winc {

ResultCode Sid::Init(PSID_IDENTIFIER_AUTHORITY identifier_authority,
                     DWORD sub_authority) {
  PSID sid;
  if (!::AllocateAndInitializeSid(identifier_authority,
                                  1, sub_authority, 0, 0, 0, 0, 0, 0, 0,
                                  &sid))
    return WINC_ERROR_SID;
  ResultCode rc = Init(sid);
  ::FreeSid(sid);
  return rc;
}

ResultCode Sid::Init(PSID data) {
  if (!::CopySid(sizeof(data_), data_, data))
    return WINC_ERROR_SID;
  return WINC_OK;
}

ResultCode Sid::Init(WELL_KNOWN_SID_TYPE type) {
  DWORD size = sizeof(data_);
  if (!::CreateWellKnownSid(type, NULL, data_, &size))
    return WINC_ERROR_SID;
  return WINC_OK;
}

}
