# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

include(CPackComponent)

# cpack_add_install_type(typename
#   [DISPLAY_NAME name]
# )

# cpack_add_component_group(groupname
#   [DISPLAY_NAME name]
#   [DESCRIPTION description]
#   [PARENT_GROUP parent]
#   [EXPANDED]
#   [BOLD_TITLE]
# )

# cpack_add_component(compname
#   [DISPLAY_NAME name]
#   [DESCRIPTION description]
#   [HIDDEN | REQUIRED | DISABLED ]
#   [GROUP group]
#   [DEPENDS comp1 comp2 ... ]
#   [INSTALL_TYPES type1 type2 ... ]
#   [DOWNLOADED]
#   [ARCHIVE_FILE filename]
#   [PLIST filename]
# )

cpack_add_component(bitcoin
  DISPLAY_NAME "bitcoin"
  DESCRIPTION "Bitcoin wrapper executable that can call other executables"
)

cpack_add_component(bitcoin_cli
  DISPLAY_NAME "bitcoin-cli"
  DESCRIPTION "Bitcoin JSON-RPC client"
)

cpack_add_component(bitcoin_node
  DISPLAY_NAME "bitcoin-node"
  # DESCRIPTION ""
)

cpack_add_component(bitcoin_tx
  DISPLAY_NAME "bitcoin-tx"
  DESCRIPTION "CLI Bitcoin transaction editor utility"
)

cpack_add_component(bitcoin_util
  DISPLAY_NAME "bitcoin-util"
  DESCRIPTION "CLI Bitcoin utility"
)

cpack_add_component(bitcoin_wallet
  DISPLAY_NAME "bitcoin-wallet"
  DESCRIPTION "CLI Bitcoin wallet utility"
)

cpack_add_component(bitcoind
  DISPLAY_NAME "bitcoind"
  DESCRIPTION "Bitcoin node with a JSON-RPC server"
)

set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
set(CPACK_NSIS_MUI_ICON "${BitcoinCore_SOURCE_DIR}/share/pixmaps/bitcoin.ico")
set(CPACK_NSIS_MUI_HEADERIMAGE "${BitcoinCore_SOURCE_DIR}/share/pixmaps/nsis-header.bmp")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${BitcoinCore_SOURCE_DIR}/share/pixmaps/nsis-wizard.bmp")
set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${BitcoinCore_SOURCE_DIR}/share/pixmaps/nsis-wizard.bmp")

set(CPACK_RESOURCE_FILE_LICENSE "${BitcoinCore_SOURCE_DIR}/COPYING")
set(CPACK_STRIP_FILES ON)

set(CPACK_SOURCE_IGNORE_FILES
  "/\\\\.cache/"
  "/\\\\.git/"
  "/depends/SDKs/"
  "/depends/work/"
  "/depends/built/"
  "/depends/sources/"
  "/depends/x86_64.*"
  "/depends/amd64.*"
  "/depends/i686.*"
  "/depends/mips.*"
  "/depends/arm.*"
  "/depends/aarch64.*"
  "/depends/powerpc.*"
  "/depends/riscv32.*"
  "/depends/riscv64.*"
  "/depends/s390x.*"
)

include(CPack)

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

function(add_maintenance_targets)
  if(NOT TARGET Python3::Interpreter)
    return()
  endif()

  foreach(target IN ITEMS bitcoin bitcoind bitcoin-node bitcoin-qt bitcoin-gui bitcoin-cli bitcoin-tx bitcoin-util bitcoin-wallet test_bitcoin bench_bitcoin)
    if(TARGET ${target})
      list(APPEND executables $<TARGET_FILE:${target}>)
    endif()
  endforeach()

  add_custom_target(check-symbols
    COMMAND ${CMAKE_COMMAND} -E echo "Running symbol and dynamic library checks..."
    COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/contrib/guix/symbol-check.py ${executables}
    VERBATIM
  )

  add_custom_target(check-security
    COMMAND ${CMAKE_COMMAND} -E echo "Checking binary security..."
    COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/contrib/guix/security-check.py ${executables}
    VERBATIM
  )
endfunction()

function(add_macos_deploy_target)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND TARGET bitcoin-qt)
    set(macos_app "Bitcoin-Qt.app")
    # Populate Contents subdirectory.
    configure_file(${PROJECT_SOURCE_DIR}/share/qt/Info.plist.in ${macos_app}/Contents/Info.plist NO_SOURCE_PERMISSIONS)
    file(CONFIGURE OUTPUT ${macos_app}/Contents/PkgInfo CONTENT "APPL????")
    # Populate Contents/Resources subdirectory.
    file(CONFIGURE OUTPUT ${macos_app}/Contents/Resources/empty.lproj CONTENT "")
    configure_file(${PROJECT_SOURCE_DIR}/src/qt/res/icons/bitcoin.icns ${macos_app}/Contents/Resources/bitcoin.icns NO_SOURCE_PERMISSIONS COPYONLY)
    file(CONFIGURE OUTPUT ${macos_app}/Contents/Resources/Base.lproj/InfoPlist.strings
      CONTENT "{ CFBundleDisplayName = \"@CLIENT_NAME@\"; CFBundleName = \"@CLIENT_NAME@\"; }"
    )

    add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/${macos_app}/Contents/MacOS/Bitcoin-Qt
      COMMAND ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR} --config $<CONFIG> --component bitcoin-qt --prefix ${macos_app}/Contents/MacOS --strip
      COMMAND ${CMAKE_COMMAND} -E rename ${macos_app}/Contents/MacOS/bin/$<TARGET_FILE_NAME:bitcoin-qt> ${macos_app}/Contents/MacOS/Bitcoin-Qt
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${macos_app}/Contents/MacOS/bin
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${macos_app}/Contents/MacOS/share
      VERBATIM
    )

    set(macos_zip "bitcoin-macos-app")
    if(CMAKE_HOST_APPLE)
      add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/${macos_zip}.zip
        COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/contrib/macdeploy/macdeployqtplus ${macos_app} -translations-dir=${QT_TRANSLATIONS_DIR} -zip=${macos_zip}
        DEPENDS ${PROJECT_BINARY_DIR}/${macos_app}/Contents/MacOS/Bitcoin-Qt
        VERBATIM
      )
      add_custom_target(deploydir
        DEPENDS ${PROJECT_BINARY_DIR}/${macos_zip}.zip
      )
      add_custom_target(deploy
        DEPENDS ${PROJECT_BINARY_DIR}/${macos_zip}.zip
      )
    else()
      add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/dist/${macos_app}/Contents/MacOS/Bitcoin-Qt
        COMMAND ${CMAKE_COMMAND} -E env OBJDUMP=${CMAKE_OBJDUMP} $<TARGET_FILE:Python3::Interpreter> ${PROJECT_SOURCE_DIR}/contrib/macdeploy/macdeployqtplus ${macos_app} -translations-dir=${QT_TRANSLATIONS_DIR}
        DEPENDS ${PROJECT_BINARY_DIR}/${macos_app}/Contents/MacOS/Bitcoin-Qt
        VERBATIM
      )
      add_custom_target(deploydir
        DEPENDS ${PROJECT_BINARY_DIR}/dist/${macos_app}/Contents/MacOS/Bitcoin-Qt
      )

      find_program(ZIP_EXECUTABLE zip)
      if(NOT ZIP_EXECUTABLE)
        add_custom_target(deploy
          COMMAND ${CMAKE_COMMAND} -E echo "Error: ZIP not found"
        )
      else()
        add_custom_command(
          OUTPUT ${PROJECT_BINARY_DIR}/dist/${macos_zip}.zip
          WORKING_DIRECTORY dist
          COMMAND ${PROJECT_SOURCE_DIR}/cmake/script/macos_zip.sh ${ZIP_EXECUTABLE} ${macos_zip}.zip
          VERBATIM
        )
        add_custom_target(deploy
          DEPENDS ${PROJECT_BINARY_DIR}/dist/${macos_zip}.zip
        )
      endif()
    endif()
    add_dependencies(deploydir bitcoin-qt)
    add_dependencies(deploy deploydir)
  endif()
endfunction()
