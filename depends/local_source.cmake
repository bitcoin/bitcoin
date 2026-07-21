# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

cmake_minimum_required(VERSION 3.22)

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E tar cf "${OUTPUT}"
    --format=gnutar "--mtime=1970-01-01 00:00:00 UTC" -- .
  WORKING_DIRECTORY "${LOCAL_DIR}"
  COMMAND_ERROR_IS_FATAL ANY
)
