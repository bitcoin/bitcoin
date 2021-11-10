#ifndef BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_STD_ATOMIC_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_STD_ATOMIC_HPP_INCLUDED

//
//  boost/detail/atomic_count_std_atomic.hpp
//
//  atomic_count for std::atomic
//
//  Copyright 2013 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <atomic>
#include <cstdint>

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using std::atomic atomic_count")

#endif

namespace boost
{

namespace detail
{

class atomic_count
{
public:

    explicit atomic_count( long v ): value_( static_cast< std::int_least32_t >( v ) )
    {
    }

    long operator++()
    {
        return value_.fetch_add( 1, std::memory_order_acq_rel ) + 1;
    }

    long operator--()
    {
        return value_.fetch_sub( 1, std::memory_order_acq_rel ) - 1;
    }

    operator long() const
    {
        return value_.load( std::memory_order_acquire );
    }

private:

    atomic_count(atomic_count const &);
    atomic_count & operator=(atomic_count const &);

    std::atomic_int_least32_t value_;
};

} // namespace detail

} // namespace boost

#endif // #ifndef BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_STD_ATOMIC_HPP_INCLUDED
