// Copyright 2017 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef CRC32C_CRC32C_READ_LE_H_
#define CRC32C_CRC32C_READ_LE_H_

#include <cstdint>
#include <cstring>

namespace crc32c {

// Reads a little-endian 32-bit integer from a 32-bit-aligned buffer.
inline uint32_t ReadUint32LE(const uint8_t* buffer) {
  return ((static_cast<uint32_t>(static_cast<uint8_t>(buffer[0]))) |
          (static_cast<uint32_t>(static_cast<uint8_t>(buffer[1])) << 8) |
          (static_cast<uint32_t>(static_cast<uint8_t>(buffer[2])) << 16) |
          (static_cast<uint32_t>(static_cast<uint8_t>(buffer[3])) << 24));
}

// Reads a little-endian 64-bit integer from a 64-bit-aligned buffer.
inline uint64_t ReadUint64LE(const uint8_t* buffer) {
  return ((static_cast<uint64_t>(static_cast<uint8_t>(buffer[0]))) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buffer[1])) << 8) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buffer[2])) << 16) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buffer[3])) << 24) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buffer[4])) << 32) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buffer[5])) << 40) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buffer[6])) << 48) |
          (static_cast<uint64_t>(static_cast<uint8_t>(buffer[7])) << 56));
}

}  // namespace crc32c

#endif  // CRC32C_CRC32C_READ_LE_H_
