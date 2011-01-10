# Copyright (c) 2009-2010 Satoshi Nakamoto
# Distributed under the MIT/X11 software license, see the accompanying
# file license.txt or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required (VERSION 2.6)
project (bitcoin)

OPTION(BITCOIN_BUILD_GUI "Build Server GUI (bitcoin)" ON)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/Modules)

# Find Boost components
find_package(Boost COMPONENTS system filesystem program_options thread REQUIRED)


# Use wxWidgets module built into CMake
IF(BITCOIN_BUILD_GUI)
	SET(wxWidgets_USE_DEBUG OFF)
	SET(wxWidgets_USE_UNICODE ON)
	SET(wxWidgets_USE_UNIVERSAL ON)
	SET(wxWidgets_USE_STATIC ON)
	
	FIND_PACKAGE(wxWidgets 2.9 REQUIRED base core adv html)
	
	IF(wxWidgets_FOUND)
		INCLUDE_DIRECTORIES(${wxWidgets_INCLUDE_DIRS})
	ENDIF(wxWidgets_FOUND)
ENDIF(BITCOIN_BUILD_GUI)

# Use OpenSSL module built into CMake
FIND_PACKAGE(OpenSSL REQUIRED)

# Custom find for BerkeleyDB
find_package(BerkeleyDB 4.7 REQUIRED)


set(COMMON_LIBS crypto ${OPENSSL_LIBRARIES} ${BerkeleyDB_LIBRARIES}
  ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_THREAD_LIBRARY})

include_directories(${OPENSSL_INCLUDE_DIR} ${BerkeleyDB_INCLUDE_DIR})

add_definitions(-D__WXGTK__ -DNOPCH -DFOURWAYSSE2 -DUSE_SSL -Wno-invalid-offsetof)


add_library(btc db.cpp irc.cpp net.cpp rpc.cpp script.cpp sha256.cpp util.cpp)

#add_library(sha256 sha256.cpp) -msse2 -march=amdfam10 from original makefile
add_subdirectory(cryptopp)


add_executable(bitcoind main.cpp init.cpp)
target_link_libraries(bitcoind btc cryptopp ${COMMON_LIBS})
IF(BITCOIN_BUILD_GUI)

  add_library(btc_ui ui.cpp uibase.cpp init.cpp )
  set_target_properties(btc_ui PROPERTIES COMPILE_FLAGS -DGUI)
  
  add_executable(bitcoin main.cpp init.cpp)
  set_target_properties(bitcoin PROPERTIES COMPILE_FLAGS -DGUI)
  target_link_libraries(bitcoin btc btc_ui cryptopp 
    ${wxWidgets_LIBRARIES} ${COMMON_LIBS})
  
ENDIF(BITCOIN_BUILD_GUI)

IF(WIN32)
	TARGET_LINK_LIBRARIES(bitcoin winmm.lib shlwapi.lib)
ENDIF(WIN32)
