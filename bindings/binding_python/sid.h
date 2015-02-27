// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_BINDING_PYTHON_SID_H_
#define WINC_BINDING_PYTHON_SID_H_

#include <Python.h>
#include <winc.h>

namespace winc {

namespace python {

struct SidObject {
  PyObject_HEAD
  Sid sid;
};

int InitSidType();

extern PyTypeObject g_sid_type;

}

}

#endif
