# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(GNUInstallDirs)

add_library(bitcoinconsensus
  ../script/bitcoinconsensus.cpp
  ../util/strencodings.cpp
)
target_compile_definitions(bitcoinconsensus
  PRIVATE
    BUILD_BITCOIN_INTERNAL
)
target_link_libraries(bitcoinconsensus
  PRIVATE
    core_interface
    external_lib_interface
    bitcoin_consensus
    bitcoin_crypto
    secp256k1
)
set_target_properties(bitcoinconsensus PROPERTIES
  SOVERSION 0
  VERSION 0.0.0
)

add_library(libbitcoinconsensus ALIAS bitcoinconsensus)

# When building the static library, install all static libraries the
# libbitcoinconsensus depends on.
if(NOT BUILD_SHARED_LIBS)
  # Recursively get all the static libraries a target depends on and put them in libs_out
  function(get_libbitcoinconsensus_static_link_libs target libs_out)
    get_target_property(linked_libraries ${target} LINK_LIBRARIES)
    foreach(dep ${linked_libraries})
      if(TARGET ${dep})
        add_dependencies(bitcoinconsensus ${dep})
        get_target_property(dep_type ${dep} TYPE)
        if(dep_type STREQUAL "STATIC_LIBRARY")
          list(APPEND ${libs_out} ${dep})
          get_libbitcoinconsensus_static_link_libs(${dep} ${libs_out})
        endif()
      endif()
    endforeach()
    set(${libs_out} ${${libs_out}} PARENT_SCOPE)
  endfunction()

  set(all_consensus_static_link_libs "")
  get_libbitcoinconsensus_static_link_libs(bitcoinconsensus all_consensus_static_link_libs)

  install(TARGETS ${all_consensus_static_link_libs} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libbitcoinconsensus)
  list(TRANSFORM all_consensus_static_link_libs PREPEND "-l")
  # LIBS_PRIVATE is substituted in the pkg-config file.
  list(JOIN all_consensus_static_link_libs " " LIBS_PRIVATE)
  set(MAYBE_DEFINE_STATIC_LIBBITCOINCONSENSUS "-DSTATIC_LIBBITCOINCONSENSUS")
else()
  set(MAYBE_DEFINE_STATIC_LIBBITCOINCONSENSUS )
endif()

configure_file(${PROJECT_SOURCE_DIR}/libbitcoinconsensus.pc.in ${PROJECT_BINARY_DIR}/libbitcoinconsensus.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/libbitcoinconsensus.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig" COMPONENT bitcoinconsensus)

install(TARGETS bitcoinconsensus
  RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT bitcoinconsensus
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT bitcoinconsensus
  ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT bitcoinconsensus
)
install(FILES ../script/bitcoinconsensus.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
