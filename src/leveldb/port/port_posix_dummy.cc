// Copyright 2018 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Dummy file for when no accelerated CRC32C is available on the platform.

#include <stdint.h>
#include <string.h>
#include "port/port.h"

uint32_t AcceleratedCRC32C(uint32_t crc, const char* buf, size_t size) {
  return 0;
}

}  // namespace port
}  // namespace leveldb
