include (CheckCXXSourceCompiles)
set (SOURCES ${SOURCES}
   ${SOURCE_DIR}/src/snappy-1.1.3/snappy.cc
   ${SOURCE_DIR}/src/snappy-1.1.3/snappy-sinksource.cc
   ${SOURCE_DIR}/src/snappy-1.1.3/snappy-stubs-internal.cc
   ${SOURCE_DIR}/src/snappy-1.1.3/snappy-c.cc
)

if (NOT WIN32)
   include (TestBigEndian)
   test_big_endian (WORDS_BIG_ENDIAN)
   if (WORDS_BIG_ENDIAN)
      add_definitions (-DWORDS_BIGENDIAN=1)
   endif ()
endif ()

set (HAVE_STDINT_H 0)
set (HAVE_STDDEF_H 0)
set (HAVE_SYS_UIO_H 0)

# See https://github.com/google/snappy/blob/master/CMakeLists.txt
check_include_files ("byteswap.h" HAVE_BYTESWAP_H)
check_include_files ("dlfcn.h" HAVE_DLFCN_H)
check_include_files ("inttypes.h" HAVE_INTTYPES_H)
check_include_files ("memory.h" HAVE_MEMORY_H)
check_include_files ("stddef.h" HAVE_STDDEF_H)
check_include_files ("stdint.h" HAVE_STDINT_H)
check_include_files ("stdlib.h" HAVE_STDLIB_H)
check_include_files ("strings.h" HAVE_STRINGS_H)
check_include_files ("string.h" HAVE_STRING_H)
check_include_files ("sys/byteswap.h" HAVE_SYS_BYTESWAP_H)
check_include_files ("sys/endian.h" HAVE_SYS_ENDIAN_H)
check_include_files ("sys/mman.h" HAVE_SYS_MMAN_H)
check_include_files ("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_files ("sys/stat.h" HAVE_SYS_STAT_H)
check_include_files ("sys/time.h" HAVE_SYS_TIME_H)
check_include_files ("sys/types.h" HAVE_SYS_TYPES_H)
check_include_files ("sys/uio.h" HAVE_SYS_UIO_H)
check_include_files ("unistd.h" HAVE_UNISTD_H)
check_include_files ("windows.h" HAVE_WINDOWS_H)

check_cxx_source_compiles("int main(void) { return __builtin_expect(0, 1); }" HAVE_BUILTIN_EXPECT)
check_cxx_source_compiles("int main(void) { return __builtin_ctzll(0); }" HAVE_BUILTIN_CTZ)

set (ac_cv_have_stdint_h ${HAVE_STDINT_H})
set (ac_cv_have_stddef_h ${HAVE_STDDEF_H})
set (ac_cv_have_sys_uio_h ${HAVE_SYS_UIO_H})
configure_file (
   "${SOURCE_DIR}/src/snappy-1.1.3/snappy-stubs-public.h.in"
   "${PROJECT_BINARY_DIR}/src/snappy-1.1.3/snappy-stubs-public.h"
)
message (STATUS "Enabling snappy compression (bundled)")

list (
   APPEND
   MONGOC_INTERNAL_INCLUDE_DIRS
   "${SOURCE_DIR}/src/snappy-1.1.3"
   "${PROJECT_BINARY_DIR}/src/snappy-1.1.3"
)
