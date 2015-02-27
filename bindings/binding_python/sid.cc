// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/binding_python/sid.h"

#include <Python.h>
#include <Sddl.h>
#include <string>

#include "bindings/binding_python/error.h"

using std::wstring;

namespace winc {

namespace python {

namespace {

PyObject *CreateSidObject(PyTypeObject *subtype,
                          PyObject *args, PyObject *kwds) {
  PyObject *obj = subtype->tp_alloc(subtype, 0);
  if (!obj)
    return NULL;
  SidObject *sobj = reinterpret_cast<SidObject *>(obj);
  new (&sobj->sid) Sid;
  return obj;
}

void DeleteSidObject(PyObject *self) {
  SidObject *sobj = reinterpret_cast<SidObject *>(self);
  sobj->sid.~Sid();
  Py_TYPE(self)->tp_free(self);
}

int InitSidObject(PyObject *self, PyObject *args, PyObject *kwds) {
  SidObject *sobj = reinterpret_cast<SidObject *>(self);
  static char *kwlist[] = {"string_sid_or_well_known_type", NULL};
  PyObject *arg;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &arg))
    return -1;
#if PY_MAJOR_VERSION >= 3
  if (PyLong_Check(arg)) {
#else
  if (PyInt_Check(arg) || PyLong_Check(arg)) {
#endif
    int type = _PyLong_AsInt(arg);
    if (type == -1 && PyErr_Occurred())
      return -1;
    ResultCode rc = sobj->sid.Init(static_cast<WELL_KNOWN_SID_TYPE>(type));
    if (rc != WINC_OK) {
      SetErrorFromResultCode(rc);
      return -1;
    }
    return 0;
  }
  
  if (PyUnicode_Check(arg)) {
    PSID sid;
    Py_UNICODE *string_sid = PyUnicode_AsUnicode(arg);
    if (!::ConvertStringSidToSidW(string_sid, &sid)) {
      PyErr_SetFromWindowsErr(::GetLastError());
      return -1;
    }
    ResultCode rc = sobj->sid.Init(sid);
    ::LocalFree(sid);
    return 0;
  }

#if PY_MAJOR_VERSION >= 3
  PyErr_SetString(PyExc_TypeError, "string or integer required");
#else
  PyErr_SetString(PyExc_TypeError, "unicode or integer required");
#endif
  return -1;
}

PyObject *ReprSidObject(PyObject *self) {
  SidObject *sobj = reinterpret_cast<SidObject *>(self);
  LPWSTR string_sid;
  if (!::ConvertSidToStringSidW(sobj->sid.data(), &string_sid))
    return PyErr_SetFromWindowsErr(::GetLastError());
  PyObject *string_sid_obj = PyUnicode_FromWideChar(string_sid, -1);
  ::LocalFree(string_sid);
  if (!string_sid_obj)
    return NULL;
  PyObject *string_repr = Py_TYPE(string_sid_obj)->tp_repr(string_sid_obj);
  wstring repr = L"Sid(";
  repr += PyUnicode_AsUnicode(string_repr);
  Py_DECREF(string_repr);
  repr.push_back(L')');
  return PyUnicode_FromWideChar(repr.data(), repr.size());
}

}

PyTypeObject g_sid_type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "winc.Sid",        // tp_name
  sizeof(SidObject), // tp_basicsize
};

int InitSidType() {
  g_sid_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  g_sid_type.tp_new = CreateSidObject;
  g_sid_type.tp_dealloc = DeleteSidObject;
  g_sid_type.tp_init = InitSidObject;
  g_sid_type.tp_repr = ReprSidObject;
  if (PyType_Ready(&g_sid_type) < 0)
    return -1;
  return 0;
}

}

}
