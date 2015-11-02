// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_BINDING_PYTHON_CONTAINER_H_
#define WINC_BINDING_PYTHON_CONTAINER_H_

#include <Python.h>
#include <winc.h>

namespace winc {

namespace python {

struct ContainerObject {
  PyObject_HEAD
  Container container;
  // Borrow reference from the container
  Policy *policy;
};

int InitContainerType();

extern PyTypeObject g_container_type;

}

}

#endif
