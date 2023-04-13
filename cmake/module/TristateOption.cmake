# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# A tri-state option with three possible values: AUTO, OFF and ON (case-insensitive).
# TODO: This function will be removed before merging into master.
function(tristate_option variable description_text auto_means_on_condition_text default_value)
  set(${variable} ${default_value} CACHE STRING
    "${description_text} \"AUTO\" means \"ON\" ${auto_means_on_condition_text}"
  )

  set(expected_values AUTO OFF ON)
  set_property(CACHE ${variable} PROPERTY STRINGS ${expected_values})

  string(TOUPPER "${${variable}}" value)
  if(NOT value IN_LIST expected_values)
    message(FATAL_ERROR "${variable} value is \"${${variable}}\", but must be one of \"AUTO\", \"OFF\" or \"ON\".")
  endif()

  set(${${variable}} ${value} PARENT_SCOPE)
endfunction()
