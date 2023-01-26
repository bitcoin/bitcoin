# BLS sample code
add_executable(bls_sample bls_smpl.cpp)
target_link_libraries(bls_sample PRIVATE bls::bls384_256)
target_compile_definitions(bls_sample  PRIVATE BLS_DONT_EXPORT)

add_executable(bls12_381_sample bls12_381_smpl.cpp)
target_link_libraries(bls12_381_sample PRIVATE bls::bls384_256)
target_compile_definitions(bls12_381_sample  PRIVATE BLS_DONT_EXPORT)

add_executable(minsample minsample.c)
target_link_libraries(minsample PRIVATE bls::bls384_256)
target_compile_definitions(minsample  PRIVATE BLS_DONT_EXPORT)
