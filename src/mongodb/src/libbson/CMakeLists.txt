cmake_minimum_required(VERSION 3.1)

project (libbson C)

set(ENABLE_STATIC AUTO CACHE STRING "Build static libbson. Set to ON/AUTO/OFF, default AUTO.")
option(ENABLE_TESTS "Build libbson tests." ON)
option(ENABLE_EXAMPLES "Build libbson examples." OFF)

if (ENABLE_STATIC STREQUAL ON OR ENABLE_STATIC STREQUAL AUTO)
   # In the future we may need to check whether static dependencies are
   # available. For now, AUTO means ON.
   set (BSON_ENABLE_STATIC ON)
endif ()

if (ENABLE_TESTS AND NOT BSON_ENABLE_STATIC)
   message (FATAL_ERROR "-DENABLE_STATIC=OFF also requires -DENABLE_TESTS=OFF")
endif ()

if (NOT WIN32)
    message(WARNING "CMake support is experimental and may not produce production quality artifacts")
endif ()

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(TestBigEndian)
include(InstallRequiredSystemLibraries)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/build/cmake)

# Set BSON_MAJOR_VERSION, BSON_MINOR_VERSION, etc.
include(LoadVersion)
LoadVersion(${PROJECT_SOURCE_DIR}/VERSION_CURRENT BSON)
LoadVersion(${PROJECT_SOURCE_DIR}/VERSION_RELEASED BSON_RELEASED)

message("Current version (from VERSION_CURRENT file): ${BSON_VERSION}")
if (NOT ${BSON_VERSION} STREQUAL ${BSON_RELEASED_VERSION})
   message("Previous release (from VERSION_RELEASED file): ${BSON_RELEASED_VERSION}")
endif()

set (SOURCE_DIR "${PROJECT_SOURCE_DIR}/")

set (BSON_API_VERSION 1.0)

set (CPACK_RESOURCE_FILE_LICENSE "${SOURCE_DIR}/COPYING")
set (CPACK_PACKAGE_VERSION_MAJOR ${BSON_MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${BSON_MINOR_VERSION})

include (CPack)
TEST_BIG_ENDIAN(BSON_BIG_ENDIAN)

# These need proper checks like automake.
set (BSON_PTHREAD_ONCE_INIT_NEEDS_BRACES 0)

#librt needed on linux for clock_gettime
find_library(RT_LIBRARY rt)
if (RT_LIBRARY)
   #set required libraries for CHECK_FUNCTION_EXISTS
   set(CMAKE_REQUIRED_LIBRARIES ${RT_LIBRARY})
   set(BSON_LIBS ${BSON_LIBS} ${RT_LIBRARY})
endif()

# See https://public.kitware.com/Bug/view.php?id=15659
CHECK_SYMBOL_EXISTS(snprintf stdio.h BSON_HAVE_SNPRINTF)
if (NOT BSON_HAVE_SNPRINTF)
  set(BSON_HAVE_SNPRINTF 0)
else ()
  set(BSON_HAVE_SNPRINTF 1)
endif ()

CHECK_FUNCTION_EXISTS(reallocf BSON_HAVE_REALLOCF)
if (NOT BSON_HAVE_REALLOCF)
  set(BSON_HAVE_REALLOCF 0)
endif ()

CHECK_STRUCT_HAS_MEMBER("struct timespec" tv_sec time.h BSON_HAVE_TIMESPEC)
if (NOT BSON_HAVE_TIMESPEC)
  message(STATUS "    no timespec struct")
  set(BSON_HAVE_TIMESPEC 0)
else ()
  message(STATUS "    struct timespec found")
  set(BSON_HAVE_TIMESPEC 1)
endif ()

CHECK_SYMBOL_EXISTS(gmtime_r time.h BSON_HAVE_GMTIME_R)
if (NOT BSON_HAVE_GMTIME_R)
   set(BSON_HAVE_GMTIME_R 0)
else ()
   set(BSON_HAVE_GMTIME_R 1)
endif ()

if (WIN32)
   set (BSON_OS 2)
else ()
   set (BSON_OS 1)
endif ()

if (MSVC)
   set (BSON_HAVE_CLOCK_GETTIME 0)
   set (BSON_HAVE_STDBOOL_H 0)
   set (BSON_HAVE_STRNLEN 0)
   set (BSON_EXTRA_ALIGN 1)
   set (BSON_HAVE_SYSCALL_TID 0)
else ()
   CHECK_FUNCTION_EXISTS(clock_gettime BSON_HAVE_CLOCK_GETTIME)
   if (NOT BSON_HAVE_CLOCK_GETTIME)
      set(BSON_HAVE_CLOCK_GETTIME 0)
   endif ()
   CHECK_FUNCTION_EXISTS(strnlen BSON_HAVE_STRNLEN)
   if (NOT BSON_HAVE_STRNLEN)
      set(BSON_HAVE_STRNLEN 0)
   endif ()
   CHECK_INCLUDE_FILE(stdbool.h BSON_HAVE_STDBOOL_H)
   if (NOT BSON_HAVE_STDBOOL_H)
      set(BSON_HAVE_STDBOOL_H 0)
   endif ()
   set (BSON_EXTRA_ALIGN 1)
   CHECK_SYMBOL_EXISTS(SYS_gettid sys/syscall.h BSON_HAVE_SYSCALL_TID)
   if (NOT BSON_HAVE_SYSCALL_TID OR APPLE)
      set (BSON_HAVE_SYSCALL_TID 0)
   endif ()
   CHECK_INCLUDE_FILE(strings.h HAVE_STRINGS_H)
endif ()

set (BSON_HAVE_ATOMIC_32_ADD_AND_FETCH 1)
set (BSON_HAVE_ATOMIC_64_ADD_AND_FETCH 1)

if (BSON_BIG_ENDIAN)
   set (BSON_BYTE_ORDER 4321)
else ()
   set (BSON_BYTE_ORDER 1234)
endif ()

configure_file (
   "${SOURCE_DIR}/src/bson/bson-config.h.in"
   "${PROJECT_BINARY_DIR}/src/bson/bson-config.h"
)

configure_file (
   "${SOURCE_DIR}/src/bson/bson-version.h.in"
   "${PROJECT_BINARY_DIR}/src/bson/bson-version.h"
)

configure_file (
   "${PROJECT_SOURCE_DIR}/build/cmake/bson/bson-stdint.h"
   "${PROJECT_BINARY_DIR}/src/bson/bson-stdint.h"
)

include_directories("${PROJECT_BINARY_DIR}/src/bson")
include_directories("${SOURCE_DIR}/src/bson")
include_directories("${SOURCE_DIR}/src")

if (APPLE)
   cmake_policy(SET CMP0042 OLD)
endif()

set (SOURCES
   ${SOURCE_DIR}/src/bson/bcon.c
   ${SOURCE_DIR}/src/bson/bson.c
   ${SOURCE_DIR}/src/bson/bson-atomic.c
   ${SOURCE_DIR}/src/bson/bson-clock.c
   ${SOURCE_DIR}/src/bson/bson-context.c
   ${SOURCE_DIR}/src/bson/bson-decimal128.c
   ${SOURCE_DIR}/src/bson/bson-error.c
   ${SOURCE_DIR}/src/bson/bson-iso8601.c
   ${SOURCE_DIR}/src/bson/bson-iter.c
   ${SOURCE_DIR}/src/bson/bson-json.c
   ${SOURCE_DIR}/src/bson/bson-keys.c
   ${SOURCE_DIR}/src/bson/bson-md5.c
   ${SOURCE_DIR}/src/bson/bson-memory.c
   ${SOURCE_DIR}/src/bson/bson-oid.c
   ${SOURCE_DIR}/src/bson/bson-reader.c
   ${SOURCE_DIR}/src/bson/bson-string.c
   ${SOURCE_DIR}/src/bson/bson-timegm.c
   ${SOURCE_DIR}/src/bson/bson-utf8.c
   ${SOURCE_DIR}/src/bson/bson-value.c
   ${SOURCE_DIR}/src/bson/bson-version-functions.c
   ${SOURCE_DIR}/src/bson/bson-writer.c
   ${SOURCE_DIR}/src/jsonsl/jsonsl.c
)

set (HEADERS
   ${PROJECT_BINARY_DIR}/src/bson/bson-config.h
   ${PROJECT_BINARY_DIR}/src/bson/bson-stdint.h
   ${PROJECT_BINARY_DIR}/src/bson/bson-version.h
   ${SOURCE_DIR}/src/bson/bcon.h
   ${SOURCE_DIR}/src/bson/bson-atomic.h
   ${SOURCE_DIR}/src/bson/bson-clock.h
   ${SOURCE_DIR}/src/bson/bson-compat.h
   ${SOURCE_DIR}/src/bson/bson-context.h
   ${SOURCE_DIR}/src/bson/bson-decimal128.h
   ${SOURCE_DIR}/src/bson/bson-endian.h
   ${SOURCE_DIR}/src/bson/bson-error.h
   ${SOURCE_DIR}/src/bson/bson.h
   ${SOURCE_DIR}/src/bson/bson-iter.h
   ${SOURCE_DIR}/src/bson/bson-json.h
   ${SOURCE_DIR}/src/bson/bson-keys.h
   ${SOURCE_DIR}/src/bson/bson-macros.h
   ${SOURCE_DIR}/src/bson/bson-md5.h
   ${SOURCE_DIR}/src/bson/bson-memory.h
   ${SOURCE_DIR}/src/bson/bson-oid.h
   ${SOURCE_DIR}/src/bson/bson-reader.h
   ${SOURCE_DIR}/src/bson/bson-stdint-win32.h
   ${SOURCE_DIR}/src/bson/bson-string.h
   ${SOURCE_DIR}/src/bson/bson-types.h
   ${SOURCE_DIR}/src/bson/bson-utf8.h
   ${SOURCE_DIR}/src/bson/bson-value.h
   ${SOURCE_DIR}/src/bson/bson-version-functions.h
   ${SOURCE_DIR}/src/bson/bson-writer.h
)

add_library(bson_shared SHARED ${SOURCES} ${HEADERS})
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set_target_properties(bson_shared PROPERTIES COMPILE_DEFINITIONS "BSON_COMPILATION;JSONSL_PARSE_NAN")
# CMake wants to make different symlinks than the Autotools, see
# https://cmake.org/pipermail/cmake/2007-September/016501.html
# This hack sets up standard symlink, libbson-1.0.so -> libbson-1.0.0.so
set_target_properties(bson_shared PROPERTIES VERSION 0 SOVERSION ${BSON_MAJOR_VERSION})
set_target_properties(bson_shared PROPERTIES OUTPUT_NAME "bson-${BSON_API_VERSION}" PREFIX "lib")

if (RT_LIBRARY)
   target_link_libraries (bson_shared ${RT_LIBRARY})
endif()

find_library(M_LIBRARY m)
if (M_LIBRARY)
   target_link_libraries (bson_shared ${M_LIBRARY})
   set(BSON_LIBS ${BSON_LIBS} ${M_LIBRARY})
endif()

set(THREADS_PREFER_PTHREAD_FLAG 1)
find_package (Threads REQUIRED)
target_link_libraries(bson_shared Threads::Threads)
if(CMAKE_USE_PTHREADS_INIT)
   set(BSON_LIBS ${BSON_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()

if (NOT UNIX)
   # gethostbyname
   target_link_libraries (bson_shared ws2_32)
   # Can't find_library () system dependencies
   # must be handled specially since we can't resolve them
   set(BSON_SYSTEM_LIBS ${BSON_SYSTEM_LIBS} ws2_32)
endif()

if (BSON_ENABLE_STATIC)
   add_library(bson_static STATIC ${SOURCES} ${HEADERS})
   set_target_properties(bson_static PROPERTIES COMPILE_DEFINITIONS "BSON_COMPILATION;BSON_STATIC;JSONSL_PARSE_NAN")
   set_target_properties(bson_static PROPERTIES VERSION ${BSON_VERSION})
   set_target_properties(bson_static PROPERTIES OUTPUT_NAME "bson-static-${BSON_API_VERSION}")
   target_link_libraries(bson_static Threads::Threads)
   if (RT_LIBRARY)
      target_link_libraries (bson_static ${RT_LIBRARY})
   endif()
   if (M_LIBRARY)
      target_link_libraries (bson_static ${M_LIBRARY})
   endif()
   if (NOT UNIX)
      # gethostbyname
      target_link_libraries (bson_static ws2_32)
   endif ()
endif ()

if (ENABLE_TESTS)
    enable_testing()

    add_definitions("-DBINARY_DIR=\"${SOURCE_DIR}/tests/binary\"")
    add_definitions("-DJSON_DIR=\"${SOURCE_DIR}/tests/json\"")

    set (BSON_TEST_SOURCES
         ${SOURCE_DIR}/tests/corpus-test.c
         ${SOURCE_DIR}/tests/corpus-test.h
         ${SOURCE_DIR}/tests/TestSuite.c
         ${SOURCE_DIR}/tests/TestSuite.h
         ${SOURCE_DIR}/tests/test-libbson.c
         ${SOURCE_DIR}/tests/test-atomic.c
         ${SOURCE_DIR}/tests/test-bson.c
         ${SOURCE_DIR}/tests/test-bson-corpus.c
         ${SOURCE_DIR}/tests/test-endian.c
         ${SOURCE_DIR}/tests/test-clock.c
         ${SOURCE_DIR}/tests/test-decimal128.c
         ${SOURCE_DIR}/tests/test-error.c
         ${SOURCE_DIR}/tests/test-iso8601.c
         ${SOURCE_DIR}/tests/test-iter.c
         ${SOURCE_DIR}/tests/test-json.c
         ${SOURCE_DIR}/tests/test-oid.c
         ${SOURCE_DIR}/tests/test-reader.c
         ${SOURCE_DIR}/tests/test-string.c
         ${SOURCE_DIR}/tests/test-utf8.c
         ${SOURCE_DIR}/tests/test-value.c
         ${SOURCE_DIR}/tests/test-version.c
         ${SOURCE_DIR}/tests/test-writer.c
         ${SOURCE_DIR}/tests/test-bcon-basic.c
         ${SOURCE_DIR}/tests/test-bcon-extract.c
         ${SOURCE_DIR}/tests/json-test.c
         ${SOURCE_DIR}/tests/json-test.c
         ${SOURCE_DIR}/tests/json-test.h
    )

    add_executable (test-libbson ${BSON_TEST_SOURCES})

    target_link_libraries(test-libbson bson_shared bson_static)
    add_test(NAME test-libbson COMMAND test-libbson)

    file(COPY ${SOURCE_DIR}/tests/binary ${SOURCE_DIR}/tests/json
         DESTINATION ${PROJECT_BINARY_DIR}/tests)
endif ()  # ENABLE_TESTS

function (add_example bin src)
    set (BSON_EXAMPLE_SOURCES ${SOURCE_DIR}/${src})
    add_executable (${bin} ${BSON_EXAMPLE_SOURCES})

    # Link against the shared lib like normal apps
    target_link_libraries(${bin} bson_shared)

    set (EXAMPLES ${EXAMPLES} ${bin})
endfunction ()

if (ENABLE_EXAMPLES)
    add_example (bcon-col-view examples/bcon-col-view.c)
    add_example (bcon-speed examples/bcon-speed.c)
    add_example (bson-metrics examples/bson-metrics.c)
    # Uses getopt ()
    #add_example (bson-streaming-reader examples/bson-streaming-reader.c)
    add_example (bson-to-json examples/bson-to-json.c)
    add_example (bson-validate examples/bson-validate.c)
    add_example (json-to-bson examples/json-to-bson.c)
endif () # ENABLE_EXAMPLES

set (BSON_HEADER_INSTALL_DIR "include/libbson-${BSON_API_VERSION}")
install(
  TARGETS bson_shared ${EXAMPLES}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)
if (BSON_ENABLE_STATIC)
   install(
     TARGETS bson_static ${EXAMPLES}
     LIBRARY DESTINATION lib
     ARCHIVE DESTINATION lib
     RUNTIME DESTINATION bin
   )
endif ()
install(
  FILES ${HEADERS}
  DESTINATION ${BSON_HEADER_INSTALL_DIR}
)

set(LIBBSON_LIBS "")
foreach(_lib ${CMAKE_C_IMPLICIT_LINK_LIBRARIES} ${BSON_LIBS})
   if(_lib MATCHES ".*/.*" OR _lib MATCHES "^-")
    set(LIBBSON_LIBS          "${LIBBSON_LIBS} ${_lib}")
  else()
    set(LIBBSON_LIBS          "${LIBBSON_LIBS} -l${_lib}")
  endif()
endforeach()
# System dependencies don't match the above regexs, but also don't want the -l
foreach(_lib ${BSON_SYSTEM_LIBS})
    set(LIBBSON_LIBS          "${LIBBSON_LIBS} ${_lib}")
endforeach()

set(VERSION "${BSON_VERSION}")
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(libdir "\${prefix}/lib")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libbson-1.0.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/src/libbson-1.0.pc
    @ONLY)

install(
   FILES
   ${CMAKE_CURRENT_BINARY_DIR}/src/libbson-1.0.pc
   DESTINATION
   lib/pkgconfig
)

if (BSON_ENABLE_STATIC)
   configure_file(
       ${CMAKE_CURRENT_SOURCE_DIR}/src/libbson-static-1.0.pc.in
       ${CMAKE_CURRENT_BINARY_DIR}/src/libbson-static-1.0.pc
       @ONLY)

   install(
      FILES
      ${CMAKE_CURRENT_BINARY_DIR}/src/libbson-static-1.0.pc
      DESTINATION
      lib/pkgconfig
   )
endif ()


include (build/cmake/BSONPackage.cmake)
