// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_BINDING_PYTHON_LOGON_H_
#define WINC_BINDING_PYTHON_LOGON_H_

#include <Python.h>
#include <winc.h>
#include <memory>

namespace winc {

namespace python {

struct LogonObject {
  PyObject_HEAD
  std::unique_ptr<Logon> logon;
};

int InitLogonTypes();

extern PyTypeObject g_logon_type;
extern PyTypeObject g_current_logon_type;
extern PyTypeObject g_user_logon_type;

}

}

#endif
