// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Windows.h>
#include <deque>
#include <string>
#include <cstring>

using namespace std;

int wmain(int argc, wchar_t *argv[]) {
  deque<wstring> q;

  if (argc == 1) {
    // Get all logical drives as starting point
    DWORD drive_mask = ::GetLogicalDrives();
    for (wchar_t drive_letter = L'A'; drive_mask; ++drive_letter) {
      if (drive_mask & 1)
        q.push_back(wstring{drive_letter, L':', L'\\', L'*'});
      drive_mask >>= 1;
    }
  } else {
    for (int i = 1; i < argc; ++i) {
      wstring root = argv[i];
      if (root.back() != L'\\')
        root.push_back(L'\\');
      q.push_back(root + L"*");
    }
  }

  // Find all files
  while (!q.empty()) {
    wstring root = q.front();
    q.pop_front();
    WIN32_FIND_DATAW find_data;
    HANDLE find = ::FindFirstFileW(root.c_str(), &find_data);
    if (find != INVALID_HANDLE_VALUE) {
      root.pop_back();
      BOOL has_next = TRUE;
      while (has_next) {
        if (wcscmp(find_data.cFileName, L".") &&
            wcscmp(find_data.cFileName, L"..")) {
          char access[4] = "---";
          wstring full_name = root + find_data.cFileName;
          if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            q.push_front(full_name + L"\\*");
            access[0] = 'D';
          }
          HANDLE file;
          file = ::CreateFileW(full_name.c_str(),
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
                               NULL);
          if (file != INVALID_HANDLE_VALUE) {
            access[1] = 'R';
            ::CloseHandle(file);
          }
          file = ::CreateFileW(full_name.c_str(),
                               GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
                               NULL);
          if (file != INVALID_HANDLE_VALUE) {
            access[2] = 'W';
            ::CloseHandle(file);
          }
          printf("%s %ws\n", access, full_name.c_str());
        }
        has_next = ::FindNextFileW(find, &find_data);
      }
      ::FindClose(find);
    }
  }
}
