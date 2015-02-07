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
  Target t;
  ResultCode rc = c.Spawn(exe_path, &t);
  if (rc != WINC_OK) {
    fprintf(stderr, "Spawn error %d\n", rc);
    exit(1);
  }

  ::SetConsoleCtrlHandler(NULL, TRUE);
  rc = t.Start();
  if (rc != WINC_OK) {
    fprintf(stderr, "Start error %d\n", rc);
    exit(1);
  }
  t.WaitForProcess();

  DWORD exit_code;
  ULONG64 time;
  SIZE_T peak_mem;
  t.GetProcessExitCode(&exit_code);
  t.GetJobTime(&time);
  t.GetJobPeakMemory(&peak_mem);
  fprintf(stderr, "PID %" PRIu32 "  EXIT %08" PRIX32 "  "
                  "TIME %" PRIu64 "  MEM %" PRIu64 "\n",
    t.process_id(), exit_code,
    time / 10000, static_cast<uint64_t>(peak_mem));
}
