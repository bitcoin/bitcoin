// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "leveldb.h"
#include "util.h"

#include <leveldb/env.h>
#include <leveldb/cache.h>
#include <leveldb/filter_policy.h>
#include <memenv/memenv.h>

#include <boost/filesystem.hpp>

void HandleError(const leveldb::Status &status) throw(leveldb_error) {
    if (status.ok())
        return;
    if (status.IsCorruption())
        throw leveldb_error("Database corrupted");
    if (status.IsIOError())
        throw leveldb_error("Database I/O error");
    if (status.IsNotFound())
        throw leveldb_error("Database entry missing");
    throw leveldb_error("Unknown database error");
}

static leveldb::Options GetOptions(size_t nCacheSize) {
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(nCacheSize / 2);
    options.write_buffer_size = nCacheSize / 4; // up to two write buffers may be held in memory simultaneously
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.compression = leveldb::kNoCompression;
    options.max_open_files = 64;
    return options;
}

CLevelDB::CLevelDB(const boost::filesystem::path &path, size_t nCacheSize, bool fMemory, bool fWipe) {
    penv = NULL;
    readoptions.verify_checksums = true;
    iteroptions.verify_checksums = true;
    iteroptions.fill_cache = false;
    syncoptions.sync = true;
    options = GetOptions(nCacheSize);
    options.create_if_missing = true;
    if (fMemory) {
        penv = leveldb::NewMemEnv(leveldb::Env::Default());
        options.env = penv;
    } else {
        if (fWipe) {
            printf("Wiping LevelDB in %s\n", path.string().c_str());
            leveldb::DestroyDB(path.string(), options);
        }
        boost::filesystem::create_directory(path);
        printf("Opening LevelDB in %s\n", path.string().c_str());
    }
    leveldb::Status status = leveldb::DB::Open(options, path.string(), &pdb);
    if (!status.ok())
        throw std::runtime_error(strprintf("CLevelDB(): error opening database environment %s", status.ToString().c_str()));
    printf("Opened LevelDB successfully\n");
}

CLevelDB::~CLevelDB() {
    delete pdb;
    pdb = NULL;
    delete options.filter_policy;
    options.filter_policy = NULL;
    delete options.block_cache;
    options.block_cache = NULL;
    delete penv;
    options.env = NULL;
}

bool CLevelDB::WriteBatch(CLevelDBBatch &batch, bool fSync) throw(leveldb_error) {
    leveldb::Status status = pdb->Write(fSync ? syncoptions : writeoptions, &batch.batch);
    if (!status.ok()) {
        printf("LevelDB write failure: %s\n", status.ToString().c_str());
        HandleError(status);
        return false;
    }
    return true;
}
