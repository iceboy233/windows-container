// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/binding_python/error.h"

#include <Python.h>
#include <winc_types.h>

namespace winc {

namespace python {

PyObject *g_error_class;

int InitErrorClass() {
  PyObject *error_class = PyErr_NewException("winc.Error", NULL, NULL);
  if (!error_class)
    return -1;
  g_error_class = error_class;
  return 0;
}

PyObject *SetErrorFromResultCode(ResultCode rc) {
  return PyErr_Format(g_error_class, "Winc error %d", rc);
}

}

}
