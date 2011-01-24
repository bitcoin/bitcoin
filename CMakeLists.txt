cmake_minimum_required (VERSION 2.6)
project (bitcoin)

OPTION(BITCOIN_BUILD_GUI "Build Server GUI (bitcoin)" ON)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/Modules)

# Find Boost components
set(Boost_USE_STATIC_LIBS ON)
set(Boost_ADDITIONAL_VERSIONS "1.42" "1.42.0" "1.43" "1.43.0")
find_package(Boost 1.35.0 COMPONENTS system 
                                     filesystem 
                                     program_options 
                                     thread 
                                     date_time
                                     regex REQUIRED)

if (${Boost_VERSION} EQUAL 104400 AND MSVC_VERSION EQUAL 1600)
    #Avoid error with boost 1.44 interprocess and MSVC10
    add_definitions(-DBOOST_NO_RVALUE_REFERENCES)
endif (${Boost_VERSION} EQUAL 104400 AND MSVC_VERSION EQUAL 1600)


# Use wxWidgets module built into CMake
IF(BITCOIN_BUILD_GUI)
        if (APPLE)
          # If we don't do that, OS X's built-in wxWidgets library will be used, and it's too old.
          set(wxWidgets_CONFIG_EXECUTABLE "../../deps/bin/wx-config")
        endif (APPLE)
        
	SET(wxWidgets_USE_DEBUG OFF)
	SET(wxWidgets_USE_UNICODE ON)
	SET(wxWidgets_USE_UNIVERSAL ON)
	SET(wxWidgets_USE_STATIC ON)
	
	FIND_PACKAGE(wxWidgets 2.9 REQUIRED base core adv html)
	
	IF(wxWidgets_FOUND)
		INCLUDE_DIRECTORIES(${wxWidgets_INCLUDE_DIRS})
	ENDIF(wxWidgets_FOUND)
 
        if (APPLE)
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__WXMAC_OSX__ -DNOPCH -DMSG_NOSIGNAL=0 -DUSE_SSL")
        endif (APPLE)


ENDIF(BITCOIN_BUILD_GUI)

if (MSVC)
    #MSVC generates and embeds a manifest file automatically, so we should
    #disable wx's manifest file inclusion
    add_definitions(-DwxUSE_NO_MANIFEST)
    #ASM is disabled in original bitcoin makefile for vc. Without this,
    #SHA-256 hashing is broken
    add_definitions(-DCRYPTOPP_DISABLE_ASM)
endif (MSVC)


# Use OpenSSL module built into CMake
FIND_PACKAGE(OpenSSL REQUIRED)

# Custom find for BerkeleyDB
find_package(BerkeleyDB 4.7 REQUIRED)


set(COMMON_LIBS crypto ${OPENSSL_LIBRARIES} ${BerkeleyDB_LIBRARIES}
  ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_THREAD_LIBRARY})

include_directories(${Boost_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${BerkeleyDB_INCLUDE_DIR})


add_definitions(-DNOPCH -DFOURWAYSSE2 -DUSE_SSL -Wno-invalid-offsetof)


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
  
  include( ${wxWidgets_USE_FILE} )

ENDIF(BITCOIN_BUILD_GUI)

IF(WIN32)
  # __WXMSW__ define is added by UsewxWidgets, but it is needed for
  #bitcoind also, even if wxwidgets is not found
  set_target_properties(bitcoind PROPERTIES COMPILE_DEFINITIONS __WXMSW__)
  
  TARGET_LINK_LIBRARIES(bitcoin winmm.lib shlwapi.lib) #TODO check win 7/Vista presence http://msdn.microsoft.com/en-us/library/bb759845%28v=vs.85%29.aspx
ENDIF(WIN32)
