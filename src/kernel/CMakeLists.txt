# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(bitcoinkernel
  bitcoinkernel.cpp
)
target_link_libraries(bitcoinkernel
  PRIVATE
    core_interface
    bitcoin_clientversion
    bitcoin_kernel
    bitcoin_kernel_util
    bitcoin_consensus
  PUBLIC
    Boost::headers
)

# libbitcoinkernel requires default symbol visibility, explicitly
# specify that here so that things still work even when user
# configures with -DREDUCE_EXPORTS=ON
#
# Note this is a quick hack that will be removed as we
# incrementally define what to export from the library.
set_target_properties(bitcoinkernel PROPERTIES
  CXX_VISIBILITY_PRESET default
)

# When building the static library, install all static libraries the
# bitcoinkernel depends on.
if(NOT BUILD_SHARED_LIBS)
  # Recursively get all the static libraries a target depends on and put them in libs_out
  function(get_target_static_link_libs target libs_out)
    get_target_property(linked_libraries ${target} LINK_LIBRARIES)
    foreach(dep ${linked_libraries})
      if(TARGET ${dep})
        get_target_property(dep_type ${dep} TYPE)
        if(dep_type STREQUAL "STATIC_LIBRARY")
          list(APPEND ${libs_out} ${dep})
          get_target_static_link_libs(${dep} ${libs_out})
        endif()
      endif()
    endforeach()
    set(${libs_out} ${${libs_out}} PARENT_SCOPE)
  endfunction()

  set(all_kernel_static_link_libs "")
  get_target_static_link_libs(bitcoinkernel all_kernel_static_link_libs)

  # LIBS_PRIVATE is substituted in the pkg-config file.
  set(LIBS_PRIVATE "")
  foreach(lib ${all_kernel_static_link_libs})
    install(TARGETS ${lib} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    string(APPEND LIBS_PRIVATE " -l${lib}")
  endforeach()

  string(STRIP "${LIBS_PRIVATE}" LIBS_PRIVATE)
endif()

configure_file(${PROJECT_SOURCE_DIR}/libbitcoinkernel.pc.in ${PROJECT_BINARY_DIR}/libbitcoinkernel.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/libbitcoinkernel.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

include(GNUInstallDirs)
install(TARGETS bitcoinkernel
  RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Kernel
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Kernel
  ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Kernel
)
