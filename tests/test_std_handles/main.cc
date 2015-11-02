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

  SpawnOptions options = {};
  options.stdin_handle = stdin_pipe[0];
  options.stdout_handle = stdout_pipe[1];
  Target t;
  ResultCode rc = c.Spawn(exe_path, &t, &options);
  if (rc != WINC_OK)
    return -1;
  stdin_pipe_holder.reset();
  stdout_pipe_holder.reset();
  rc = t.Start();
  if (rc != WINC_OK)
    return -1;
  if (fprintf(writefp.get(), "%d %d\n", a, b) == -1)
    return -1;
  writefp.reset();
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
  srand(::GetTickCount());

  Container c;
  while (true) {
    unsigned int spawned = 0;
    DWORD start = ::GetTickCount();
    do {
      int a = rand(), b = rand();
      int ret = call_aplusb(c, exe_path, a, b);
      if (ret != a + b) {
        fprintf(stderr, "Math error!\n");
        exit(1);
      }
      ++spawned;
    } while (::GetTickCount() - start < 1000);
    printf("Spawned %u process in 1 second\n", spawned);
  }
}
