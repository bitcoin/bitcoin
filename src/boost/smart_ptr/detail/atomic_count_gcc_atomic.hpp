#ifndef BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_GCC_ATOMIC_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_GCC_ATOMIC_HPP_INCLUDED

// boost/detail/atomic_count_gcc_atomic.hpp
//
// atomic_count for g++ 4.7+
//
// Copyright 2007, 2020 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/cstdint.hpp>

#if defined(BOOST_SP_REPORT_IMPLEMENTATION)

#include <boost/config/pragma_message.hpp>
BOOST_PRAGMA_MESSAGE("Using __atomic atomic_count")

#endif

namespace boost
{

namespace detail
{

class atomic_count
{
public:

    explicit atomic_count( long v ): value_( static_cast< boost::int_least32_t >( v ) )
    {
    }

    long operator++()
    {
        return __atomic_add_fetch( &value_, +1, __ATOMIC_ACQ_REL );
    }

    long operator--()
    {
        return __atomic_add_fetch( &value_, -1, __ATOMIC_ACQ_REL );
    }

    operator long() const
    {
        return __atomic_load_n( &value_, __ATOMIC_ACQUIRE );
    }

private:

    atomic_count(atomic_count const &);
    atomic_count & operator=(atomic_count const &);

    boost::int_least32_t value_;
};

} // namespace detail

} // namespace boost

#endif // #ifndef BOOST_SMART_PTR_DETAIL_ATOMIC_COUNT_GCC_ATOMIC_HPP_INCLUDED
