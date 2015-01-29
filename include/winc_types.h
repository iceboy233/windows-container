// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_TYPES_H_
#define WINC_TYPES_H_

namespace winc {

// Operation result codes returned by the windows container API.
enum ResultCode {
  WINC_OK = 0,
  WINC_INVALID_SID = 1,
  WINC_INVALID_TOKEN = 2,
  WINC_SPAWN_ERROR = 3,
  WINC_OS_NOT_SUPPORTED = 4,
};

}

#endif
