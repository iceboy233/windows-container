// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/desktop.h"

#include <vector>
#include <cstring>

using std::vector;

namespace {

bool AppendNameUserObject(HANDLE user_object, vector<wchar_t> &append) {
  DWORD size;
  if (!::GetUserObjectInformationW(user_object, UOI_NAME, NULL, 0, &size) &&
      ::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    return false;
  size_t orig_size = append.size();
  append.resize(orig_size + (size + 1) / 2);
  if (!::GetUserObjectInformationW(user_object, UOI_NAME, &append[orig_size], size, &size)) {
    append.resize(orig_size);
    return false;
  }
  append.resize(wcslen(append.data()));
  return true;
}

}

namespace winc {

ResultCode Desktop::GetFullName(const wchar_t **out_name) const {
  if (full_name_cache_.empty()) {
    if (!AppendNameUserObject(GetWinstaHandle(), full_name_cache_))
      return WINC_ERROR_DESKTOP;
    full_name_cache_.push_back(L'\\');
    if (!AppendNameUserObject(GetDesktopHandle(), full_name_cache_)) {
      full_name_cache_.clear();
      return WINC_ERROR_DESKTOP;
    }
    full_name_cache_.push_back(L'\0');
  }
  *out_name = full_name_cache_.data();
  return WINC_OK;
}

AlternateDesktop::AlternateDesktop()
  : hdesk_(NULL)
  {}

ResultCode AlternateDesktop::Init(DWORD access) {
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
  
  HDESK hdesk = ::CreateDesktopW(desktop_name, NULL, NULL,
                                 0, access, NULL);
  if (!hdesk)
    return WINC_ERROR_DESKTOP;

  hdesk_ = hdesk;
  return WINC_OK;
}

AlternateDesktop::~AlternateDesktop() {
  if (hdesk_ != NULL)
    ::CloseDesktop(hdesk_);
}

}
