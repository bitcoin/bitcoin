#ifndef BOOST_SMART_PTR_DETAIL_SPINLOCK_PT_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SPINLOCK_PT_HPP_INCLUDED

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

#include <pthread.h>

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using pthread_mutex spinlock emulation")

#endif

namespace boost
{

namespace detail
{

class spinlock
{
public:

    pthread_mutex_t v_;

public:

    bool try_lock()
    {
        return pthread_mutex_trylock( &v_ ) == 0;
    }

    void lock()
    {
        pthread_mutex_lock( &v_ );
    }

    void unlock()
    {
        pthread_mutex_unlock( &v_ );
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

#define BOOST_DETAIL_SPINLOCK_INIT { PTHREAD_MUTEX_INITIALIZER }

#endif // #ifndef BOOST_SMART_PTR_DETAIL_SPINLOCK_PT_HPP_INCLUDED
