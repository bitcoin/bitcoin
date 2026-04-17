# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

function(add_windows_resources target rc_file)
  if(WIN32)
    target_sources(${target} PRIVATE ${rc_file})
  endif()
endfunction()

# Add version resources and a fusion manifest to a Windows executable.
# RC_TARGET is set to the target name; RC_DESCRIPTION is the full FileDescription
# string. ICON_DIR, if provided, adds icon resources and sets ProductName to the
# project name (for GUI executables).
function(add_windows_exe_resources target description)
  if(WIN32)
    cmake_parse_arguments(PARSE_ARGV 2 arg "" "ICON_DIR" "")
    set(RC_TARGET "${target}")
    set(RC_DESCRIPTION "${description}")
    set(RC_PRODUCT_NAME "${target}")
    set(RC_ICONS "")
    if(arg_ICON_DIR)
      set(RC_PRODUCT_NAME "Bitcoin")
      string(CONCAT RC_ICONS
        "IDI_ICON1 ICON DISCARDABLE \"${arg_ICON_DIR}/bitcoin.ico\"\n"
        "IDI_ICON2 ICON DISCARDABLE \"${arg_ICON_DIR}/bitcoin_testnet.ico\"\n"
        "IDI_ICON3 ICON DISCARDABLE \"${arg_ICON_DIR}/bitcoin_signet.ico\"\n"
        "IDI_ICON4 ICON DISCARDABLE \"${arg_ICON_DIR}/bitcoin_testnet.ico\" // testnet4\n"
        "\n"
      )
    endif()
    set(RC_CLIENTVERSION_H "${PROJECT_SOURCE_DIR}/src/clientversion.h")
    configure_file(
      ${PROJECT_SOURCE_DIR}/cmake/windows-res.rc.in
      ${CMAKE_CURRENT_BINARY_DIR}/${target}-res.rc
      @ONLY
    )
    add_windows_resources(${target} ${CMAKE_CURRENT_BINARY_DIR}/${target}-res.rc)
    add_windows_application_manifest(${target})
  endif()
endfunction()

# Add a fusion manifest to Windows executables.
# See: https://learn.microsoft.com/en-us/windows/win32/sbscs/application-manifests
function(add_windows_application_manifest target)
  if(WIN32)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/windows-app.manifest.in ${target}.manifest USE_SOURCE_PERMISSIONS)
    file(CONFIGURE
      OUTPUT ${target}-manifest.rc
      CONTENT "1 /* CREATEPROCESS_MANIFEST_RESOURCE_ID */ 24 /* RT_MANIFEST */ \"${target}.manifest\""
    )
    add_windows_resources(${target} ${CMAKE_CURRENT_BINARY_DIR}/${target}-manifest.rc)
  endif()
endfunction()
