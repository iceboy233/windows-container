// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_CONTAINER_H_
#define WINC_CORE_CONTAINER_H_

#include <Windows.h>
#include <memory>

#include <winc_types.h>
#include "core/policy.h"

namespace winc {

class Target;

// The options in this structure are all optional
struct SpawnOptions {

  // The command line to be executed.
  // This parameter cannot be a pointer to read-only memory according to
  // the Windows API requirement
  wchar_t *command_line;

  uintptr_t processor_affinity;
  uintptr_t memory_limit;
  uint32_t active_process_limit;

  // Handles for redirecting standard I/O, if any of these three handles are
  // specified, the standard I/O is redirected.
  // The specified handles must be set as inheritable
  HANDLE stdin_handle;
  HANDLE stdout_handle;
  HANDLE stderr_handle;
};

class Container {
public:
  ResultCode Spawn(const wchar_t *exe_path, Target *target) {
    return Spawn(exe_path, target, nullptr);
  }

  ResultCode Spawn(const wchar_t *exe_path, Target *target,
                   SpawnOptions *options);

  // Returns a borrow reference of the mutable policy
  ResultCode GetPolicy(Policy **out_policy);

private:
  std::unique_ptr<Policy> policy_;
};

}

#endif
