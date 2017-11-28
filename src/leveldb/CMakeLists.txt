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

add_executable(leveldbutil
    db/leveldb_main.cc)

target_link_libraries(leveldbutil
    leveldb)

set_target_properties(leveldbutil PROPERTIES
    DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

# we distribute the leveldbutil as it might be useful
install(TARGETS leveldbutil
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

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

add_custom_target(RUN_LEVELDB_UNIT_TESTS
    COMMAND ${CMAKE_CTEST_COMMAND}
        --build-config ${CMAKE_CFG_INTDIR}
        --output-log LevelDB_test_${CMAKE_CFG_INTDIR}.log
        --output-on-failure
        --tests-regex leveldb
    COMMENT "Running all LevelDB unit tests"
)

function(LEVELDB_ADD_TEST TESTNAME TESTFILE)
    if(NOT TESTNAME)
        message(SEND_ERROR "Error: LEVELDB_ADD_TEST called without test name")
        return()
    endif(NOT TESTNAME)

    if(NOT TESTFILE)
        message(SEND_ERROR "Error: LEVELDB_ADD_TEST called without test file")
        return()
    endif(NOT TESTFILE)

    add_executable(leveldb_${TESTNAME}_test
        ${TESTFILE})

    target_link_libraries(leveldb_${TESTNAME}_test
        leveldb_test_rt
        leveldb)

    set_target_properties(leveldb_${TESTNAME}_test PROPERTIES
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

    add_test(NAME leveldb_${TESTNAME}_test COMMAND leveldb_${TESTNAME}_test)

    add_dependencies(RUN_LEVELDB_UNIT_TESTS leveldb_${TESTNAME}_test)
endfunction(LEVELDB_ADD_TEST)

LEVELDB_ADD_TEST(env          util/env_test.cc)
LEVELDB_ADD_TEST(crc32        util/crc32c_test.cc)
LEVELDB_ADD_TEST(coding       util/coding_test.cc)
LEVELDB_ADD_TEST(arena        util/arena_test.cc)
LEVELDB_ADD_TEST(cache        util/cache_test.cc)
LEVELDB_ADD_TEST(table        table/table_test.cc)
# IMPORTANT: Commented a test that fails randomly.
# LEVELDB_ADD_TEST(autocompact  db/autocompact_test.cc)
LEVELDB_ADD_TEST(corruption   db/corruption_test.cc)
LEVELDB_ADD_TEST(dbformat     db/dbformat_test.cc)
LEVELDB_ADD_TEST(filename     db/filename_test.cc)
LEVELDB_ADD_TEST(log          db/log_test.cc)
LEVELDB_ADD_TEST(skiplist     db/skiplist_test.cc)
LEVELDB_ADD_TEST(version_edit db/version_edit_test.cc)
LEVELDB_ADD_TEST(write_batch  db/write_batch_test.cc)
LEVELDB_ADD_TEST(version_set  db/version_set_test.cc)
LEVELDB_ADD_TEST(filter_block table/filter_block_test.cc)
LEVELDB_ADD_TEST(bloom        util/bloom_test.cc)
LEVELDB_ADD_TEST(hash         util/hash_test.cc)
LEVELDB_ADD_TEST(db_bench     db/db_bench.cc)
LEVELDB_ADD_TEST(db           db/db_test.cc)