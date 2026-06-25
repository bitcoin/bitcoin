# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

#[=======================================================================[
FindQRencode
------------

Finds the QRencode header and library.

This is a wrapper around find_package()/pkg_check_modules() commands that:
 - facilitates searching in various build environments
 - prints a standard log message

#]=======================================================================]

find_path(QRencode_INCLUDE_DIR
  NAMES qrencode.h
)
find_library(QRencode_LIBRARY_RELEASE
  NAMES qrencode
)
find_library(QRencode_LIBRARY_DEBUG
  NAMES qrencoded qrencode
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QRencode
  REQUIRED_VARS QRencode_LIBRARY_RELEASE QRencode_INCLUDE_DIR
)

if(QRencode_FOUND AND NOT TARGET QRencode::QRencode)
  add_library(QRencode::QRencode UNKNOWN IMPORTED)
  set_target_properties(QRencode::QRencode PROPERTIES
    IMPORTED_CONFIGURATIONS RELEASE
    IMPORTED_LOCATION_RELEASE "${QRencode_LIBRARY_RELEASE}"
    INTERFACE_INCLUDE_DIRECTORIES "${QRencode_INCLUDE_DIR}"
  )
  if(QRencode_LIBRARY_DEBUG)
    set_property(TARGET QRencode::QRencode APPEND PROPERTY
      IMPORTED_CONFIGURATIONS DEBUG
    )
    set_target_properties(QRencode::QRencode PROPERTIES
      IMPORTED_LOCATION_DEBUG "${QRencode_LIBRARY_DEBUG}"
    )
  endif()
endif()

mark_as_advanced(
  QRencode_INCLUDE_DIR
  QRencode_LIBRARY_RELEASE
  QRencode_LIBRARY_DEBUG
)
