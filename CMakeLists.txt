cmake_minimum_required(VERSION 3.0)

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

add_subdirectory(src)

