// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_DESKTOP_H_
#define WINC_CORE_DESKTOP_H_

#include <Windows.h>
#include <string>

#include <winc_types.h>

namespace winc {

class Desktop {
public:
  virtual ~Desktop() =default;
  virtual bool IsDefaultDesktop() =0;
  virtual HDESK GetDesktopHandle() =0;
  virtual HWINSTA GetWinstaHandle() =0;
  ResultCode GetFullName(std::wstring *out_name);
};

class DesktopWithDefaultWinsta : public Desktop {
public:
  virtual HWINSTA GetWinstaHandle() {
    return ::GetProcessWindowStation();
  }
};

class DefaultDesktop : public DesktopWithDefaultWinsta {
public:
  virtual bool IsDefaultDesktop() {
    return true;
  }

  virtual HDESK GetDesktopHandle() {
    return ::GetThreadDesktop(::GetCurrentThreadId());
  }
};

class AlternateDesktop : public DesktopWithDefaultWinsta {
public:
  AlternateDesktop();
  virtual ~AlternateDesktop();

  ResultCode Init(DWORD access);

  virtual bool IsDefaultDesktop() {
    return false;
  }

  virtual HDESK GetDesktopHandle() {
    return hdesk_;
  }

private:
  HDESK hdesk_;

private:
  AlternateDesktop(const AlternateDesktop &) =delete;
  void operator=(const AlternateDesktop &) =delete;
};

}

#endif
