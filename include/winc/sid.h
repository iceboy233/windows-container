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
  // Create SID with one sub-authority
  ResultCode Init(PSID_IDENTIFIER_AUTHORITY identifier_authority,
                  DWORD sub_authority);

  // Create SID object from existing raw SID data
  ResultCode Init(PSID data);

  // Create SID object from well-known type identifier
  ResultCode Init(WELL_KNOWN_SID_TYPE type);

  // Access raw data of the SID, which should not be modified
  // Return non-const pointer for compatibility with the Windows API
  PSID data() const {
    return reinterpret_cast<PSID>(const_cast<BYTE *>(data_));
  }

  // Get the length of the SID
  DWORD GetLength() const {
    return ::GetLengthSid(data());
  }

  bool operator==(const Sid &other) const {
    return ::EqualSid(data(), other.data()) != 0;
  }

  bool operator!=(const Sid &other) const {
    return !operator==(other);
  }

private:
  BYTE data_[SECURITY_MAX_SID_SIZE];
};

}

#endif
