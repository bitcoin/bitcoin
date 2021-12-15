// Copyright 2017 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "crc32c/crc32c.h"

#include <cstddef>
#include <cstdint>

#include "./crc32c_arm64.h"
#include "./crc32c_arm64_check.h"
#include "./crc32c_internal.h"
#include "./crc32c_sse42.h"
#include "./crc32c_sse42_check.h"

namespace crc32c {

uint32_t Extend(uint32_t crc, const uint8_t* data, size_t count) {
#if HAVE_SSE42 && (defined(_M_X64) || defined(__x86_64__))
  static bool can_use_sse42 = CanUseSse42();
  if (can_use_sse42) return ExtendSse42(crc, data, count);
#elif HAVE_ARM64_CRC32C
  static bool can_use_arm64_crc32 = CanUseArm64Crc32();
  if (can_use_arm64_crc32) return ExtendArm64(crc, data, count);
#endif  // HAVE_SSE42 && (defined(_M_X64) || defined(__x86_64__))

  return ExtendPortable(crc, data, count);
}

extern "C" uint32_t crc32c_extend(uint32_t crc, const uint8_t* data,
                                  size_t count) {
  return crc32c::Extend(crc, data, count);
}

extern "C" uint32_t crc32c_value(const uint8_t* data, size_t count) {
  return crc32c::Crc32c(data, count);
}

}  // namespace crc32c
