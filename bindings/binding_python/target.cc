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
  tobj->container_object = NULL;
  return obj;
}

void DeleteTargetObject(PyObject *self) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  tobj->target.~TargetDirector();
  Py_XDECREF(tobj->container_object);
  Py_TYPE(self)->tp_free(self);
}

PyObject *StartTargetObject(PyObject *self, PyObject *args) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  // The event dispatcher owns the target object
  Py_INCREF(self);
  PyEval_InitThreads();
  ResultCode rc;
  Py_BEGIN_ALLOW_THREADS
  rc = tobj->target.Start(true);
  Py_END_ALLOW_THREADS
  if (rc != WINC_OK) {
    Py_DECREF(self);
    return SetErrorFromResultCode(rc);
  }
  Py_INCREF(self);
  return self;
}

PyObject *WaitForProcessTargetObject(PyObject *self, PyObject *args) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  ResultCode rc;
  Py_BEGIN_ALLOW_THREADS
  rc = tobj->target.WaitForProcess();
  Py_END_ALLOW_THREADS
  if (rc != WINC_OK)
    return SetErrorFromResultCode(rc);
  Py_INCREF(self);
  return self;
}

PyObject *TerminateJobTargetObject(PyObject *self, PyObject *args) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  unsigned int exit_code = 0;
  if (!PyArg_ParseTuple(args, "|I", &exit_code))
    return NULL;
  ResultCode rc = tobj->target.TerminateJob(exit_code);
  if (rc != WINC_OK)
    return SetErrorFromResultCode(rc);
  Py_INCREF(self);
  return self;
}

PyObject *GetProcessIdTargetObject(PyObject *self, void *closure) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  return PyLong_FromUnsignedLong(tobj->target.process_id());
}

PyObject *GetJobTimeTargetObject(PyObject *self, void *closure) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  ULONG64 job_time;
  ResultCode rc = tobj->target.GetJobTime(&job_time);
  if (rc != WINC_OK)
    return SetErrorFromResultCode(rc);
  return PyLong_FromUnsignedLongLong(job_time);
}

PyObject *GetProcessTimeTargetObject(PyObject *self, void *closure) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  ULONG64 process_time;
  ResultCode rc = tobj->target.GetProcessTime(&process_time);
  if (rc != WINC_OK)
    return SetErrorFromResultCode(rc);
  return PyLong_FromUnsignedLongLong(process_time);
}

PyObject *GetProcessCycleTargetObject(PyObject *self, void *closure) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  ULONG64 process_cycle;
  ResultCode rc = tobj->target.GetProcessCycle(&process_cycle);
  if (rc != WINC_OK)
    return SetErrorFromResultCode(rc);
  return PyLong_FromUnsignedLongLong(process_cycle);
}

PyObject *GetJobPeakMemoryTargetObject(PyObject *self, void *closure) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  SIZE_T job_peak_memory;
  ResultCode rc = tobj->target.GetJobPeakMemory(&job_peak_memory);
  if (rc != WINC_OK)
    return SetErrorFromResultCode(rc);
  return PyLong_FromSize_t(job_peak_memory);
}

PyObject *GetProcessPeakMemoryTargetObject(PyObject *self, void *closure) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  SIZE_T process_peak_memory;
  ResultCode rc = tobj->target.GetJobPeakMemory(&process_peak_memory);
  if (rc != WINC_OK)
    return SetErrorFromResultCode(rc);
  return PyLong_FromSize_t(process_peak_memory);
}

PyObject *GetProcessExitCodeTargetObject(PyObject *self, void *closure) {
  TargetObject *tobj = reinterpret_cast<TargetObject *>(self);
  DWORD exit_code;
  ResultCode rc = tobj->target.GetProcessExitCode(&exit_code);
  if (rc != WINC_OK)
    return SetErrorFromResultCode(rc);
  return PyLong_FromUnsignedLong(exit_code);
}

PyMethodDef target_methods[] = {
  {"start",            StartTargetObject,          METH_NOARGS},
  {"wait_for_process", WaitForProcessTargetObject, METH_NOARGS},
  {"terminate_job",    TerminateJobTargetObject,   METH_VARARGS},
  {NULL}
};

PyGetSetDef target_getset[] = {
  {"process_id",          GetProcessIdTargetObject,         NULL},
  {"job_time",            GetJobTimeTargetObject,           NULL},
  {"process_time",        GetProcessTimeTargetObject,       NULL},
  {"process_cycle",       GetProcessCycleTargetObject,      NULL},
  {"job_peak_memory",     GetJobPeakMemoryTargetObject,     NULL},
  {"process_peak_memory", GetProcessPeakMemoryTargetObject, NULL},
  {"process_exit_code",   GetProcessExitCodeTargetObject,   NULL},
  {NULL}
};

}

void TargetDirector::OnActiveProcessLimit() {
  TargetObject *tobj = CONTAINING_RECORD(this, TargetObject, target);
  PyGILState_STATE gstate = PyGILState_Ensure();
  if (!PyObject_CallMethod(reinterpret_cast<PyObject *>(tobj),
                           "on_active_process_limit", NULL))
    PyErr_Clear();
  PyGILState_Release(gstate);
}

void TargetDirector::OnExitAll() {
  TargetObject *tobj = CONTAINING_RECORD(this, TargetObject, target);
  PyGILState_STATE gstate = PyGILState_Ensure();
  if (!PyObject_CallMethod(reinterpret_cast<PyObject *>(tobj),
                           "on_exit_all", NULL))
    PyErr_Clear();
  // No further message will be dispatched, release the target object
  Py_DECREF(tobj);
  PyGILState_Release(gstate);
}

void TargetDirector::OnNewProcess(DWORD process_id) {
  TargetObject *tobj = CONTAINING_RECORD(this, TargetObject, target);
  PyGILState_STATE gstate = PyGILState_Ensure();
  if (!PyObject_CallMethod(reinterpret_cast<PyObject *>(tobj),
                           "on_new_process", "I", process_id))
    PyErr_Clear();
  PyGILState_Release(gstate);
}

void TargetDirector::OnExitProcess(DWORD process_id) {
  TargetObject *tobj = CONTAINING_RECORD(this, TargetObject, target);
  PyGILState_STATE gstate = PyGILState_Ensure();
  if (!PyObject_CallMethod(reinterpret_cast<PyObject *>(tobj),
                           "on_exit_process", "I", process_id))
    PyErr_Clear();
  PyGILState_Release(gstate);
}

void TargetDirector::OnMemoryLimit(DWORD process_id) {
  TargetObject *tobj = CONTAINING_RECORD(this, TargetObject, target);
  PyGILState_STATE gstate = PyGILState_Ensure();
  if (!PyObject_CallMethod(reinterpret_cast<PyObject *>(tobj),
                           "on_memory_limit", "I", process_id))
    PyErr_Clear();
  PyGILState_Release(gstate);
}

PyTypeObject g_target_type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "winc.Target",        // tp_name
  sizeof(TargetObject), // tp_basicsize
};

int InitTargetType() {
  g_target_type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  g_target_type.tp_methods = target_methods;
  g_target_type.tp_getset = target_getset;
  g_target_type.tp_new = CreateTargetObject;
  g_target_type.tp_dealloc = DeleteTargetObject;
  if (PyType_Ready(&g_target_type) < 0)
    return -1;
  return 0;
}

}

}
