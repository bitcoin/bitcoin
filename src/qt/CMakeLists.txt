# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# See:
#  - https://cmake.org/cmake/help/latest/manual/cmake-qt.7.html
#  - https://doc.qt.io/qt-5/cmake-manual.html

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms)

set(qt_minimum_required_version 5.11.3)
set(qt_package ${WITH_GUI})

set(qt_components Core Gui Widgets Network LinguistTools)
find_package(${qt_package} ${qt_minimum_required_version} REQUIRED
  COMPONENTS ${qt_components}
  HINTS ${qt5_brew_prefix}
  PATH_SUFFIXES Qt5 # Required on OpenBSD systems.
)
message(STATUS "Found Qt: ${${qt_package}_DIR} (found suitable version \"${${qt_package}_VERSION}\", minimum required is \"${qt_minimum_required_version}\")")

file(GLOB ts_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} locale/*.ts)
set_source_files_properties(${ts_files} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/locale)
qt5_add_translation(qm_files ${ts_files})

add_compile_definitions(QT_NO_KEYWORDS QT_USE_QSTRINGBUILDER)

add_library(bitcoinqt STATIC EXCLUDE_FROM_ALL
  bantablemodel.cpp
  bitcoin.cpp
  bitcoin.qrc
  bitcoin_locale.qrc
  bitcoinaddressvalidator.cpp
  bitcoinamountfield.cpp
  bitcoingui.cpp
  bitcoinunits.cpp
  clientmodel.cpp
  csvmodelwriter.cpp
  guiutil.cpp
  initexecutor.cpp
  intro.cpp
  modaloverlay.cpp
  networkstyle.cpp
  notificator.cpp
  optionsdialog.cpp
  optionsmodel.cpp
  peertablemodel.cpp
  peertablesortproxy.cpp
  platformstyle.cpp
  qvalidatedlineedit.cpp
  qvaluecombobox.cpp
  rpcconsole.cpp
  splashscreen.cpp
  trafficgraphwidget.cpp
  utilitydialog.cpp
  $<$<PLATFORM_ID:Windows>:winshutdownmonitor.cpp>
  $<$<PLATFORM_ID:Darwin>:macdockiconhandler.mm>
  $<$<PLATFORM_ID:Darwin>:macnotificationhandler.mm>
  $<$<PLATFORM_ID:Darwin>:macos_appnap.mm>
)
target_include_directories(bitcoinqt
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)
target_link_libraries(bitcoinqt
  PUBLIC
    ${qt_package}::Widgets
  PRIVATE
    bitcoin_cli
    leveldb
    Boost::headers
    $<TARGET_NAME_IF_EXISTS:NATPMP::NATPMP>
    $<TARGET_NAME_IF_EXISTS:MiniUPnPc::MiniUPnPc>
    $<TARGET_NAME_IF_EXISTS:PkgConfig::libqrencode>
    $<$<CXX_COMPILER_ID:MSVC>:shlwapi>
)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_link_libraries(bitcoinqt PRIVATE "-framework AppKit")
  target_compile_options(bitcoinqt PRIVATE "-Wno-error=deprecated-declarations")
endif()

if(ENABLE_WALLET)
  target_sources(bitcoinqt
    PRIVATE
      addressbookpage.cpp
      addresstablemodel.cpp
      askpassphrasedialog.cpp
      coincontroldialog.cpp
      coincontroltreewidget.cpp
      createwalletdialog.cpp
      editaddressdialog.cpp
      openuridialog.cpp
      overviewpage.cpp
      paymentserver.cpp
      psbtoperationsdialog.cpp
      qrimagewidget.cpp
      receivecoinsdialog.cpp
      receiverequestdialog.cpp
      recentrequeststablemodel.cpp
      sendcoinsdialog.cpp
      sendcoinsentry.cpp
      signverifymessagedialog.cpp
      transactiondesc.cpp
      transactiondescdialog.cpp
      transactionfilterproxy.cpp
      transactionoverviewwidget.cpp
      transactionrecord.cpp
      transactiontablemodel.cpp
      transactionview.cpp
      walletcontroller.cpp
      walletframe.cpp
      walletmodel.cpp
      walletmodeltransaction.cpp
      walletview.cpp
  )
  target_link_libraries(bitcoinqt
    PRIVATE
      bitcoin_wallet
      ${qt_package}::Network
  )
endif()

if(CMAKE_CROSSCOMPILING AND ${qt_package}_DIR STREQUAL "${QT_CROSS_LIBS_PATH}/cmake/${qt_package}")
  target_compile_definitions(bitcoinqt PRIVATE QT_STATICPLUGIN)
  set_property(TARGET Qt5::Core APPEND PROPERTY INTERFACE_LINK_LIBRARIES
    ${QT_CROSS_LIBS_PATH}/libqtpcre2.a
    ${QT_CROSS_LIBS_PATH}/libqtlibpng.a
    ${QT_CROSS_LIBS_PATH}/libqtharfbuzz.a
    $<$<PLATFORM_ID:Linux>:dl>
  )
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET Qt5::QXcbIntegrationPlugin)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_XCB)
    set_property(TARGET Qt5::Core APPEND PROPERTY INTERFACE_LINK_LIBRARIES
      -L${QT_CROSS_LIBS_PATH}
      freetype
      fontconfig
      xcb
      xcb-icccm
      xcb-image
      xcb-keysyms
      xcb-randr
      xcb-render
      xcb-render-util
      xcb-shape
      xcb-shm
      xcb-sync
      xcb-xfixes
      xcb-xinerama
      xcb-xkb
      xkbcommon
      xkbcommon-x11
    )
  elseif(WIN32 AND TARGET Qt5::QWindowsIntegrationPlugin AND TARGET Qt5::QWindowsVistaStylePlugin)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_WINDOWS)
    target_link_libraries(Qt5::Core INTERFACE
      dwmapi
      imm32
      netapi32
      shlwapi
      userenv
      uxtheme
      version
      winmm
      wtsapi32
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND TARGET Qt5::QCocoaIntegrationPlugin AND TARGET Qt5::QMacStylePlugin)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_COCOA)
    target_link_libraries(Qt5::Core INTERFACE
      "-framework AppKit"
      "-framework Carbon"
      "-framework CoreVideo"
      "-framework IOKit"
      "-framework IOSurface"
      "-framework Metal"
      "-framework QuartzCore"
      "-framework Security"
    )
  endif()
endif()

add_executable(bitcoin-qt
  main.cpp
  ../init/bitcoin-qt.cpp
)
target_link_libraries(bitcoin-qt
  bitcoinqt
  bitcoin_node
)
set(installable_targets bitcoin-qt)

if(MULTIPROCESS)
  add_executable(bitcoin-gui
    main.cpp
    ../init/bitcoin-gui.cpp
  )
  target_link_libraries(bitcoin-gui
    bitcoinqt
    bitcoin_node
    bitcoin_ipc
  )
  list(APPEND installable_targets bitcoin-gui)
endif()

install(TARGETS ${installable_targets}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT GUI
)
