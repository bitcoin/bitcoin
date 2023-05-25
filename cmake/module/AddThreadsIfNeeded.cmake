# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

function(add_threads_if_needed)
  # TODO: Not all targets, which will be added in the future,
  #       require Threads. Therefore, a proper check will be
  #       appropriate here.

  if(CMAKE_C_COMPILER_LOADED)
    message(FATAL_ERROR [=[
  To make FindThreads check C++ language features, C language must be
  disabled. This is essential, at least, when cross-compiling for MinGW-w64
  because two different threading models are available.
    ]=] )
  endif()

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  set_target_properties(Threads::Threads PROPERTIES IMPORTED_GLOBAL TRUE)

  set(thread_local)
  if(MINGW)
    #[=[
    mingw32's implementation of thread_local has been shown to behave
    erroneously under concurrent usage.
    See:
     - https://github.com/bitcoin/bitcoin/pull/15849
     - https://gist.github.com/jamesob/fe9a872051a88b2025b1aa37bfa98605
    ]=]
  elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    #[=[
    FreeBSD's implementation of thread_local is buggy.
    See:
     - https://github.com/bitcoin/bitcoin/pull/16059
     - https://groups.google.com/d/msg/bsdmailinglist/22ncTZAbDp4/Dii_pII5AwAJ
    ]=]
  elseif(THREADLOCAL)
    set(thread_local "$<$<COMPILE_FEATURES:cxx_thread_local>:HAVE_THREAD_LOCAL>")
  endif()
  set(THREAD_LOCAL_IF_AVAILABLE "${thread_local}" PARENT_SCOPE)
endfunction()
