# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

# Evaluates config-specific generator expressions in a list.
# Recognizable patterns are:
#  - $<$<CONFIG:[config]>:[value]>
#  - $<$<NOT:$<CONFIG:[config]>>:[value]>
function(evaluate_generator_expressions list config)
  set(input ${${list}})
  set(result)
  foreach(token IN LISTS input)
    if(token MATCHES "\\$<\\$<CONFIG:([^>]+)>:([^>]+)>")
      if(CMAKE_MATCH_1 STREQUAL config)
        list(APPEND result ${CMAKE_MATCH_2})
      endif()
    elseif(token MATCHES "\\$<\\$<NOT:\\$<CONFIG:([^>]+)>>:([^>]+)>")
      if(NOT CMAKE_MATCH_1 STREQUAL config)
        list(APPEND result ${CMAKE_MATCH_2})
      endif()
    else()
      list(APPEND result ${token})
    endif()
  endforeach()
  set(${list} ${result} PARENT_SCOPE)
endfunction()


# Gets target's interface properties recursively.
function(get_target_interface var config target property)
  get_target_property(result ${target} INTERFACE_${property})
  if(result)
    # On systems where pthread functionality is not provided by
    # the C library implementation, the CMake FindThreads module
    # sets the Threads::Threads target's compile options to
    # generator expressions that evaluate to `-pthread` in this
    # project.
    # To improve the readability of the configuration summary,
    # we skip these generator expressions.
    if(${target} STREQUAL "Threads::Threads" AND ${property} STREQUAL "COMPILE_OPTIONS")
      set(result -pthread)
    else()
      evaluate_generator_expressions(result "${config}")
    endif()
    list(JOIN result " " result)
  else()
    set(result)
  endif()

  get_target_property(dependencies ${target} INTERFACE_LINK_LIBRARIES)
  if(dependencies)
    evaluate_generator_expressions(dependencies "${config}")
    foreach(dependency IN LISTS dependencies)
      if(TARGET ${dependency})
        get_target_interface(dep_result "${config}" ${dependency} ${property})
        string(STRIP "${result} ${dep_result}" result)
      endif()
    endforeach()
  endif()

  set(${var} "${result}" PARENT_SCOPE)
endfunction()
