// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_BINDING_PYTHON_TARGET_H_
#define WINC_BINDING_PYTHON_TARGET_H_

#include <Python.h>
#include <winc.h>

namespace winc {

namespace python {

struct ContainerObject;

class TargetDirector : public Target {
public:
  TargetDirector()
    : Target(true)
    {}
  virtual ~TargetDirector() = default;

protected:
  virtual void OnActiveProcessLimit() override;
  virtual void OnExitAll() override;
  virtual void OnNewProcess(DWORD process_id) override;
  virtual void OnExitProcess(DWORD process_id) override;
  virtual void OnMemoryLimit(DWORD process_id) override;
};

struct TargetObject {
  PyObject_HEAD
  TargetDirector target;
  // Reference to the container object
  ContainerObject *container_object;
};

int InitTargetType();

extern PyTypeObject g_target_type;

}

}

#endif
