add_executable(test_kernel
    test_kernel.cpp
)

target_link_libraries(test_kernel
  PRIVATE
    core_interface
    bitcoinkernel
    Boost::headers
)

add_test(NAME test_kernel COMMAND test_kernel)
