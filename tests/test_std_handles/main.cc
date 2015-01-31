// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <memory>

#include <winc.h>
#include "core/util.h"

using namespace std;
using namespace winc;

namespace {

struct FPDeleter {
  void operator()(FILE *fp) const {
    fclose(fp);
  }
};

typedef unique_ptr<FILE, FPDeleter> unique_fp;

}

int call_aplusb(Container &c, const wchar_t *exe_path, int a, int b) {
  HANDLE stdin_pipe[2], stdout_pipe[2];
  SECURITY_ATTRIBUTES sa = {};
  sa.bInheritHandle = TRUE;
  if (!::CreatePipe(&stdin_pipe[0], &stdin_pipe[1], &sa, 0))
    return -1;
  unique_handle stdin_pipe_holder(stdin_pipe[0]);
  unique_fp writefp(_fdopen(
    _open_osfhandle(reinterpret_cast<intptr_t>(stdin_pipe[1]), 0),
    "w"));
  if (!::CreatePipe(&stdout_pipe[0], &stdout_pipe[1], &sa, 0))
    return -1;
  unique_fp readfp(_fdopen(
    _open_osfhandle(reinterpret_cast<intptr_t>(stdout_pipe[0]), 0),
    "r"));
  unique_handle stdout_pipe_holder(stdout_pipe[1]);

  IoHandles io_handles = {};
  io_handles.stdin_handle = stdin_pipe[0];
  io_handles.stdout_handle = stdout_pipe[1];
  TargetProcess *process;
  ResultCode rc = c.Spawn(exe_path, nullptr, &io_handles, &process);
  if (rc != WINC_OK)
    return -1;
  unique_ptr<TargetProcess> process_holder(process);
  stdin_pipe_holder.reset();
  stdout_pipe_holder.reset();

  // TODO(iceboy): Potential deadlock, fix it when the async iface is ready
  if (fprintf(writefp.get(), "%d %d\n", a, b) == -1)
    return -1;
  writefp.reset();

  rc = process->Run();
  if (rc != WINC_OK)
    return -1;

  int ret;
  if (fscanf_s(readfp.get(), "%d", &ret) == EOF)
    return -1;
  return ret;
}

int main() {
  wchar_t exe_path[MAX_PATH];
  ::GetModuleFileNameW(NULL, exe_path, MAX_PATH);
  wchar_t *slash = exe_path + wcslen(exe_path);
  while (*--slash != L'\\');
  *++slash = L'\0';
  wcscat_s(exe_path, L"payload_aplusb.exe");

  Container c;
  printf("%d\n", call_aplusb(c, exe_path, 1, 2));
}
