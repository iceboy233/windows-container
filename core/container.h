// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINC_CORE_CONTAINER_H_
#define WINC_CORE_CONTAINER_H_

#include <memory>

#include <winc_types.h>

namespace winc {

class Policy;

class Container {
public:
  Container();
  ~Container();
  ResultCode Spawn(const wchar_t *exe_path,
                   wchar_t *command_line = nullptr/*,
                   std handles,
                   TargetProcess *out_process*/);
  
  const Policy *policy();
  void set_policy(std::unique_ptr<Policy> &policy);

private:
  void CreateDefaultPolicy();

private:
  std::unique_ptr<Policy> policy_;

private:
  Container(const Container &) =delete;
  void operator=(const Container &) =delete;
};

}

#endif
