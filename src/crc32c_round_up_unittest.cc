// Copyright 2017 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "./crc32c_round_up.h"

#include <cstddef>
#include <cstdint>

#include "gtest/gtest.h"

namespace crc32c {

TEST(CRC32CRoundUpTest, RoundUpUintptr) {
  uintptr_t zero = 0;

  ASSERT_EQ(zero, RoundUp<1>(zero));
  ASSERT_EQ(1U, RoundUp<1>(1U));
  ASSERT_EQ(2U, RoundUp<1>(2U));
  ASSERT_EQ(3U, RoundUp<1>(3U));
  ASSERT_EQ(~static_cast<uintptr_t>(0), RoundUp<1>(~static_cast<uintptr_t>(0)));
  ASSERT_EQ(~static_cast<uintptr_t>(1), RoundUp<1>(~static_cast<uintptr_t>(1)));
  ASSERT_EQ(~static_cast<uintptr_t>(2), RoundUp<1>(~static_cast<uintptr_t>(2)));
  ASSERT_EQ(~static_cast<uintptr_t>(3), RoundUp<1>(~static_cast<uintptr_t>(3)));

  ASSERT_EQ(zero, RoundUp<2>(zero));
  ASSERT_EQ(2U, RoundUp<2>(1U));
  ASSERT_EQ(2U, RoundUp<2>(2U));
  ASSERT_EQ(4U, RoundUp<2>(3U));
  ASSERT_EQ(4U, RoundUp<2>(4U));
  ASSERT_EQ(6U, RoundUp<2>(5U));
  ASSERT_EQ(6U, RoundUp<2>(6U));
  ASSERT_EQ(8U, RoundUp<2>(7U));
  ASSERT_EQ(8U, RoundUp<2>(8U));
  ASSERT_EQ(~static_cast<uintptr_t>(1), RoundUp<2>(~static_cast<uintptr_t>(1)));
  ASSERT_EQ(~static_cast<uintptr_t>(1), RoundUp<2>(~static_cast<uintptr_t>(2)));
  ASSERT_EQ(~static_cast<uintptr_t>(3), RoundUp<2>(~static_cast<uintptr_t>(3)));
  ASSERT_EQ(~static_cast<uintptr_t>(3), RoundUp<2>(~static_cast<uintptr_t>(4)));

  ASSERT_EQ(zero, RoundUp<4>(zero));
  ASSERT_EQ(4U, RoundUp<4>(1U));
  ASSERT_EQ(4U, RoundUp<4>(2U));
  ASSERT_EQ(4U, RoundUp<4>(3U));
  ASSERT_EQ(4U, RoundUp<4>(4U));
  ASSERT_EQ(8U, RoundUp<4>(5U));
  ASSERT_EQ(8U, RoundUp<4>(6U));
  ASSERT_EQ(8U, RoundUp<4>(7U));
  ASSERT_EQ(8U, RoundUp<4>(8U));
  ASSERT_EQ(~static_cast<uintptr_t>(3), RoundUp<4>(~static_cast<uintptr_t>(3)));
  ASSERT_EQ(~static_cast<uintptr_t>(3), RoundUp<4>(~static_cast<uintptr_t>(4)));
  ASSERT_EQ(~static_cast<uintptr_t>(3), RoundUp<4>(~static_cast<uintptr_t>(5)));
  ASSERT_EQ(~static_cast<uintptr_t>(3), RoundUp<4>(~static_cast<uintptr_t>(6)));
  ASSERT_EQ(~static_cast<uintptr_t>(7), RoundUp<4>(~static_cast<uintptr_t>(7)));
  ASSERT_EQ(~static_cast<uintptr_t>(7), RoundUp<4>(~static_cast<uintptr_t>(8)));
  ASSERT_EQ(~static_cast<uintptr_t>(7), RoundUp<4>(~static_cast<uintptr_t>(9)));
}

TEST(CRC32CRoundUpTest, RoundUpPointer) {
  uintptr_t zero = 0, three = 3, four = 4, seven = 7, eight = 8;

  const uint8_t* zero_ptr = reinterpret_cast<const uint8_t*>(zero);
  const uint8_t* three_ptr = reinterpret_cast<const uint8_t*>(three);
  const uint8_t* four_ptr = reinterpret_cast<const uint8_t*>(four);
  const uint8_t* seven_ptr = reinterpret_cast<const uint8_t*>(seven);
  const uint8_t* eight_ptr = reinterpret_cast<uint8_t*>(eight);

  ASSERT_EQ(zero_ptr, RoundUp<1>(zero_ptr));
  ASSERT_EQ(zero_ptr, RoundUp<4>(zero_ptr));
  ASSERT_EQ(zero_ptr, RoundUp<8>(zero_ptr));

  ASSERT_EQ(three_ptr, RoundUp<1>(three_ptr));
  ASSERT_EQ(four_ptr, RoundUp<4>(three_ptr));
  ASSERT_EQ(eight_ptr, RoundUp<8>(three_ptr));

  ASSERT_EQ(four_ptr, RoundUp<1>(four_ptr));
  ASSERT_EQ(four_ptr, RoundUp<4>(four_ptr));
  ASSERT_EQ(eight_ptr, RoundUp<8>(four_ptr));

  ASSERT_EQ(seven_ptr, RoundUp<1>(seven_ptr));
  ASSERT_EQ(eight_ptr, RoundUp<4>(seven_ptr));
  ASSERT_EQ(eight_ptr, RoundUp<8>(four_ptr));
}

}  // namespace crc32c
