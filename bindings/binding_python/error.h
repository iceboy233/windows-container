// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_BINDING_PYTHON_ERROR_H_
#define WINC_BINDING_PYTHON_ERROR_H_

#include <Python.h>
#include <winc_types.h>

namespace winc {

namespace python {

int InitErrorClass();

// Always returns NULL
PyObject *SetErrorFromResultCode(ResultCode rc);

extern PyObject *g_error_class;

}

}

#endif
