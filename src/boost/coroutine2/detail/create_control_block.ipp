
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_CREATE_CONTROLBLOCK_IPP
#define BOOST_COROUTINES2_DETAIL_CREATE_CONTROLBLOCK_IPP

#include <cstddef>
#include <memory>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/context/preallocated.hpp>
#include <boost/context/stack_context.hpp>

#include <boost/coroutine2/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

template< typename ControlBlock, typename StackAllocator, typename Fn >
ControlBlock * create_control_block( StackAllocator && salloc, Fn && fn) {
    auto sctx = salloc.allocate();
    // reserve space for control structure
#if defined(BOOST_NO_CXX11_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
    void * sp = static_cast< char * >( sctx.sp) - sizeof( ControlBlock);
    const std::size_t size = sctx.size - sizeof( ControlBlock);
#else
    constexpr std::size_t func_alignment = 64; // alignof( ControlBlock);
    constexpr std::size_t func_size = sizeof( ControlBlock);
    // reserve space on stack
    void * sp = static_cast< char * >( sctx.sp) - func_size - func_alignment;
    // align sp pointer
    std::size_t space = func_size + func_alignment;
    sp = std::align( func_alignment, func_size, sp, space);
    BOOST_ASSERT( nullptr != sp);
    // calculate remaining size
    const std::size_t size = sctx.size - ( static_cast< char * >( sctx.sp) - static_cast< char * >( sp) );
#endif
    // placment new for control structure on coroutine stack
    return new ( sp) ControlBlock{ context::preallocated( sp, size, sctx),
                                   std::forward< StackAllocator >( salloc), std::forward< Fn >( fn) };
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_CREATE_CONTROLBLOCK_IPP
