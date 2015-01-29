// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_SID_H_
#define WINC_CORE_SID_H_

#include <Windows.h>

#include <winc_types.h>

namespace winc {

class Sid {
public:
  // Create SID object from existing raw SID data
  void Init(SID *data);

  // Create SID object from well-known type identifier
  ResultCode Init(WELL_KNOWN_SID_TYPE type);

  // Access raw data of the SID, which should not be modified
  // Return non-const pointer for compatibility with the Windows API
  SID *data() const {
    return reinterpret_cast<SID *>(const_cast<BYTE *>(data_));
  }

private:
  BYTE data_[SECURITY_MAX_SID_SIZE];
};

}

#endif
