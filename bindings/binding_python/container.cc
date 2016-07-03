// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/binding_python/container.h"

#include <Python.h>
#include <winc.h>
#include <memory>
#include <vector>

#include "bindings/binding_python/error.h"
#include "bindings/binding_python/logon.h"
#include "bindings/binding_python/sid.h"
#include "bindings/binding_python/target.h"

using std::shared_ptr;
using std::vector;

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
  ResultCode rc = cobj->container.GetPolicy(&cobj->policy);
  if (rc != WINC_OK) {
    cobj->container.~Container();
    subtype->tp_free(obj);
    SetErrorFromResultCode(rc);
    return NULL;
  }
  return obj;
}

void DeleteContainerObject(PyObject *self) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  cobj->container.~Container();
  Py_TYPE(self)->tp_free(self);
}

PyObject *GetUseDesktopPolicyObject(PyObject *self, void *closure) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  return PyBool_FromLong(cobj->policy->use_desktop());
}

int SetUseDesktopPolicyObject(PyObject *self,
                              PyObject *value, void *closure) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return -1;
  }
  if (!PyBool_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "bool expected");
    return -1;
  }
  cobj->policy->set_use_desktop(value == Py_True);
  return 0;
}

PyObject *GetJobBasicLimitPolicyObject(PyObject *self, void *closure) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  return PyLong_FromUnsignedLong(cobj->policy->job_basic_limit());
}

int SetJobBasicLimitPolicyObject(PyObject *self,
                                 PyObject *value, void *closure) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
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
  cobj->policy->set_job_basic_limit(ulong_val);
  return 0;
}

PyObject *GetJobUILimitPolicyObject(PyObject *self, void *closure) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  return PyLong_FromUnsignedLong(cobj->policy->job_ui_limit());
}

int SetJobUILimitPolicyObject(PyObject *self,
                              PyObject *value, void *closure) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
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
  cobj->policy->set_job_ui_limit(ulong_val);
  return 0;
}

PyObject *GetLogonPolicyObject(PyObject *self, void *closure) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  LogonObject *lobj = PyObject_New(LogonObject, &g_logon_type);
  if (!lobj)
    return NULL;
  new (&lobj->logon) shared_ptr<Logon>;
  cobj->policy->GetLogon(&lobj->logon);
  return reinterpret_cast<PyObject *>(lobj);
}

int SetLogonPolicyObject(PyObject *self, PyObject *value, void *closure) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return -1;
  }
  if (!PyType_IsSubtype(Py_TYPE(value), &g_logon_type)) {
    PyErr_SetString(PyExc_TypeError, "logon object required");
    return -1;
  }
  LogonObject *lobj = reinterpret_cast<LogonObject *>(value);
  if (!lobj->logon) {
    PyErr_SetString(PyExc_RuntimeError, "logon object not initialized");
    return -1;
  }
  cobj->policy->SetLogon(lobj->logon);
  return 0;
}

PyObject *AddRestrictedSidPolicyObject(PyObject *self, PyObject *args) {
  SidObject *sobj;
  if (!PyArg_ParseTuple(args, "O!", &g_sid_type, &sobj))
    return NULL;
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  cobj->policy->AddRestrictSid(sobj->sid);
  Py_RETURN_NONE;
}

PyObject *RemoveRestrictedSidPolicyObject(PyObject *self, PyObject *args) {
  SidObject *sobj;
  if (!PyArg_ParseTuple(args, "O!", &g_sid_type, &sobj))
    return NULL;
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  cobj->policy->RemoveRestrictSid(sobj->sid);
  Py_RETURN_NONE;
}

PyObject *GetRestrictedSids(PyObject *self, void *closure) {
  ContainerObject *cobj = reinterpret_cast<ContainerObject *>(self);
  if (!cobj->policy) {
    PyErr_SetString(PyExc_RuntimeError, "not initialized");
    return NULL;
  }
  vector<Sid> sids = cobj->policy->restricted_sids();
  PyObject *tuple = PyTuple_New(sids.size());
  if (!tuple)
    return NULL;
  for (size_t index = 0; index < sids.size(); ++index) {
    SidObject *sobj = PyObject_New(SidObject, &g_sid_type);
    if (!sobj) {
      Py_DECREF(tuple);
      return NULL;
    }
    new (&sobj->sid) Sid;
    PyTuple_SetItem(tuple, index, reinterpret_cast<PyObject *>(sobj));
    ResultCode rc = sobj->sid.Init(sids[index].data());
    if (rc != WINC_OK) {
      Py_DECREF(tuple);
      return SetErrorFromResultCode(rc);
    }
  }
  return tuple;
}

void *GetOptionalPointer(PyObject *object) {
  if (!object)
    return NULL;
#if PY_MAJOR_VERSION >= 3
  if (!PyLong_Check(object)) {
#else
  if (!PyInt_Check(object) && !PyLong_Check(object)) {
#endif
    PyErr_SetString(PyExc_TypeError, "integer expected");
    return NULL;
  }
  return PyLong_AsVoidPtr(object);
}

HANDLE GetInheritableHandle(PyObject *handle, unique_handle *out_holder) {
  HANDLE object = GetOptionalPointer(handle);
  if (!object)
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
                           "command_line", "current_directory",
                           "processor_affinity",
                           "memory_limit", "active_process_limit",
                           "stdin_handle", "stdout_handle", "stderr_handle",
                           NULL};
  Py_UNICODE *exe_path;
  PyObject *target = NULL;
  Py_UNICODE *command_line = NULL;
  Py_UNICODE *current_directory = NULL;
  PyObject *processor_affinity = NULL;
  PyObject *memory_limit = NULL;
  unsigned int active_process_limit = 0;
  PyObject *stdin_handle = NULL;
  PyObject *stdout_handle = NULL;
  PyObject *stderr_handle = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "u|O!uuOOIOOO", kwlist,
                                   &exe_path,
                                   &g_target_type, &target,
                                   &command_line,
                                   &current_directory,
                                   &processor_affinity,
                                   &memory_limit,
                                   &active_process_limit,
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
  options.command_line = command_line;
  options.current_directory = current_directory;
  if (!(options.processor_affinity = reinterpret_cast<uintptr_t>(
      GetOptionalPointer(processor_affinity))) && PyErr_Occurred()) {
    Py_DECREF(target);
    return NULL;
  }
  if (!(options.memory_limit = reinterpret_cast<uintptr_t>(
      GetOptionalPointer(memory_limit))) && PyErr_Occurred()) {
    Py_DECREF(target);
    return NULL;
  }
  options.active_process_limit = active_process_limit;
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

PyMethodDef container_methods[] = {
  {"spawn",
   reinterpret_cast<PyCFunction>(SpawnContainerObject),
   METH_VARARGS | METH_KEYWORDS},
  {"add_restricted_sid", AddRestrictedSidPolicyObject, METH_VARARGS},
  {"remove_restricted_sid", RemoveRestrictedSidPolicyObject, METH_VARARGS},
  {NULL}
};

PyGetSetDef container_getset[] = {
  {"use_desktop", GetUseDesktopPolicyObject, SetUseDesktopPolicyObject},
  {"job_basic_limit", GetJobBasicLimitPolicyObject, SetJobBasicLimitPolicyObject},
  {"job_ui_limit", GetJobUILimitPolicyObject, SetJobUILimitPolicyObject},
  {"logon", GetLogonPolicyObject, SetLogonPolicyObject},
  {"restricted_sids", GetRestrictedSids, NULL},
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
