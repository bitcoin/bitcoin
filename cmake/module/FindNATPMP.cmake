# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

find_path(NATPMP_INCLUDE_DIR
  NAMES natpmp.h
)

find_library(NATPMP_LIBRARY
  NAMES natpmp
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NATPMP
  REQUIRED_VARS NATPMP_LIBRARY NATPMP_INCLUDE_DIR
)

if(NATPMP_FOUND AND NOT TARGET NATPMP::NATPMP)
  add_library(NATPMP::NATPMP UNKNOWN IMPORTED)
  set_target_properties(NATPMP::NATPMP PROPERTIES
    IMPORTED_LOCATION "${NATPMP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${NATPMP_INCLUDE_DIR}"
  )
  set_property(TARGET NATPMP::NATPMP PROPERTY
    INTERFACE_COMPILE_DEFINITIONS USE_NATPMP=1 $<$<PLATFORM_ID:Windows>:NATPMP_STATICLIB>
  )
endif()

mark_as_advanced(
  NATPMP_INCLUDE_DIR
  NATPMP_LIBRARY
)
