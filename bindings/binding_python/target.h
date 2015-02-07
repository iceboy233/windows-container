// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_BINDING_PYTHON_TARGET_H_
#define WINC_BINDING_PYTHON_TARGET_H_

#include <Python.h>
#include <winc.h>

namespace winc {

namespace python {

class TargetDirector : public Target {
public:
  TargetDirector()
    : Target(true)
    {}
};

struct TargetObject {
  PyObject_HEAD
  TargetDirector target;
};

int InitTargetType();

extern PyObject *g_target_type;

}

}

#endif
