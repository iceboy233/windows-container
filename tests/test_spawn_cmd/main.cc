// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Windows.h>
#include <cstring>

#include <winc.h>

using winc::Container;
using winc::TargetProcess;

int main() {
  Container c;
  wchar_t cmd_path[MAX_PATH];
  ::GetSystemDirectoryW(cmd_path, MAX_PATH);
  ::wcscat_s(cmd_path, L"\\cmd.exe");
  TargetProcess *process;
  c.Spawn(cmd_path, nullptr, nullptr, &process);
}
