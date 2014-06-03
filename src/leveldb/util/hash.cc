// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <string.h>
#include "util/coding.h"
#include "util/hash.h"

namespace leveldb {

uint32_t Hash(const char* data, size_t n, uint32_t seed) {
  // Similar to murmur hash
  const uint32_t m = 0xc6a4a793;
  const uint32_t r = 24;
  const char* limit = data + n;
  uint32_t h = seed ^ (n * m);

  // Pick up four bytes at a time
  while (data + 4 <= limit) {
    uint32_t w = DecodeFixed32(data);
    data += 4;
    h += w;
    h *= m;
    h ^= (h >> 16);
  }

  // Pick up remaining bytes
  switch (limit - data) {
    case 3:
      h += data[2] << 16;
      // fall through
    case 2:
      h += data[1] << 8;
      // fall through
    case 1:
      h += data[0];
      h *= m;
      h ^= (h >> r);
      break;
  }
  return h;
}


}  // namespace leveldb
