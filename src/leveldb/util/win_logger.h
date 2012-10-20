// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Logger implementation for Windows

#ifndef STORAGE_LEVELDB_UTIL_WIN_LOGGER_H_
#define STORAGE_LEVELDB_UTIL_WIN_LOGGER_H_

#include <stdio.h>
#include "leveldb/env.h"

namespace leveldb {

class WinLogger : public Logger {
 private:
  FILE* file_;
 public:
  explicit WinLogger(FILE* f) : file_(f) { assert(file_); }
  virtual ~WinLogger() {
    fclose(file_);
  }
  virtual void Logv(const char* format, va_list ap);

};

}
#endif  // STORAGE_LEVELDB_UTIL_WIN_LOGGER_H_
