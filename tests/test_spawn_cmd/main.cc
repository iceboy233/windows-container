// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Windows.h>
#include <cstring>
#include <cstdio>

#include <winc.h>

using namespace winc;

int main() {
  Container c;
  wchar_t cmd_path[MAX_PATH];
  ::GetSystemDirectoryW(cmd_path, MAX_PATH);
  ::wcscat_s(cmd_path, L"\\cmd.exe");
  TargetProcess *process;
  ResultCode rc = c.Spawn(cmd_path, nullptr, nullptr, &process);
  if (rc != WINC_OK) {
    fprintf(stderr, "Spawn error %d\n", rc);
    return 1;
  }
  fprintf(stderr, "Process %d created\n", process->process_id());
  process->Run();
  fprintf(stderr, "Process %d exited\n", process->process_id());
  delete process;
}
