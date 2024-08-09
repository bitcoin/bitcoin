# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

macro(add_windows_resources target rc_file)
  if(WIN32)
    target_sources(${target} PRIVATE ${rc_file})
    set_property(SOURCE ${rc_file}
      APPEND PROPERTY COMPILE_DEFINITIONS WINDRES_PREPROC
    )
  endif()
endmacro()
