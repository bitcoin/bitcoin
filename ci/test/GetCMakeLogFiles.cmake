# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
  set(log_files "CMakeFiles/CMakeConfigureLog.yaml")
else()
  set(log_files "CMakeFiles/CMakeOutput.log CMakeFiles/CMakeError.log")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E echo ${log_files})
