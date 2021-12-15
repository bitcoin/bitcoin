// Copyright 2017 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef CRC32C_HAVE_CONFIG_H
#include "crc32c/crc32c_config.h"
#endif

#include "gtest/gtest.h"

#if CRC32C_TESTS_BUILT_WITH_GLOG
#include "glog/logging.h"
#endif  // CRC32C_TESTS_BUILT_WITH_GLOG

int main(int argc, char** argv) {
#if CRC32C_TESTS_BUILT_WITH_GLOG
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif  // CRC32C_TESTS_BUILT_WITH_GLOG
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
