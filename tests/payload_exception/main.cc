// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Windows.h>

int main() {
  ::RaiseException(0xABADFEED, 0, 0, NULL);
}
