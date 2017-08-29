set (PACKAGE_INCLUDE_INSTALL_DIRS ${BSON_HEADER_INSTALL_DIR})
set (PACKAGE_LIBRARY_INSTALL_DIRS lib)
set (PACKAGE_LIBRARIES bson-1.0)

include (CMakePackageConfigHelpers)

# These aren't pkg-config files, they're CMake package configuration files.
function (install_package_config_file prefix)
   foreach (suffix "config.cmake" "config-version.cmake")
      configure_package_config_file (
         build/cmake/libbson-${prefix}-${suffix}.in
         ${CMAKE_CURRENT_BINARY_DIR}/libbson-${prefix}-${suffix}
         INSTALL_DESTINATION lib/cmake/libbson-${prefix}
         PATH_VARS PACKAGE_INCLUDE_INSTALL_DIRS PACKAGE_LIBRARY_INSTALL_DIRS
      )

      install (
         FILES
           ${CMAKE_CURRENT_BINARY_DIR}/libbson-${prefix}-${suffix}
         DESTINATION
           lib/cmake/libbson-${prefix}
      )
   endforeach ()
endfunction ()

install_package_config_file ("1.0")

if (BSON_ENABLE_STATIC)
   install_package_config_file ("static-1.0")
endif ()
