#ifndef BOOST_SMART_PTR_DETAIL_LWM_STD_MUTEX_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_LWM_STD_MUTEX_HPP_INCLUDED

// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt)

#include <boost/assert.hpp>
#include <mutex>

namespace boost
{

namespace detail
{

class lightweight_mutex
{
private:

    std::mutex m_;

    lightweight_mutex(lightweight_mutex const &);
    lightweight_mutex & operator=(lightweight_mutex const &);

public:

    lightweight_mutex()
    {
    }

    class scoped_lock;
    friend class scoped_lock;

    class scoped_lock
    {
    private:

        std::mutex & m_;

        scoped_lock(scoped_lock const &);
        scoped_lock & operator=(scoped_lock const &);

    public:

        scoped_lock( lightweight_mutex & m ): m_( m.m_ )
        {
            m_.lock();
        }

        ~scoped_lock()
        {
            m_.unlock();
        }
    };
};

} // namespace detail

} // namespace boost

#endif // #ifndef BOOST_SMART_PTR_DETAIL_LWM_STD_MUTEX_HPP_INCLUDED
