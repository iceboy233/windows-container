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
class Debugger;

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
                   SpawnOptions *options OPTIONAL,
                   IoHandles *io_handles OPTIONAL,
                   TargetProcess **out_process);
  
  static ResultCode CreateDefaultPolicy(Policy **out_policy);
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
  TargetProcess(std::unique_ptr<JobObject> &job_object,
                std::unique_ptr<Debugger> &debugger,
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

  ResultCode GetJobTime(ULONG64 *out_time);
  ResultCode GetProcessTime(ULONG64 *out_time);
  ResultCode GetProcessCycle(ULONG64 *out_cycle);
  ResultCode GetJobPeakMemory(SIZE_T *out_size);
  ResultCode GetProcessPeakMemory(SIZE_T *out_size);

private:
  std::unique_ptr<JobObject> job_object_;
  std::unique_ptr<Debugger> debugger_;
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
