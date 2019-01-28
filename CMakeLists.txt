cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Boost COMPONENTS thread program_options filesystem unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(BDB REQUIRED)
find_package(CURL REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Protobuf REQUIRED)

add_definitions(-DHAVE_ENDIAN_H=1)
add_definitions(-DHAVE_DECL_HTOBE16=1)
add_definitions(-DHAVE_DECL_HTOLE16=1)
add_definitions(-DHAVE_DECL_BE16TOH=1)
add_definitions(-DHAVE_DECL_LE16TOH=1)
add_definitions(-DHAVE_DECL_HTOBE32=1)
add_definitions(-DHAVE_DECL_HTOLE32=1)
add_definitions(-DHAVE_DECL_BE32TOH=1)
add_definitions(-DHAVE_DECL_LE32TOH=1)
add_definitions(-DHAVE_DECL_HTOBE64=1)
add_definitions(-DHAVE_DECL_HTOLE64=1)
add_definitions(-DHAVE_DECL_BE64TOH=1)
add_definitions(-DHAVE_DECL_LE64TOH=1)
add_definitions(-DHAVE_DECL_BSWAP_16=1)
add_definitions(-DHAVE_DECL_BSWAP_32=1)
add_definitions(-DHAVE_DECL_BSWAP_64=1)

add_definitions(-DHAVE_WORKING_BOOST_SLEEP_FOR=1)

add_definitions(-DUSE_NUM_NONE=1)
add_definitions(-DUSE_FIELD_10X26=1)
add_definitions(-DUSE_FIELD_INV_BUILTIN=1)
add_definitions(-DUSE_SCALAR_8X32=1)
add_definitions(-DUSE_SCALAR_INV_BUILTIN=1)
add_definitions(-DENABLE_MODULE_RECOVERY=1)

add_definitions(-DENABLE_WALLET=1)

message(${CURL_LIBRARIES})
message(${CURL_INCLUDE_DIR})
message(${CURL_FOUND})
if(CURL_FOUND)
  set(CURL_LIBRARIES ${CURL_LIBRARY})
  set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})

  if(NOT TARGET CURL::libcurl)
    add_library(CURL::libcurl UNKNOWN IMPORTED)
    set_target_properties(CURL::libcurl PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}")

    if(EXISTS "${CURL_LIBRARY}")
      set_target_properties(CURL::libcurl PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${CURL_LIBRARY}")
    endif()
    if(CURL_LIBRARY_RELEASE)
      set_property(TARGET CURL::libcurl APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(CURL::libcurl PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION_RELEASE "${CURL_LIBRARY_RELEASE}")
    endif()
    if(CURL_LIBRARY_DEBUG)
      set_property(TARGET CURL::libcurl APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(CURL::libcurl PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION_DEBUG "${CURL_LIBRARY_DEBUG}")
    endif()
  endif()
endif()


add_subdirectory(src/univalue)
add_subdirectory(src/secp256k1)
add_subdirectory(src/test)
add_subdirectory(src)
add_subdirectory(src/qt)

