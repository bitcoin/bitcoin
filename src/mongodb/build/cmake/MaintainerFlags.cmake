if (ENABLE_MAINTAINER_FLAGS)
   include(CheckCCompilerFlag)

   message(STATUS "Detecting available maintainer flags")
   file (READ "build/maintainer-flags.txt" MAINTAINER_FLAGS)

   # Convert file contents into a CMake list (where each element in the list
   # is one line of the file)
   #
   string (REGEX REPLACE ";" "\\\\;" MAINTAINER_FLAGS "${MAINTAINER_FLAGS}")
   string (REGEX REPLACE "\n" ";" MAINTAINER_FLAGS "${MAINTAINER_FLAGS}")

   foreach (MAINTAINER_FLAG ${MAINTAINER_FLAGS})
      # Avoid useless "Performing Test FLAG_OK" message.
      set (MESSAGES_ENABLED 0)
      check_c_compiler_flag ("${MAINTAINER_FLAG}" FLAG_OK)
      set (MESSAGES_ENABLED 1)
      if (FLAG_OK)
         message (STATUS "C compiler accepts ${MAINTAINER_FLAG}")
         set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MAINTAINER_FLAG}")
      else ()
         message (STATUS "C compiler does not accept ${MAINTAINER_FLAG}")
      endif ()
      unset (FLAG_OK CACHE)
   endforeach ()

   message(STATUS "Maintainer flags: ${CMAKE_C_FLAGS}")
endif ()
