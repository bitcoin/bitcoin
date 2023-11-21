add_library(example INTERFACE)
target_include_directories(example INTERFACE
  ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(example INTERFACE
  secp256k1
  $<$<PLATFORM_ID:Windows>:bcrypt>
)
if(NOT BUILD_SHARED_LIBS AND MSVC)
  target_link_options(example INTERFACE /IGNORE:4217)
endif()

add_executable(ecdsa_example ecdsa.c)
target_link_libraries(ecdsa_example example)
add_test(NAME ecdsa_example COMMAND ecdsa_example)

if(SECP256K1_ENABLE_MODULE_ECDH)
  add_executable(ecdh_example ecdh.c)
  target_link_libraries(ecdh_example example)
  add_test(NAME ecdh_example COMMAND ecdh_example)
endif()

if(SECP256K1_ENABLE_MODULE_SCHNORRSIG)
  add_executable(schnorr_example schnorr.c)
  target_link_libraries(schnorr_example example)
  add_test(NAME schnorr_example COMMAND schnorr_example)
endif()
