# This CMakeLists.txt is not meant to actually work!
# It only serves as a dummy project to make CLion work properly when it comes to symbol resolution and all the nice
# features dependent on that. Building must still be done on the command line using the automake build chain
# If you load this project in CLion and would like to run/debug executables, make sure to remove the "Build" entry from
# the run/debug configuration as otherwise CLion will try to build this project with cmake, failing horribly.
# You'll also have to manually change the executable in the configuration to the correct path of the already built executable

cmake_minimum_required(VERSION 3.7)
project(dash)

set(CMAKE_CXX_STANDARD 11)

include_directories(
        src
        src/leveldb/include
        src/univalue/include
)

add_definitions(
        -DENABLE_WALLET=1
)

file(GLOB SOURCE_FILES
        src/bench/*.cpp
        src/bench/*.h
        src/compat/*.cpp
        src/compat/*.h
        src/consensus/*.h
        src/consensus/*.cpp
        src/crypto/*.c
        src/crypto/*.h
        src/crypto/*.cpp
        src/leveldb/db/*.cc
        src/leveldb/db/*.h
        src/leveldb/include/*.h
        src/policy/*.cpp
        src/policy/*.h
        src/primitives/*.cpp
        src/primitives/*.h
        src/qt/test/*.cpp
        src/qt/test/*.h
        src/qt/*.cpp
        src/qt/*.h
        src/script/*.cpp
        src/script/*.h
        src/secp256k1/include/*.h
        src/test/*.cpp
        src/test/*.h
        src/univalue/include/*.h
        src/univalue/lib/*.cpp
        src/univalue/lib/*.h
        src/wallet/test/*.cpp
        src/wallet/*.cpp
        src/wallet/*.h
        src/zmq/*.cpp
        src/zmq/*.h
        src/*.cpp
        src/*.h
        src/evo/*.h
        src/evo/*.cpp
        src/rpc/*.cpp
        src/rpc/*.h
        )

add_executable(dash ${SOURCE_FILES})
