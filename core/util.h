// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_UTIL_H_
#define WINC_CORE_UTIL_H_

#include <Windows.h>
#include <memory>
#include <type_traits>

#include <winc_types.h>

namespace winc {

struct CloseHandleDeleter {
  void operator()(HANDLE object) const {
    ::CloseHandle(object);
  }
};

typedef std::unique_ptr<std::remove_pointer<HANDLE>::type,
                        CloseHandleDeleter> unique_handle;

class ProcThreadAttributeList {
public:
  ProcThreadAttributeList()
    : data_(nullptr)
    {}

  ~ProcThreadAttributeList();

  ResultCode Init(DWORD attribute_count, DWORD flags);
  ResultCode Update(DWORD flags, DWORD_PTR attribute,
                    PVOID value, SIZE_T size);

  LPPROC_THREAD_ATTRIBUTE_LIST data() const {
    return data_;
  }

private:
  LPPROC_THREAD_ATTRIBUTE_LIST data_;
  
private:
  ProcThreadAttributeList(const ProcThreadAttributeList &) =delete;
  void operator=(const ProcThreadAttributeList &) =delete;
};

}

#endif
