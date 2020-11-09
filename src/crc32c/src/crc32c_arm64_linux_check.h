// Copyright 2017 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// ARM Linux-specific code checking for the availability of CRC32C instructions.

#ifndef CRC32C_CRC32C_ARM_LINUX_CHECK_H_
#define CRC32C_CRC32C_ARM_LINUX_CHECK_H_

// X86-specific code checking for the availability of SSE4.2 instructions.

#include <cstddef>
#include <cstdint>

#ifdef CRC32C_HAVE_CONFIG_H
#include "crc32c/crc32c_config.h"
#endif

#if HAVE_ARM64_CRC32C

#if HAVE_STRONG_GETAUXVAL
#include <sys/auxv.h>
#elif HAVE_WEAK_GETAUXVAL
// getauxval() is not available on Android until API level 20. Link it as a weak
// symbol.
extern "C" unsigned long getauxval(unsigned long type) __attribute__((weak));

#define AT_HWCAP 16
#endif  // HAVE_STRONG_GETAUXVAL || HAVE_WEAK_GETAUXVAL

namespace crc32c {

inline bool CanUseArm64Linux() {
#if HAVE_STRONG_GETAUXVAL || HAVE_WEAK_GETAUXVAL
  // From 'arch/arm64/include/uapi/asm/hwcap.h' in Linux kernel source code.
  constexpr unsigned long kHWCAP_PMULL = 1 << 4;
  constexpr unsigned long kHWCAP_CRC32 = 1 << 7;
  unsigned long hwcap = (&getauxval != nullptr) ? getauxval(AT_HWCAP) : 0;
  return (hwcap & (kHWCAP_PMULL | kHWCAP_CRC32)) ==
         (kHWCAP_PMULL | kHWCAP_CRC32);
#else
  return false;
#endif  // HAVE_STRONG_GETAUXVAL || HAVE_WEAK_GETAUXVAL
}

}  // namespace crc32c

#endif  // HAVE_ARM64_CRC32C

#endif  // CRC32C_CRC32C_ARM_LINUX_CHECK_H_
