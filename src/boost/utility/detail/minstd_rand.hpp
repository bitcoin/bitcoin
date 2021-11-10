#ifndef BOOST_UTILITY_DETAIL_MINSTD_RAND_HPP_INCLUDED
#define BOOST_UTILITY_DETAIL_MINSTD_RAND_HPP_INCLUDED

// Copyright 2017 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// An implementation of minstd_rand that does not require
// the Random library

#include <boost/cstdint.hpp>

namespace boost
{
namespace detail
{

class minstd_rand
{
private:

    boost::uint_least32_t x_;

    enum { a = 48271, m = 2147483647 };

public:

    minstd_rand(): x_( 1 )
    {
    }

    explicit minstd_rand( boost::uint_least32_t x ): x_( x % m )
    {
        if( x_ == 0 )
        {
            x_ = 1;
        }
    }

    boost::uint_least32_t operator()()
    {
        boost::uint_least64_t y = x_;

        y = ( a * y ) % m;

        x_ = static_cast<boost::uint_least32_t>( y );

        return x_;
    }
};

} // namespace detail
} // namespace boost

#endif // #ifndef BOOST_UTILITY_DETAIL_MINSTD_RAND_HPP_INCLUDED
