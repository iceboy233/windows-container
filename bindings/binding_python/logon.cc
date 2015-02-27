// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/binding_python/logon.h"

#include <Python.h>
#include <Windows.h>
#include <winc.h>
#include <memory>

#include "bindings/binding_python/error.h"

using std::make_unique;
using std::unique_ptr;

namespace winc {

namespace python {

namespace {

PyObject *CreateLogonObject(PyTypeObject *subtype,
                            PyObject *args, PyObject *kwds) {
  PyObject *obj = subtype->tp_alloc(subtype, 0);
  if (!obj)
    return NULL;
  LogonObject *lobj = reinterpret_cast<LogonObject *>(obj);
  new (&lobj->logon) unique_ptr<Logon>;
  return obj;
}

void DeleteLogonObject(PyObject *self) {
  LogonObject *lobj = reinterpret_cast<LogonObject *>(self);
  lobj->logon.~unique_ptr();
  Py_TYPE(self)->tp_free(self);
}

int InitLogonObject(PyObject *self, PyObject *args, PyObject *kwds) {
  PyErr_SetString(PyExc_RuntimeError, "cannot instantiate abstract type");
  return -1;
}

int InitCurrentLogonObject(PyObject *self, PyObject *args, PyObject *kwds) {
  static char *kwlist[] = {"integrity_level", NULL};
  DWORD integrity_level_obj = SECURITY_MANDATORY_LOW_RID;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|I", kwlist,
                                   &integrity_level_obj))
    return -1;

  auto logon = make_unique<CurrentLogon>();
  ResultCode rc = logon->Init(integrity_level_obj);
  if (rc != WINC_OK) {
    SetErrorFromResultCode(rc);
    return -1;
  }

  LogonObject *lobj = reinterpret_cast<LogonObject *>(self);
  lobj->logon = move(logon);
  return 0;
}

int InitUserLogonObject(PyObject *self, PyObject *args, PyObject *kwds) {
  static char *kwlist[] = {"username", "password", "integrity_level", NULL};
  Py_UNICODE *username, *password;
  DWORD integrity_level_obj = SECURITY_MANDATORY_LOW_RID;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "uu|I", kwlist,
                                   &username, &password,
                                   &integrity_level_obj))
    return -1;

  auto logon = make_unique<UserLogon>();
  ResultCode rc = logon->Init(username, password, integrity_level_obj);
  if (rc != WINC_OK) {
    SetErrorFromResultCode(rc);
    return -1;
  }

  LogonObject *lobj = reinterpret_cast<LogonObject *>(self);
  lobj->logon = move(logon);
  return 0;
}

}

PyTypeObject g_logon_type {
  PyVarObject_HEAD_INIT(NULL, 0)
  "winc.Logon",        // tp_name
  sizeof(LogonObject), // tp_basicsize
};

PyTypeObject g_current_logon_type {
  PyVarObject_HEAD_INIT(NULL, 0)
  "winc.CurrentLogon", // tp_name
  sizeof(LogonObject), // tp_basicsize
};

PyTypeObject g_user_logon_type {
  PyVarObject_HEAD_INIT(NULL, 0)
  "winc.UserLogon",    // tp_name
  sizeof(LogonObject), // tp_basicsize
};

int InitLogonTypes() {
  g_logon_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  g_logon_type.tp_new = CreateLogonObject;
  g_logon_type.tp_dealloc = DeleteLogonObject;
  g_logon_type.tp_init = InitLogonObject;
  if (PyType_Ready(&g_logon_type) < 0)
    return -1;
  g_current_logon_type.tp_base = &g_logon_type;
  g_current_logon_type.tp_flags = Py_TPFLAGS_DEFAULT;
  g_current_logon_type.tp_init = InitCurrentLogonObject;
  if (PyType_Ready(&g_current_logon_type) < 0)
    return -1;
  g_user_logon_type.tp_base = &g_logon_type;
  g_user_logon_type.tp_flags = Py_TPFLAGS_DEFAULT;
  g_user_logon_type.tp_init = InitUserLogonObject;
  if (PyType_Ready(&g_user_logon_type) < 0)
    return -1;
  return 0;
}

}

}
