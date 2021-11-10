/*
 *  (C) Copyright Nick Thompson 2018.
 *  Use, modification and distribution are subject to the
 *  Boost Software License, Version 1.0. (See accompanying file
 *  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef BOOST_INTEGER_EXTENDED_EUCLIDEAN_HPP
#define BOOST_INTEGER_EXTENDED_EUCLIDEAN_HPP
#include <limits>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/core/swap.hpp>
#include <boost/core/enable_if.hpp>

namespace boost { namespace integer {

// From "The Joy of Factoring", Algorithm 2.7, with a small optimization to remove tmps from Wikipedia.
// Solves mx + ny = gcd(m,n). Returns tuple with (gcd(m,n), x, y).

template<class Z>
struct euclidean_result_t
{
    Z gcd;
    Z x;
    Z y;
};

template<class Z>
typename boost::enable_if_c< std::numeric_limits< Z >::is_signed, euclidean_result_t< Z > >::type
extended_euclidean(Z m, Z n)
{
    if (m < 1 || n < 1)
    {
        BOOST_THROW_EXCEPTION(std::domain_error("extended_euclidean: arguments must be strictly positive"));
    }

    bool swapped = false;
    if (m < n)
    {
        swapped = true;
        boost::swap(m, n);
    }
    Z u0 = m;
    Z u1 = 1;
    Z u2 = 0;
    Z v0 = n;
    Z v1 = 0;
    Z v2 = 1;
    Z w0;
    Z w1;
    Z w2;
    while(v0 > 0)
    {
        Z q = u0/v0;
        w0 = u0 - q*v0;
        w1 = u1 - q*v1;
        w2 = u2 - q*v2;
        u0 = v0;
        u1 = v1;
        u2 = v2;
        v0 = w0;
        v1 = w1;
        v2 = w2;
    }

    euclidean_result_t< Z > result;
    result.gcd = u0;
    if (!swapped)
    {
        result.x = u1;
        result.y = u2;
    }
    else
    {
        result.x = u2;
        result.y = u1;
    }

    return result;
}

}}
#endif
