// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "leveldb/c.h"

#include <stdlib.h>
#include <unistd.h>
#include "leveldb/cache.h"
#include "leveldb/comparator.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/filter_policy.h"
#include "leveldb/iterator.h"
#include "leveldb/options.h"
#include "leveldb/status.h"
#include "leveldb/write_batch.h"

using leveldb::Cache;
using leveldb::Comparator;
using leveldb::CompressionType;
using leveldb::DB;
using leveldb::Env;
using leveldb::FileLock;
using leveldb::FilterPolicy;
using leveldb::Iterator;
using leveldb::Logger;
using leveldb::NewBloomFilterPolicy;
using leveldb::NewLRUCache;
using leveldb::Options;
using leveldb::RandomAccessFile;
using leveldb::Range;
using leveldb::ReadOptions;
using leveldb::SequentialFile;
using leveldb::Slice;
using leveldb::Snapshot;
using leveldb::Status;
using leveldb::WritableFile;
using leveldb::WriteBatch;
using leveldb::WriteOptions;

extern "C" {

struct leveldb_t              { DB*               rep; };
struct leveldb_iterator_t     { Iterator*         rep; };
struct leveldb_writebatch_t   { WriteBatch        rep; };
struct leveldb_snapshot_t     { const Snapshot*   rep; };
struct leveldb_readoptions_t  { ReadOptions       rep; };
struct leveldb_writeoptions_t { WriteOptions      rep; };
struct leveldb_options_t      { Options           rep; };
struct leveldb_cache_t        { Cache*            rep; };
struct leveldb_seqfile_t      { SequentialFile*   rep; };
struct leveldb_randomfile_t   { RandomAccessFile* rep; };
struct leveldb_writablefile_t { WritableFile*     rep; };
struct leveldb_logger_t       { Logger*           rep; };
struct leveldb_filelock_t     { FileLock*         rep; };

struct leveldb_comparator_t : public Comparator {
  void* state_;
  void (*destructor_)(void*);
  int (*compare_)(
      void*,
      const char* a, size_t alen,
      const char* b, size_t blen);
  const char* (*name_)(void*);

  virtual ~leveldb_comparator_t() {
    (*destructor_)(state_);
  }

  virtual int Compare(const Slice& a, const Slice& b) const {
    return (*compare_)(state_, a.data(), a.size(), b.data(), b.size());
  }

  virtual const char* Name() const {
    return (*name_)(state_);
  }

  // No-ops since the C binding does not support key shortening methods.
  virtual void FindShortestSeparator(std::string*, const Slice&) const { }
  virtual void FindShortSuccessor(std::string* key) const { }
};

struct leveldb_filterpolicy_t : public FilterPolicy {
  void* state_;
  void (*destructor_)(void*);
  const char* (*name_)(void*);
  char* (*create_)(
      void*,
      const char* const* key_array, const size_t* key_length_array,
      int num_keys,
      size_t* filter_length);
  unsigned char (*key_match_)(
      void*,
      const char* key, size_t length,
      const char* filter, size_t filter_length);

  virtual ~leveldb_filterpolicy_t() {
    (*destructor_)(state_);
  }

  virtual const char* Name() const {
    return (*name_)(state_);
  }

  virtual void CreateFilter(const Slice* keys, int n, std::string* dst) const {
    std::vector<const char*> key_pointers(n);
    std::vector<size_t> key_sizes(n);
    for (int i = 0; i < n; i++) {
      key_pointers[i] = keys[i].data();
      key_sizes[i] = keys[i].size();
    }
    size_t len;
    char* filter = (*create_)(state_, &key_pointers[0], &key_sizes[0], n, &len);
    dst->append(filter, len);
    free(filter);
  }

  virtual bool KeyMayMatch(const Slice& key, const Slice& filter) const {
    return (*key_match_)(state_, key.data(), key.size(),
                         filter.data(), filter.size());
  }
};

struct leveldb_env_t {
  Env* rep;
  bool is_default;
};

static bool SaveError(char** errptr, const Status& s) {
  assert(errptr != NULL);
  if (s.ok()) {
    return false;
  } else if (*errptr == NULL) {
    *errptr = strdup(s.ToString().c_str());
  } else {
    // TODO(sanjay): Merge with existing error?
    free(*errptr);
    *errptr = strdup(s.ToString().c_str());
  }
  return true;
}

static char* CopyString(const std::string& str) {
  char* result = reinterpret_cast<char*>(malloc(sizeof(char) * str.size()));
  memcpy(result, str.data(), sizeof(char) * str.size());
  return result;
}

leveldb_t* leveldb_open(
    const leveldb_options_t* options,
    const char* name,
    char** errptr) {
  DB* db;
  if (SaveError(errptr, DB::Open(options->rep, std::string(name), &db))) {
    return NULL;
  }
  leveldb_t* result = new leveldb_t;
  result->rep = db;
  return result;
}

void leveldb_close(leveldb_t* db) {
  delete db->rep;
  delete db;
}

void leveldb_put(
    leveldb_t* db,
    const leveldb_writeoptions_t* options,
    const char* key, size_t keylen,
    const char* val, size_t vallen,
    char** errptr) {
  SaveError(errptr,
            db->rep->Put(options->rep, Slice(key, keylen), Slice(val, vallen)));
}

void leveldb_delete(
    leveldb_t* db,
    const leveldb_writeoptions_t* options,
    const char* key, size_t keylen,
    char** errptr) {
  SaveError(errptr, db->rep->Delete(options->rep, Slice(key, keylen)));
}


void leveldb_write(
    leveldb_t* db,
    const leveldb_writeoptions_t* options,
    leveldb_writebatch_t* batch,
    char** errptr) {
  SaveError(errptr, db->rep->Write(options->rep, &batch->rep));
}

char* leveldb_get(
    leveldb_t* db,
    const leveldb_readoptions_t* options,
    const char* key, size_t keylen,
    size_t* vallen,
    char** errptr) {
  char* result = NULL;
  std::string tmp;
  Status s = db->rep->Get(options->rep, Slice(key, keylen), &tmp);
  if (s.ok()) {
    *vallen = tmp.size();
    result = CopyString(tmp);
  } else {
    *vallen = 0;
    if (!s.IsNotFound()) {
      SaveError(errptr, s);
    }
  }
  return result;
}

leveldb_iterator_t* leveldb_create_iterator(
    leveldb_t* db,
    const leveldb_readoptions_t* options) {
  leveldb_iterator_t* result = new leveldb_iterator_t;
  result->rep = db->rep->NewIterator(options->rep);
  return result;
}

const leveldb_snapshot_t* leveldb_create_snapshot(
    leveldb_t* db) {
  leveldb_snapshot_t* result = new leveldb_snapshot_t;
  result->rep = db->rep->GetSnapshot();
  return result;
}

void leveldb_release_snapshot(
    leveldb_t* db,
    const leveldb_snapshot_t* snapshot) {
  db->rep->ReleaseSnapshot(snapshot->rep);
  delete snapshot;
}

char* leveldb_property_value(
    leveldb_t* db,
    const char* propname) {
  std::string tmp;
  if (db->rep->GetProperty(Slice(propname), &tmp)) {
    // We use strdup() since we expect human readable output.
    return strdup(tmp.c_str());
  } else {
    return NULL;
  }
}

void leveldb_approximate_sizes(
    leveldb_t* db,
    int num_ranges,
    const char* const* range_start_key, const size_t* range_start_key_len,
    const char* const* range_limit_key, const size_t* range_limit_key_len,
    uint64_t* sizes) {
  Range* ranges = new Range[num_ranges];
  for (int i = 0; i < num_ranges; i++) {
    ranges[i].start = Slice(range_start_key[i], range_start_key_len[i]);
    ranges[i].limit = Slice(range_limit_key[i], range_limit_key_len[i]);
  }
  db->rep->GetApproximateSizes(ranges, num_ranges, sizes);
  delete[] ranges;
}

void leveldb_compact_range(
    leveldb_t* db,
    const char* start_key, size_t start_key_len,
    const char* limit_key, size_t limit_key_len) {
  Slice a, b;
  db->rep->CompactRange(
      // Pass NULL Slice if corresponding "const char*" is NULL
      (start_key ? (a = Slice(start_key, start_key_len), &a) : NULL),
      (limit_key ? (b = Slice(limit_key, limit_key_len), &b) : NULL));
}

void leveldb_destroy_db(
    const leveldb_options_t* options,
    const char* name,
    char** errptr) {
  SaveError(errptr, DestroyDB(name, options->rep));
}

void leveldb_repair_db(
    const leveldb_options_t* options,
    const char* name,
    char** errptr) {
  SaveError(errptr, RepairDB(name, options->rep));
}

void leveldb_iter_destroy(leveldb_iterator_t* iter) {
  delete iter->rep;
  delete iter;
}

unsigned char leveldb_iter_valid(const leveldb_iterator_t* iter) {
  return iter->rep->Valid();
}

void leveldb_iter_seek_to_first(leveldb_iterator_t* iter) {
  iter->rep->SeekToFirst();
}

void leveldb_iter_seek_to_last(leveldb_iterator_t* iter) {
  iter->rep->SeekToLast();
}

void leveldb_iter_seek(leveldb_iterator_t* iter, const char* k, size_t klen) {
  iter->rep->Seek(Slice(k, klen));
}

void leveldb_iter_next(leveldb_iterator_t* iter) {
  iter->rep->Next();
}

void leveldb_iter_prev(leveldb_iterator_t* iter) {
  iter->rep->Prev();
}

const char* leveldb_iter_key(const leveldb_iterator_t* iter, size_t* klen) {
  Slice s = iter->rep->key();
  *klen = s.size();
  return s.data();
}

const char* leveldb_iter_value(const leveldb_iterator_t* iter, size_t* vlen) {
  Slice s = iter->rep->value();
  *vlen = s.size();
  return s.data();
}

void leveldb_iter_get_error(const leveldb_iterator_t* iter, char** errptr) {
  SaveError(errptr, iter->rep->status());
}

leveldb_writebatch_t* leveldb_writebatch_create() {
  return new leveldb_writebatch_t;
}

void leveldb_writebatch_destroy(leveldb_writebatch_t* b) {
  delete b;
}

void leveldb_writebatch_clear(leveldb_writebatch_t* b) {
  b->rep.Clear();
}

void leveldb_writebatch_put(
    leveldb_writebatch_t* b,
    const char* key, size_t klen,
    const char* val, size_t vlen) {
  b->rep.Put(Slice(key, klen), Slice(val, vlen));
}

void leveldb_writebatch_delete(
    leveldb_writebatch_t* b,
    const char* key, size_t klen) {
  b->rep.Delete(Slice(key, klen));
}

void leveldb_writebatch_iterate(
    leveldb_writebatch_t* b,
    void* state,
    void (*put)(void*, const char* k, size_t klen, const char* v, size_t vlen),
    void (*deleted)(void*, const char* k, size_t klen)) {
  class H : public WriteBatch::Handler {
   public:
    void* state_;
    void (*put_)(void*, const char* k, size_t klen, const char* v, size_t vlen);
    void (*deleted_)(void*, const char* k, size_t klen);
    virtual void Put(const Slice& key, const Slice& value) {
      (*put_)(state_, key.data(), key.size(), value.data(), value.size());
    }
    virtual void Delete(const Slice& key) {
      (*deleted_)(state_, key.data(), key.size());
    }
  };
  H handler;
  handler.state_ = state;
  handler.put_ = put;
  handler.deleted_ = deleted;
  b->rep.Iterate(&handler);
}

leveldb_options_t* leveldb_options_create() {
  return new leveldb_options_t;
}

void leveldb_options_destroy(leveldb_options_t* options) {
  delete options;
}

void leveldb_options_set_comparator(
    leveldb_options_t* opt,
    leveldb_comparator_t* cmp) {
  opt->rep.comparator = cmp;
}

void leveldb_options_set_filter_policy(
    leveldb_options_t* opt,
    leveldb_filterpolicy_t* policy) {
  opt->rep.filter_policy = policy;
}

void leveldb_options_set_create_if_missing(
    leveldb_options_t* opt, unsigned char v) {
  opt->rep.create_if_missing = v;
}

void leveldb_options_set_error_if_exists(
    leveldb_options_t* opt, unsigned char v) {
  opt->rep.error_if_exists = v;
}

void leveldb_options_set_paranoid_checks(
    leveldb_options_t* opt, unsigned char v) {
  opt->rep.paranoid_checks = v;
}

void leveldb_options_set_env(leveldb_options_t* opt, leveldb_env_t* env) {
  opt->rep.env = (env ? env->rep : NULL);
}

void leveldb_options_set_info_log(leveldb_options_t* opt, leveldb_logger_t* l) {
  opt->rep.info_log = (l ? l->rep : NULL);
}

void leveldb_options_set_write_buffer_size(leveldb_options_t* opt, size_t s) {
  opt->rep.write_buffer_size = s;
}

void leveldb_options_set_max_open_files(leveldb_options_t* opt, int n) {
  opt->rep.max_open_files = n;
}

void leveldb_options_set_cache(leveldb_options_t* opt, leveldb_cache_t* c) {
  opt->rep.block_cache = c->rep;
}

void leveldb_options_set_block_size(leveldb_options_t* opt, size_t s) {
  opt->rep.block_size = s;
}

void leveldb_options_set_block_restart_interval(leveldb_options_t* opt, int n) {
  opt->rep.block_restart_interval = n;
}

void leveldb_options_set_compression(leveldb_options_t* opt, int t) {
  opt->rep.compression = static_cast<CompressionType>(t);
}

leveldb_comparator_t* leveldb_comparator_create(
    void* state,
    void (*destructor)(void*),
    int (*compare)(
        void*,
        const char* a, size_t alen,
        const char* b, size_t blen),
    const char* (*name)(void*)) {
  leveldb_comparator_t* result = new leveldb_comparator_t;
  result->state_ = state;
  result->destructor_ = destructor;
  result->compare_ = compare;
  result->name_ = name;
  return result;
}

void leveldb_comparator_destroy(leveldb_comparator_t* cmp) {
  delete cmp;
}

leveldb_filterpolicy_t* leveldb_filterpolicy_create(
    void* state,
    void (*destructor)(void*),
    char* (*create_filter)(
        void*,
        const char* const* key_array, const size_t* key_length_array,
        int num_keys,
        size_t* filter_length),
    unsigned char (*key_may_match)(
        void*,
        const char* key, size_t length,
        const char* filter, size_t filter_length),
    const char* (*name)(void*)) {
  leveldb_filterpolicy_t* result = new leveldb_filterpolicy_t;
  result->state_ = state;
  result->destructor_ = destructor;
  result->create_ = create_filter;
  result->key_match_ = key_may_match;
  result->name_ = name;
  return result;
}

void leveldb_filterpolicy_destroy(leveldb_filterpolicy_t* filter) {
  delete filter;
}

leveldb_filterpolicy_t* leveldb_filterpolicy_create_bloom(int bits_per_key) {
  // Make a leveldb_filterpolicy_t, but override all of its methods so
  // they delegate to a NewBloomFilterPolicy() instead of user
  // supplied C functions.
  struct Wrapper : public leveldb_filterpolicy_t {
    const FilterPolicy* rep_;
    ~Wrapper() { delete rep_; }
    const char* Name() const { return rep_->Name(); }
    void CreateFilter(const Slice* keys, int n, std::string* dst) const {
      return rep_->CreateFilter(keys, n, dst);
    }
    bool KeyMayMatch(const Slice& key, const Slice& filter) const {
      return rep_->KeyMayMatch(key, filter);
    }
    static void DoNothing(void*) { }
  };
  Wrapper* wrapper = new Wrapper;
  wrapper->rep_ = NewBloomFilterPolicy(bits_per_key);
  wrapper->state_ = NULL;
  wrapper->destructor_ = &Wrapper::DoNothing;
  return wrapper;
}

leveldb_readoptions_t* leveldb_readoptions_create() {
  return new leveldb_readoptions_t;
}

void leveldb_readoptions_destroy(leveldb_readoptions_t* opt) {
  delete opt;
}

void leveldb_readoptions_set_verify_checksums(
    leveldb_readoptions_t* opt,
    unsigned char v) {
  opt->rep.verify_checksums = v;
}

void leveldb_readoptions_set_fill_cache(
    leveldb_readoptions_t* opt, unsigned char v) {
  opt->rep.fill_cache = v;
}

void leveldb_readoptions_set_snapshot(
    leveldb_readoptions_t* opt,
    const leveldb_snapshot_t* snap) {
  opt->rep.snapshot = (snap ? snap->rep : NULL);
}

leveldb_writeoptions_t* leveldb_writeoptions_create() {
  return new leveldb_writeoptions_t;
}

void leveldb_writeoptions_destroy(leveldb_writeoptions_t* opt) {
  delete opt;
}

void leveldb_writeoptions_set_sync(
    leveldb_writeoptions_t* opt, unsigned char v) {
  opt->rep.sync = v;
}

leveldb_cache_t* leveldb_cache_create_lru(size_t capacity) {
  leveldb_cache_t* c = new leveldb_cache_t;
  c->rep = NewLRUCache(capacity);
  return c;
}

void leveldb_cache_destroy(leveldb_cache_t* cache) {
  delete cache->rep;
  delete cache;
}

leveldb_env_t* leveldb_create_default_env() {
  leveldb_env_t* result = new leveldb_env_t;
  result->rep = Env::Default();
  result->is_default = true;
  return result;
}

void leveldb_env_destroy(leveldb_env_t* env) {
  if (!env->is_default) delete env->rep;
  delete env;
}

}  // end extern "C"
