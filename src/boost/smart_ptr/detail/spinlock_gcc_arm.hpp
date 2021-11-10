#ifndef BOOST_SMART_PTR_DETAIL_SPINLOCK_GCC_ARM_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SPINLOCK_GCC_ARM_HPP_INCLUDED

//
//  Copyright (c) 2008, 2011 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/smart_ptr/detail/yield_k.hpp>

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using g++/ARM spinlock")

#endif

#if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7S__)

# define BOOST_SP_ARM_BARRIER "dmb"
# define BOOST_SP_ARM_HAS_LDREX

#elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__)

# define BOOST_SP_ARM_BARRIER "mcr p15, 0, r0, c7, c10, 5"
# define BOOST_SP_ARM_HAS_LDREX

#else

# define BOOST_SP_ARM_BARRIER ""

#endif

namespace boost
{

namespace detail
{

class spinlock
{
public:

    int v_;

public:

    bool try_lock()
    {
        int r;

#ifdef BOOST_SP_ARM_HAS_LDREX

        __asm__ __volatile__(
            "ldrex %0, [%2]; \n"
            "cmp %0, %1; \n"
            "strexne %0, %1, [%2]; \n"
            BOOST_SP_ARM_BARRIER :
            "=&r"( r ): // outputs
            "r"( 1 ), "r"( &v_ ): // inputs
            "memory", "cc" );

#else

        __asm__ __volatile__(
            "swp %0, %1, [%2];\n"
            BOOST_SP_ARM_BARRIER :
            "=&r"( r ): // outputs
            "r"( 1 ), "r"( &v_ ): // inputs
            "memory", "cc" );

#endif

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
        __asm__ __volatile__( BOOST_SP_ARM_BARRIER ::: "memory" );
        *const_cast< int volatile* >( &v_ ) = 0;
        __asm__ __volatile__( BOOST_SP_ARM_BARRIER ::: "memory" );
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

#undef BOOST_SP_ARM_BARRIER
#undef BOOST_SP_ARM_HAS_LDREX

#endif // #ifndef BOOST_SMART_PTR_DETAIL_SPINLOCK_GCC_ARM_HPP_INCLUDED
