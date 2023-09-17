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
  add_test(NAME ${test_name} COMMAND ${target_name})
  if(BUILD_SHARED_LIBS AND MSVC)
    # The DLL must reside either in the same folder where the executable is
    # or somewhere in PATH. Using the latter option.
    set_tests_properties(${test_name} PROPERTIES
      ENVIRONMENT "PATH=$<TARGET_FILE_DIR:secp256k1>;$ENV{PATH}"
    )
  endif()
endfunction()

add_example(ecdsa)

if(SECP256K1_ENABLE_MODULE_ECDH)
  add_example(ecdh)
endif()

if(SECP256K1_ENABLE_MODULE_SCHNORRSIG)
  add_example(schnorr)
endif()
