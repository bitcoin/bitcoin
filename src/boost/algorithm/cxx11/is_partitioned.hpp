/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/// \file  is_partitioned.hpp
/// \brief Tell if a sequence is partitioned
/// \author Marshall Clow

#ifndef BOOST_ALGORITHM_IS_PARTITIONED_HPP
#define BOOST_ALGORITHM_IS_PARTITIONED_HPP

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost { namespace algorithm {

/// \fn is_partitioned ( InputIterator first, InputIterator last, UnaryPredicate p )
/// \brief Tests to see if a sequence is partitioned according to a predicate. 
///	   In other words, all the items in the sequence that satisfy the predicate are at the beginning of the sequence.
/// 
/// \param first    The start of the input sequence
/// \param last     One past the end of the input sequence
/// \param p        The predicate to test the values with
/// \note           This function is part of the C++2011 standard library.
template <typename InputIterator, typename UnaryPredicate>
BOOST_CXX14_CONSTEXPR bool is_partitioned ( InputIterator first, InputIterator last, UnaryPredicate p )
{
//  Run through the part that satisfy the predicate
    for ( ; first != last; ++first )
        if ( !p (*first))
            break;
//  Now the part that does not satisfy the predicate
    for ( ; first != last; ++first )
        if ( p (*first))
            return false;
    return true;
}

/// \fn is_partitioned ( const Range &r, UnaryPredicate p )
/// \brief Tests to see if a sequence is partitioned according to a predicate. 
///	   In other words, all the items in the sequence that satisfy the predicate are at the beginning of the sequence.
/// 
/// \param r        The input range
/// \param p        The predicate to test the values with
///
template <typename Range, typename UnaryPredicate>
BOOST_CXX14_CONSTEXPR bool is_partitioned ( const Range &r, UnaryPredicate p )
{
    return boost::algorithm::is_partitioned (boost::begin(r), boost::end(r), p);
}


}}

#endif  // BOOST_ALGORITHM_IS_PARTITIONED_HPP
