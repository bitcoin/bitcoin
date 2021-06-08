CMAKE_MINIMUM_REQUIRED(VERSION 3.14.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

include_directories(
        ${INCLUDE_DIRECTORIES}
        ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/relic/include
        ${CMAKE_BINARY_DIR}/contrib/relic/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/catch
)

file(GLOB_RECURSE WRAP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/wrappers/*.h)
file(GLOB_RECURSE WRAP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/wrappers/*.cpp)

add_executable(blsjs ${CMAKE_CURRENT_SOURCE_DIR}/jsbindings.cpp
        ${WRAP_HEADERS} ${WRAP_SRC}  ${CMAKE_CURRENT_SOURCE_DIR}/helpers.h ${CMAKE_CURRENT_SOURCE_DIR}/helpers.cpp
        )
add_custom_target(install_npm_dependencies npm ci)
add_dependencies(blsjs install_npm_dependencies)

if (SODIUM_FOUND)
    target_link_libraries(blsjs PRIVATE blstmp relic_s sodium)
else ()
    target_link_libraries(blsjs PRIVATE blstmp relic_s)
endif ()

# Copy necessary files for the npm package
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/package.json package.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/package-lock.json package-lock.json COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/blsjs.d.ts blsjs.d.ts COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/README.md README.md COPYONLY)

# Copy test files
file(GLOB JS_BINDINGS_TESTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/tests/ ${CMAKE_CURRENT_SOURCE_DIR}/tests/*)
foreach(file ${JS_BINDINGS_TESTS})
    message(FILE ${file})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/${file} tests/${file} COPYONLY)
endforeach()

set_target_properties(blsjs PROPERTIES LINK_FLAGS "--bind -Oz --closure 1 -s MODULARIZE=1")
