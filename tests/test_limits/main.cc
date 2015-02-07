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

class MyTarget : public Target {
public:
  MyTarget()
    : Target(true)
    , my_event_(NULL)
    {}

  virtual ~MyTarget() override {
    if (my_event_)
      ::CloseHandle(my_event_);
  }

  ResultCode Init() {
    HANDLE e = ::CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!e)
      return WINC_ERROR_TARGET;
    my_event_ = e;
    return WINC_OK;
  }

  virtual void OnActiveProcessLimit() override {
    fprintf(stderr, "Active process limit exceeded\n");
    ::SetEvent(my_event_);
  }

  virtual void OnMemoryLimit(DWORD process_id) override {
    fprintf(stderr, "Memory limit exceeded, PID = %" PRIu32 "\n",
            process_id);
    ::SetEvent(my_event_);
  }

  ResultCode Wait() {
    if (::WaitForSingleObject(my_event_, INFINITE) == WAIT_FAILED)
      return WINC_ERROR_TARGET;
    return Target::WaitForProcess();
  }

private:
  HANDLE my_event_;
};

int main() {
  wchar_t exe_path[MAX_PATH];
  ::GetModuleFileNameW(NULL, exe_path, MAX_PATH);
  wchar_t *slash = exe_path + wcslen(exe_path);
  while (*--slash != L'\\');
  *++slash = L'\0';
  wcscat_s(exe_path, L"payload_staticalloc.exe");

  Container c;
  MyTarget t;
  ResultCode rc = t.Init();
  if (rc != WINC_OK) {
    fprintf(stderr, "Target error %d\n", rc);
    exit(1);
  }
  SpawnOptions o = {};
  o.memory_limit = 10485760;
  rc = c.Spawn(exe_path, &t, &o);
  if (rc != WINC_OK) {
    fprintf(stderr, "Spawn error %d\n", rc);
    exit(1);
  }

  rc = t.Start();
  if (rc != WINC_OK) {
    fprintf(stderr, "Start error %d\n", rc);
    exit(1);
  }
  t.Wait();

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
