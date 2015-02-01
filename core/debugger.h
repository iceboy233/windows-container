// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_DEBUGGER_H_
#define WINC_CORE_DEBUGGER_H_

#include <Windows.h>

#include <winc_types.h>

namespace winc {

class Debugger {
public:
  Debugger() =default;
  ~Debugger() =default;

  ResultCode Init(HANDLE process);

private:
  Debugger(Debugger &) =delete;
  void operator=(Debugger &) =delete;
};

}

#endif
