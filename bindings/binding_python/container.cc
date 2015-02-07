// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/binding_python/container.h"

#include <Python.h>
#include <winc.h>

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

PyObject *SpawnContainerObject(PyObject *self,
                               PyObject *args, PyObject *kwds) {
  static char *kwlist[] = {"exe_path", "target", NULL};
  Py_UNICODE *exe_path;
  PyObject *target = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "u|O!", kwlist,
                                   &exe_path,
                                   g_target_type, &target))
    return NULL;
  if (target) {
    Py_INCREF(target);
  } else {
    target = PyObject_CallObject(g_target_type, NULL);
    if (!target)
      return NULL;
  }
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  TargetObject *tobj = reinterpret_cast<TargetObject *>(target);
  ResultCode rc = cobj->container.Spawn(exe_path, &tobj->target);
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
