// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_TARGET_H_
#define WINC_CORE_TARGET_H_

#include <Windows.h>
#include <memory>

#include <winc_types.h>
#include "core/util.h"

namespace winc {

class Container;
class JobObject;

class Target {
public:
  Target();
  virtual ~Target();

private:
  friend class Container;
  void Assign(DWORD process_id, std::unique_ptr<JobObject> &job_object,
              unique_handle &process_handle, unique_handle &thread_handle);

public:
  DWORD process_id() {
    return process_id_;
  }

  ResultCode Start() {
    return Start(false);
  }

  ResultCode Start(bool listen);
  ResultCode WaitForProcess();
  ResultCode TerminateJob(UINT exit_code);
  ResultCode GetJobTime(ULONG64 *out_time);
  ResultCode GetProcessTime(ULONG64 *out_time);
  ResultCode GetProcessCycle(ULONG64 *out_cycle);
  ResultCode GetJobPeakMemory(SIZE_T *out_size);
  ResultCode GetProcessPeakMemory(SIZE_T *out_size);
  ResultCode GetProcessExitCode(DWORD *out_code);

protected:
  friend class JobObject;
  virtual void OnActiveProcessLimit() {}
  virtual void OnExitAll() {}
  virtual void OnNewProcess(DWORD process_id) {}
  virtual void OnExitProcess(DWORD process_id) {}
  virtual void OnMemoryLimit(DWORD process_id) {}

private:
  bool listening_;
  DWORD process_id_;
  std::unique_ptr<JobObject> job_object_;
  unique_handle process_handle_;
  unique_handle thread_handle_;

private:
  Target(const Target &) = delete;
  void operator=(const Target &) = delete;
};

}

#endif
