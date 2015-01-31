// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_TYPES_H_
#define WINC_TYPES_H_

namespace winc {

// Operation result codes returned by the windows container API.
enum ResultCode {
  WINC_OK = 0,
  WINC_ERROR_SID = 1,
  WINC_ERROR_TOKEN = 2,
  WINC_ERROR_SPAWN = 3,
  WINC_ERROR_DESKTOP = 4,
  WINC_ERROR_JOB_OBJECT = 5,
  WINC_ERROR_RUN = 6,
  WINC_ERROR_UTIL = 7,
  WINC_OS_NOT_SUPPORTED = 10,
};

}

#endif
