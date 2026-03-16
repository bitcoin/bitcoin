# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Define HAVE_PTHREAD_* variables depending on what pthread functions are
# available.

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)

cmake_push_check_state()
set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
check_cxx_source_compiles("
  #include <pthread.h>
  int main(int argc, char** argv)
  {
    char thread_name[16];
    return pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name));
  }"
  HAVE_PTHREAD_GETNAME_NP)

check_cxx_source_compiles("
  #include <cstdint>
  #include <pthread.h>
  int main(int argc, char** argv)
  {
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return 0;
  }"
  HAVE_PTHREAD_THREADID_NP)

check_cxx_source_compiles("
  #include <pthread.h>
  #include <pthread_np.h>
  int main(int argc, char** argv)
  {
    return pthread_getthreadid_np();
  }"
  HAVE_PTHREAD_GETTHREADID_NP)
cmake_pop_check_state()
