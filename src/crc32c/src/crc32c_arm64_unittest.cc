// Copyright 2017 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "gtest/gtest.h"

#include "./crc32c_arm64.h"
#include "./crc32c_extend_unittests.h"

namespace crc32c {

#if HAVE_ARM64_CRC32C

struct Arm64TestTraits {
  static uint32_t Extend(uint32_t crc, const uint8_t* data, size_t count) {
    return ExtendArm64(crc, data, count);
  }
};

INSTANTIATE_TYPED_TEST_SUITE_P(Arm64, ExtendTest, Arm64TestTraits);

#endif  // HAVE_ARM64_CRC32C

}  // namespace crc32c
