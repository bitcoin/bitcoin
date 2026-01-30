include_guard(GLOBAL)

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)
cmake_push_check_state(RESET)

# Check for clmul instructions support.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_REQUIRED_FLAGS "-mpclmul")
endif()
check_cxx_source_compiles("
  #include <immintrin.h>
  #include <stdint.h>

  int main()
  {
    __m128i a = _mm_cvtsi64_si128((uint64_t)7);
    __m128i b = _mm_clmulepi64_si128(a, a, 37);
    __m128i c = _mm_srli_epi64(b, 41);
    __m128i d = _mm_xor_si128(b, c);
    uint64_t e = _mm_cvtsi128_si64(d);
    return e == 0;
  }
  " HAVE_CLMUL
)
if(HAVE_CLMUL)
  set(CLMUL_CXXFLAGS ${CMAKE_REQUIRED_FLAGS})
endif()

if(CMAKE_CXX_STANDARD LESS 20)
  # Check for working clz builtins.
  check_cxx_source_compiles("
    int main()
    {
      unsigned a = __builtin_clz(1);
      unsigned long b = __builtin_clzl(1);
      unsigned long long c = __builtin_clzll(1);
    }
    " HAVE_CLZ
  )
endif()

cmake_pop_check_state()
