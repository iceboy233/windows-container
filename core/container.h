// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_CONTAINER_H_
#define WINC_CORE_CONTAINER_H_

#include <Windows.h>
#include <memory>

#include <winc_types.h>

namespace winc {

class Policy;
class Target;

struct IoHandles {
  HANDLE stdin_handle;
  HANDLE stdout_handle;
  HANDLE stderr_handle;
};

struct SpawnOptions {
  wchar_t *command_line;
  uintptr_t processor_affinity;
  uintptr_t memory_limit;
  uint32_t active_process_limit;
};

class Container {
public:
  Container();
  ~Container();
  ResultCode Spawn(const wchar_t *exe_path,
                   Target *target,
                   SpawnOptions *options OPTIONAL,
                   IoHandles *io_handles OPTIONAL);
  
  static ResultCode CreateDefaultPolicy(Policy **out_policy);
  const Policy *policy();
  void set_policy(std::unique_ptr<Policy> &policy);

private:
  std::unique_ptr<Policy> policy_;

private:
  Container(const Container &) = delete;
  void operator=(const Container &) = delete;
};

}

#endif
