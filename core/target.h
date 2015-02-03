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
  ResultCode Init(DWORD process_id,
                  std::unique_ptr<JobObject> &job_object,
                  unique_handle &process_handle,
                  unique_handle &thread_handle);

public:
  DWORD process_id() {
    return process_id_;
  }

  ResultCode Start();
  ResultCode GetJobTime(ULONG64 *out_time);
  ResultCode GetProcessTime(ULONG64 *out_time);
  ResultCode GetProcessCycle(ULONG64 *out_cycle);
  ResultCode GetJobPeakMemory(SIZE_T *out_size);
  ResultCode GetProcessPeakMemory(SIZE_T *out_size);
  ResultCode GetProcessExitCode(DWORD *out_code);

protected:
  virtual ResultCode Init() {
    return WINC_OK;
  }

  ResultCode ListenToEvents();
  friend class JobObject;
  virtual void OnNewProcess(DWORD process_id) {};
  virtual void OnExitProcess(DWORD process_id) {};
  virtual void OnExitAll() {};

private:
  std::unique_ptr<JobObject> job_object_;
  DWORD process_id_;
  unique_handle process_handle_;
  unique_handle thread_handle_;

private:
  Target(const Target &) = delete;
  void operator=(const Target &) = delete;
};

class WaitableTarget : public Target {
public:
  WaitableTarget()
    : exit_all_event_(NULL)
    {}

  virtual ~WaitableTarget() override;
  ResultCode Wait(DWORD timeout);
  ResultCode Wait() {
    return Wait(INFINITE);
  }

protected:
  virtual ResultCode Init() override;
  virtual void OnExitAll() override;

private:
  HANDLE exit_all_event_;
};

}

#endif
