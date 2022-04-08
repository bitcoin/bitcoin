# BLS Tests
find_package(Threads REQUIRED)

add_executable(bls_c256_test bls_c256_test.cpp)
target_link_libraries(bls_c256_test PRIVATE bls::bls256 Threads::Threads)
target_compile_definitions(bls_c256_test PRIVATE BLS_DONT_EXPORT)
add_test(NAME bls_c256_test COMMAND bls_c256_test)

add_executable(bls_c384_test bls_c384_test.cpp)
target_link_libraries(bls_c384_test PRIVATE bls::bls384 Threads::Threads)
target_compile_definitions(bls_c384_test  PRIVATE BLS_DONT_EXPORT)
add_test(NAME bls_c384_test COMMAND bls_c384_test)

add_executable(bls_c384_256_test bls_c384_256_test.cpp)
target_link_libraries(bls_c384_256_test PRIVATE bls::bls384_256 Threads::Threads)
target_compile_definitions(bls_c384_256_test  PRIVATE BLS_DONT_EXPORT)
add_test(NAME bls_c384_256_test COMMAND bls_c384_256_test)

add_executable(bls256_test bls256_test.cpp)
target_link_libraries(bls256_test PRIVATE bls::bls256 Threads::Threads)
target_compile_definitions(bls256_test PRIVATE BLS_DONT_EXPORT)
add_test(NAME bls256_test COMMAND bls256_test)

add_executable(bls384_test bls384_test.cpp)
target_link_libraries(bls384_test PRIVATE bls::bls384 Threads::Threads)
target_compile_definitions(bls384_test  PRIVATE BLS_DONT_EXPORT)
add_test(NAME bls384_test COMMAND bls384_test)

add_executable(bls384_256_test bls384_256_test.cpp)
target_link_libraries(bls384_256_test PRIVATE bls::bls384_256 Threads::Threads)
target_compile_definitions(bls384_256_test  PRIVATE BLS_DONT_EXPORT)
add_test(NAME bls384_256_test COMMAND bls384_256_test)
