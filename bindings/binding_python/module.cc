// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Python.h>

#include "bindings/binding_python/error.h"
#include "bindings/binding_python/container.h"
#include "bindings/binding_python/target.h"
#include "bindings/binding_python/policy.h"

using namespace winc::python;

PyMODINIT_FUNC initwinc() {
  InitErrorClass();
  InitContainerType();
  InitTargetType();
  InitPolicyType();

  PyObject *module = Py_InitModule("winc", NULL);
  Py_INCREF(g_error_class);
  PyModule_AddObject(module, "Error", g_error_class);
  Py_INCREF(g_container_type);
  PyModule_AddObject(module, "Container", g_container_type);
  Py_INCREF(g_target_type);
  PyModule_AddObject(module, "Target", g_target_type);
  Py_INCREF(g_policy_type);
  PyModule_AddObject(module, "Policy", g_policy_type);
}
