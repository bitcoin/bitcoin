// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/log_writer.h"

#include <stdint.h>
#include "leveldb/env.h"
#include "util/coding.h"
#include "util/crc32c.h"

namespace leveldb {
namespace log {

static void InitTypeCrc(uint32_t* type_crc) {
  for (int i = 0; i <= kMaxRecordType; i++) {
    char t = static_cast<char>(i);
    type_crc[i] = crc32c::Value(&t, 1);
  }
}

Writer::Writer(WritableFile* dest)
    : dest_(dest),
      block_offset_(0) {
  InitTypeCrc(type_crc_);
}

Writer::Writer(WritableFile* dest, uint64_t dest_length)
    : dest_(dest), block_offset_(dest_length % kBlockSize) {
  InitTypeCrc(type_crc_);
}

Writer::~Writer() {
}

Status Writer::AddRecord(const Slice& slice) {
  const char* ptr = slice.data();
  size_t left = slice.size();

  // Fragment the record if necessary and emit it.  Note that if slice
  // is empty, we still want to iterate once to emit a single
  // zero-length record
  Status s;
  bool begin = true;
  do {
    const int leftover = kBlockSize - block_offset_;
    assert(leftover >= 0);
    if (leftover < kHeaderSize) {
      // Switch to a new block
      if (leftover > 0) {
        // Fill the trailer (literal below relies on kHeaderSize being 7)
        assert(kHeaderSize == 7);
        dest_->Append(Slice("\x00\x00\x00\x00\x00\x00", leftover));
      }
      block_offset_ = 0;
    }

    // Invariant: we never leave < kHeaderSize bytes in a block.
    assert(kBlockSize - block_offset_ - kHeaderSize >= 0);

    const size_t avail = kBlockSize - block_offset_ - kHeaderSize;
    const size_t fragment_length = (left < avail) ? left : avail;

    RecordType type;
    const bool end = (left == fragment_length);
    if (begin && end) {
      type = kFullType;
    } else if (begin) {
      type = kFirstType;
    } else if (end) {
      type = kLastType;
    } else {
      type = kMiddleType;
    }

    s = EmitPhysicalRecord(type, ptr, fragment_length);
    ptr += fragment_length;
    left -= fragment_length;
    begin = false;
  } while (s.ok() && left > 0);
  return s;
}

Status Writer::EmitPhysicalRecord(RecordType t, const char* ptr, size_t n) {
  assert(n <= 0xffff);  // Must fit in two bytes
  assert(block_offset_ + kHeaderSize + n <= kBlockSize);

  // Format the header
  char buf[kHeaderSize];
  buf[4] = static_cast<char>(n & 0xff);
  buf[5] = static_cast<char>(n >> 8);
  buf[6] = static_cast<char>(t);

  // Compute the crc of the record type and the payload.
  uint32_t crc = crc32c::Extend(type_crc_[t], ptr, n);
  crc = crc32c::Mask(crc);                 // Adjust for storage
  EncodeFixed32(buf, crc);

  // Write the header and the payload
  Status s = dest_->Append(Slice(buf, kHeaderSize));
  if (s.ok()) {
    s = dest_->Append(Slice(ptr, n));
    if (s.ok()) {
      s = dest_->Flush();
    }
  }
  block_offset_ += kHeaderSize + n;
  return s;
}

}  // namespace log
}  // namespace leveldb
