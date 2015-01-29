// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <winc_types.h>

namespace winc {

class Container {
public:
  Container() {}
  ~Container() {}
  ResultCode Spawn(const wchar_t *command_line,
                   const wchar_t *exe_path = nullptr/*,
                   std handles,
                   process information*/);

private:
  Container(const Container &)=delete;
  void operator=(const Container &)=delete;
};

}
