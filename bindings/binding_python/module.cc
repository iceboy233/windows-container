// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Python.h>
#include <Windows.h>

#include "bindings/binding_python/error.h"
#include "bindings/binding_python/container.h"
#include "bindings/binding_python/target.h"
#include "bindings/binding_python/logon.h"
#include "bindings/binding_python/sid.h"

using namespace winc::python;

namespace {

void BuildSidObject(PyObject *module, const char *name,
                    WELL_KNOWN_SID_TYPE type) {
  PyObject *obj = PyObject_CallFunction(reinterpret_cast<PyObject *>(&g_sid_type),
                                        "i", static_cast<int>(type));
  if (!obj)
    return;
  if (PyModule_AddObject(module, name, obj) < 0)
    Py_DECREF(obj);
}

#if PY_MAJOR_VERSION >= 3

struct PyModuleDef winc_module = {
  PyModuleDef_HEAD_INIT,
  "winc",
  NULL,
  -1
};

#endif

}

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_winc() {
#else
PyMODINIT_FUNC initwinc() {
#endif
  InitErrorClass();
  InitContainerType();
  InitTargetType();
  InitLogonTypes();
  InitSidType();

#if PY_MAJOR_VERSION >= 3
  PyObject *module = PyModule_Create(&winc_module);
#else
  PyObject *module = Py_InitModule("winc", NULL);
#endif
  Py_INCREF(g_error_class);
  PyModule_AddObject(module, "Error", g_error_class);
  Py_INCREF(&g_container_type);
  PyModule_AddObject(module, "Container",
                     reinterpret_cast<PyObject *>(&g_container_type));
  Py_INCREF(&g_target_type);
  PyModule_AddObject(module, "Target",
                     reinterpret_cast<PyObject *>(&g_target_type));
  Py_INCREF(&g_logon_type);
  PyModule_AddObject(module, "Logon",
                     reinterpret_cast<PyObject *>(&g_logon_type));
  Py_INCREF(&g_current_logon_type);
  PyModule_AddObject(module, "CurrentLogon",
                     reinterpret_cast<PyObject *>(&g_current_logon_type));
  Py_INCREF(&g_user_logon_type);
  PyModule_AddObject(module, "UserLogon",
                     reinterpret_cast<PyObject *>(&g_user_logon_type));
  Py_INCREF(&g_sid_type);
  PyModule_AddObject(module, "Sid",
                     reinterpret_cast<PyObject *>(&g_sid_type));

  PyModule_AddObject(module, "LOW_INTEGRITY_LEVEL",
                     PyLong_FromUnsignedLong(SECURITY_MANDATORY_LOW_RID));
  PyModule_AddObject(module, "MEDIUM_INTEGRITY_LEVEL",
                     PyLong_FromUnsignedLong(SECURITY_MANDATORY_MEDIUM_RID));
  PyModule_AddObject(module, "HIGH_INTEGRITY_LEVEL",
                     PyLong_FromUnsignedLong(SECURITY_MANDATORY_HIGH_RID));

  BuildSidObject(module, "WinNullSid", WinNullSid);
  BuildSidObject(module, "WinWorldSid", WinWorldSid);
  BuildSidObject(module, "WinInteractiveSid", WinInteractiveSid);
  BuildSidObject(module, "WinAuthenticatedUserSid", WinAuthenticatedUserSid);
  BuildSidObject(module, "WinRestrictedCodeSid", WinRestrictedCodeSid);
  BuildSidObject(module, "WinBuiltinUsersSid", WinBuiltinUsersSid);

#if PY_MAJOR_VERSION >= 3
  return module;
#endif
}
