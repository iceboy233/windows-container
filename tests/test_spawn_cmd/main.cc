// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cinttypes>

#include <winc.h>

using namespace winc;

int main() {
  wchar_t cmd_path[MAX_PATH];
  ::GetSystemDirectoryW(cmd_path, MAX_PATH);
  wcscat_s(cmd_path, L"\\cmd.exe");

  Container c;
  TargetProcess *process;
  ResultCode rc = c.Spawn(cmd_path, nullptr, nullptr, &process);
  if (rc != WINC_OK) {
    fprintf(stderr, "Spawn error %d\n", rc);
    exit(1);
  }

  ::SetConsoleCtrlHandler(NULL, TRUE);
  fprintf(stderr, "PID %" PRIu32 " CREATED\n", process->process_id());
  rc = process->Run();
  if (rc != WINC_OK) {
    fprintf(stderr, "Run error %d\n", rc);
    exit(1);
  }

  ULONG64 time;
  SIZE_T peak_mem;
  process->GetJobTime(&time);
  process->GetJobPeakMemory(&peak_mem);
  fprintf(stderr, "PID %" PRIu32 "  TIME %" PRIu64 "  MEM %" PRIu64 "\n",
    process->process_id(), time / 10000, static_cast<uint64_t>(peak_mem));
  delete process;
}
