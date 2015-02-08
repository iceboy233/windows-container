// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_BINDING_PYTHON_POLICY_H_
#define WINC_BINDING_PYTHON_POLICY_H_

#include <Python.h>
#include <winc.h>

namespace winc {

namespace python {

struct ContainerObject;

struct PolicyObject {
  PyObject_HEAD
  // Reference to the container object
  ContainerObject *container_object;
  // Borrow reference from the container object
  Policy *policy;
};

int InitPolicyType();

extern PyObject *g_policy_type;

}

}


#endif
