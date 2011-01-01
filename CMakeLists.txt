# Copyright (c) 2009-2010 Satoshi Nakamoto
# Distributed under the MIT/X11 software license, see the accompanying
# file license.txt or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required (VERSION 2.6)
project (bitcoin)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/Modules)

find_package(Boost COMPONENTS system filesystem program_options thread REQUIRED)
find_package(wxWidgets 2.9 COMPONENTS base core REQUIRED)
find_package(BerkeleyDB 4.7 REQUIRED)

set(COMMON_LIBS crypto ssl ${BerkeleyDB_LIBRARIES}
  ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_THREAD_LIBRARY})

include_directories(${wxWidgets_INCLUDE_DIRS} ${BerkeleyDB_INCLUDE_DIR})

add_definitions(-D__WXGTK__ -DNOPCH -DFOURWAYSSE2 -DUSE_SSL -Wno-invalid-offsetof)


add_library(btc db.cpp irc.cpp net.cpp rpc.cpp script.cpp sha256.cpp util.cpp)

#add_library(sha256 sha256.cpp) -msse2 -march=amdfam10 from original makefile
add_subdirectory(cryptopp)

add_library(btc_ui ui.cpp uibase.cpp init.cpp )
set_target_properties(btc_ui PROPERTIES COMPILE_FLAGS -DGUI)

add_executable(bitcoind main.cpp init.cpp)
target_link_libraries(bitcoind btc cryptopp ${COMMON_LIBS})

add_executable(bitcoin main.cpp init.cpp)
set_target_properties(bitcoin PROPERTIES COMPILE_FLAGS -DGUI)
target_link_libraries(bitcoin btc btc_ui cryptopp 
  ${wxWidgets_LIBRARIES} ${COMMON_LIBS})
