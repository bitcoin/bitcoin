# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(add_boost_if_needed)
  #[=[
  TODO: Not all targets, which will be added in the future, require
        Boost. Therefore, a proper check will be appropriate here.

  Implementation notes:
  Although only Boost headers are used to build Bitcoin Core,
  we still leverage a standard CMake's approach to handle
  dependencies, i.e., the Boost::headers "library".
  A command target_link_libraries(target PRIVATE Boost::headers)
  will propagate Boost::headers usage requirements to the target.
  For Boost::headers such usage requirements is an include
  directory and other added INTERFACE properties.
  ]=]

  if(CMAKE_HOST_APPLE)
    find_program(HOMEBREW_EXECUTABLE brew)
    if(HOMEBREW_EXECUTABLE)
      execute_process(
        COMMAND ${HOMEBREW_EXECUTABLE} --prefix boost
        OUTPUT_VARIABLE Boost_ROOT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    endif()
  endif()

  find_package(Boost 1.73.0 REQUIRED CONFIG)
  mark_as_advanced(Boost_INCLUDE_DIR boost_headers_DIR)
  # Workaround for a bug in NetBSD pkgsrc.
  # See: https://github.com/NetBSD/pkgsrc/issues/167.
  if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    get_filename_component(_boost_include_dir "${boost_headers_DIR}/../../../include/" ABSOLUTE)
    set_target_properties(Boost::headers PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${_boost_include_dir}
    )
    unset(_boost_include_dir)
  endif()
  set_target_properties(Boost::headers PROPERTIES IMPORTED_GLOBAL TRUE)
  target_compile_definitions(Boost::headers INTERFACE
    # We don't use multi_index serialization.
    BOOST_MULTI_INDEX_DISABLE_SERIALIZATION
  )
  if(DEFINED VCPKG_TARGET_TRIPLET)
    # Workaround for https://github.com/microsoft/vcpkg/issues/36955.
    target_compile_definitions(Boost::headers INTERFACE
      BOOST_NO_USER_CONFIG
    )
  endif()

  # Prevent use of std::unary_function, which was removed in C++17,
  # and will generate warnings with newer compilers for Boost
  # older than 1.80.
  # See: https://github.com/boostorg/config/pull/430.
  set(CMAKE_REQUIRED_DEFINITIONS -DBOOST_NO_CXX98_FUNCTION_BASE)
  get_target_property(CMAKE_REQUIRED_INCLUDES Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
  set(CMAKE_REQUIRED_FLAGS ${working_compiler_werror_flag})
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("
    #include <boost/config.hpp>
    " NO_DIAGNOSTICS_BOOST_NO_CXX98_FUNCTION_BASE
  )
  if(NO_DIAGNOSTICS_BOOST_NO_CXX98_FUNCTION_BASE)
    target_compile_definitions(Boost::headers INTERFACE
      BOOST_NO_CXX98_FUNCTION_BASE
    )
  endif()

  # Some package managers, such as vcpkg, vendor Boost.Test separately
  # from the rest of the headers, so we have to check for it individually.
  if(BUILD_TESTS AND DEFINED VCPKG_TARGET_TRIPLET)
    find_package(boost_included_unit_test_framework ${Boost_VERSION} EXACT REQUIRED CONFIG)
  endif()

endfunction()
