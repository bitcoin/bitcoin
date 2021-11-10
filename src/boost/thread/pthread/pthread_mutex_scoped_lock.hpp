#ifndef BOOST_PTHREAD_MUTEX_SCOPED_LOCK_HPP
#define BOOST_PTHREAD_MUTEX_SCOPED_LOCK_HPP
//  (C) Copyright 2007-8 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <pthread.h>
#include <boost/assert.hpp>
#include <boost/thread/pthread/pthread_helpers.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    namespace pthread
    {
        class pthread_mutex_scoped_lock
        {
            pthread_mutex_t* m;
            bool locked;
        public:
            explicit pthread_mutex_scoped_lock(pthread_mutex_t* m_) BOOST_NOEXCEPT:
                m(m_),locked(true)
            {
                BOOST_VERIFY(!posix::pthread_mutex_lock(m));
            }
            void unlock() BOOST_NOEXCEPT
            {
                BOOST_VERIFY(!posix::pthread_mutex_unlock(m));
                locked=false;
            }
            void unlock_if_locked() BOOST_NOEXCEPT
            {
              if(locked)
              {
                  unlock();
              }
            }
            ~pthread_mutex_scoped_lock() BOOST_NOEXCEPT
            {
                if(locked)
                {
                    unlock();
                }
            }

        };

        class pthread_mutex_scoped_unlock
        {
            pthread_mutex_t* m;
        public:
            explicit pthread_mutex_scoped_unlock(pthread_mutex_t* m_) BOOST_NOEXCEPT:
                m(m_)
            {
                BOOST_VERIFY(!posix::pthread_mutex_unlock(m));
            }
            ~pthread_mutex_scoped_unlock() BOOST_NOEXCEPT
            {
                BOOST_VERIFY(!posix::pthread_mutex_lock(m));
            }

        };
    }
}

#include <boost/config/abi_suffix.hpp>

#endif
