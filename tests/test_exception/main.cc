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
  wchar_t exe_path[MAX_PATH];
  ::GetModuleFileNameW(NULL, exe_path, MAX_PATH);
  wchar_t *slash = exe_path + wcslen(exe_path);
  while (*--slash != L'\\');
  *++slash = L'\0';
  wcscat_s(exe_path, L"payload_exception.exe");

  Container c;
  TargetProcess *process;
  ResultCode rc = c.Spawn(exe_path, nullptr, nullptr, &process);
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
