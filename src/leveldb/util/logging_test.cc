// Copyright (c) 2018 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <limits>
#include <string>

#include "leveldb/slice.h"
#include "util/logging.h"
#include "util/testharness.h"

namespace leveldb {

class Logging {};

TEST(Logging, NumberToString) {
  ASSERT_EQ("0", NumberToString(0));
  ASSERT_EQ("1", NumberToString(1));
  ASSERT_EQ("9", NumberToString(9));

  ASSERT_EQ("10", NumberToString(10));
  ASSERT_EQ("11", NumberToString(11));
  ASSERT_EQ("19", NumberToString(19));
  ASSERT_EQ("99", NumberToString(99));

  ASSERT_EQ("100", NumberToString(100));
  ASSERT_EQ("109", NumberToString(109));
  ASSERT_EQ("190", NumberToString(190));
  ASSERT_EQ("123", NumberToString(123));
  ASSERT_EQ("12345678", NumberToString(12345678));

  static_assert(std::numeric_limits<uint64_t>::max() == 18446744073709551615U,
                "Test consistency check");
  ASSERT_EQ("18446744073709551000", NumberToString(18446744073709551000U));
  ASSERT_EQ("18446744073709551600", NumberToString(18446744073709551600U));
  ASSERT_EQ("18446744073709551610", NumberToString(18446744073709551610U));
  ASSERT_EQ("18446744073709551614", NumberToString(18446744073709551614U));
  ASSERT_EQ("18446744073709551615", NumberToString(18446744073709551615U));
}

void ConsumeDecimalNumberRoundtripTest(uint64_t number,
                                       const std::string& padding = "") {
  std::string decimal_number = NumberToString(number);
  std::string input_string = decimal_number + padding;
  Slice input(input_string);
  Slice output = input;
  uint64_t result;
  ASSERT_TRUE(ConsumeDecimalNumber(&output, &result));
  ASSERT_EQ(number, result);
  ASSERT_EQ(decimal_number.size(), output.data() - input.data());
  ASSERT_EQ(padding.size(), output.size());
}

TEST(Logging, ConsumeDecimalNumberRoundtrip) {
  ConsumeDecimalNumberRoundtripTest(0);
  ConsumeDecimalNumberRoundtripTest(1);
  ConsumeDecimalNumberRoundtripTest(9);

  ConsumeDecimalNumberRoundtripTest(10);
  ConsumeDecimalNumberRoundtripTest(11);
  ConsumeDecimalNumberRoundtripTest(19);
  ConsumeDecimalNumberRoundtripTest(99);

  ConsumeDecimalNumberRoundtripTest(100);
  ConsumeDecimalNumberRoundtripTest(109);
  ConsumeDecimalNumberRoundtripTest(190);
  ConsumeDecimalNumberRoundtripTest(123);
  ASSERT_EQ("12345678", NumberToString(12345678));

  for (uint64_t i = 0; i < 100; ++i) {
    uint64_t large_number = std::numeric_limits<uint64_t>::max() - i;
    ConsumeDecimalNumberRoundtripTest(large_number);
  }
}

TEST(Logging, ConsumeDecimalNumberRoundtripWithPadding) {
  ConsumeDecimalNumberRoundtripTest(0, " ");
  ConsumeDecimalNumberRoundtripTest(1, "abc");
  ConsumeDecimalNumberRoundtripTest(9, "x");

  ConsumeDecimalNumberRoundtripTest(10, "_");
  ConsumeDecimalNumberRoundtripTest(11, std::string("\0\0\0", 3));
  ConsumeDecimalNumberRoundtripTest(19, "abc");
  ConsumeDecimalNumberRoundtripTest(99, "padding");

  ConsumeDecimalNumberRoundtripTest(100, " ");

  for (uint64_t i = 0; i < 100; ++i) {
    uint64_t large_number = std::numeric_limits<uint64_t>::max() - i;
    ConsumeDecimalNumberRoundtripTest(large_number, "pad");
  }
}

void ConsumeDecimalNumberOverflowTest(const std::string& input_string) {
  Slice input(input_string);
  Slice output = input;
  uint64_t result;
  ASSERT_EQ(false, ConsumeDecimalNumber(&output, &result));
}

TEST(Logging, ConsumeDecimalNumberOverflow) {
  static_assert(std::numeric_limits<uint64_t>::max() == 18446744073709551615U,
                "Test consistency check");
  ConsumeDecimalNumberOverflowTest("18446744073709551616");
  ConsumeDecimalNumberOverflowTest("18446744073709551617");
  ConsumeDecimalNumberOverflowTest("18446744073709551618");
  ConsumeDecimalNumberOverflowTest("18446744073709551619");
  ConsumeDecimalNumberOverflowTest("18446744073709551620");
  ConsumeDecimalNumberOverflowTest("18446744073709551621");
  ConsumeDecimalNumberOverflowTest("18446744073709551622");
  ConsumeDecimalNumberOverflowTest("18446744073709551623");
  ConsumeDecimalNumberOverflowTest("18446744073709551624");
  ConsumeDecimalNumberOverflowTest("18446744073709551625");
  ConsumeDecimalNumberOverflowTest("18446744073709551626");

  ConsumeDecimalNumberOverflowTest("18446744073709551700");

  ConsumeDecimalNumberOverflowTest("99999999999999999999");
}

void ConsumeDecimalNumberNoDigitsTest(const std::string& input_string) {
  Slice input(input_string);
  Slice output = input;
  uint64_t result;
  ASSERT_EQ(false, ConsumeDecimalNumber(&output, &result));
  ASSERT_EQ(input.data(), output.data());
  ASSERT_EQ(input.size(), output.size());
}

TEST(Logging, ConsumeDecimalNumberNoDigits) {
  ConsumeDecimalNumberNoDigitsTest("");
  ConsumeDecimalNumberNoDigitsTest(" ");
  ConsumeDecimalNumberNoDigitsTest("a");
  ConsumeDecimalNumberNoDigitsTest(" 123");
  ConsumeDecimalNumberNoDigitsTest("a123");
  ConsumeDecimalNumberNoDigitsTest(std::string("\000123", 4));
  ConsumeDecimalNumberNoDigitsTest(std::string("\177123", 4));
  ConsumeDecimalNumberNoDigitsTest(std::string("\377123", 4));
}

}  // namespace leveldb

int main(int argc, char** argv) { return leveldb::test::RunAllTests(); }
