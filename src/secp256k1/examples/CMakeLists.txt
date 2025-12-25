function(add_example name)
  set(target_name ${name}_example)
  add_executable(${target_name} ${name}.c)
  target_include_directories(${target_name} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
  )
  target_link_libraries(${target_name}
    secp256k1
    $<$<PLATFORM_ID:Windows>:bcrypt>
  )
  set(test_name ${name}_example)
  add_test(NAME secp256k1_${test_name} COMMAND ${target_name})
endfunction()

add_example(ecdsa)

if(SECP256K1_ENABLE_MODULE_ECDH)
  add_example(ecdh)
endif()

if(SECP256K1_ENABLE_MODULE_SCHNORRSIG)
  add_example(schnorr)
endif()

if(SECP256K1_ENABLE_MODULE_ELLSWIFT)
  add_example(ellswift)
endif()

if(SECP256K1_ENABLE_MODULE_MUSIG)
  add_example(musig)
endif()
