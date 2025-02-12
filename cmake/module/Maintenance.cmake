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

function(add_maintenance_targets)
  if(NOT TARGET Python3::Interpreter)
    return()
  endif()

  foreach(target IN ITEMS bitcoin bitcoind bitcoin-qt bitcoin-cli bitcoin-tx bitcoin-util bitcoin-wallet test_bitcoin bench_bitcoin)
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

function(add_windows_deploy_target)
  if(MINGW AND TARGET bitcoin AND TARGET bitcoin-qt AND TARGET bitcoind AND TARGET bitcoin-cli AND TARGET bitcoin-tx AND TARGET bitcoin-wallet AND TARGET bitcoin-util AND TARGET test_bitcoin)
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
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:bitcoin-qt> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-qt>
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:bitcoind> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoind>
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:bitcoin-cli> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-cli>
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:bitcoin-tx> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-tx>
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:bitcoin-wallet> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-wallet>
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:bitcoin-util> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:bitcoin-util>
      COMMAND ${CMAKE_STRIP} $<TARGET_FILE:test_bitcoin> -o ${PROJECT_BINARY_DIR}/release/$<TARGET_FILE_NAME:test_bitcoin>
      COMMAND ${MAKENSIS_EXECUTABLE} -V2 ${PROJECT_BINARY_DIR}/bitcoin-win64-setup.nsi
      VERBATIM
    )
    add_custom_target(deploy DEPENDS ${PROJECT_BINARY_DIR}/bitcoin-win64-setup.exe)
  endif()
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

    string(REPLACE " " "-" osx_volname ${CLIENT_NAME})
    if(CMAKE_HOST_APPLE)
      add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/${osx_volname}.zip
        COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/contrib/macdeploy/macdeployqtplus ${macos_app} ${osx_volname} -translations-dir=${QT_TRANSLATIONS_DIR} -zip
        DEPENDS ${PROJECT_BINARY_DIR}/${macos_app}/Contents/MacOS/Bitcoin-Qt
        VERBATIM
      )
      add_custom_target(deploydir
        DEPENDS ${PROJECT_BINARY_DIR}/${osx_volname}.zip
      )
      add_custom_target(deploy
        DEPENDS ${PROJECT_BINARY_DIR}/${osx_volname}.zip
      )
    else()
      add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/dist/${macos_app}/Contents/MacOS/Bitcoin-Qt
        COMMAND ${CMAKE_COMMAND} -E env OBJDUMP=${CMAKE_OBJDUMP} $<TARGET_FILE:Python3::Interpreter> ${PROJECT_SOURCE_DIR}/contrib/macdeploy/macdeployqtplus ${macos_app} ${osx_volname} -translations-dir=${QT_TRANSLATIONS_DIR}
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
          OUTPUT ${PROJECT_BINARY_DIR}/dist/${osx_volname}.zip
          WORKING_DIRECTORY dist
          COMMAND ${PROJECT_SOURCE_DIR}/cmake/script/macos_zip.sh ${ZIP_EXECUTABLE} ${osx_volname}.zip
          VERBATIM
        )
        add_custom_target(deploy
          DEPENDS ${PROJECT_BINARY_DIR}/dist/${osx_volname}.zip
        )
      endif()
    endif()
    add_dependencies(deploydir bitcoin-qt)
    add_dependencies(deploy deploydir)
  endif()
endfunction()
