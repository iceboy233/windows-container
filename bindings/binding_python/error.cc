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
  const char *desc = "unknown error";
  switch (rc) {
  case WINC_ERROR_SID:
    desc = "SID error";
    break;
  case WINC_ERROR_LOGON:
    desc = "logon error";
    break;
  case WINC_ERROR_SPAWN:
    desc = "spawn error";
    break;
  case WINC_ERROR_DESKTOP:
    desc = "desktop error";
    break;
  case WINC_ERROR_JOB_OBJECT:
    desc = "job object error";
    break;
  case WINC_ERROR_TARGET:
    desc = "target error";
    break;
  case WINC_ERROR_UTIL:
    desc = "util error";
    break;
  case WINC_ERROR_COMPLETION_PORT:
    desc = "completion port error";
    break;
  case WINC_PRIVILEGE_NOT_HELD:
    desc = "privilege not held";
    break;
  }
  return PyErr_Format(g_error_class, "%s (%d)", desc, rc);
}

}

}
