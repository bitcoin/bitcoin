# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set(input_variables
  PROJECT_SOURCE_DIR
  COPYRIGHT_HOLDERS
  LCONVERT_EXECUTABLE
  LUPDATE_EXECUTABLE
  PYTHON_EXECUTABLE
  SED_EXECUTABLE
  XGETTEXT_EXECUTABLE
)

foreach(var IN LISTS input_variables)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "Variable '${var}' is not defined!")
  endif()
endforeach()

file(GLOB_RECURSE translatable_sources
  "${PROJECT_SOURCE_DIR}/src/*.h"
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/*.mm"
)

file(GLOB_RECURSE qt_translatable_sources
  "${PROJECT_SOURCE_DIR}/src/qt/*.h"
  "${PROJECT_SOURCE_DIR}/src/qt/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/qt/*.mm"
)

file(GLOB ui_files
  "${PROJECT_SOURCE_DIR}/src/qt/forms/*.ui"
)

set(subtrees crc32c crypto/ctaes leveldb minisketch secp256k1)
set(exclude_dirs bench compat crypto support test univalue)
foreach(directory IN LISTS subtrees exclude_dirs)
  list(FILTER translatable_sources
    EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/src/${directory}/.*"
  )
endforeach()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E env
    XGETTEXT=${XGETTEXT_EXECUTABLE}
    COPYRIGHT_HOLDERS=${COPYRIGHT_HOLDERS}
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_LIST_DIR}/extract_strings_qt.py
    ${translatable_sources}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND ${LUPDATE_EXECUTABLE}
    -no-obsolete
    -I ${PROJECT_SOURCE_DIR}/src
    -locations relative
    ${ui_files}
    ${qt_translatable_sources}
    ${PROJECT_SOURCE_DIR}/src/qt/bitcoinstrings.cpp
    -ts ${PROJECT_SOURCE_DIR}/src/qt/locale/bitcoin_en.ts
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND ${LCONVERT_EXECUTABLE}
    -drop-translations
    -o ${PROJECT_SOURCE_DIR}/src/qt/locale/bitcoin_en.xlf
    -i ${PROJECT_SOURCE_DIR}/src/qt/locale/bitcoin_en.ts
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND ${SED_EXECUTABLE}
    -i.old
    -e "s|source-language=\"en\" target-language=\"en\"|source-language=\"en\"|"
    -e "/<target xml:space=\"preserve\"><\\/target>/d"
    ${PROJECT_SOURCE_DIR}/src/qt/locale/bitcoin_en.xlf
  COMMAND_ERROR_IS_FATAL ANY
)

file(REMOVE "${PROJECT_SOURCE_DIR}/src/qt/locale/bitcoin_en.xlf.old")
