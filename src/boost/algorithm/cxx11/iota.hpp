/* 
   Copyright (c) Marshall Clow 2008-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/// \file  iota.hpp
/// \brief Generate an increasing series
/// \author Marshall Clow

#ifndef BOOST_ALGORITHM_IOTA_HPP
#define BOOST_ALGORITHM_IOTA_HPP

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost { namespace algorithm {

/// \fn iota ( ForwardIterator first, ForwardIterator last, T value )
/// \brief Generates an increasing sequence of values, and stores them in [first, last)
/// 
/// \param first    The start of the input sequence
/// \param last     One past the end of the input sequence
/// \param value    The initial value of the sequence to be generated
/// \note           This function is part of the C++2011 standard library.
template <typename ForwardIterator, typename T>
BOOST_CXX14_CONSTEXPR void iota ( ForwardIterator first, ForwardIterator last, T value )
{
    for ( ; first != last; ++first, ++value )
        *first = value;
}

/// \fn iota ( Range &r, T value )
/// \brief Generates an increasing sequence of values, and stores them in the input Range.
/// 
/// \param r        The input range
/// \param value    The initial value of the sequence to be generated
///
template <typename Range, typename T>
BOOST_CXX14_CONSTEXPR void iota ( Range &r, T value )
{
    boost::algorithm::iota (boost::begin(r), boost::end(r), value);
}


/// \fn iota_n ( OutputIterator out, T value, std::size_t n )
/// \brief Generates an increasing sequence of values, and stores them in the input Range.
/// 
/// \param out      An output iterator to write the results into
/// \param value    The initial value of the sequence to be generated
/// \param n        The number of items to write
///
template <typename OutputIterator, typename T>
BOOST_CXX14_CONSTEXPR OutputIterator iota_n ( OutputIterator out, T value, std::size_t n )
{
    for ( ; n > 0; --n, ++value )
        *out++ = value;

    return out;
}

}}

#endif  // BOOST_ALGORITHM_IOTA_HPP
