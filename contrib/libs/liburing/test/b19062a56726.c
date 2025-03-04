#include "../config-host.h"
/* SPDX-License-Identifier: MIT */
// autogenerated by syzkaller (https://github.com/google/syzkaller)

#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include "liburing.h"
#include "helpers.h"
#include "../src/syscall.h"

#ifndef CONFIG_USE_SANITIZER
int main(int argc, char *argv[])
{
  if (argc > 1)
    return T_EXIT_SKIP;

  mmap((void *) 0x20000000, 0x1000000, 3, MAP_ANON|MAP_PRIVATE, -1, 0);

  *(uint32_t*)0x20000200 = 0;
  *(uint32_t*)0x20000204 = 0;
  *(uint32_t*)0x20000208 = 5;
  *(uint32_t*)0x2000020c = 0x400;
  *(uint32_t*)0x20000210 = 0;
  *(uint32_t*)0x20000214 = 0;
  *(uint32_t*)0x20000218 = 0;
  *(uint32_t*)0x2000021c = 0;
  *(uint32_t*)0x20000220 = 0;
  *(uint32_t*)0x20000224 = 0;
  *(uint32_t*)0x20000228 = 0;
  *(uint32_t*)0x2000022c = 0;
  *(uint32_t*)0x20000230 = 0;
  *(uint32_t*)0x20000234 = 0;
  *(uint32_t*)0x20000238 = 0;
  *(uint32_t*)0x2000023c = 0;
  *(uint32_t*)0x20000240 = 0;
  *(uint32_t*)0x20000244 = 0;
  *(uint64_t*)0x20000248 = 0;
  *(uint32_t*)0x20000250 = 0;
  *(uint32_t*)0x20000254 = 0;
  *(uint32_t*)0x20000258 = 0;
  *(uint32_t*)0x2000025c = 0;
  *(uint32_t*)0x20000260 = 0;
  *(uint32_t*)0x20000264 = 0;
  *(uint32_t*)0x20000268 = 0;
  *(uint32_t*)0x2000026c = 0;
  *(uint64_t*)0x20000270 = 0;
  __sys_io_uring_setup(0xc9f, (struct io_uring_params *) 0x20000200);
  return T_EXIT_PASS;
}
#else
int main(int argc, char *argv[])
{
	return T_EXIT_SKIP;
}
#endif
