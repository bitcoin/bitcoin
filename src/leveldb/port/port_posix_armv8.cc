// Copyright 2018 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A portable implementation of crc32c, optimized to handle
// four bytes at a time.
//
// In a separate source file to allow this accelerated CRC32C function to be
// compiled with the appropriate compiler flags to enable ARMV8 CRC32C
// instructions.

#include <stdint.h>
#include <string.h>
#include "port/port.h"

#if defined(LEVELDB_PLATFORM_POSIX_ARMV8)
#include <arm_acle.h>
#endif  // defined(LEVELDB_PLATFORM_POSIX_ARMV8)

namespace leveldb {
namespace port {

#if defined(LEVELDB_PLATFORM_POSIX_ARMV8)
// Used to fetch a naturally-aligned 32-bit word in little endian byte-order
static inline uint32_t LE_LOAD32(const uint8_t *p) {
  // ARMv8 only, so ensured that |p| is always little-endian.
  uint32_t word;
  memcpy(&word, p, sizeof(word));
  return word;
}

// Used to fetch a naturally-aligned 64-bit word in little endian byte-order
static inline uint64_t LE_LOAD64(const uint8_t *p) {
  uint64_t dword;
  memcpy(&dword, p, sizeof(dword));
  return dword;
}
#endif

uint32_t AcceleratedCRC32C(uint32_t crc, const char* buf, size_t size) {
#if !defined(LEVELDB_PLATFORM_POSIX_ARMV8)
  return 0;
#else

  const uint8_t *p = reinterpret_cast<const uint8_t *>(buf);
  const uint8_t *e = p + size;
  uint32_t l = crc ^ 0xffffffffu;

#define STEP1 do {                              \
    l = __crc32cb(l, *p++);                  \
} while (0)
#define STEP4 do {                              \
    l = __crc32cw(l, LE_LOAD32(p));         \
    p += 4;                                     \
} while (0)
#define STEP8 do {                              \
    l = __crc32cd(l, LE_LOAD64(p));         \
    p += 8;                                     \
} while (0)

  if (size > 16) {
    // Process unaligned bytes
    for (unsigned int i = reinterpret_cast<uintptr_t>(p) % 8; i; --i) {
      STEP1;
    }
    // Process 8 bytes at a time
    while ((e-p) >= 8) {
      STEP8;
    }
    // Process 4 bytes at a time
    if ((e-p) >= 4) {
      STEP4;
    }
  }
  // Process the last few bytes
  while (p != e) {
    STEP1;
  }
#undef STEP8
#undef STEP4
#undef STEP1
  return l ^ 0xffffffffu;
#endif  // defined(LEVELDB_PLATFORM_POSIX_ARMV8)
}

}  // namespace port
}  // namespace leveldb
