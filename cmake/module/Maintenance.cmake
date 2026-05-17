# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

function(setup_split_debug_script)
  if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(OBJCOPY ${CMAKE_OBJCOPY})
    set(STRIP ${CMAKE_STRIP})
    configure_file(
      contrib/devtools/split-debug.sh.in split-debug.sh
      FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE
                       GROUP_READ GROUP_EXECUTE
                       WORLD_READ
      @ONLY
    )
  endif()
endfunction()

function(add_windows_deploy_target)
  if(MINGW AND TARGET bitcoin AND TARGET bitcoind AND TARGET bitcoin-cli AND TARGET test_bitcoin)
    find_program(MAKENSIS_EXECUTABLE makensis)
    if(NOT MAKENSIS_EXECUTABLE)
      add_custom_target(deploy
        COMMAND ${CMAKE_COMMAND} -E echo "Error: NSIS not found"
      )
      return()
    endif()

    # TODO: Consider replacing this code with the CPack NSIS Generator.
    #       See https://cmake.org/cmake/help/latest/cpack_gen/nsis.html
    include(GenerateSetupNsi)
    generate_setup_nsi()
    add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/bitcoin-win64-setup.exe
      COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/release
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:bitcoin> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin>
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:bitcoind> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoind>
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:bitcoin-cli> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-cli>
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:test_bitcoin> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:test_bitcoin>
      COMMAND ${MAKENSIS_EXECUTABLE} -V2 ${PROJECT_BINARY_DIR}/bitcoin-win64-setup.nsi
      VERBATIM
    )
    add_custom_target(deploy DEPENDS ${PROJECT_BINARY_DIR}/bitcoin-win64-setup.exe)
  endif()
endfunction()

function(add_macos_deploy_target)
endfunction()
