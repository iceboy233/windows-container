// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/desktop.h"

#include <malloc.h>
#include <string>
#include <cstring>

using std::wstring;

namespace {

bool AppendNameUserObject(HANDLE user_object, wstring *append) {
  DWORD size;
  if (!::GetUserObjectInformationW(user_object, UOI_NAME, NULL, 0, &size) &&
      ::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    return false;
  wchar_t *buffer = reinterpret_cast<wchar_t *>(_malloca(size));
  if (!::GetUserObjectInformationW(user_object, UOI_NAME, buffer, size, &size)) {
    _freea(buffer);
    return false;
  }
  append->insert(append->end(), buffer, buffer + wcslen(buffer));
  _freea(buffer);
  return true;
}

}

namespace winc {

ResultCode Desktop::GetFullName(wstring *out_name) {
  out_name->clear();
  if (!AppendNameUserObject(GetWinstaHandle(), out_name))
    return WINC_ERROR_DESKTOP;
  out_name->push_back('\\');
  if (!AppendNameUserObject(GetDesktopHandle(), out_name))
    return WINC_ERROR_DESKTOP;
  return WINC_OK;
}

ResultCode AlternateDesktop::Init() {
  HCRYPTPROV cryptprov;
  if (!::CryptAcquireContextW(&cryptprov,
    NULL, MS_DEF_PROV_W, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    return WINC_ERROR_DESKTOP;
  DWORD random[2];
  BOOL success = ::CryptGenRandom(cryptprov,
    sizeof(random), reinterpret_cast<BYTE *>(random));
  ::CryptReleaseContext(cryptprov, 0);
  if (!success)
    return WINC_ERROR_DESKTOP;

  wchar_t desktop_name[32];
  swprintf_s(desktop_name, L"winc_%08X_%08X%08X",
    GetCurrentProcessId(), random[0], random[1]);
  
  HDESK hdesk = ::CreateDesktopW(desktop_name, NULL, NULL, 0,
    DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_WRITEOBJECTS |
    READ_CONTROL | WRITE_DAC | DESKTOP_SWITCHDESKTOP, NULL);
  if (!hdesk)
    return WINC_ERROR_DESKTOP;

  hdesk_ = hdesk;
  return WINC_OK;
}

AlternateDesktop::~AlternateDesktop() {
  ::CloseDesktop(hdesk_);
}

}
