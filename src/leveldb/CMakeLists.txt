cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

project(leveldb CXX)    
set(CMAKE_DEBUG_POSTFIX "d")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREAD ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost COMPONENTS 
    date_time
    filesystem
    system
    REQUIRED)

set(SNAPPY_LIBRARY "")

string(REGEX MATCH "clang" CLANG ${CMAKE_CXX_COMPILER})

if(CMAKE_COMPILER_IS_GNUCXX OR CLANG)
    find_library(Pthread_LIBRARY pthread)
    find_library(Realtime_LIBRARY rt)
    # find library can be problematic with stdc++ which is why we hardwire the link
    set(Stdcpp_LIBRARY stdc++)
else(CMAKE_COMPILER_IS_GNUCXX OR CLANG)
    set(Pthread_LIBRARY "")
    set(Realtime_LIBRARY "")
    set(Stdcpp_LIBRARY "")
endif(CMAKE_COMPILER_IS_GNUCXX OR CLANG)

include_directories(${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    include)

if(MSVC)
    add_compile_options(
        /D_CRT_SECURE_NO_WARNINGS
        /wd4389 # signed/unsigned mismatch
        /wd4800 # constructor never returns, potential memory leak because of a singleton pattern
        /wd4722 # unreachable code because of singleton pattern
        /wd4702 # bool cast performance warning
    )
else()
    add_compile_options(
        -Wno-sign-compare
        -std=c++11
    )
endif()

add_definitions(
    -DLEVELDB_ATOMIC_PRESENT
)

set(LEVEL_DB_FILES
    include/leveldb/c.h
    include/leveldb/cache.h
    include/leveldb/comparator.h
    include/leveldb/db.h
    include/leveldb/dumpfile.h
    include/leveldb/env.h
    include/leveldb/iterator.h
    include/leveldb/filter_policy.h
    include/leveldb/iterator.h
    include/leveldb/options.h
    include/leveldb/slice.h
    include/leveldb/status.h
    include/leveldb/table.h
    include/leveldb/table_builder.h
    include/leveldb/write_batch.h
	db/builder.cc
    db/builder.h
	db/db_impl.cc
    db/db_impl.h
	db/db_iter.cc
    db/db_iter.h
    db/dbformat.cc
    db/dbformat.h
    db/dumpfile.cc
	db/filename.cc
    db/filename.h
    db/log_format.h
	db/log_reader.cc
    db/log_reader.h
	db/log_writer.cc
    db/log_writer.h
    db/skiplist.h
    db/snapshot.h
	db/memtable.cc
    db/memtable.h
	db/repair.cc
	db/table_cache.cc
    db/table_cache.h
	db/version_edit.cc
    db/version_edit.h
	db/version_set.cc
    db/version_set.h
	db/write_batch.cc
	table/block.cc
    table/block.h
	table/block_builder.cc
    table/block_builder.h
    table/filter_block.cc
    table/filter_block.h
	table/format.cc
    table/format.h
	table/iterator.cc
    table/iterator_wrapper.h
	table/merger.cc
    table/merger.h
	table/table.cc
	table/table_builder.cc
	table/two_level_iterator.cc
    table/two_level_iterator.h
	util/arena.cc
    util/arena.h
    util/bloom.cc
	util/cache.cc
	util/coding.cc
    util/coding.h
	util/comparator.cc
	util/crc32c.cc
    util/crc32c.h
	util/env.cc
    util/filter_policy.cc
	util/hash.cc
    util/hash.h
	util/histogram.cc
    util/histogram.h
	util/logging.cc
    util/logging.h
    util/mutexlock.h
	util/options.cc
    util/random.h
	util/status.cc
    port/port.h)

if(WIN32)
    list(APPEND LEVEL_DB_FILES
        port/port_win.h
        port/port_win.cc
        util/win_logger.h
        util/win_logger.cc
        util/env_boost.cc)
else()
    list(APPEND LEVEL_DB_FILES
        port/port_posix.h
        port/port_posix.cc
        util/posix_logger.h
        util/env_posix.cc)
endif()

add_library(memenv helpers/memenv/memenv.cc)

target_include_directories(memenv
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(leveldb ${LEVEL_DB_FILES})

target_include_directories(leveldb 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} 
)

target_link_libraries(leveldb 
    PRIVATE
    ${Boost_LIBRARIES}
    ${Pthread_LIBRARY}
)

##################################### TESTS #######################################
# Every leveldb test file has to be compiled as an independant binary
# because of the test framework used by leveldb.
add_library(leveldb_test_rt 
    util/testutil.h
    util/testutil.cc
    util/testharness.h
    util/testharness.cc)

target_include_directories(leveldb_test_rt
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include
)
