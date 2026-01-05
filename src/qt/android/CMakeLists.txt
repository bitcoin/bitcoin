# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Packaging output directory
set(ANDROID_PACKAGE_DIR ${CMAKE_BINARY_DIR}/android_package)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  PATTERN "CMakeLists.txt" EXCLUDE
  PATTERN "template_deploy.json" EXCLUDE
)

# Do NOT copy the generic Qt Java bindings sources (QtActivity, QtService, etc.)
# as they are already provided in the Qt6Android*.jar files bundled with Qt.
# Copy only our custom application subclass sources.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/org DESTINATION ${ANDROID_PACKAGE_DIR}/src)

file(COPY ${ANDROID_SHARED_CXX_LIB} DESTINATION libs/${CMAKE_ANDROID_ARCH_ABI})

# Build a shared library instead of an executable so that
# androiddeployqt can package it and Java can load it via
# System.loadLibrary().  The output name must be unique per ABI.
add_library(bitcoin-qt-android SHARED
  ../main.cpp
  ../../init/bitcoin-qt.cpp
)
if(DEFINED ANDROID_ABI)
  string(REPLACE "-" "_" _android_abi_sanitized "${ANDROID_ABI}")
  set_target_properties(bitcoin-qt-android PROPERTIES OUTPUT_NAME "bitcoin-qt_${_android_abi_sanitized}")
endif()
# Allow duplicate JNI_OnLoad coming from QtCore and the platform plugin.
target_link_options(bitcoin-qt-android PRIVATE -Wl,--allow-multiple-definition)

# Set the same compile definitions and include directories as the main bitcoinqt library
target_compile_definitions(bitcoin-qt-android
  PRIVATE
    QT_NO_KEYWORDS
    QT_USE_QSTRINGBUILDER
)

target_include_directories(bitcoin-qt-android
  PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)

# Link the same libraries as the main bitcoin-qt executable
target_link_libraries(bitcoin-qt-android
  core_interface
  bitcoinqt
  bitcoin_node
)

# Import Qt plugins for Android
import_plugins(bitcoin-qt-android)

# Provide a custom Android package source directory so that a user-supplied
# AndroidManifest.xml (and any extra Java/Kotlin sources or resources) found
# under <project>/android will be merged into the package produced by
# androiddeployqt. This lets us tweak the manifest without touching Qt's
# own template. In addition, pass command-line arguments to the Qt loader
# via the dedicated manifest <meta-data> entry.
set_target_properties(bitcoin-qt-android PROPERTIES
  QT_ANDROID_PACKAGE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/android"
  # Qt's deployment JSON generator (qt6_android_generate_deployment_settings)
  # doesn't currently pick up QT_ANDROID_PACKAGE_SOURCE_DIR. Instead it looks
  # for a private property called _qt_android_native_package_source_dir. Set
  # it explicitly so the generated armv7a_deploy.json contains the
  # "android-package-source-directory" entry and thus makes
  # androiddeployqt copy our custom template from <project>/android verbatim.
  _qt_android_native_package_source_dir "${PROJECT_SOURCE_DIR}/android"
  QT_ANDROID_APPLICATION_ARGUMENTS "-signet"
)

# cmake_path(RELATIVE_PATH GRADLEW_EXECUTABLE BASE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# === Generate deploy.json ==========================================
# The template contains placeholders. Replace those with the actual locations
# discovered from the toolchain and build variables.

set(TEMPLATE_DEPLOY_JSON ${CMAKE_CURRENT_SOURCE_DIR}/template_deploy.json)
set(DEPLOY_JSON_PATH     ${CMAKE_BINARY_DIR}/${CMAKE_ANDROID_ARCH_ABI}_deploy.json)

if(NOT EXISTS "${TEMPLATE_DEPLOY_JSON}")
  message(FATAL_ERROR "Expected template deploy file not found at ${TEMPLATE_DEPLOY_JSON}")
endif()

# Map toolchain variables to those used in template_deploy.json
set(QT_ANDROID_SDK          "${Qt6_ROOT}")
set(QT_HOST_PATH            "${QT_HOST_PATH}")
set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT ANDROID_API_LEVEL AND ANDROID_PLATFORM MATCHES "android-([0-9]+)")
  set(ANDROID_API_LEVEL "${CMAKE_MATCH_1}")
endif()

# Verify essential variables
foreach(_var QT_ANDROID_SDK QT_HOST_PATH ANDROID_SDK_ROOT ANDROID_NDK_ROOT ANDROID_API_LEVEL)
  if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
    message(FATAL_ERROR "${_var} is not defined; cannot generate deploy json")
  endif()
endforeach()

# Current package source dir inside build tree.
file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}" _package_src_dir)

# Configure the template
configure_file("${TEMPLATE_DEPLOY_JSON}" "${DEPLOY_JSON_PATH}.tmp" @ONLY)

# Path to androiddeployqt taken from the host Qt installation.
set(ANDROIDDEPLOYQT_EXECUTABLE "${QT_HOST_PATH}/bin/androiddeployqt")

if(NOT EXISTS "${ANDROIDDEPLOYQT_EXECUTABLE}")
  message(FATAL_ERROR "androiddeployqt was not found at ${ANDROIDDEPLOYQT_EXECUTABLE}")
endif()

# Filter JSON to only include the current ABI (using the existing Python script logic)
set(FILTER_JSON_SCRIPT ${CMAKE_BINARY_DIR}/filter_json.py)
file(WRITE ${FILTER_JSON_SCRIPT}
"import json
import sys

with open(sys.argv[1], 'r') as f:
    data = json.load(f)

current_abi = sys.argv[2]

# Filter out all ABIs except the current one
for key in ['qt', 'qtDataDirectory', 'qtLibExecsDirectory', 'qtLibsDirectory', 'qtPluginsDirectory']:
    if key in data:
        filtered_dict = {}
        if current_abi in data[key]:
            filtered_dict[current_abi] = data[key][current_abi]
        data[key] = filtered_dict

if 'architectures' in data:
    filtered_arch = {}
    if current_abi in data['architectures']:
        filtered_arch[current_abi] = data['architectures'][current_abi]
    data['architectures'] = filtered_arch

with open(sys.argv[3], 'w') as f:
    json.dump(data, f, indent=2)
")

execute_process(
    COMMAND python3 ${FILTER_JSON_SCRIPT} "${DEPLOY_JSON_PATH}.tmp" ${CMAKE_ANDROID_ARCH_ABI} "${DEPLOY_JSON_PATH}"
)

# The deployment settings JSON we just ensured exists.
set(ANDROID_DEPLOY_JSON "${DEPLOY_JSON_PATH}")

add_custom_target(apk_package
  COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_PACKAGE_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bitcoin-qt-android> ${ANDROID_PACKAGE_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}/libbitcoin-qt_${CMAKE_ANDROID_ARCH_ABI}.so
  COMMAND ${ANDROIDDEPLOYQT_EXECUTABLE}
          --input ${ANDROID_DEPLOY_JSON}
          --output ${ANDROID_PACKAGE_DIR}
          --gradle --verbose
  VERBATIM
)
