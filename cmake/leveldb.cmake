# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# This file is part of the transition from Autotools to CMake. Once CMake
# support has been merged we should switch to using the upstream CMake
# buildsystem.

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(F_FULLFSYNC "fcntl.h" HAVE_FULLFSYNC)

add_library(leveldb STATIC EXCLUDE_FROM_ALL
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/builder.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/c.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/db_impl.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/db_iter.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/dbformat.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/dumpfile.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/filename.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/log_reader.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/log_writer.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/memtable.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/repair.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/table_cache.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/version_edit.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/version_set.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/db/write_batch.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/table/block.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/table/block_builder.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/table/filter_block.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/table/format.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/table/iterator.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/table/merger.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/table/table.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/table/table_builder.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/table/two_level_iterator.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/arena.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/bloom.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/cache.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/coding.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/comparator.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/crc32c.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/env.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/filter_policy.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/hash.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/histogram.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/logging.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/options.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/util/status.cc
  ${PROJECT_SOURCE_DIR}/src/leveldb/helpers/memenv/memenv.cc
)
if(WIN32)
  target_sources(leveldb PRIVATE ${PROJECT_SOURCE_DIR}/src/leveldb/util/env_windows.cc)
  set_property(SOURCE ${PROJECT_SOURCE_DIR}/src/leveldb/util/env_windows.cc
    APPEND PROPERTY COMPILE_OPTIONS $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/wd4722>
  )
else()
  target_sources(leveldb PRIVATE ${PROJECT_SOURCE_DIR}/src/leveldb/util/env_posix.cc)
endif()

target_compile_definitions(leveldb
  PRIVATE
    HAVE_SNAPPY=0
    HAVE_CRC32C=1
    HAVE_FDATASYNC=$<BOOL:${HAVE_FDATASYNC}>
    HAVE_FULLFSYNC=$<BOOL:${HAVE_FULLFSYNC}>
    HAVE_O_CLOEXEC=$<BOOL:${HAVE_O_CLOEXEC}>
    FALLTHROUGH_INTENDED=[[fallthrough]]
    LEVELDB_IS_BIG_ENDIAN=${WORDS_BIGENDIAN}
)

if(WIN32)
  target_compile_definitions(leveldb
    PRIVATE
      LEVELDB_PLATFORM_WINDOWS
      _UNICODE
      UNICODE
      __USE_MINGW_ANSI_STDIO=1
  )
else()
  target_compile_definitions(leveldb PRIVATE LEVELDB_PLATFORM_POSIX)
endif()

target_include_directories(leveldb
  PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/leveldb>
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/leveldb/include>
)

#TODO: figure out how to filter out:
# -Wconditional-uninitialized -Werror=conditional-uninitialized -Wsuggest-override -Werror=suggest-override

target_link_libraries(leveldb PRIVATE crc32c)
