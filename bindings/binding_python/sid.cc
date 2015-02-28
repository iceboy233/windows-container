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

PyObject *StrSidObject(PyObject *self) {
  SidObject *sobj = reinterpret_cast<SidObject *>(self);
  LPWSTR string_sid;
  if (!::ConvertSidToStringSidW(sobj->sid.data(), &string_sid))
    return PyErr_SetFromWindowsErr(::GetLastError());
  PyObject *str = PyUnicode_FromWideChar(string_sid, wcslen(string_sid));
  ::LocalFree(string_sid);
  return str;
}

PyObject *ReprSidObject(PyObject *self) {
  PyObject *str = StrSidObject(self);
  if (!str)
    return NULL;
#if PY_MAJOR_VERSION >= 3
  wstring repr = L"Sid('";
#else
  wstring repr = L"Sid(u'";
#endif
  repr += PyUnicode_AsUnicode(str);
  Py_DECREF(str);
  repr += L"')";
  return PyUnicode_FromWideChar(repr.data(), repr.size());
}

PyObject *CompareSidObject(PyObject *obj1, PyObject *obj2, int op) {
  SidObject *sobj1 = reinterpret_cast<SidObject *>(obj1);
  SidObject *sobj2 = reinterpret_cast<SidObject *>(obj2);
  PyObject *result;

  switch (op) {
  case Py_EQ:
    result = (sobj1->sid == sobj2->sid) ? Py_True : Py_False;
    break;
  case Py_NE:
    result = (sobj1->sid != sobj2->sid) ? Py_True : Py_False;
    break;
  default:
    result = Py_NotImplemented;
    break;
  }

  Py_INCREF(result);
  return result;
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
  g_sid_type.tp_str = StrSidObject;
  g_sid_type.tp_repr = ReprSidObject;
  g_sid_type.tp_richcompare = CompareSidObject;
  if (PyType_Ready(&g_sid_type) < 0)
    return -1;
  return 0;
}

}

}
