# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# A fast key-value storage library that provides
# an ordered mapping from string keys to string values.
# Subtree at https://github.com/bitcoin-core/leveldb-subtree
# Upstream at https://github.com/google/leveldb
add_library(leveldb STATIC EXCLUDE_FROM_ALL)

target_sources(leveldb
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/builder.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/c.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/dbformat.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/db_impl.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/db_iter.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/dumpfile.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/filename.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/log_reader.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/log_writer.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/memtable.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/repair.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/table_cache.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/version_edit.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/version_set.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/db/write_batch.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/helpers/memenv/memenv.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/table/block_builder.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/table/block.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/table/filter_block.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/table/format.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/table/iterator.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/table/merger.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/table/table_builder.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/table/table.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/table/two_level_iterator.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/arena.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/bloom.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/cache.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/coding.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/comparator.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/crc32c.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/env.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/filter_policy.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/hash.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/histogram.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/logging.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/options.cc
    ${CMAKE_SOURCE_DIR}/src/leveldb/util/status.cc
)
if(WIN32)
  target_sources(leveldb
    PRIVATE
      ${CMAKE_SOURCE_DIR}/src/leveldb/util/env_windows.cc
  )
else()
  target_sources(leveldb
    PRIVATE
      ${CMAKE_SOURCE_DIR}/src/leveldb/util/env_posix.cc
  )
endif()

target_include_directories(leveldb
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/leveldb>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/leveldb/helpers/memenv>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/leveldb/include>
)

target_compile_definitions(leveldb
  PRIVATE
    __STDC_LIMIT_MACROS
    HAVE_SNAPPY=0
    HAVE_CRC32C=1
    HAVE_FDATASYNC=${HAVE_FDATASYNC}
    FALLTHROUGH_INTENDED=[[fallthrough]]
    LEVELDB_IS_BIG_ENDIAN=${WORDS_BIGENDIAN}
)

check_cxx_symbol_exists(F_FULLFSYNC "fcntl.h" HAVE_FULLFSYNC)
if(HAVE_FULLFSYNC)
  target_compile_definitions(leveldb PRIVATE HAVE_FULLFSYNC)
endif()

check_cxx_symbol_exists(O_CLOEXEC "fcntl.h" HAVE_O_CLOEXEC)
if(HAVE_O_CLOEXEC)
  target_compile_definitions(leveldb PRIVATE HAVE_O_CLOEXEC)
endif()

if(WIN32)
  target_compile_definitions(leveldb
    PRIVATE
      LEVELDB_PLATFORM_WINDOWS
      _UNICODE
      UNICODE
      __USE_MINGW_ANSI_STDIO=1
  )
else()
  target_compile_definitions(leveldb
    PRIVATE
      LEVELDB_PLATFORM_POSIX
  )
endif()

target_link_libraries(leveldb
  PRIVATE
    crc32c
)
