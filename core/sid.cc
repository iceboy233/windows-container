// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/sid.h"

namespace winc {

ResultCode Sid::Init(WELL_KNOWN_SID_TYPE type) {
  DWORD size = sizeof(data_);
  if (!::CreateWellKnownSid(type, NULL, data_, &size))
    return WINC_INVALID_SID_TYPE;
  return WINC_OK;
}

}
