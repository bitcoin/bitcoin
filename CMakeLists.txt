# This CMakeLists.txt is not meant to actually work!
# It only serves as a dummy project to make CLion work properly when it comes to symbol resolution and all the nice
# features dependent on that. Building must still be done on the command line using the automake build chain
# If you load this project in CLion and would like to run/debug executables, make sure to remove the "Build" entry from
# the run/debug configuration as otherwise CLion will try to build this project with cmake, failing horribly.
# You'll also have to manually change the executable in the configuration to the correct path of the already built executable

cmake_minimum_required(VERSION 3.7)
project(dash)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        src
        src/qt/forms
        src/leveldb/include
        src/univalue/include
)

if(UNIX AND NOT APPLE)
    set(DEPENDS_PREFIX depends/x86_64-pc-linux-gnu)
elseif(APPLE)
    set(DEPENDS_PREFIX depends/x86_64-apple-darwin14)
elseif(WIN32)
    set(DEPENDS_PREFIX depends/x86_64-w64-mingw32)
endif()

message(STATUS "DEPENDS_PREFIX: ${DEPENDS_PREFIX}")

if(DEFINED DEPENDS_PREFIX)
    include_directories(${DEPENDS_PREFIX}/include)
    include_directories(${DEPENDS_PREFIX}/include/QtWidgets)
endif()

add_definitions(
        -DENABLE_CRASH_HOOKS=1
        -DENABLE_STACKTRACES=1
        -DENABLE_WALLET=1
)

file(GLOB SOURCE_FILES
        src/*.cpp
        src/*.h
        src/bench/*.cpp
        src/bench/*.h
        src/bls/*.cpp
        src/bls/*.h
        src/coinjoin/*.cpp
        src/coinjoin/*.h
        src/compat/*.cpp
        src/compat/*.h
        src/consensus/*.cpp
        src/consensus/*.h
        src/crypto/*.c
        src/crypto/*.cpp
        src/crypto/*.h
        src/evo/*.cpp
        src/evo/*.h
        src/leveldb/db/*.cc
        src/leveldb/db/*.h
        src/leveldb/include/*.h
        src/llmq/*.cpp
        src/llmq/*.h
        src/masternode/*.cpp
        src/masternode/*.h
        src/policy/*.cpp
        src/policy/*.h
        src/primitives/*.cpp
        src/primitives/*.h
        src/qt/*.cpp
        src/qt/*.h
        src/qt/test/*.cpp
        src/qt/test/*.h
        src/rpc/*.cpp
        src/rpc/*.h
        src/script/*.cpp
        src/script/*.h
        src/secp256k1/include/*.h
        src/test/*.cpp
        src/test/*.h
        src/univalue/include/*.h
        src/univalue/lib/*.cpp
        src/univalue/lib/*.h
        src/wallet/*.cpp
        src/wallet/*.h
        src/wallet/test/*.cpp
        src/zmq/*.cpp
        src/zmq/*.h
        )

add_executable(dash ${SOURCE_FILES})
