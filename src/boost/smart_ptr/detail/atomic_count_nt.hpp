#ifndef BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_NT_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_NT_HPP_INCLUDED

//
//  boost/detail/atomic_count_nt.hpp
//
//  Trivial atomic_count for the single-threaded case
//
//  http://gcc.gnu.org/onlinedocs/porting/Thread-safety.html
//
//  Copyright 2013 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using single-threaded, non-atomic atomic_count")

#endif

namespace boost
{

namespace detail
{

class atomic_count
{
public:

    explicit atomic_count( long v ): value_( v )
    {
    }

    long operator++()
    {
        return ++value_;
    }

    long operator--()
    {
        return --value_;
    }

    operator long() const
    {
        return value_;
    }

private:

    atomic_count(atomic_count const &);
    atomic_count & operator=(atomic_count const &);

    long value_;
};

} // namespace detail

} // namespace boost

#endif // #ifndef BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_NT_HPP_INCLUDED
