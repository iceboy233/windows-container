// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/binding_python/container.h"

#include <Python.h>
#include <winc.h>

#include "core/util.h"
#include "bindings/binding_python/error.h"
#include "bindings/binding_python/target.h"
#include "bindings/binding_python/policy.h"

namespace winc {

namespace python {

namespace {

PyObject *CreateContainerObject(PyTypeObject *subtype,
                                PyObject *args, PyObject *kwds) {
  PyObject *obj = subtype->tp_alloc(subtype, 0);
  if (!obj)
    return NULL;
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(obj);
  new (&cobj->container) Container;
  return obj;
}

void DeleteContainerObject(PyObject *self) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  cobj->container.~Container();
  Py_TYPE(self)->tp_free(self);
}

HANDLE GetInheritableHandle(PyObject *handle, unique_handle *out_holder) {
  if (!handle)
    return NULL;
#if PY_MAJOR_VERSION >= 3
  if (!PyLong_Check(handle)) {
#else
  if (!PyInt_Check(handle) && !PyLong_Check(handle)) {
#endif
    PyErr_SetString(PyExc_TypeError, "integer expected");
    return NULL;
  }
  HANDLE object = PyLong_AsVoidPtr(handle);
  if (!object && PyErr_Occurred())
    return NULL;
  DWORD flags;
  if (!::GetHandleInformation(object, &flags)) {
    PyErr_SetExcFromWindowsErr(PyExc_WindowsError, ::GetLastError());
    return NULL;
  }
  if (flags & HANDLE_FLAG_INHERIT)
    return object;
  HANDLE object_dup;
  if (!::DuplicateHandle(::GetCurrentProcess(), object,
                         ::GetCurrentProcess(), &object_dup,
                         0, TRUE, DUPLICATE_SAME_ACCESS)) {
    PyErr_SetExcFromWindowsErr(PyExc_WindowsError, ::GetLastError());
    return NULL;
  }
  out_holder->reset(object_dup);
  return object_dup;
}

PyObject *SpawnContainerObject(PyObject *self,
                               PyObject *args, PyObject *kwds) {
  static char *kwlist[] = {"exe_path", "target",
                           "stdin_handle", "stdout_handle", "stderr_handle",
                           NULL};
  Py_UNICODE *exe_path;
  PyObject *target = NULL;
  PyObject *stdin_handle = NULL;
  PyObject *stdout_handle = NULL;
  PyObject *stderr_handle = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "u|O!OOO", kwlist,
                                   &exe_path,
                                   g_target_type, &target,
                                   &stdin_handle,
                                   &stdout_handle,
                                   &stderr_handle))
    return NULL;
  if (target) {
    Py_INCREF(target);
  } else {
    target = PyObject_CallObject(reinterpret_cast<PyObject *>(&g_target_type),
                                 NULL);
    if (!target)
      return NULL;
  }
  TargetObject *tobj = reinterpret_cast<TargetObject *>(target);
  if (tobj->container_object) {
    Py_DECREF(target);
    PyErr_SetString(g_error_class, "target already in use");
    return NULL;
  }
  unique_handle stdin_holder, stdout_holder, stderr_holder;
  SpawnOptions options = {};
  options.stdin_handle = GetInheritableHandle(stdin_handle, &stdin_holder);
  if (!options.stdin_handle && PyErr_Occurred()) {
    Py_DECREF(target);
    return NULL;
  }
  options.stdout_handle = GetInheritableHandle(stdout_handle, &stdout_holder);
  if (!options.stdout_handle && PyErr_Occurred()) {
    Py_DECREF(target);
    return NULL;
  }
  options.stderr_handle = GetInheritableHandle(stderr_handle, &stderr_holder);
  if (!options.stderr_handle && PyErr_Occurred()) {
    Py_DECREF(target);
    return NULL;
  }
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  ResultCode rc;
  Py_BEGIN_ALLOW_THREADS
  rc = cobj->container.Spawn(exe_path, &tobj->target, &options);
  Py_END_ALLOW_THREADS
  if (rc != WINC_OK) {
    Py_DECREF(target);
    return SetErrorFromResultCode(rc);
  }
  // The lifecycle of the container must be longer than the target,
  // so we keep an implicit reference here
  Py_INCREF(cobj);
  tobj->container_object = cobj;
  return target;
}

PyObject *GetPolicyContainerObject(PyObject *self, void *closure) {
  return PyObject_CallFunctionObjArgs(reinterpret_cast<PyObject *>(&g_policy_type),
                                      self, NULL);
}

PyMethodDef container_methods[] = {
  {"spawn",
   reinterpret_cast<PyCFunction>(SpawnContainerObject),
   METH_VARARGS | METH_KEYWORDS},
  {NULL}
};

PyGetSetDef container_getset[] = {
  {"policy", GetPolicyContainerObject, NULL},
  {NULL}
};

}

PyTypeObject g_container_type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "winc.Container",         // tp_name
  sizeof(ContainerObject),  // tp_basicsize
};

int InitContainerType() {
  g_container_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  g_container_type.tp_methods = container_methods;
  g_container_type.tp_getset = container_getset;
  g_container_type.tp_new = CreateContainerObject;
  g_container_type.tp_dealloc = DeleteContainerObject;
  if (PyType_Ready(&g_container_type) < 0)
    return -1;
  return 0;
}

}

}
