# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

get_filename_component(binary_directory ${CMAKE_CXX_COMPILER} DIRECTORY)
file(
  COPY ${binary_directory}/../sysroot/usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-android/libc++_shared.so
  DESTINATION libs/${CMAKE_ANDROID_ARCH_ABI}
)

set(qt_source_archive "${CMAKE_SOURCE_DIR}/depends/sources/qtbase-everywhere-opensource-src-${${qt_package}_VERSION}.tar.xz")
set(qt_source_directory "qtbase-everywhere-src-${${qt_package}_VERSION}")

execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND tar xf ${qt_source_archive} -C ${CMAKE_CURRENT_BINARY_DIR}/src/ ${qt_source_directory}/src/android/jar/src --strip-components=5
  COMMAND tar xf ${qt_source_archive} -C ${CMAKE_CURRENT_BINARY_DIR}/src/ ${qt_source_directory}/src/android/java/src --strip-components=5
)

add_custom_target(apk_package
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bitcoin-qt> libs/${CMAKE_ANDROID_ARCH_ABI}/libbitcoin-qt_${CMAKE_ANDROID_ARCH_ABI}.so
  COMMAND gradle wrapper --gradle-version=6.6.1
  COMMAND ./gradlew build
  VERBATIM
)
