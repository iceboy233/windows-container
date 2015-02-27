// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/binding_python/policy.h"

#include <Python.h>

#include "bindings/binding_python/container.h"
#include "bindings/binding_python/error.h"

namespace winc {

namespace python {

namespace {

PyObject *CreatePolicyObject(PyTypeObject *subtype,
                             PyObject *args, PyObject *kwds) {
  PyObject *obj = subtype->tp_alloc(subtype, 0);
  if (!obj)
    return NULL;
  PolicyObject *pobj = reinterpret_cast<PolicyObject *>(obj);
  pobj->container_object = NULL;
  pobj->policy = NULL;
  return obj;
}

void DeletePolicyObject(PyObject *self) {
  PolicyObject *pobj = reinterpret_cast<PolicyObject *>(self);
  Py_XDECREF(pobj->container_object);
  Py_TYPE(self)->tp_free(self);
}

int InitPolicyObject(PyObject *self, PyObject *args, PyObject *kwds) {
  PolicyObject *pobj = reinterpret_cast<PolicyObject *>(self);
  static char *kwlist[] = {"container", NULL};
  PyObject *container;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist,
                                   g_container_type, &container))
    return -1;
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(container);
  Policy *policy;
  ResultCode rc;
  Py_BEGIN_ALLOW_THREADS
  rc = cobj->container.GetPolicy(&policy);
  Py_END_ALLOW_THREADS
  if (rc != WINC_OK) {
    SetErrorFromResultCode(rc);
    return -1;
  }
  Py_XDECREF(pobj->container_object);
  Py_INCREF(cobj);
  pobj->container_object = cobj;
  pobj->policy = policy;
  return 0;
}

PyObject *GetUseDesktopPolicyObject(PyObject *self, void *closure) {
  PolicyObject *pobj = reinterpret_cast<PolicyObject *>(self);
  if (!pobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  return PyBool_FromLong(pobj->policy->use_desktop());
}

int SetUseDesktopPolicyObject(PyObject *self,
                              PyObject *value, void *closure) {
  PolicyObject *pobj = reinterpret_cast<PolicyObject *>(self);
  if (!pobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return -1;
  }
  if (!PyBool_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "bool expected");
    return -1;
  }
  pobj->policy->set_use_desktop(value == Py_True);
  return 0;
}

PyObject *GetJobBasicLimitPolicyObject(PyObject *self, void *closure) {
  PolicyObject *pobj = reinterpret_cast<PolicyObject *>(self);
  if (!pobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  return PyLong_FromUnsignedLong(pobj->policy->job_basic_limit());
}

int SetJobBasicLimitPolicyObject(PyObject *self,
                                 PyObject *value, void *closure) {
  PolicyObject *pobj = reinterpret_cast<PolicyObject *>(self);
  if (!pobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return -1;
  }
#if PY_MAJOR_VERSION >= 3
  if (!PyLong_Check(value)) {
#else
  if (!PyInt_Check(value) && !PyLong_Check(value)) {
#endif
    PyErr_SetString(PyExc_TypeError, "integer expected");
    return -1;
  }
  unsigned long ulong_val = PyLong_AsUnsignedLong(value);
  if (ulong_val == static_cast<unsigned long>(-1) && PyErr_Occurred())
    return -1;
  pobj->policy->set_job_basic_limit(ulong_val);
  return 0;
}

PyObject *GetJobUILimitPolicyObject(PyObject *self, void *closure) {
  PolicyObject *pobj = reinterpret_cast<PolicyObject *>(self);
  if (!pobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  return PyLong_FromUnsignedLong(pobj->policy->job_ui_limit());
}

int SetJobUILimitPolicyObject(PyObject *self,
                              PyObject *value, void *closure) {
  PolicyObject *pobj = reinterpret_cast<PolicyObject *>(self);
  if (!pobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return -1;
  }
#if PY_MAJOR_VERSION >= 3
  if (!PyLong_Check(value)) {
#else
  if (!PyInt_Check(value) && !PyLong_Check(value)) {
#endif
    PyErr_SetString(PyExc_TypeError, "integer expected");
    return -1;
  }
  unsigned long ulong_val = PyLong_AsUnsignedLong(value);
  if (ulong_val == static_cast<unsigned long>(-1) && PyErr_Occurred())
    return -1;
  pobj->policy->set_job_ui_limit(ulong_val);
  return 0;
}

PyMethodDef policy_methods[] = {
  {NULL}
};

PyGetSetDef policy_getset[] = {
  {"use_desktop", GetUseDesktopPolicyObject, SetUseDesktopPolicyObject},
  {"job_basic_limit", GetJobBasicLimitPolicyObject, SetJobBasicLimitPolicyObject},
  {"job_ui_limit", GetJobUILimitPolicyObject, SetJobUILimitPolicyObject},
  {NULL}
};

}

PyTypeObject g_policy_type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "winc.Policy",        // tp_name
  sizeof(PolicyObject), // tp_basicsize
};

int InitPolicyType() {
  g_policy_type.tp_flags = Py_TPFLAGS_DEFAULT;
  g_policy_type.tp_methods = policy_methods;
  g_policy_type.tp_getset = policy_getset;
  g_policy_type.tp_new = CreatePolicyObject;
  g_policy_type.tp_dealloc = DeletePolicyObject;
  g_policy_type.tp_init = InitPolicyObject;
  if (PyType_Ready(&g_policy_type) < 0)
    return -1;
  return 0;
}

}

}
