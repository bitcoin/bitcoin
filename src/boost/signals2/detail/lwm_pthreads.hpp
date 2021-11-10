//
//  boost/signals2/detail/lwm_pthreads.hpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2008 Frank Mori Hess
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SIGNALS2_LWM_PTHREADS_HPP
#define BOOST_SIGNALS2_LWM_PTHREADS_HPP

// MS compatible compilers support #pragma once

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assert.hpp>
#include <pthread.h>

namespace boost
{

namespace signals2
{

class mutex
{
private:

    pthread_mutex_t m_;

    mutex(mutex const &);
    mutex & operator=(mutex const &);

public:

    mutex()
    {

// HPUX 10.20 / DCE has a nonstandard pthread_mutex_init

#if defined(__hpux) && defined(_DECTHREADS_)
        BOOST_VERIFY(pthread_mutex_init(&m_, pthread_mutexattr_default) == 0);
#else
        BOOST_VERIFY(pthread_mutex_init(&m_, 0) == 0);
#endif
    }

    ~mutex()
    {
        BOOST_VERIFY(pthread_mutex_destroy(&m_) == 0);
    }

    void lock()
    {
        BOOST_VERIFY(pthread_mutex_lock(&m_) == 0);
    }

    bool try_lock()
    {
        return pthread_mutex_trylock(&m_) == 0;
    }

    void unlock()
    {
        BOOST_VERIFY(pthread_mutex_unlock(&m_) == 0);
    }
};

} // namespace signals2

} // namespace boost

#endif // #ifndef BOOST_SIGNALS2_LWM_PTHREADS_HPP
