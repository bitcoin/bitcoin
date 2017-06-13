// Copyright 2016 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A portable implementation of crc32c, optimized to handle
// four bytes at a time.
//
// In a separate source file to allow this accelerated CRC32C function to be
// compiled with the appropriate compiler flags to enable x86 SSE 4.2
// instructions.

#include <stdint.h>
#include <string.h>
#include "port/port.h"

#if defined(LEVELDB_PLATFORM_POSIX_SSE)

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) && defined(__SSE4_2__)
#include <nmmintrin.h>
#include <cpuid.h>
#endif

#endif  // defined(LEVELDB_PLATFORM_POSIX_SSE)

namespace leveldb {
namespace port {

#if defined(LEVELDB_PLATFORM_POSIX_SSE)

// Used to fetch a naturally-aligned 32-bit word in little endian byte-order
static inline uint32_t LE_LOAD32(const uint8_t *p) {
  // SSE is x86 only, so ensured that |p| is always little-endian.
  uint32_t word;
  memcpy(&word, p, sizeof(word));
  return word;
}

#if defined(_M_X64) || defined(__x86_64__)  // LE_LOAD64 is only used on x64.

// Used to fetch a naturally-aligned 64-bit word in little endian byte-order
static inline uint64_t LE_LOAD64(const uint8_t *p) {
  uint64_t dword;
  memcpy(&dword, p, sizeof(dword));
  return dword;
}

#endif  // defined(_M_X64) || defined(__x86_64__)

static inline bool HaveSSE42() {
#if defined(_MSC_VER)
  int cpu_info[4];
  __cpuid(cpu_info, 1);
  return (cpu_info[2] & (1 << 20)) != 0;
#elif defined(__GNUC__)
  unsigned int eax, ebx, ecx, edx;
  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
  return (ecx & (1 << 20)) != 0;
#else
  return false;
#endif
}

#endif  // defined(LEVELDB_PLATFORM_POSIX_SSE)

// For further improvements see Intel publication at:
// http://download.intel.com/design/intarch/papers/323405.pdf
uint32_t AcceleratedCRC32C(uint32_t crc, const char* buf, size_t size) {
#if !defined(LEVELDB_PLATFORM_POSIX_SSE)
  return 0;
#else
  static bool have = HaveSSE42();
  if (!have) {
    return 0;
  }

  const uint8_t *p = reinterpret_cast<const uint8_t *>(buf);
  const uint8_t *e = p + size;
  uint32_t l = crc ^ 0xffffffffu;

#define STEP1 do {                              \
    l = _mm_crc32_u8(l, *p++);                  \
} while (0)
#define STEP4 do {                              \
    l = _mm_crc32_u32(l, LE_LOAD32(p));         \
    p += 4;                                     \
} while (0)
#define STEP8 do {                              \
    l = _mm_crc32_u64(l, LE_LOAD64(p));         \
    p += 8;                                     \
} while (0)

  if (size > 16) {
    // Process unaligned bytes
    for (unsigned int i = reinterpret_cast<uintptr_t>(p) % 8; i; --i) {
      STEP1;
    }

    // _mm_crc32_u64 is only available on x64.
#if defined(_M_X64) || defined(__x86_64__)
    // Process 8 bytes at a time
    while ((e-p) >= 8) {
      STEP8;
    }
    // Process 4 bytes at a time
    if ((e-p) >= 4) {
      STEP4;
    }
#else  // !(defined(_M_X64) || defined(__x86_64__))
    // Process 4 bytes at a time
    while ((e-p) >= 4) {
      STEP4;
    }
#endif  // defined(_M_X64) || defined(__x86_64__)
  }
  // Process the last few bytes
  while (p != e) {
    STEP1;
  }
#undef STEP8
#undef STEP4
#undef STEP1
  return l ^ 0xffffffffu;
#endif  // defined(LEVELDB_PLATFORM_POSIX_SSE)
}

}  // namespace port
}  // namespace leveldb
