# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

macro(fatal_error)
  message(FATAL_ERROR "\n"
    "Usage:\n"
    "  cmake -D BUILD_INFO_HEADER_PATH=<path> [-D SOURCE_DIR=<path>] -P ${CMAKE_CURRENT_LIST_FILE}\n"
    "All specified paths must be absolute ones.\n"
  )
endmacro()

if(DEFINED BUILD_INFO_HEADER_PATH AND IS_ABSOLUTE "${BUILD_INFO_HEADER_PATH}")
  if(EXISTS "${BUILD_INFO_HEADER_PATH}")
    file(STRINGS ${BUILD_INFO_HEADER_PATH} INFO LIMIT_COUNT 1)
  endif()
else()
  fatal_error()
endif()

if(DEFINED SOURCE_DIR)
  if(IS_ABSOLUTE "${SOURCE_DIR}" AND IS_DIRECTORY "${SOURCE_DIR}")
    set(WORKING_DIR ${SOURCE_DIR})
  else()
    fatal_error()
  endif()
else()
  set(WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(GIT_TAG)
set(GIT_COMMIT)
if(NOT "$ENV{BITCOIN_GENBUILD_NO_GIT}" STREQUAL "1")
  find_package(Git QUIET)
  if(Git_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree
      WORKING_DIRECTORY ${WORKING_DIR}
      OUTPUT_VARIABLE IS_INSIDE_WORK_TREE
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    if(IS_INSIDE_WORK_TREE)
      # Clean 'dirty' status of touched files that haven't been modified.
      execute_process(
        COMMAND ${GIT_EXECUTABLE} diff
        WORKING_DIRECTORY ${WORKING_DIR}
        OUTPUT_QUIET
        ERROR_QUIET
      )

      execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --abbrev=0
        WORKING_DIRECTORY ${WORKING_DIR}
        OUTPUT_VARIABLE MOST_RECENT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )

      execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list -1 ${MOST_RECENT_TAG}
        WORKING_DIRECTORY ${WORKING_DIR}
        OUTPUT_VARIABLE MOST_RECENT_TAG_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )

      execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${WORKING_DIR}
        OUTPUT_VARIABLE HEAD_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
      )

      execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
        WORKING_DIRECTORY ${WORKING_DIR}
        RESULT_VARIABLE IS_DIRTY
      )

      if(HEAD_COMMIT STREQUAL MOST_RECENT_TAG_COMMIT AND NOT IS_DIRTY)
        # If latest commit is tagged and not dirty, then use the tag name.
        set(GIT_TAG ${MOST_RECENT_TAG})
      else()
        # Otherwise, generate suffix from git, i.e. string like "0e0a5173fae3-dirty".
        execute_process(
          COMMAND ${GIT_EXECUTABLE} rev-parse --short=12 HEAD
          WORKING_DIRECTORY ${WORKING_DIR}
          OUTPUT_VARIABLE GIT_COMMIT
          OUTPUT_STRIP_TRAILING_WHITESPACE
          ERROR_QUIET
        )
        if(IS_DIRTY)
          string(APPEND GIT_COMMIT "-dirty")
        endif()
      endif()
    endif()
  endif()
endif()

if(GIT_TAG)
  set(NEWINFO "#define BUILD_GIT_TAG \"${GIT_TAG}\"")
elseif(GIT_COMMIT)
  set(NEWINFO "#define BUILD_GIT_COMMIT \"${GIT_COMMIT}\"")
else()
  set(NEWINFO "// No build information available")
endif()

# Only update the header if necessary.
if(NOT "${INFO}" STREQUAL "${NEWINFO}")
  file(WRITE ${BUILD_INFO_HEADER_PATH} "${NEWINFO}\n")
endif()
