cmake_minimum_required(VERSION 3.16)
project(pq-shield LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT LIBOQS_ROOT)
  set(LIBOQS_ROOT "${CMAKE_SOURCE_DIR}/../../../liboqs/build" CACHE PATH "liboqs build dir")
endif()

add_library(pqshield STATIC src/pq_shield_core.cpp)
target_include_directories(pqshield PUBLIC src "${LIBOQS_ROOT}/include")
target_link_libraries(pqshield PUBLIC "${LIBOQS_ROOT}/lib/liboqs.a" crypto ssl)

add_executable(pq-shield src/pq_shield.cpp)
target_link_libraries(pq-shield PRIVATE pqshield)

add_executable(pq-shield-audit src/audit.cpp)
target_link_libraries(pq-shield-audit PRIVATE pqshield)

add_executable(test-pq-shield test/test_pq_shield.cpp)
target_link_libraries(test-pq-shield PRIVATE pqshield)
enable_testing()
add_test(NAME pq-shield-test COMMAND test-pq-shield)
