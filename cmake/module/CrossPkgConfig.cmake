# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

find_package(PkgConfig REQUIRED)

function(remove_isystem_from_include_directories_internal target)
  get_target_property(include_directories ${target} INTERFACE_INCLUDE_DIRECTORIES)
  if(include_directories)
    list(REMOVE_ITEM include_directories -isystem)
    set_target_properties(${target} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${include_directories}")
  endif()
endfunction()

macro(cross_pkg_check_modules prefix)
  if(CMAKE_CROSSCOMPILING)
    set(pkg_config_path_saved "$ENV{PKG_CONFIG_PATH}")
    set(pkg_config_libdir_saved "$ENV{PKG_CONFIG_LIBDIR}")
    set(ENV{PKG_CONFIG_PATH} ${PKG_CONFIG_PATH})
    set(ENV{PKG_CONFIG_LIBDIR} ${PKG_CONFIG_LIBDIR})
    pkg_check_modules(${prefix} ${ARGN})
    set(ENV{PKG_CONFIG_PATH} ${pkg_config_path_saved})
    set(ENV{PKG_CONFIG_LIBDIR} ${pkg_config_libdir_saved})
  else()
    pkg_check_modules(${prefix} ${ARGN})
  endif()

  # A workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/20652.
  if(CMAKE_VERSION VERSION_LESS 3.17.3 AND TARGET PkgConfig::${prefix})
    remove_isystem_from_include_directories_internal(PkgConfig::${prefix})
  endif()
endmacro()
