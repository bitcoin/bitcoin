#ifndef BOOST_SMART_PTR_DETAIL_LWM_PTHREADS_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_LWM_PTHREADS_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//
//  boost/detail/lwm_pthreads.hpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/assert.hpp>
#include <pthread.h>

namespace boost
{

namespace detail
{

class lightweight_mutex
{
private:

    pthread_mutex_t m_;

    lightweight_mutex(lightweight_mutex const &);
    lightweight_mutex & operator=(lightweight_mutex const &);

public:

    lightweight_mutex()
    {

// HPUX 10.20 / DCE has a nonstandard pthread_mutex_init

#if defined(__hpux) && defined(_DECTHREADS_)
        BOOST_VERIFY( pthread_mutex_init( &m_, pthread_mutexattr_default ) == 0 );
#else
        BOOST_VERIFY( pthread_mutex_init( &m_, 0 ) == 0 );
#endif
    }

    ~lightweight_mutex()
    {
        BOOST_VERIFY( pthread_mutex_destroy( &m_ ) == 0 );
    }

    class scoped_lock;
    friend class scoped_lock;

    class scoped_lock
    {
    private:

        pthread_mutex_t & m_;

        scoped_lock(scoped_lock const &);
        scoped_lock & operator=(scoped_lock const &);

    public:

        scoped_lock(lightweight_mutex & m): m_(m.m_)
        {
            BOOST_VERIFY( pthread_mutex_lock( &m_ ) == 0 );
        }

        ~scoped_lock()
        {
            BOOST_VERIFY( pthread_mutex_unlock( &m_ ) == 0 );
        }
    };
};

} // namespace detail

} // namespace boost

#endif // #ifndef BOOST_SMART_PTR_DETAIL_LWM_PTHREADS_HPP_INCLUDED
