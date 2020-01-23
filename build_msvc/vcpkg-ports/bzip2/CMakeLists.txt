cmake_minimum_required(VERSION 3.0)
project(bzip2 C)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-DBZ_DEBUG) # enable extra assertions
endif()

set(BZ2_SOURCES
    blocksort.c
    huffman.c
    crctable.c
    randtable.c
    compress.c
    decompress.c
    bzlib.c)

add_library(bz2 ${BZ2_SOURCES})
set_target_properties(bz2 PROPERTIES
    DEBUG_POSTFIX d
    VERSION 1.0.6
    SOVERSION 1.0)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(bz2 PRIVATE -DBZ_BUILD_DLL)
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()

install(TARGETS bz2
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)

if(NOT BZIP2_SKIP_TOOLS)
    add_executable(bzip2 bzip2.c ${BZ2_SOURCES})
    add_executable(bzip2recover bzip2recover.c ${BZ2_SOURCES})
    install(TARGETS bzip2 bzip2recover DESTINATION tools/bzip2)
endif()

if(NOT BZIP2_SKIP_HEADERS)
    install(FILES bzlib.h DESTINATION include)
endif()
