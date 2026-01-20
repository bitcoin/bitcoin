include_guard(GLOBAL)
include(CheckCSourceCompiles)

function(check_memory_sanitizer output)
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  check_c_source_compiles("
    #if defined(__has_feature)
    #  if __has_feature(memory_sanitizer)
         /* MemorySanitizer is enabled. */
    #  elif
    #    error \"MemorySanitizer is disabled.\"
    #  endif
    #else
    #  error \"__has_feature is not defined.\"
    #endif
  " HAVE_MSAN)
  set(${output} ${HAVE_MSAN} PARENT_SCOPE)
endfunction()
