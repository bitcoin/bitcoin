# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  PATTERN "CMakeLists.txt" EXCLUDE
)

file(COPY ${ANDROID_APK_SRC}/src/org DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/src)

file(COPY ${ANDROID_SHARED_CXX_LIB} DESTINATION libs/${CMAKE_ANDROID_ARCH_ABI})

cmake_path(RELATIVE_PATH GRADLEW_EXECUTABLE BASE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(apk_package
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bitcoin-qt> libs/${CMAKE_ANDROID_ARCH_ABI}/libbitcoin-qt_${CMAKE_ANDROID_ARCH_ABI}.so
  COMMAND ${GRADLEW_EXECUTABLE} build
  VERBATIM
)
