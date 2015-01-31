// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_CONTAINER_H_
#define WINC_CORE_CONTAINER_H_

#include <Windows.h>
#include <memory>
#include <cstdint>

#include <winc_types.h>
#include "core/util.h"

namespace winc {

class Policy;
class TargetProcess;
class Desktop;
class JobObject;

struct IoHandles {
  HANDLE stdin_handle;
  HANDLE stdout_handle;
  HANDLE stderr_handle;
};

struct SpawnOptions {
  uint32_t reserved;
  wchar_t *command_line;
  uint32_t time_limit_ms;
  uint32_t memory_limit_kb;
};

class Container {
public:
  Container();
  ~Container();
  ResultCode Spawn(const wchar_t *exe_path,
                   SpawnOptions *options OPTIONAL,
                   IoHandles *io_handles OPTIONAL,
                   TargetProcess **out_process);
  
  static Policy *CreateDefaultPolicy();
  const Policy *policy();
  void set_policy(std::unique_ptr<Policy> &policy);

private:
  std::unique_ptr<Policy> policy_;

private:
  Container(const Container &) =delete;
  void operator=(const Container &) =delete;
};

class TargetProcess {
private:
  friend class Container;
  TargetProcess(std::unique_ptr<Desktop> &desktop,
                std::unique_ptr<JobObject> &job_object,
                DWORD process_id, DWORD thread_id,
                unique_handle &process_handle,
                unique_handle &thread_handle);
public:
  ~TargetProcess();

  DWORD process_id() {
    return process_id_;
  }

  DWORD thread_id() {
    return thread_id_;
  }

  // TODO(iceboy): monitor, status, async iface?
  ResultCode Run();

private:
  std::unique_ptr<Desktop> desktop_;
  std::unique_ptr<JobObject> job_object_;
  DWORD process_id_;
  DWORD thread_id_;
  unique_handle process_handle_;
  unique_handle thread_handle_;

private:
  TargetProcess(const TargetProcess &) =delete;
  void operator=(const TargetProcess &) =delete;
};

}

#endif
