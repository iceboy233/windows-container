// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/binding_python/container.h"

#include <Python.h>
#include <winc.h>

#include "core/util.h"
#include "bindings/binding_python/error.h"
#include "bindings/binding_python/target.h"

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
  HANDLE object = PyLong_AsVoidPtr(handle);
  if (!object)
    return NULL;
  DWORD flags;
  if (!::GetHandleInformation(object, &flags))
    return NULL;
  if (flags & HANDLE_FLAG_INHERIT)
    return object;
  HANDLE object_dup;
  if (!::DuplicateHandle(::GetCurrentProcess(), object,
                         ::GetCurrentProcess(), &object_dup,
                         0, TRUE, DUPLICATE_SAME_ACCESS))
    return NULL;
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
    target = PyObject_CallObject(g_target_type, NULL);
    if (!target)
      return NULL;
  }
  ResultCode rc;
  Py_BEGIN_ALLOW_THREADS
  unique_handle stdin_holder, stdout_holder, stderr_holder;
  SpawnOptions options = {};
  options.stdin_handle = GetInheritableHandle(stdin_handle, &stdin_holder);
  options.stdout_handle = GetInheritableHandle(stdout_handle, &stdout_holder);
  options.stderr_handle = GetInheritableHandle(stderr_handle, &stderr_holder);
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  TargetObject *tobj = reinterpret_cast<TargetObject *>(target);
  rc = cobj->container.Spawn(exe_path, &tobj->target, &options);
  Py_END_ALLOW_THREADS
  if (rc != WINC_OK) {
    Py_DECREF(target);
    return SetErrorFromResultCode(rc);
  }
  return target;
}

PyTypeObject container_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                        // ob_size
  "winc.Container",         // tp_name
  sizeof(ContainerObject),  // tp_basicsize
};

PyMethodDef container_methods[] = {
  {"spawn", reinterpret_cast<PyCFunction>(SpawnContainerObject),
    METH_VARARGS | METH_KEYWORDS},
  {NULL, NULL}
};

}

PyObject *g_container_type;

int InitContainerType() {
  container_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  container_type.tp_methods = container_methods;
  container_type.tp_new = CreateContainerObject;
  container_type.tp_dealloc = DeleteContainerObject;
  if (PyType_Ready(&container_type) < 0)
    return -1;
  g_container_type = reinterpret_cast<PyObject *>(&container_type);
  return 0;
}

}

}
