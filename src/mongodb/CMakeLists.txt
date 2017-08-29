cmake_minimum_required(VERSION 3.1)

set (MESSAGES_ENABLED 1)

function(message)
   list (GET ARGV 0 MessageType)
   if (MESSAGES_ENABLED)
      list (REMOVE_AT ARGV 0)
      _message (${MessageType} "${ARGV}")
   endif()
endfunction()

set (ENABLE_SSL AUTO CACHE STRING
     "Enable TLS connections and SCRAM-SHA-1 authentication.\ Options are
     \"DARWIN\" to use Apple's Secure Transport, \"WINDOWS\" to use Windows
     Secure Channel, \"OPENSSL\", \"AUTO\",\ or \"OFF\". These options are
     case-sensitive. The default is \"AUTO\". Note\ that SCRAM-SHA-1 is
     required for authenticating to MongoDB 3.0 and later.")

set (ENABLE_SASL AUTO CACHE STRING
     "Enable SASL authentication (Kerberos).\ Options are \"CYRUS\" to use Cyrus
     SASL, \"SSPI\" to use Windows Native SSPI, \"GSSAPI\" to use macOS Native GSS,
     \"AUTO\",\ or \"OFF\". These options are case-sensitive.")

set(ENABLE_STATIC AUTO CACHE STRING "Build static libmongoc. Set to ON/AUTO/OFF, default AUTO.")
option(ENABLE_TESTS "Build MongoDB C Driver tests." ON)
option(ENABLE_EXAMPLES "Build MongoDB C Driver examples." ON)
option(ENABLE_MAINTAINER_FLAGS "Use strict compiler checks" OFF)
option(ENABLE_AUTOMATIC_INIT_AND_CLEANUP "Enable automatic init and cleanup (GCC only)" ON)
option(ENABLE_CRYPTO_SYSTEM_PROFILE "Use system crypto profile (OpenSSL only)" OFF)
option(ENABLE_TRACING "Turn on verbose debug output" OFF)
set(ENABLE_SNAPPY bundled CACHE STRING "Enable snappy support")
set(ENABLE_ZLIB bundled CACHE STRING "Enable zlib support")

if (NOT WIN32)
    message(WARNING "CMake support is experimental and may not produce production quality artifacts")
endif ()

if (ENABLE_SNAPPY STREQUAL "bundled")
project (libmongoc C CXX)
else ()
project (libmongoc C)
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/build/cmake)

include(InstallRequiredSystemLibraries)

# Set MONGOC_MAJOR_VERSION, MONGOC_MINOR_VERSION, etc.
include(LoadVersion)
LoadVersion(${PROJECT_SOURCE_DIR}/VERSION_CURRENT MONGOC)
LoadVersion(${PROJECT_SOURCE_DIR}/VERSION_RELEASED MONGOC_RELEASED)

message("Current version (from VERSION_CURRENT file): ${MONGOC_VERSION}")
if (NOT ${MONGOC_VERSION} STREQUAL ${MONGOC_RELEASED_VERSION})
   message("Previous release (from VERSION_RELEASED file): ${MONGOC_RELEASED_VERSION}")
endif()

include(MaintainerFlags)

# The input variable BSON_ROOT_DIR is respected for backwards compatibility,
# but you should use the standard CMAKE_PREFIX_PATH instead.
message (STATUS "Searching for libbson CMake packages")
find_package (libbson-1.0
   "${MONGOC_MAJOR_VERSION}.${MONGOC_MINOR_VERSION}.${MONGOC_MICRO_VERSION}"
   HINTS
   ${BSON_ROOT_DIR}
   REQUIRED)

message ("--   libbson found version \"${BSON_VERSION}\"")
message ("--   libbson include path \"${BSON_INCLUDE_DIRS}\"")
message ("--   libbson libraries \"${BSON_LIBRARIES}\"")

if (ENABLE_STATIC STREQUAL ON OR ENABLE_STATIC STREQUAL AUTO)
   find_package (libbson-static-1.0
      "${MONGOC_MAJOR_VERSION}.${MONGOC_MINOR_VERSION}.${MONGOC_MICRO_VERSION}"
      HINTS
      ${BSON_ROOT_DIR})

   if (ENABLE_STATIC STREQUAL ON AND NOT BSON_STATIC_LIBRARY)
      message (FATAL_ERROR "Static libbson not found. Pass -DENABLE_STATIC=OFF")
   endif ()

   if (BSON_STATIC_LIBRARY)
      set (MONGOC_ENABLE_STATIC ON)
   endif ()

   message ("--   libbson-static found version \"${BSON_STATIC_VERSION}\"")
   message ("--   libbson-static include path \"${BSON_STATIC_INCLUDE_DIRS}\"")
   message ("--   libbson-static libraries \"${BSON_STATIC_LIBRARIES}\"")
endif ()

if (ENABLE_TESTS AND NOT MONGOC_ENABLE_STATIC)
   message (FATAL_ERROR "ENABLE_TESTS requires ENABLE_STATIC")
endif ()

if (NOT (ENABLE_SSL STREQUAL DARWIN
         OR ENABLE_SSL STREQUAL WINDOWS
         OR ENABLE_SSL STREQUAL OPENSSL
         OR ENABLE_SSL STREQUAL AUTO
         OR ENABLE_SSL STREQUAL OFF))
   message (FATAL_ERROR
            "ENABLE_SSL option must be DARWIN, WINDOWS, OPENSSL, AUTO, or OFF")
endif()

if (NOT ENABLE_SSL STREQUAL OFF)
   # If "AUTO", try OpenSSL. In version 2.0 we'll default to "DARWIN" on Mac.
   if (ENABLE_SSL STREQUAL AUTO OR ENABLE_SSL STREQUAL OPENSSL)
      # Sets OPENSSL_FOUND on success.
      include (FindOpenSSL)
   endif ()

   if (ENABLE_SSL STREQUAL LIBRESSL)
      set (LIBRESSL 1)
   endif ()

   if (ENABLE_SSL STREQUAL DARWIN)
      if (APPLE)
         set (SECURE_TRANSPORT 1)
      else ()
         message (FATAL_ERROR "ENABLE_SSL=DARWIN only supported on Mac OS X")
      endif ()
   endif ()

   if (ENABLE_SSL STREQUAL AUTO AND NOT OPENSSL_FOUND AND APPLE)
      set (SECURE_TRANSPORT 1)
   endif ()

   if (ENABLE_SSL STREQUAL WINDOWS)
      if (WIN32)
         set (SECURE_CHANNEL 1)
      else ()
         message (FATAL_ERROR "ENABLE_SSL=WINDOWS only supported on Windows")
      endif ()
   endif ()

   if (ENABLE_SSL STREQUAL AUTO AND NOT OPENSSL_FOUND AND WIN32)
      set (SECURE_CHANNEL 1)
   endif ()

   if (NOT OPENSSL_FOUND AND NOT SECURE_TRANSPORT AND NOT SECURE_CHANNEL AND NOT LIBRESSL)
      if (ENABLE_SSL STREQUAL AUTO)
         set (ENABLE_SSL OFF)
      else ()
         message (FATAL_ERROR "No SSL library found")
      endif ()
   endif ()
endif ()

# Defaults.
set (MONGOC_ENABLE_SSL 0)
set (MONGOC_ENABLE_SSL_OPENSSL 0)
set (MONGOC_ENABLE_SSL_LIBRESSL 0)
set (MONGOC_ENABLE_SSL_SECURE_TRANSPORT 0)
set (MONGOC_ENABLE_SSL_SECURE_CHANNEL 0)

set (MONGOC_ENABLE_CRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_CNG 0)

set (MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE 0)

set (MONGOC_ENABLE_COMPRESSION 0)
set (MONGOC_ENABLE_COMPRESSION_SNAPPY 0)
set (MONGOC_ENABLE_COMPRESSION_ZLIB 0)

if (OPENSSL_FOUND)
   if (WIN32 AND OPENSSL_VERSION GREATER 1.1 AND NOT
         ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} GREATER 3.7)
      message (FATAL_ERROR "Building against OpenSSL 1.1.0 and later requires CMake 3.7 or later (hint:"
         "You can also compile against Windows Secure Transport with -DENABLE_SSL=WINDOWS")
   endif ()
   set (MONGOC_ENABLE_SSL 1)
   set (MONGOC_ENABLE_SSL_OPENSSL 1)
   set (MONGOC_ENABLE_CRYPTO 1)
   set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 1)
elseif (SECURE_TRANSPORT)
   set (MONGOC_ENABLE_SSL 1)
   set (MONGOC_ENABLE_SSL_SECURE_TRANSPORT 1)
   set (MONGOC_ENABLE_CRYPTO 1)
   set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 1)
elseif (SECURE_CHANNEL)
   set (MONGOC_ENABLE_SSL 1)
   set (MONGOC_ENABLE_SSL_SECURE_CHANNEL 1)
   set (MONGOC_ENABLE_CRYPTO 1)
   set (MONGOC_ENABLE_CRYPTO_CNG 1)
elseif (LIBRESSL)
   set (MONGOC_ENABLE_SSL 1)
   set (MONGOC_ENABLE_SSL_LIBRESSL 1)
   set (MONGOC_ENABLE_CRYPTO 1)
   set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 1)
endif ()

if (ENABLE_CRYPTO_SYSTEM_PROFILE)
   if (OPENSSL_FOUND)
      set (MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE 1)
   else ()
      message (FATAL_ERROR "ENABLE_CRYPTO_SYSTEM_PROFILE only available with OpenSSL")
   endif ()
endif ()

include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(ssl ASN1_STRING_get0_data "openssl/asn1.h" MONGOC_HAVE_ASN1_STRING_GET0_DATA)
if (NOT MONGOC_HAVE_ASN1_STRING_GET0_DATA)
   set (MONGOC_HAVE_ASN1_STRING_GET0_DATA 0)
endif ()

if (NOT (ENABLE_SASL STREQUAL CYRUS
         OR ENABLE_SASL STREQUAL GSSAPI
         OR ENABLE_SASL STREQUAL SSPI
         OR ENABLE_SASL STREQUAL AUTO
         OR ENABLE_SASL STREQUAL OFF))
   message (FATAL_ERROR
      "ENABLE_SASL option must be CYRUS, GSSAPI, SSPI, AUTO, or OFF")
endif()

# Defaults.
set (MONGOC_ENABLE_SASL 0)
set (MONGOC_ENABLE_SASL_CYRUS 0)
set (MONGOC_ENABLE_SASL_GSSAPI 0)
set (MONGOC_ENABLE_SASL_SSPI 0)
set (MONGOC_HAVE_SASL_CLIENT_DONE 0)

if (NOT ENABLE_SASL STREQUAL OFF)
   if (ENABLE_SASL STREQUAL AUTO OR ENABLE_SASL STREQUAL CYRUS)
      # Sets SASL_LIBS.
      include(FindSASL2)
   endif()

   if (SASL_FOUND)
      set (MONGOC_ENABLE_SASL 1)
      set (MONGOC_ENABLE_SASL_CYRUS 1)
   elseif ((ENABLE_SASL STREQUAL SSPI OR ENABLE_SASL STREQUAL AUTO) AND WIN32)
      set (MONGOC_ENABLE_SASL 1)
      set (MONGOC_ENABLE_SASL_SSPI 1)
   elseif ((ENABLE_SASL STREQUAL GSSAPI OR ENABLE_SASL STREQUAL AUTO) AND DARWIN)
      set (MONGOC_ENABLE_SASL 1)
      set (MONGOC_ENABLE_SASL_GSSAPI 1)
   endif ()
else ()
   set (MONGOC_ENABLE_SASL 0)
endif()


if (ENABLE_AUTOMATIC_INIT_AND_CLEANUP)
   set (MONGOC_NO_AUTOMATIC_GLOBALS 0)
else ()
   set (MONGOC_NO_AUTOMATIC_GLOBALS 1)
endif ()

include(CheckIncludeFiles)
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)

if (MINGW)
   # Define _CRT_RAND_S, so that MinGW's stdlib.h declares Microsoft's rand_s().
   message (" MinGW: define _CRT_RAND_S")
   set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_RAND_S")
endif ()

include(CheckTypeSize)
if (WIN32)
   SET(CMAKE_EXTRA_INCLUDE_FILES "ws2tcpip.h")
else()
   SET(CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
endif ()
CHECK_TYPE_SIZE(socklen_t HAVE_SOCKLEN)
SET(CMAKE_EXTRA_INCLUDE_FILES)

if (HAVE_SOCKLEN)
   set(MONGOC_HAVE_SOCKLEN 1)
   set (MONGOC_SOCKET_ARG3 "socklen_t")
else()
   set(MONGOC_HAVE_SOCKLEN 0)
   set (MONGOC_SOCKET_ARG3 "int")
endif()

function (mongoc_get_accept_args ARG2 ARG3)
   SET (VAR 0)
   foreach (ARG2_VAL "struct sockaddr" "void")
   foreach (ARG3_VAL "socklen_t" "size_t" "int")

   MATH (EXPR VAR "${VAR}+1")

   FILE (WRITE ${CMAKE_CURRENT_BINARY_DIR}/accept_test${VAR}.c
   "#include <sys/types.h>
    #include <sys/socket.h>

    int main ()
    {
      int a = 0;
      ${ARG2_VAL} *b = 0;
      ${ARG3_VAL} *c = 0;
      accept (a, b, c);
      return 0;
   }
  ")

  TRY_COMPILE (RES ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/accept_test${VAR}.c CMAKE_FLAGS
  "-Werror -DCMAKE_CXX_LINK_EXECUTABLE='echo not linking now...'" OUTPUT_VARIABLE LOG2)

  if (RES)
     message (
        STATUS
        "Detected parameters: accept (int, ${ARG2_VAL} *, ${ARG3_VAL} *)")

     set (${ARG2} ${ARG2_VAL} PARENT_SCOPE)
     set (${ARG3} ${ARG3_VAL} PARENT_SCOPE)
     return ()
  endif ()

  endforeach ()
  endforeach ()

endfunction ()

# Reasonable defaults.
set (MONGOC_SOCKET_ARG2 "struct sockaddr")
set (MONGOC_SOCKET_ARG3 "socklen_t")

if (NOT WIN32)
   mongoc_get_accept_args (MONGOC_SOCKET_ARG2 MONGOC_SOCKET_ARG3)
endif ()

set (SOURCE_DIR "${PROJECT_SOURCE_DIR}/")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/build/cmake)

set (SOURCE_DIR "${PROJECT_SOURCE_DIR}/")

set (MONGOC_API_VERSION 1.0)

set (CPACK_RESOURCE_FILE_LICENSE "${SOURCE_DIR}/COPYING")
set (CPACK_PACKAGE_VERSION_MAJOR ${MONGOC_MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${MONGOC_MINOR_VERSION})

include (CPack)

set (MONGOC_HAVE_WEAK_SYMBOLS 0)

set (MONGOC_CC ${CMAKE_C_COMPILER})
set (MONGOC_USER_SET_CFLAGS ${CMAKE_C_FLAGS})
set (MONGOC_USER_SET_LDFLAGS ${CMAKE_EXE_LINKER_FLAGS})

if (ENABLE_TRACING)
   set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMONGOC_TRACE")
endif ()

configure_file (
   "${SOURCE_DIR}/src/mongoc/mongoc-config.h.in"
   "${PROJECT_BINARY_DIR}/src/mongoc/mongoc-config.h"
)

configure_file (
   "${SOURCE_DIR}/src/mongoc/mongoc-version.h.in"
   "${PROJECT_BINARY_DIR}/src/mongoc/mongoc-version.h"
)

include_directories("${PROJECT_BINARY_DIR}/src/mongoc")
include_directories("${SOURCE_DIR}/src/mongoc")

if (UNIX AND NOT APPLE)
   set(SHM_LIB -lrt)
endif()

if (APPLE)
   cmake_policy(SET CMP0042 OLD)
endif()

if (MSVC)
   add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

add_definitions(-D_GNU_SOURCE)
add_definitions(-D_BSD_SOURCE)
add_definitions(-D_DEFAULT_SOURCE)
if (APPLE)
   # Until CDRIVER-520.
   add_definitions(-Wno-deprecated-declarations)
endif()

set (SOURCES
   ${SOURCE_DIR}/src/mongoc/mongoc-apm.c
   ${SOURCE_DIR}/src/mongoc/mongoc-array.c
   ${SOURCE_DIR}/src/mongoc/mongoc-async.c
   ${SOURCE_DIR}/src/mongoc/mongoc-async-cmd.c
   ${SOURCE_DIR}/src/mongoc/mongoc-b64.c
   ${SOURCE_DIR}/src/mongoc/mongoc-buffer.c
   ${SOURCE_DIR}/src/mongoc/mongoc-bulk-operation.c
   ${SOURCE_DIR}/src/mongoc/mongoc-client.c
   ${SOURCE_DIR}/src/mongoc/mongoc-client-pool.c
   ${SOURCE_DIR}/src/mongoc/mongoc-cluster.c
   ${SOURCE_DIR}/src/mongoc/mongoc-collection.c
   ${SOURCE_DIR}/src/mongoc/mongoc-compression.c
   ${SOURCE_DIR}/src/mongoc/mongoc-counters.c
   ${SOURCE_DIR}/src/mongoc/mongoc-cursor-array.c
   ${SOURCE_DIR}/src/mongoc/mongoc-cursor.c
   ${SOURCE_DIR}/src/mongoc/mongoc-cursor-array.c
   ${SOURCE_DIR}/src/mongoc/mongoc-cursor-cursorid.c
   ${SOURCE_DIR}/src/mongoc/mongoc-cursor-transform.c
   ${SOURCE_DIR}/src/mongoc/mongoc-database.c
   ${SOURCE_DIR}/src/mongoc/mongoc-find-and-modify.c
   ${SOURCE_DIR}/src/mongoc/mongoc-init.c
   ${SOURCE_DIR}/src/mongoc/mongoc-gridfs.c
   ${SOURCE_DIR}/src/mongoc/mongoc-gridfs-file.c
   ${SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-list.c
   ${SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-page.c
   ${SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-list.c
   ${SOURCE_DIR}/src/mongoc/mongoc-handshake.c
   ${SOURCE_DIR}/src/mongoc/mongoc-host-list.c
   ${SOURCE_DIR}/src/mongoc/mongoc-index.c
   ${SOURCE_DIR}/src/mongoc/mongoc-init.c
   ${SOURCE_DIR}/src/mongoc/mongoc-list.c
   ${SOURCE_DIR}/src/mongoc/mongoc-linux-distro-scanner.c
   ${SOURCE_DIR}/src/mongoc/mongoc-log.c
   ${SOURCE_DIR}/src/mongoc/mongoc-matcher.c
   ${SOURCE_DIR}/src/mongoc/mongoc-matcher-op.c
   ${SOURCE_DIR}/src/mongoc/mongoc-memcmp.c
   ${SOURCE_DIR}/src/mongoc/mongoc-cmd.c
   ${SOURCE_DIR}/src/mongoc/mongoc-queue.c
   ${SOURCE_DIR}/src/mongoc/mongoc-read-concern.c
   ${SOURCE_DIR}/src/mongoc/mongoc-read-prefs.c
   ${SOURCE_DIR}/src/mongoc/mongoc-rpc.c
   ${SOURCE_DIR}/src/mongoc/mongoc-server-description.c
   ${SOURCE_DIR}/src/mongoc/mongoc-server-stream.c
   ${SOURCE_DIR}/src/mongoc/mongoc-set.c
   ${SOURCE_DIR}/src/mongoc/mongoc-socket.c
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-buffered.c
   ${SOURCE_DIR}/src/mongoc/mongoc-stream.c
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-buffered.c
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-file.c
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-gridfs.c
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-socket.c
   ${SOURCE_DIR}/src/mongoc/mongoc-topology.c
   ${SOURCE_DIR}/src/mongoc/mongoc-topology-description.c
   ${SOURCE_DIR}/src/mongoc/mongoc-topology-description-apm.c
   ${SOURCE_DIR}/src/mongoc/mongoc-topology-scanner.c
   ${SOURCE_DIR}/src/mongoc/mongoc-uri.c
   ${SOURCE_DIR}/src/mongoc/mongoc-util.c
   ${SOURCE_DIR}/src/mongoc/mongoc-version-functions.c
   ${SOURCE_DIR}/src/mongoc/mongoc-write-command.c
   ${SOURCE_DIR}/src/mongoc/mongoc-write-concern.c
)

set (HEADERS
   ${PROJECT_BINARY_DIR}/src/mongoc/mongoc-config.h
   ${PROJECT_BINARY_DIR}/src/mongoc/mongoc-version.h
   ${SOURCE_DIR}/src/mongoc/mongoc.h
   ${SOURCE_DIR}/src/mongoc/mongoc-apm.h
   ${SOURCE_DIR}/src/mongoc/mongoc-bulk-operation.h
   ${SOURCE_DIR}/src/mongoc/mongoc-client.h
   ${SOURCE_DIR}/src/mongoc/mongoc-client-pool.h
   ${SOURCE_DIR}/src/mongoc/mongoc-collection.h
   ${SOURCE_DIR}/src/mongoc/mongoc-cursor.h
   ${SOURCE_DIR}/src/mongoc/mongoc-database.h
   ${SOURCE_DIR}/src/mongoc/mongoc-error.h
   ${SOURCE_DIR}/src/mongoc/mongoc-flags.h
   ${SOURCE_DIR}/src/mongoc/mongoc-find-and-modify.h
   ${SOURCE_DIR}/src/mongoc/mongoc-gridfs.h
   ${SOURCE_DIR}/src/mongoc/mongoc-gridfs-file.h
   ${SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-page.h
   ${SOURCE_DIR}/src/mongoc/mongoc-gridfs-file-list.h
   ${SOURCE_DIR}/src/mongoc/mongoc-handshake.h
   ${SOURCE_DIR}/src/mongoc/mongoc-host-list.h
   ${SOURCE_DIR}/src/mongoc/mongoc-init.h
   ${SOURCE_DIR}/src/mongoc/mongoc-index.h
   ${SOURCE_DIR}/src/mongoc/mongoc-iovec.h
   ${SOURCE_DIR}/src/mongoc/mongoc-log.h
   ${SOURCE_DIR}/src/mongoc/mongoc-macros.h
   ${SOURCE_DIR}/src/mongoc/mongoc-matcher.h
   ${SOURCE_DIR}/src/mongoc/mongoc-opcode.h
   ${SOURCE_DIR}/src/mongoc/mongoc-read-concern.h
   ${SOURCE_DIR}/src/mongoc/mongoc-read-prefs.h
   ${SOURCE_DIR}/src/mongoc/mongoc-server-description.h
   ${SOURCE_DIR}/src/mongoc/mongoc-socket.h
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-tls-libressl.h
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-tls-openssl.h
   ${SOURCE_DIR}/src/mongoc/mongoc-stream.h
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-buffered.h
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-file.h
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-gridfs.h
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-socket.h
   ${SOURCE_DIR}/src/mongoc/mongoc-topology-description.h
   ${SOURCE_DIR}/src/mongoc/mongoc-uri.h
   ${SOURCE_DIR}/src/mongoc/mongoc-version-functions.h
   ${SOURCE_DIR}/src/mongoc/mongoc-write-concern.h
   ${SOURCE_DIR}/src/mongoc/mongoc-rand.h
   ${SOURCE_DIR}/src/mongoc/mongoc-stream-tls.h
   ${SOURCE_DIR}/src/mongoc/mongoc-ssl.h
)

if (NOT ENABLE_SSL STREQUAL OFF)
   set (SOURCES ${SOURCES}
        ${SOURCE_DIR}/src/mongoc/mongoc-crypto.c
        ${SOURCE_DIR}/src/mongoc/mongoc-scram.c
        ${SOURCE_DIR}/src/mongoc/mongoc-stream-tls.c
        ${SOURCE_DIR}/src/mongoc/mongoc-ssl.c
   )

   if (OPENSSL_FOUND)
      message (STATUS "Compiling against OpenSSL")
      set (SOURCES ${SOURCES}
           ${SOURCE_DIR}/src/mongoc/mongoc-crypto-openssl.c
           ${SOURCE_DIR}/src/mongoc/mongoc-rand-openssl.c
           ${SOURCE_DIR}/src/mongoc/mongoc-stream-tls-openssl.c
           ${SOURCE_DIR}/src/mongoc/mongoc-stream-tls-openssl-bio.c
           ${SOURCE_DIR}/src/mongoc/mongoc-openssl.c
      )
      set (SSL_LIBS ${OPENSSL_LIBRARIES})
      include_directories (${OPENSSL_INCLUDE_DIR})
      if(WIN32)
         set(SSL_LIBS ${SSL_LIBS} crypt32.lib)
      endif()
   elseif (SECURE_TRANSPORT)
      message (STATUS "Compiling against Secure Transport")
      set (SOURCES ${SOURCES}
              ${SOURCE_DIR}/src/mongoc/mongoc-crypto-common-crypto.c
              ${SOURCE_DIR}/src/mongoc/mongoc-rand-common-crypto.c
              ${SOURCE_DIR}/src/mongoc/mongoc-stream-tls-secure-transport.c
              ${SOURCE_DIR}/src/mongoc/mongoc-secure-transport.c
      )
      set (SSL_LIBS "-framework CoreFoundation -framework Security")
   elseif (SECURE_CHANNEL)
      message (STATUS "Compiling against Secure Channel")
      set (SOURCES ${SOURCES}
              ${SOURCE_DIR}/src/mongoc/mongoc-crypto-cng.c
              ${SOURCE_DIR}/src/mongoc/mongoc-rand-cng.c
              ${SOURCE_DIR}/src/mongoc/mongoc-stream-tls-secure-channel.c
              ${SOURCE_DIR}/src/mongoc/mongoc-secure-channel.c
      )
      set(SSL_LIBS secur32.lib crypt32.lib Bcrypt.lib)
   elseif (LIBRESSL)
      message (STATUS "Compiling against LibreSSL")
      set (SOURCES ${SOURCES}
              ${SOURCE_DIR}/src/mongoc/mongoc-crypto-openssl.c
              ${SOURCE_DIR}/src/mongoc/mongoc-rand-openssl.c
              ${SOURCE_DIR}/src/mongoc/mongoc-stream-tls-libressl.c
              ${SOURCE_DIR}/src/mongoc/mongoc-libressl.c
      )
      set(SSL_LIBS -ltls -lcrypto)
   endif ()
else ()
   message (STATUS "SSL disabled")
endif () # ENABLE_SSL


if (MONGOC_ENABLE_SASL)
   set (SOURCES ${SOURCES} ${SOURCE_DIR}/src/mongoc/mongoc-cluster-sasl.c)
   set (SOURCES ${SOURCES} ${SOURCE_DIR}/src/mongoc/mongoc-sasl.c)
   if (MONGOC_ENABLE_SASL_CYRUS)
      message (STATUS "Compiling against Cyrus SASL")
      set (SOURCES ${SOURCES} ${SOURCE_DIR}/src/mongoc/mongoc-cluster-cyrus.c)
      set (SOURCES ${SOURCES} ${SOURCE_DIR}/src/mongoc/mongoc-cyrus.c)
      include_directories(${SASL_INCLUDE_DIRS})
   elseif (MONGOC_ENABLE_SASL_SSPI)
      message (STATUS "Compiling against Windows SSPI")
      set (SOURCES ${SOURCES} ${SOURCE_DIR}/src/mongoc/mongoc-cluster-sspi.c)
      set (SOURCES ${SOURCES} ${SOURCE_DIR}/src/mongoc/mongoc-sspi.c)
      set (SASL_LIBS secur32.lib crypt32.lib Shlwapi.lib)
   elseif (MONGOC_ENABLE_SASL_GSSAPI)
      set (SOURCES ${SOURCES} ${SOURCE_DIR}/src/mongoc/mongoc-cluster-gssapi.c)
      set (SOURCES ${SOURCES} ${SOURCE_DIR}/src/mongoc/mongoc-gssapi.c)
      if (APPLE)
         message (STATUS "Compiling against macOS GSS")
         set (LIBS ${LIBS} -framework GSS)
      else ()
         message (FATAL_ERROR "gssapi missing krb5-config support in cmake")
      endif()
   endif()
else()
   message (STATUS "SASL disabled")
endif()


set (MONGOC_INTERNAL_INCLUDE_DIRS "")

if (ENABLE_SNAPPY STREQUAL "bundled")
   set (MONGOC_ENABLE_COMPRESSION 1)
   set (MONGOC_ENABLE_COMPRESSION_SNAPPY 1)
   include(FindSnappy)
endif ()

if (ENABLE_ZLIB STREQUAL "bundled")
   message (STATUS "Enabling zlib compression (bundled)")
   check_include_files ("unistd.h" HAVE_UNISTD_H)
   check_include_files ("stdarg.h" HAVE_STDARG_H)
   set (MONGOC_ENABLE_COMPRESSION 1)
   set (MONGOC_ENABLE_COMPRESSION_ZLIB 1)
   set (SOURCES ${SOURCES}
      ${SOURCE_DIR}/src/zlib-1.2.11/adler32.c
      ${SOURCE_DIR}/src/zlib-1.2.11/crc32.c
      ${SOURCE_DIR}/src/zlib-1.2.11/deflate.c
      ${SOURCE_DIR}/src/zlib-1.2.11/infback.c
      ${SOURCE_DIR}/src/zlib-1.2.11/inffast.c
      ${SOURCE_DIR}/src/zlib-1.2.11/inflate.c
      ${SOURCE_DIR}/src/zlib-1.2.11/inftrees.c
      ${SOURCE_DIR}/src/zlib-1.2.11/trees.c
      ${SOURCE_DIR}/src/zlib-1.2.11/zutil.c
      ${SOURCE_DIR}/src/zlib-1.2.11/compress.c
      ${SOURCE_DIR}/src/zlib-1.2.11/uncompr.c
      ${SOURCE_DIR}/src/zlib-1.2.11/gzclose.c
      ${SOURCE_DIR}/src/zlib-1.2.11/gzlib.c
      ${SOURCE_DIR}/src/zlib-1.2.11/gzread.c
      ${SOURCE_DIR}/src/zlib-1.2.11/gzwrite.c
   )
   configure_file (
      "${SOURCE_DIR}/src/zlib-1.2.11/zconf.h.in"
      "${PROJECT_BINARY_DIR}/src/zlib-1.2.11/zconf.h"
   )

   list (
      APPEND
      MONGOC_INTERNAL_INCLUDE_DIRS
      "${SOURCE_DIR}/src/zlib-1.2.11"
      "${PROJECT_BINARY_DIR}/src/zlib-1.2.11"
   )
endif ()

set(THREADS_PREFER_PTHREAD_FLAG 1)
find_package (Threads REQUIRED)
if(CMAKE_USE_PTHREADS_INIT)
   set(THREAD_LIB ${CMAKE_THREAD_LIBS_INIT})
endif()

set (LIBS ${SASL_LIBS} ${SSL_LIBS} ${SHM_LIB} Threads::Threads)
if(WIN32)
   set (LIBS ${LIBS} ws2_32)
endif()

add_library(mongoc_shared SHARED ${SOURCES} ${HEADERS})
set_target_properties(mongoc_shared PROPERTIES CMAKE_CXX_VISIBILITY_PRESET hidden)
target_link_libraries (mongoc_shared ${LIBS} ${BSON_LIBRARIES})
target_include_directories (mongoc_shared BEFORE PUBLIC ${BSON_INCLUDE_DIRS} ${MONGOC_INTERNAL_INCLUDE_DIRS})
target_compile_definitions (mongoc_shared PUBLIC MONGOC_COMPILATION ${BSON_DEFINITIONS})

# CMake wants to make different symlinks than the Autotools, see
# https://cmake.org/pipermail/cmake/2007-September/016501.html
# This hack sets up standard symlink, libmongoc-1.0.so -> libmongoc-1.0.0.so
set_target_properties(mongoc_shared PROPERTIES VERSION 0 SOVERSION ${MONGOC_MAJOR_VERSION})
set_target_properties(mongoc_shared PROPERTIES OUTPUT_NAME "mongoc-${MONGOC_API_VERSION}" PREFIX "lib")

if (MONGOC_ENABLE_STATIC)
   add_library(mongoc_static STATIC ${SOURCES} ${HEADERS})
   target_link_libraries (mongoc_static ${LIBS} ${BSON_STATIC_LIBRARIES})
   target_include_directories (mongoc_static BEFORE PUBLIC ${BSON_STATIC_INCLUDE_DIRS} ${MONGOC_INTERNAL_INCLUDE_DIRS})
   target_compile_definitions (mongoc_static PUBLIC MONGOC_COMPILATION MONGOC_STATIC ${BSON_STATIC_DEFINITIONS})
   set_target_properties(mongoc_static PROPERTIES VERSION ${MONGOC_VERSION})
   set_target_properties(mongoc_static PROPERTIES OUTPUT_NAME "mongoc-static-${MONGOC_API_VERSION}")
endif ()

if (ENABLE_SNAPPY STREQUAL "bundled")
   set_target_properties(mongoc_shared PROPERTIES LINKER_LANGUAGE CXX)
   if (MONGOC_ENABLE_STATIC)
      set_target_properties(mongoc_static PROPERTIES LINKER_LANGUAGE CXX)
   endif ()
endif ()
function(mongoc_add_test test use_shared)
   if (ENABLE_TESTS)
      add_executable(${test} ${ARGN})
      if (NOT MSVC)
         # We've tests that test our deprecated api. MSVC 2013 will complain about invalid flag
         set_source_files_properties(${ARGN} PROPERTIES COMPILE_FLAGS -Wno-deprecated-declarations)
      endif()
      if (${use_shared})
         target_link_libraries(${test} mongoc_shared)
         target_include_directories(${test} PRIVATE ${BSON_INCLUDE_DIRS} ${MONGOC_INTERNAL_INCLUDE_DIRS})
      else()
         target_compile_definitions(${test} PUBLIC "MONGOC_COMPILATION")
         target_link_libraries(${test} mongoc_static)
         target_include_directories(${test} PRIVATE ${BSON_STATIC_INCLUDE_DIRS} ${MONGOC_INTERNAL_INCLUDE_DIRS})
      endif()
      if(WIN32)
         target_link_libraries(${test} shlwapi)
      endif()
   endif()
endfunction()

function(mongoc_add_example example use_shared)
   if (ENABLE_EXAMPLES)
      add_executable(${example} ${ARGN})
      if (${use_shared})
         target_link_libraries(${example} mongoc_shared)
      else()
         target_link_libraries(${example} mongoc_static)
      endif()
      if(WIN32)
         target_link_libraries(${example} shlwapi)
      endif()
      set (EXAMPLES ${EXAMPLES} ${example})
      if (ENABLE_SNAPPY STREQUAL "bundled")
         set_target_properties(${example} PROPERTIES LINKER_LANGUAGE CXX)
         set_target_properties(${example} PROPERTIES LINKER_LANGUAGE CXX)
      endif ()
   endif ()
endfunction()

set(test-libmongoc-sources
   ${SOURCE_DIR}/tests/debug-stream.c
   ${SOURCE_DIR}/tests/ha-test.c
   ${SOURCE_DIR}/tests/json-test.c
   ${SOURCE_DIR}/tests/mock_server/future.c
   ${SOURCE_DIR}/tests/mock_server/future-functions.c
   ${SOURCE_DIR}/tests/mock_server/future-value.c
   ${SOURCE_DIR}/tests/mock_server/sync-queue.c
   ${SOURCE_DIR}/tests/mock_server/mock-rs.c
   ${SOURCE_DIR}/tests/mock_server/mock-server.c
   ${SOURCE_DIR}/tests/mock_server/request.c
   ${SOURCE_DIR}/tests/test-conveniences.c
   ${SOURCE_DIR}/tests/test-bulk.c
   ${SOURCE_DIR}/tests/test-libmongoc.c
   ${SOURCE_DIR}/tests/test-mongoc-array.c
   ${SOURCE_DIR}/tests/test-mongoc-async.c
   ${SOURCE_DIR}/tests/test-mongoc-buffer.c
   ${SOURCE_DIR}/tests/test-mongoc-client.c
   ${SOURCE_DIR}/tests/test-bulk.c
   ${SOURCE_DIR}/tests/test-mongoc-client-pool.c
   ${SOURCE_DIR}/tests/test-mongoc-cluster.c
   ${SOURCE_DIR}/tests/test-mongoc-collection.c
   ${SOURCE_DIR}/tests/test-mongoc-collection-find.c
   ${SOURCE_DIR}/tests/test-mongoc-collection-find-with-opts.c
   ${SOURCE_DIR}/tests/test-mongoc-connection-uri.c
   ${SOURCE_DIR}/tests/test-mongoc-command-monitoring.c
   ${SOURCE_DIR}/tests/test-mongoc-cursor.c
   ${SOURCE_DIR}/tests/test-mongoc-database.c
   ${SOURCE_DIR}/tests/test-mongoc-error.c
   ${SOURCE_DIR}/tests/test-mongoc-exhaust.c
   ${SOURCE_DIR}/tests/test-mongoc-find-and-modify.c
   ${SOURCE_DIR}/tests/test-mongoc-gridfs.c
   ${SOURCE_DIR}/tests/test-mongoc-gridfs-file-page.c
   ${SOURCE_DIR}/tests/test-mongoc-handshake.c
   ${SOURCE_DIR}/tests/test-mongoc-linux-distro-scanner.c
   ${SOURCE_DIR}/tests/test-mongoc-list.c
   ${SOURCE_DIR}/tests/test-mongoc-log.c
   ${SOURCE_DIR}/tests/test-mongoc-matcher.c
   ${SOURCE_DIR}/tests/test-mongoc-max-staleness.c
   ${SOURCE_DIR}/tests/test-mongoc-queue.c
   ${SOURCE_DIR}/tests/test-mongoc-read-prefs.c
   ${SOURCE_DIR}/tests/test-mongoc-rpc.c
   ${SOURCE_DIR}/tests/test-mongoc-sample-commands.c
   ${SOURCE_DIR}/tests/test-mongoc-sdam.c
   ${SOURCE_DIR}/tests/test-mongoc-sdam-monitoring.c
   ${SOURCE_DIR}/tests/test-mongoc-server-selection.c
   ${SOURCE_DIR}/tests/test-mongoc-server-selection-errors.c
   ${SOURCE_DIR}/tests/test-mongoc-set.c
   ${SOURCE_DIR}/tests/test-mongoc-socket.c
   ${SOURCE_DIR}/tests/test-mongoc-stream.c
   ${SOURCE_DIR}/tests/test-mongoc-thread.c
   ${SOURCE_DIR}/tests/test-mongoc-topology.c
   ${SOURCE_DIR}/tests/test-mongoc-topology-description.c
   ${SOURCE_DIR}/tests/test-mongoc-topology-reconcile.c
   ${SOURCE_DIR}/tests/test-mongoc-topology-scanner.c
   ${SOURCE_DIR}/tests/test-mongoc-uri.c
   ${SOURCE_DIR}/tests/test-mongoc-version.c
   ${SOURCE_DIR}/tests/test-mongoc-usleep.c
   ${SOURCE_DIR}/tests/test-mongoc-util.c
   ${SOURCE_DIR}/tests/test-mongoc-write-concern.c
   ${SOURCE_DIR}/tests/test-write-commands.c
   ${SOURCE_DIR}/tests/TestSuite.c
)

if (MONGOC_ENABLE_SSL)
   set(test-libmongoc-sources ${test-libmongoc-sources}
      ${SOURCE_DIR}/tests/test-x509.c
      ${SOURCE_DIR}/tests/ssl-test.c
      ${SOURCE_DIR}/tests/test-mongoc-stream-tls.c
      ${SOURCE_DIR}/tests/test-mongoc-stream-tls-error.c)
   mongoc_add_test(test-replica-set-ssl FALSE
      ${SOURCE_DIR}/tests/test-replica-set-ssl.c
      ${SOURCE_DIR}/tests/ha-test.c
      ${SOURCE_DIR}/tests/mongoc-tests.c
   )
endif()

if (MONGOC_ENABLE_SASL)
   set(test-libmongoc-sources ${test-libmongoc-sources}
      ${SOURCE_DIR}/tests/test-cyrus.c
   )
endif ()

mongoc_add_test(test-load FALSE
   ${SOURCE_DIR}/tests/test-load.c
   ${SOURCE_DIR}/tests/mongoc-tests.c)
mongoc_add_test(test-secondary FALSE
   ${SOURCE_DIR}/tests/test-secondary.c
   ${SOURCE_DIR}/tests/mongoc-tests.c)
mongoc_add_test(test-replica-set FALSE
   ${SOURCE_DIR}/tests/test-replica-set.c
   ${SOURCE_DIR}/tests/mongoc-tests.c
   ${SOURCE_DIR}/tests/ha-test.c)
mongoc_add_test(test-sharded-cluster FALSE
   ${SOURCE_DIR}/tests/test-sharded-cluster.c
   ${SOURCE_DIR}/tests/mongoc-tests.c
   ${SOURCE_DIR}/tests/ha-test.c)

mongoc_add_test(test-libmongoc FALSE ${test-libmongoc-sources})

if (ENABLE_TESTS)
   enable_testing()
   add_test(NAME test-libmongoc COMMAND test-libmongoc --no-fork -d)
endif ()

mongoc_add_example(example-gridfs TRUE ${SOURCE_DIR}/examples/example-gridfs.c)
mongoc_add_example(
   example-command-monitoring TRUE
   ${SOURCE_DIR}/examples/example-command-monitoring.c)
mongoc_add_example(
   example-sdam-monitoring TRUE
   ${SOURCE_DIR}/examples/example-sdam-monitoring.c)
mongoc_add_example(example-client TRUE ${SOURCE_DIR}/examples/example-client.c)
mongoc_add_example(example-command-with-opts TRUE ${SOURCE_DIR}/examples/example-command-with-opts.c)
mongoc_add_example(example-create-indexes TRUE ${SOURCE_DIR}/examples/example-create-indexes.c)
mongoc_add_example(example-scram TRUE ${SOURCE_DIR}/examples/example-scram.c)
mongoc_add_example(mongoc-dump TRUE ${SOURCE_DIR}/examples/mongoc-dump.c)
mongoc_add_example(mongoc-ping TRUE ${SOURCE_DIR}/examples/mongoc-ping.c)
mongoc_add_example(mongoc-tail TRUE ${SOURCE_DIR}/examples/mongoc-tail.c)
if (NOT WIN32)
   mongoc_add_example(example-pool TRUE ${SOURCE_DIR}/examples/example-pool.c)
endif ()

file(COPY ${SOURCE_DIR}/tests/binary DESTINATION ${PROJECT_BINARY_DIR}/tests)
file(COPY ${SOURCE_DIR}/tests/json DESTINATION ${PROJECT_BINARY_DIR}/tests)
file(COPY ${SOURCE_DIR}/tests/x509gen DESTINATION ${PROJECT_BINARY_DIR}/tests)
file(COPY ${SOURCE_DIR}/tests/release_files DESTINATION ${PROJECT_BINARY_DIR}/tests)

install(
  TARGETS mongoc_shared ${EXAMPLES}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

if (MONGOC_ENABLE_STATIC)
   install(
     TARGETS mongoc_shared mongoc_static ${EXAMPLES}
     LIBRARY DESTINATION lib
     ARCHIVE DESTINATION lib
     RUNTIME DESTINATION bin
   )
endif ()

set (MONGOC_HEADER_INSTALL_DIR "include/libmongoc-${MONGOC_API_VERSION}")

install(
  FILES ${HEADERS}
  DESTINATION "${MONGOC_HEADER_INSTALL_DIR}"
)

# Define pkg-config files
set(VERSION "${MONGOC_VERSION}")
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(libdir "\${prefix}/lib")

foreach(FLAG ${SASL_LIBS} ${SSL_LIBS} ${SHM_LIB} ${THREAD_LIB} ${ZLIB_LIBS} ${SNAPPY_LIBS})
   if ( IS_ABSOLUTE "${FLAG}" )
      get_filename_component(FLAG_DIR "${FLAG}" DIRECTORY)
      get_filename_component(FLAG_FILE "${FLAG}" NAME_WE)
      STRING(REGEX REPLACE "^lib" "" FLAG_FILE "${FLAG_FILE}")
      set(MONGOC_LIBS "${MONGOC_LIBS} -L${FLAG_DIR} -l${FLAG_FILE}")
   elseif (FLAG MATCHES "^-.*")
      # Flag starts with dash, add it as-is.
      set(pkg_config_libs "${pkg_config_libs} ${FLAG}")
   else ()
      # Flag doesn't start with dash, add it with a dash.
      set(MONGOC_LIBS "${MONGOC_LIBS} -l${FLAG}")
   endif ()
endforeach()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/libmongoc-1.0.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-1.0.pc
@ONLY)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-1.0.pc
  DESTINATION lib/pkgconfig
)
if (MONGOC_ENABLE_STATIC)
   configure_file(
     ${CMAKE_CURRENT_SOURCE_DIR}/src/libmongoc-static-1.0.pc.in
     ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-static-1.0.pc
   @ONLY)
   install(
     FILES ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-static-1.0.pc
     DESTINATION lib/pkgconfig
   )
endif ()
# Deprecated alias for libmongoc-1.0.pc, see CDRIVER-2086.
if (MONGOC_ENABLE_SSL)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libmongoc-ssl-1.0.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-ssl-1.0.pc
  @ONLY)
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/src/libmongoc-ssl-1.0.pc
    DESTINATION lib/pkgconfig
  )
endif()

include (build/cmake/MongoCPackage.cmake)

