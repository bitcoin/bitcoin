//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_PREFETCH_H
#define BOOST_CONTEXT_DETAIL_PREFETCH_H

#include <cstddef>
#include <cstdint>

#include <boost/config.hpp>
#include <boost/predef.h>

#include <boost/context/detail/config.hpp>

#if BOOST_COMP_INTEL || BOOST_COMP_INTEL_EMULATED
#include <immintrin.h>
#endif

#if BOOST_COMP_MSVC && !defined(_M_ARM) && !defined(_M_ARM64)
#include <mmintrin.h>
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {
namespace detail {

#if BOOST_COMP_GNUC || BOOST_COMP_CLANG
#define BOOST_HAS_PREFETCH 1
BOOST_FORCEINLINE
void prefetch( void * addr) {
    // L1 cache : hint == 1
    __builtin_prefetch( addr, 1, 1);
}
#elif BOOST_COMP_INTEL || BOOST_COMP_INTEL_EMULATED
#define BOOST_HAS_PREFETCH 1
BOOST_FORCEINLINE
void prefetch( void * addr) {
    // L1 cache : hint == _MM_HINT_T0
    _mm_prefetch( (const char *)addr, _MM_HINT_T0);
}
#elif BOOST_COMP_MSVC && !defined(_M_ARM) && !defined(_M_ARM64)
#define BOOST_HAS_PREFETCH 1
BOOST_FORCEINLINE
void prefetch( void * addr) {
    // L1 cache : hint == _MM_HINT_T0
    _mm_prefetch( (const char *)addr, _MM_HINT_T0);
}
#endif

inline
void prefetch_range( void * addr, std::size_t len) {
#if defined(BOOST_HAS_PREFETCH)
    void * vp = addr;
    void * end = reinterpret_cast< void * >(
        reinterpret_cast< uintptr_t >( addr) + static_cast< uintptr_t >( len) );
    while ( vp < end) {
        prefetch( vp);
        vp = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( vp) + static_cast< uintptr_t >( prefetch_stride) );
    }
#endif
}

#undef BOOST_HAS_PREFETCH

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_DETAIL_PREFETCH_H
