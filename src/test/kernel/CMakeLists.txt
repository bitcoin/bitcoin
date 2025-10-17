add_executable(test_kernel
    test_kernel.cpp
)

target_link_libraries(test_kernel
  PRIVATE
    core_interface
    bitcoinkernel
    Boost::headers
)

set_target_properties(test_kernel PROPERTIES
  SKIP_BUILD_RPATH OFF
)

add_test(NAME test_kernel COMMAND test_kernel)
