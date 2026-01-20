// Copyright 2017 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "gtest/gtest.h"

#include "./crc32c_extend_unittests.h"
#include "./crc32c_internal.h"

namespace crc32c {

struct PortableTestTraits {
  static uint32_t Extend(uint32_t crc, const uint8_t* data, size_t count) {
    return ExtendPortable(crc, data, count);
  }
};

INSTANTIATE_TYPED_TEST_SUITE_P(Portable, ExtendTest, PortableTestTraits);

}  // namespace crc32c
