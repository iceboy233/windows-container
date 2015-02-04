// Copyright (c) 2015 Vijos Dev Team. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This program generates random sequence of N elements in O(N) time,
// and then use introsort to sort it in O(NlogN) time.

#include <Windows.h>
#include <cinttypes>
#include <cstdio>
#include <numeric>
#include <algorithm>

using namespace std;

const int N = 10000000;
int numbers[N];

int main() {
  ULONG64 cycle_time;
  ::QueryProcessCycleTime(::GetCurrentProcess(), &cycle_time);
  printf("Cycle time = %" PRIu64 "\n", cycle_time);
  ::SetProcessAffinityMask(::GetCurrentProcess(), 1);
  ULONG size = 0;
  ::QueryIdleProcessorCycleTime(&size, NULL);
  ULONG64 *idle_times = reinterpret_cast<ULONG64 *>(_alloca(size));
  ::QueryIdleProcessorCycleTime(&size, idle_times);
  printf("Started\n");
  iota(numbers, numbers + N, 0);
  random_shuffle(numbers, numbers + N);
  printf("Generated\n");
  sort(numbers, numbers + N);
  printf("Sorted\n");
  ::QueryProcessCycleTime(::GetCurrentProcess(), &cycle_time);
  ULONG64 *idle_times2 = reinterpret_cast<ULONG64 *>(_alloca(size));
  ::QueryIdleProcessorCycleTime(&size, idle_times2);
  printf("Cycle time = %" PRIu64 "\n", cycle_time);
  for (unsigned int i = 0; i < size / sizeof(ULONG64); ++i) {
    idle_times2[i] -= idle_times[i];
    printf("Idle time[%d] = %" PRIu64 "\n", i, idle_times2[i]);
  }
}
