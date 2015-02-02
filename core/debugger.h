// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_DEBUGGER_H_
#define WINC_CORE_DEBUGGER_H_

#include <Windows.h>

#include <winc_types.h>

namespace winc {

DWORD WINAPI DebuggerThread(PVOID param);

class Debugger {
public:
  Debugger()
    : process_id_(0)
    {}
  ~Debugger();

  ResultCode Init(HANDLE process, DWORD process_id);

private:
  friend DWORD WINAPI DebuggerThread(PVOID param);
  void HandleExitProcess(NTSTATUS exit_status);
  void HandleException(const EXCEPTION_RECORD &exception_record);

private:
  DWORD process_id_;

private:
  Debugger(Debugger &) =delete;
  void operator=(Debugger &) =delete;
};

}

#endif
