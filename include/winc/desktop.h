// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_DESKTOP_H_
#define WINC_CORE_DESKTOP_H_

#include <Windows.h>
#include <vector>

#include <winc_types.h>

namespace winc {

class Desktop {
public:
  virtual ~Desktop() = default;
  virtual bool IsDefaultDesktop() const = 0;
  virtual HDESK GetDesktopHandle() const = 0;
  virtual HWINSTA GetWinstaHandle() const = 0;
  ResultCode GetFullName(const wchar_t **out_name) const;

private:
  mutable std::vector<wchar_t> full_name_cache_;
};

class DesktopWithDefaultWinsta : public Desktop {
public:
  virtual HWINSTA GetWinstaHandle() const override {
    return ::GetProcessWindowStation();
  }
};

class DefaultDesktop : public DesktopWithDefaultWinsta {
public:
  virtual bool IsDefaultDesktop() const override {
    return true;
  }

  virtual HDESK GetDesktopHandle() const override {
    return ::GetThreadDesktop(::GetCurrentThreadId());
  }
};

class AlternateDesktop : public DesktopWithDefaultWinsta {
public:
  AlternateDesktop();
  virtual ~AlternateDesktop() override;

  ResultCode Init(DWORD access);

  virtual bool IsDefaultDesktop() const override {
    return false;
  }

  virtual HDESK GetDesktopHandle() const override {
    return hdesk_;
  }

private:
  HDESK hdesk_;

private:
  AlternateDesktop(const AlternateDesktop &) = delete;
  void operator=(const AlternateDesktop &) = delete;
};

}

#endif
