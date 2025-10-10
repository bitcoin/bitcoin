# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  enable_language(OBJCXX)
  set(CMAKE_OBJCXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  set(CMAKE_OBJCXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  set(CMAKE_OBJCXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  set(CMAKE_OBJCXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
  string(APPEND CMAKE_OBJCXX_COMPILE_OBJECT " ${APPEND_CPPFLAGS} ${APPEND_CXXFLAGS}")
endif()

get_target_property(qt_lib_type "Qt${WITH_QT_VERSION}::Core" TYPE)

function(import_plugins target)
  if(qt_lib_type STREQUAL "STATIC_LIBRARY")
    set(plugins "Qt${WITH_QT_VERSION}::QMinimalIntegrationPlugin")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      list(APPEND plugins "Qt${WITH_QT_VERSION}::QXcbIntegrationPlugin")
    elseif(WIN32)
      if("${WITH_QT_VERSION}" STREQUAL "5")
      list(APPEND plugins Qt5::QWindowsIntegrationPlugin Qt5::QWindowsVistaStylePlugin)
      else()  # Qt 6
        list(APPEND plugins Qt6::QWindowsIntegrationPlugin Qt6::QModernWindowsStylePlugin)
      endif()  # Qt version
    elseif(APPLE)
      list(APPEND plugins "Qt${WITH_QT_VERSION}::QCocoaIntegrationPlugin" "Qt${WITH_QT_VERSION}::QMacStylePlugin")
    endif()
    cmake_language(CALL "qt${WITH_QT_VERSION}_import_plugins" ${target}
      INCLUDE ${plugins}
      EXCLUDE_BY_TYPE
        accessiblebridge
        platforms
        platforms_darwin
        xcbglintegrations
        platformthemes
        platforminputcontexts
        generic
        iconengines
        imageformats
        egldeviceintegrations
        styles
        networkaccess
        networkinformation
        tls
    )
  endif()
endfunction()

# For Qt-specific commands and variables, please consult:
#  - https://cmake.org/cmake/help/latest/manual/cmake-qt.7.html
#  - https://doc.qt.io/qt-5/cmake-manual.html

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_MOC_OPTIONS "-p${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS forms)

# TODO: The file(GLOB ...) command should be replaced with an explicit
# file list. Such a change must be synced with the corresponding change
# to https://github.com/bitcoin-core/bitcoin-maintainer-tools/blob/main/update-translations.py
file(GLOB ts_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} locale/*.ts)
set_source_files_properties(${ts_files} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/locale)
cmake_language(CALL "qt${WITH_QT_VERSION}_add_translation" qm_files ${ts_files})

set(FONT_PATH "res/fonts/RobotoTonalMono-Bold.ttf")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/res/fonts")
add_custom_command(
  OUTPUT ${FONT_PATH}
  COMMAND ${PYTHON_COMMAND} -c "import base64, lzma, sys; sys.stdout.buffer.write(lzma.decompress(base64.b85decode(sys.stdin.read())))" < "${CMAKE_CURRENT_SOURCE_DIR}/${FONT_PATH}.in" > "${FONT_PATH}.new" && mv "${FONT_PATH}.new" "${FONT_PATH}"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FONT_PATH}.in
  VERBATIM
)

set(ICON_PATH "res/rendered_icons")
set(ICO_SPECS
  256 64 48 32 24 20 16
)
set(ICNS_SPECS
  1024 512 256 128 32 16
)
set(EXTRA_ICON_SPECS
  1024 290
)

set(ICON_SPECS ${ICO_SPECS} ${ICNS_SPECS} ${EXTRA_ICON_SPECS})
list(REMOVE_DUPLICATES ICON_SPECS)
set(bitcoin_svg "res/src/bitcoin.svg")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${ICON_PATH}")
foreach(size IN LISTS ICON_SPECS)
  set(png "${ICON_PATH}/bitcoin${size}.png")
  add_custom_command(
    OUTPUT ${png}
    COMMAND "${RSVG_CONVERT}" -f png -d ${size} -p ${size} --output "${png}" "${CMAKE_CURRENT_SOURCE_DIR}/${bitcoin_svg}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${bitcoin_svg}"
    COMMENT "Generating ${png} from ${bitcoin_svg}"
    VERBATIM
  )

  # NOTE: ImageMagick will never convert transparent PNGs to 8-bit ICOs, but GIF is fine
  set(gif "${ICON_PATH}/bitcoin${size}d8.gif")
  add_custom_command(
    OUTPUT ${gif}
    COMMAND "${IMAGEMAGICK_CONVERT}" "${png}" -colors 256 -channel A -threshold 50% "${gif}"
    DEPENDS "${png}"
    COMMENT "Generating ${gif} from ${png}"
    VERBATIM
  )
endforeach()

set(bitcoin_icns "${ICON_PATH}/bitcoin.icns")
set(ICNS_SRC_FILES ${ICNS_SPECS})
list(TRANSFORM ICNS_SRC_FILES PREPEND "${ICON_PATH}/bitcoin")
list(TRANSFORM ICNS_SRC_FILES APPEND ".png")
add_custom_command(
  OUTPUT "${bitcoin_icns}"
  COMMAND "${PNG2ICNS}" "${bitcoin_icns}" ${ICNS_SRC_FILES}
  DEPENDS ${ICNS_SRC_FILES}
  COMMENT "Generating ${bitcoin_icns} from individual images"
  VERBATIM
)
add_custom_target(generate_icns DEPENDS "${bitcoin_icns}")

set(bitcoin_ico "${ICON_PATH}/bitcoin.ico")
set(ICO_SRC_FILES ${ICO_SPECS})
list(TRANSFORM ICO_SRC_FILES PREPEND "${ICON_PATH}/bitcoin")
list(TRANSFORM ICO_SRC_FILES APPEND ".png")
list(PREPEND ICO_SRC_FILES "${ICON_PATH}/bitcoin32d8.gif")
add_custom_command(
  OUTPUT "${bitcoin_ico}"
  COMMAND "${IMAGEMAGICK_CONVERT}" ${ICO_SRC_FILES} "${bitcoin_ico}"
  DEPENDS ${ICO_SRC_FILES}
  COMMENT "Generating ${bitcoin_ico} from individual images"
  VERBATIM
)
set(testnet_ico "${ICON_PATH}/bitcoin_testnet.ico")
add_custom_command(
  OUTPUT "${testnet_ico}"
  COMMAND "${IMAGEMAGICK_CONVERT}" ${bitcoin_ico} -modulate 100,100,138.9 -colorspace HSL -channel G -evaluate subtract "11.76%" +channel -colorspace sRGB "${testnet_ico}"
  DEPENDS ${bitcoin_ico}
  COMMENT "Generating ${testnet_ico} from ${bitcoin_ico}"
  VERBATIM
)
add_custom_target(generate_ico DEPENDS "${bitcoin_ico}" "${testnet_ico}")

set(nsis_wizard_input "${ICON_PATH}/bitcoin290.png")
set(nsis_wizard_bmp "${ICON_PATH}/nsis-wizard.bmp")
add_custom_command(
  OUTPUT "${nsis_wizard_bmp}"
  COMMAND "${IMAGEMAGICK_CONVERT}" ${nsis_wizard_input} -crop 164x290+62+0 -border 0x12 -colorspace HSL -channel Saturation -separate -negate -auto-level -strip BMP3:${nsis_wizard_bmp}
  DEPENDS ${nsis_wizard_input}
  COMMENT "Generating ${nsis_wizard_bmp} from ${nsis_wizard_input}"
  VERBATIM
)
set(nsis_header_input "res/src/nsis-header.svg")
set(nsis_header_bmp "${ICON_PATH}/nsis-header.bmp")
add_custom_command(
  OUTPUT "${nsis_header_bmp}"
  COMMAND sed s/CLIENT_NAME/${CLIENT_NAME}/ < ${CMAKE_CURRENT_SOURCE_DIR}/${nsis_header_input} | ${RSVG_CONVERT} -f png -d 57 -p 57 | ${IMAGEMAGICK_CONVERT} - -background white -alpha remove BMP3:${nsis_header_bmp}
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${nsis_header_input}"
  COMMENT "Generating ${nsis_header_bmp} from ${nsis_header_input}"
  VERBATIM
)
add_custom_target(generate_nsis_images DEPENDS "${nsis_wizard_bmp}" "${nsis_header_bmp}")

configure_file(bitcoin_locale.qrc bitcoin_locale.qrc USE_SOURCE_PERMISSIONS COPYONLY)
configure_file(bitcoin_rendered.qrc bitcoin_rendered.qrc USE_SOURCE_PERMISSIONS COPYONLY)
configure_file(font.qrc font.qrc USE_SOURCE_PERMISSIONS COPYONLY)

# The bitcoinqt sources have to include headers in
# order to parse them to collect translatable strings.
add_library(bitcoinqt STATIC EXCLUDE_FROM_ALL
  bantablemodel.cpp
  bantablemodel.h
  bitcoin.cpp
  bitcoin.h
  bitcoinaddressvalidator.cpp
  bitcoinaddressvalidator.h
  bitcoinamountfield.cpp
  bitcoinamountfield.h
  bitcoingui.cpp
  bitcoingui.h
  bitcoinunits.cpp
  bitcoinunits.h
  blockview.cpp
  blockview.h
  clientmodel.cpp
  clientmodel.h
  csvmodelwriter.cpp
  csvmodelwriter.h
  guiutil.cpp
  guiutil.h
  initexecutor.cpp
  initexecutor.h
  intro.cpp
  intro.h
  $<$<PLATFORM_ID:Darwin>:macdockiconhandler.h>
  $<$<PLATFORM_ID:Darwin>:macdockiconhandler.mm>
  $<$<PLATFORM_ID:Darwin>:macnotificationhandler.h>
  $<$<PLATFORM_ID:Darwin>:macnotificationhandler.mm>
  $<$<PLATFORM_ID:Darwin>:macos_appnap.h>
  $<$<PLATFORM_ID:Darwin>:macos_appnap.mm>
  modaloverlay.cpp
  mempoolstats.cpp
  modaloverlay.h
  netwatch.cpp
  networkstyle.cpp
  networkstyle.h
  notificator.cpp
  notificator.h
  optionsdialog.cpp
  optionsdialog.h
  optionsmodel.cpp
  optionsmodel.h
  pairingpage.cpp
  pairingpage.h
  peertablemodel.cpp
  peertablemodel.h
  peertablesortproxy.cpp
  peertablesortproxy.h
  platformstyle.cpp
  platformstyle.h
  qrimagewidget.cpp
  qrimagewidget.h
  qvalidatedlineedit.cpp
  qvalidatedlineedit.h
  qvaluecombobox.cpp
  qvaluecombobox.h
  rpcconsole.cpp
  rpcconsole.h
  splashscreen.cpp
  splashscreen.h
  tonalutils.cpp
  tonalutils.h
  trafficgraphwidget.cpp
  trafficgraphwidget.h
  utilitydialog.cpp
  utilitydialog.h
  $<$<PLATFORM_ID:Windows>:winshutdownmonitor.cpp>
  $<$<PLATFORM_ID:Windows>:winshutdownmonitor.h>
  bitcoin.qrc
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin_locale.qrc
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin_rendered.qrc
  ${CMAKE_CURRENT_BINARY_DIR}/font.qrc
)
target_compile_definitions(bitcoinqt
  PUBLIC
    QT_NO_KEYWORDS
    QT_USE_QSTRINGBUILDER
)
target_include_directories(bitcoinqt
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
set_property(SOURCE macnotificationhandler.mm
  # Ignore warnings "'NSUserNotificationCenter' is deprecated: first deprecated in macOS 11.0".
  APPEND PROPERTY COMPILE_OPTIONS -Wno-deprecated-declarations
)
target_link_libraries(bitcoinqt
  PUBLIC
    "Qt${WITH_QT_VERSION}::Widgets"
  PRIVATE
    core_interface
    bitcoin_cli
    leveldb
    Boost::headers
    $<TARGET_NAME_IF_EXISTS:MiniUPnPc::MiniUPnPc>
    $<TARGET_NAME_IF_EXISTS:QRencode::QRencode>
    $<$<PLATFORM_ID:Darwin>:-framework\ AppKit>
    $<$<CXX_COMPILER_ID:MSVC>:shlwapi>
)

if(ENABLE_WALLET)
  target_sources(bitcoinqt
    PRIVATE
      addressbookpage.cpp
      addressbookpage.h
      addresstablemodel.cpp
      addresstablemodel.h
      askpassphrasedialog.cpp
      askpassphrasedialog.h
      coincontroldialog.cpp
      coincontroldialog.h
      coincontroltreewidget.cpp
      coincontroltreewidget.h
      createwalletdialog.cpp
      createwalletdialog.h
      editaddressdialog.cpp
      editaddressdialog.h
      openuridialog.cpp
      openuridialog.h
      overviewpage.cpp
      overviewpage.h
      paymentserver.cpp
      paymentserver.h
      psbtoperationsdialog.cpp
      psbtoperationsdialog.h
      qrimagewidget.cpp
      qrimagewidget.h
      receivecoinsdialog.cpp
      receivecoinsdialog.h
      receiverequestdialog.cpp
      receiverequestdialog.h
      recentrequeststablemodel.cpp
      recentrequeststablemodel.h
      sendcoinsdialog.cpp
      sendcoinsdialog.h
      sendcoinsentry.cpp
      sendcoinsentry.h
      signverifymessagedialog.cpp
      signverifymessagedialog.h
      transactiondesc.cpp
      transactiondesc.h
      transactiondescdialog.cpp
      transactiondescdialog.h
      transactionfilterproxy.cpp
      transactionfilterproxy.h
      transactionoverviewwidget.cpp
      transactionoverviewwidget.h
      transactionrecord.cpp
      transactionrecord.h
      transactiontablemodel.cpp
      transactiontablemodel.h
      transactionview.cpp
      transactionview.h
      walletcontroller.cpp
      walletcontroller.h
      walletframe.cpp
      walletframe.h
      walletmodel.cpp
      walletmodel.h
      walletmodeltransaction.cpp
      walletmodeltransaction.h
      walletview.cpp
      walletview.h
  )
  target_link_libraries(bitcoinqt
    PRIVATE
      bitcoin_wallet
      "Qt${WITH_QT_VERSION}::Network"
  )
endif()

if(WITH_DBUS)
  target_link_libraries(bitcoinqt PRIVATE "Qt${WITH_QT_VERSION}::DBus")
endif()
if(WITH_TASKBAR_PROGRESS)
  target_link_libraries(bitcoinqt
    PRIVATE
      "Qt${WITH_QT_VERSION}::WinExtras"
      $<$<PLATFORM_ID:Windows>:dwmapi>
  )
endif()

if(qt_lib_type STREQUAL "STATIC_LIBRARY")
  # We want to define static plugins to link ourselves, thus preventing
  # automatic linking against a "sane" set of default static plugins.
  cmake_language(CALL "qt${WITH_QT_VERSION}_import_plugins" bitcoinqt
    EXCLUDE_BY_TYPE
      bearer
      accessiblebridge
      platforms
      platforms_darwin
      xcbglintegrations
      platformthemes
      platforminputcontexts
      generic
      iconengines
      imageformats
      egldeviceintegrations
      styles
      networkaccess
      networkinformation
      tls
  )
endif()

add_executable(bitcoin-qt
  main.cpp
  ../init/bitcoin-qt.cpp
)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  add_dependencies(bitcoin-qt generate_ico)
endif()

if(WIN32)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/bitcoin-qt-res.rc ${CMAKE_CURRENT_BINARY_DIR}/res/bitcoin-qt-res.rc USE_SOURCE_PERMISSIONS @ONLY)
  add_windows_resources(bitcoin-qt ${CMAKE_CURRENT_BINARY_DIR}/res/bitcoin-qt-res.rc)
endif()

target_link_libraries(bitcoin-qt
  core_interface
  bitcoinqt
  bitcoin_node
)

import_plugins(bitcoin-qt)
install_binary_component(bitcoin-qt HAS_MANPAGE)
if(WIN32)
  set_target_properties(bitcoin-qt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(WITH_MULTIPROCESS)
  add_executable(bitcoin-gui
    main.cpp
    ../init/bitcoin-gui.cpp
  )
  target_link_libraries(bitcoin-gui
    core_interface
    bitcoinqt
    bitcoin_node
    bitcoin_ipc
  )
  import_plugins(bitcoin-gui)
  install_binary_component(bitcoin-gui)
  if(WIN32)
    set_target_properties(bitcoin-gui PROPERTIES WIN32_EXECUTABLE TRUE)
  endif()
endif()

if(BUILD_GUI_TESTS)
  add_subdirectory(test)
endif()


# Gets sources to be parsed to gather translatable strings.
function(get_translatable_sources var)
  set(result)
  set(targets)
  foreach(dir IN ITEMS ${ARGN})
    get_directory_property(dir_targets DIRECTORY ${PROJECT_SOURCE_DIR}/${dir} BUILDSYSTEM_TARGETS)
    list(APPEND targets ${dir_targets})
  endforeach()
  foreach(target IN LISTS targets)
    get_target_property(target_sources ${target} SOURCES)
    if(target_sources)
      foreach(source IN LISTS target_sources)
        # Get an expression from the generator expression, if any.
        if(source MATCHES ":([^>]+)>$")
          set(source ${CMAKE_MATCH_1})
        endif()
        cmake_path(GET source EXTENSION LAST_ONLY ext)
        if(ext STREQUAL ".qrc")
          continue()
        endif()
        if(NOT IS_ABSOLUTE source)
          get_target_property(target_source_dir ${target} SOURCE_DIR)
          cmake_path(APPEND target_source_dir ${source} OUTPUT_VARIABLE source)
        endif()
        get_property(is_generated
          SOURCE  ${source} TARGET_DIRECTORY ${target}
          PROPERTY GENERATED
        )
        if(NOT is_generated)
          list(APPEND result ${source})
        endif()
      endforeach()
    endif()
  endforeach()
  set(${var} ${result} PARENT_SCOPE)
endfunction()

find_program(XGETTEXT_EXECUTABLE xgettext)
find_program(SED_EXECUTABLE sed)
if(NOT XGETTEXT_EXECUTABLE)
  add_custom_target(translate
    COMMAND ${CMAKE_COMMAND} -E echo "Error: GNU gettext-tools not found"
  )
elseif(NOT SED_EXECUTABLE)
  add_custom_target(translate
    COMMAND ${CMAKE_COMMAND} -E echo "Error: GNU sed not found"
  )
else()
  set(translatable_sources_directories src src/qt src/util)
  if(ENABLE_WALLET)
    list(APPEND translatable_sources_directories src/wallet)
  endif()
  get_translatable_sources(translatable_sources ${translatable_sources_directories})
  get_translatable_sources(qt_translatable_sources src/qt)
  file(GLOB ui_files ${CMAKE_CURRENT_SOURCE_DIR}/forms/*.ui)
  add_custom_target(translate
    COMMAND ${CMAKE_COMMAND} -E env XGETTEXT=${XGETTEXT_EXECUTABLE} COPYRIGHT_HOLDERS=${COPYRIGHT_HOLDERS} ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/share/qt/extract_strings_qt.py ${translatable_sources}
    COMMAND "Qt${WITH_QT_VERSION}::lupdate" -no-obsolete -I ${PROJECT_SOURCE_DIR}/src -locations relative ${CMAKE_CURRENT_SOURCE_DIR}/bitcoinstrings.cpp ${ui_files} ${qt_translatable_sources} -ts ${CMAKE_CURRENT_SOURCE_DIR}/locale/bitcoin_en.ts
    COMMAND "Qt${WITH_QT_VERSION}::lconvert" -drop-translations -o ${CMAKE_CURRENT_SOURCE_DIR}/locale/bitcoin_en.xlf -i ${CMAKE_CURRENT_SOURCE_DIR}/locale/bitcoin_en.ts
    COMMAND ${SED_EXECUTABLE} -i.old -e "s|source-language=\"en\" target-language=\"en\"|source-language=\"en\"|" -e "/<target xml:space=\"preserve\"><\\/target>/d" ${CMAKE_CURRENT_SOURCE_DIR}/locale/bitcoin_en.xlf
    COMMAND ${CMAKE_COMMAND} -E rm ${CMAKE_CURRENT_SOURCE_DIR}/locale/bitcoin_en.xlf.old
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src
    VERBATIM
  )
endif()
