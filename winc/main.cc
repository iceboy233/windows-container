// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Windows.h>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cstdint>
#include <cinttypes>
#include <memory>
#include <vector>
#include <winc.h>

using std::make_shared;
using std::vector;
using namespace winc;

namespace {

void PrintErrorAndExit(ResultCode rc) {
  fwprintf(stderr, L"Winc error: %d\n", rc);
  exit(rc + 100);
}

template <typename T>
T GetArg(wchar_t *argv[], int &arg_index, int argc);

template <>
uint32_t GetArg<uint32_t>(wchar_t *argv[], int &arg_index, int argc) {
  if (arg_index + 1 >= argc) {
    fwprintf(stderr, L"Missing parameter: %ws\n", argv[arg_index]);
    exit(1);
  }
  uint32_t param;
  if (swscanf_s(argv[++arg_index], L"%u", &param) <= 0) {
    int err = errno;
    fwprintf(stderr, L"Integer parameter expected: %ws\n", argv[arg_index]);
    exit(err);
  }
  return param;
}

class MyTarget : public Target {
public:
  virtual void OnActiveProcessLimit() override {
    fwprintf(stderr, L"Active process limit exceeded\n");
  }

  virtual void OnExitAll() override {
    fwprintf(stderr, L"All process exited\n");
  }

  virtual void OnNewProcess(DWORD process_id) override {
    fwprintf(stderr, L"Process created, PID = %u\n", process_id);
  }

  virtual void OnExitProcess(DWORD process_id) override {
    fwprintf(stderr, L"Process exited, PID = %u\n", process_id);
  }

  virtual void OnMemoryLimit(DWORD process_id) override {
    fwprintf(stderr, L"Memory limit exceeded, PID = %u\n", process_id);
  }
};

}

int wmain(int argc, wchar_t *argv[]) {
  Container c;
  Policy *p;
  ResultCode rc = c.GetPolicy(&p);
  if (rc != WINC_OK)
    PrintErrorAndExit(rc);
  SpawnOptions o = {};
  bool verbose = false;
  bool listen = false;

  int arg_index;
  for (arg_index = 1; arg_index < argc; ++arg_index) {
    if (argv[arg_index][0] != L'-')
      break;
    if (!wcscmp(argv[arg_index], L"-v") ||
        !wcscmp(argv[arg_index], L"--verbose")) {
      if (!verbose) {
        verbose = true;
        fwprintf(stderr, L"Enabling verbose mode\n");
      }
      continue;
    }
    if (!wcscmp(argv[arg_index], L"-l") ||
        !wcscmp(argv[arg_index], L"--listen")) {
      if (!listen) {
        listen = true;
        if (verbose)
          fwprintf(stderr, L"Enabling event listener\n");
      }
      continue;
    }
    if (!wcscmp(argv[arg_index], L"--use-desktop")) {
      if (!p->use_desktop()) {
        p->set_use_desktop(true);
        if (verbose)
          fwprintf(stderr, L"Using alternate desktop\n");
      }
      continue;
    }
    if (!wcscmp(argv[arg_index], L"--use-logon")) {
      auto current_logon = make_shared<CurrentLogon>();
      rc = current_logon->Init(SECURITY_MANDATORY_LOW_RID);
      if (rc != WINC_OK)
        PrintErrorAndExit(rc);
      Sid *current_logon_sid;
      rc = current_logon->GetUserSid(&current_logon_sid);
      if (rc != WINC_OK)
        PrintErrorAndExit(rc);
      auto &restricted_sids = p->restricted_sids();
      if (find(restricted_sids.begin(),
               restricted_sids.end(),
               *current_logon_sid) == restricted_sids.end()) {
        p->AddRestrictSid(*current_logon_sid);
      }
      p->SetLogon(current_logon);
      continue;
    }
    if (!wcscmp(argv[arg_index], L"--active-process")) {
      o.active_process_limit = GetArg<uint32_t>(argv, arg_index, argc);
      if (verbose)
        fwprintf(stderr, L"Setting active process limit to %u\n",
                 o.active_process_limit);
      continue;
    }
    if (!wcscmp(argv[arg_index], L"-m") ||
        !wcscmp(argv[arg_index], L"--memory")) {
      o.memory_limit = GetArg<uint32_t>(argv, arg_index, argc);
      if (verbose)
        fwprintf(stderr, L"Setting memory limit to %" PRIuPTR "\n",
                 o.memory_limit);
      continue;
    }
    if (!wcscmp(argv[arg_index], L"--affinity")) {
      o.processor_affinity = GetArg<uint32_t>(argv, arg_index, argc);
      if (verbose)
        fwprintf(stderr, L"Setting processor affinity to %" PRIuPTR "\n",
                 o.processor_affinity);
      continue;
    }

    fwprintf(stderr, L"Unknown argument: %ws\n", argv[arg_index]);
    exit(1);
  }

  if (arg_index >= argc) {
    fwprintf(stderr, L"Missing command line\n");
    exit(1);
  }

  // Find the application path
  wchar_t name_buffer[MAX_PATH];
  if (!::SearchPathW(NULL, argv[arg_index], L".exe",
                     MAX_PATH, name_buffer, NULL) ||
      GetFileAttributesW(name_buffer) & FILE_ATTRIBUTE_DIRECTORY) {
    fwprintf(stderr, L"Application path not found\n");
    exit(1);
  }
  if (verbose)
    fwprintf(stderr, L"Application path: %ws\n", name_buffer);

  // Build command line
  vector<wchar_t> command_line;
  for (; arg_index < argc; ++arg_index) {
    bool quoted = false;
    if (wcschr(argv[arg_index], L' ')) {
      quoted = true;
      command_line.push_back(L'\"');
    }
    for (wchar_t *p = argv[arg_index]; *p; ++p) {
      if (*p == '\"')
        command_line.push_back('\\');
      command_line.push_back(*p);
    }
    if (quoted)
      command_line.push_back(L'\"');
    if (arg_index + 1 < argc) {
      command_line.push_back(L' ');
    } else {
      command_line.push_back(L'\0');
    }
  }
  o.command_line = command_line.data();
  if (verbose)
    fwprintf(stderr, L"Command line: %ws\n", command_line.data());

  MyTarget t;
  rc = c.Spawn(name_buffer, &t, &o);
  if (rc != WINC_OK)
    PrintErrorAndExit(rc);
  ::SetConsoleCtrlHandler(NULL, TRUE);
  rc = t.Start(listen);
  if (rc != WINC_OK)
    PrintErrorAndExit(rc);
  // TODO(iceboy): In listen mode, events after process exit will be discarded
  rc = t.WaitForProcess();
  if (rc != WINC_OK)
    PrintErrorAndExit(rc);
  DWORD exit_code;
  rc = t.GetProcessExitCode(&exit_code);
  if (rc != WINC_OK)
    PrintErrorAndExit(rc);
  if (verbose) {
    fwprintf(stderr, L"Process terminated with code %u\n", exit_code);
  }
  return exit_code;
}
