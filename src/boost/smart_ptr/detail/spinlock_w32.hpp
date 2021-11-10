#ifndef BOOST_SMART_PTR_DETAIL_SPINLOCK_W32_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SPINLOCK_W32_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/smart_ptr/detail/sp_interlocked.hpp>
#include <boost/smart_ptr/detail/yield_k.hpp>

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using Win32 spinlock")

#endif

// BOOST_COMPILER_FENCE

#if defined(__INTEL_COMPILER)

#define BOOST_COMPILER_FENCE __memory_barrier();

#elif defined( _MSC_VER ) && _MSC_VER >= 1310

extern "C" void _ReadWriteBarrier();
#pragma intrinsic( _ReadWriteBarrier )

#define BOOST_COMPILER_FENCE _ReadWriteBarrier();

#elif defined(__GNUC__)

#define BOOST_COMPILER_FENCE __asm__ __volatile__( "" : : : "memory" );

#else

#define BOOST_COMPILER_FENCE

#endif

//

namespace boost
{

namespace detail
{

class spinlock
{
public:

    long v_;

public:

    bool try_lock()
    {
        long r = BOOST_SP_INTERLOCKED_EXCHANGE( &v_, 1 );

        BOOST_COMPILER_FENCE

        return r == 0;
    }

    void lock()
    {
        for( unsigned k = 0; !try_lock(); ++k )
        {
            boost::detail::yield( k );
        }
    }

    void unlock()
    {
        BOOST_COMPILER_FENCE
        *const_cast< long volatile* >( &v_ ) = 0;
    }

public:

    class scoped_lock
    {
    private:

        spinlock & sp_;

        scoped_lock( scoped_lock const & );
        scoped_lock & operator=( scoped_lock const & );

    public:

        explicit scoped_lock( spinlock & sp ): sp_( sp )
        {
            sp.lock();
        }

        ~scoped_lock()
        {
            sp_.unlock();
        }
    };
};

} // namespace detail
} // namespace boost

#define BOOST_DETAIL_SPINLOCK_INIT {0}

#endif // #ifndef BOOST_SMART_PTR_DETAIL_SPINLOCK_W32_HPP_INCLUDED
