// Copyright 2017 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "./crc32c_prefetch.h"

// There is no easy way to test cache prefetching. We can only test that the
// crc32c_prefetch.h header compiles on its own, so it doesn't have any unstated
// dependencies.
