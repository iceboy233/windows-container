// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_UTIL_H_
#define WINC_CORE_UTIL_H_

#include <Windows.h>
#include <memory>
#include <type_traits>

namespace winc {

struct CloseHandleDeleter {
  void operator()(HANDLE object) const {
    ::CloseHandle(object);
  }
};

typedef std::unique_ptr<std::remove_pointer<HANDLE>::type,
                        CloseHandleDeleter> unique_handle;

}

#endif
