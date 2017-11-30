cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")

find_package(Boost COMPONENTS thread program_options REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(BDB REQUIRED)

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

add_definitions(-DHAVE_WORKING_BOOST_SLEEP_FOR=1)

add_definitions(-DUSE_NUM_NONE=1)
add_definitions(-DUSE_FIELD_10X26=1)
add_definitions(-DUSE_FIELD_INV_BUILTIN=1)
add_definitions(-DUSE_SCALAR_8X32=1)
add_definitions(-DUSE_SCALAR_INV_BUILTIN=1)
add_definitions(-DENABLE_MODULE_RECOVERY=1)

add_definitions(-DENABLE_WALLET=1)


add_subdirectory(src/univalue)
add_subdirectory(src/secp256k1)
add_subdirectory(src)

