// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_HELPERS_MEMENV_MEMENV_H_
#define STORAGE_LEVELDB_HELPERS_MEMENV_MEMENV_H_

#include "leveldb/export.h"

namespace leveldb {

class Env;

// Returns a new environment that stores its data in memory and delegates
// all non-file-storage tasks to base_env. The caller must delete the result
// when it is no longer needed.
// *base_env must remain live while the result is in use.
LEVELDB_EXPORT Env* NewMemEnv(Env* base_env);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_HELPERS_MEMENV_MEMENV_H_
