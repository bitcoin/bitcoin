
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_SEGMENTED_STACK_ALLOCATOR_H
#define BOOST_COROUTINES_SEGMENTED_STACK_ALLOCATOR_H

#include <cstddef>
#include <new>

#include <boost/config.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/stack_context.hpp>
#include <boost/coroutine/stack_traits.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

// forward declaration for splitstack-functions defined in libgcc
extern "C" {
void *__splitstack_makecontext( std::size_t,
                                void * [BOOST_CONTEXT_SEGMENTS],
                                std::size_t *);

void __splitstack_releasecontext( void * [BOOST_CONTEXT_SEGMENTS]);

void __splitstack_resetcontext( void * [BOOST_CONTEXT_SEGMENTS]);

void __splitstack_block_signals_context( void * [BOOST_CONTEXT_SEGMENTS],
                                         int * new_value, int * old_value);
}

namespace boost {
namespace coroutines {

template< typename traitsT >
struct basic_segmented_stack_allocator
{
    typedef traitsT traits_type;

    void allocate( stack_context & ctx, std::size_t size = traits_type::minimum_size() )
    {
        void * limit = __splitstack_makecontext( size, ctx.segments_ctx, & ctx.size);
        if ( ! limit) throw std::bad_alloc();

        // ctx.size is already filled by __splitstack_makecontext
        ctx.sp = static_cast< char * >( limit) + ctx.size;

        int off = 0;
        __splitstack_block_signals_context( ctx.segments_ctx, & off, 0);
    }

    void deallocate( stack_context & ctx)
    { __splitstack_releasecontext( ctx.segments_ctx); }
};

typedef basic_segmented_stack_allocator< stack_traits > segmented_stack_allocator;

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_SEGMENTED_STACK_ALLOCATOR_H
