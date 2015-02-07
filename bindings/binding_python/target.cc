// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/binding_python/target.h"

#include <Python.h>
#include <winc.h>

#include "bindings/binding_python/error.h"

namespace winc {

namespace python {

namespace {

PyObject *CreateTargetObject(PyTypeObject *subtype,
                             PyObject *args, PyObject *kwds) {
  PyObject *obj = subtype->tp_alloc(subtype, 0);
  if (!obj)
    return NULL;
  TargetObject *tobj = reinterpret_cast<TargetObject *>(obj);
  new (&tobj->target) TargetDirector;
  return obj;
}

void DeleteTargetObject(PyObject *self) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  tobj->target.~TargetDirector();
  Py_TYPE(self)->tp_free(self);
}

PyObject *StartTargetObject(PyObject *self, PyObject *args) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  ResultCode rc = tobj->target.Start();
  if (rc != WINC_OK)
    return SetErrorFromResultCode(rc);
  Py_RETURN_NONE;
}

PyTypeObject target_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                    // ob_size
  "winc.Target",        // tp_name
  sizeof(TargetObject), // tp_basicsize
};

PyMethodDef target_methods[] = {
  {"start", StartTargetObject, METH_NOARGS},
  {NULL, NULL}
};

}

PyObject *g_target_type;

int InitTargetType() {
  target_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  target_type.tp_methods = target_methods;
  target_type.tp_new = CreateTargetObject;
  target_type.tp_dealloc = DeleteTargetObject;
  if (PyType_Ready(&target_type) < 0)
    return -1;
  g_target_type = reinterpret_cast<PyObject *>(&target_type);
  return 0;
}

}

}
