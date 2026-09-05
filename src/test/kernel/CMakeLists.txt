add_executable(test_kernel
    test_kernel.cpp
)

target_link_libraries(test_kernel
  PRIVATE
    core_interface
    bitcoinkernel
    Boost::headers
)

if(ENABLE_SCRIPT_TRACE)
  target_compile_definitions(test_kernel PRIVATE ENABLE_SCRIPT_TRACE)
endif()

add_windows_application_manifest(test_kernel)

add_test(NAME test_kernel COMMAND test_kernel)
