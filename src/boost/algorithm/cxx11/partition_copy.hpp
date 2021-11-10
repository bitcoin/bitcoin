/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/// \file  partition_copy.hpp
/// \brief Copy a subset of a sequence to a new sequence
/// \author Marshall Clow

#ifndef BOOST_ALGORITHM_PARTITION_COPY_HPP
#define BOOST_ALGORITHM_PARTITION_COPY_HPP

#include <utility>  // for std::pair

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost { namespace algorithm {

/// \fn partition_copy ( InputIterator first, InputIterator last,
///     OutputIterator1 out_true, OutputIterator2 out_false, UnaryPredicate p )
/// \brief Copies the elements that satisfy the predicate p from the range [first, last) 
///     to the range beginning at d_first_true, and
///     copies the elements that do not satisfy p to the range beginning at d_first_false.
///
/// 
/// \param first     The start of the input sequence
/// \param last      One past the end of the input sequence
/// \param out_true  An output iterator to write the elements that satisfy the predicate into
/// \param out_false An output iterator to write the elements that do not satisfy the predicate into
/// \param p         A predicate for dividing the elements of the input sequence.
///
/// \note            This function is part of the C++2011 standard library.
template <typename InputIterator, 
        typename OutputIterator1, typename OutputIterator2, typename UnaryPredicate>
BOOST_CXX14_CONSTEXPR std::pair<OutputIterator1, OutputIterator2>
partition_copy ( InputIterator first, InputIterator last,
        OutputIterator1 out_true, OutputIterator2 out_false, UnaryPredicate p )
{
    for ( ; first != last; ++first )
        if ( p (*first))
            *out_true++ = *first;
        else
            *out_false++ = *first;
    return std::pair<OutputIterator1, OutputIterator2> ( out_true, out_false );
}

/// \fn partition_copy ( const Range &r, 
///     OutputIterator1 out_true, OutputIterator2 out_false, UnaryPredicate p )
/// 
/// \param r         The input range
/// \param out_true  An output iterator to write the elements that satisfy the predicate into
/// \param out_false An output iterator to write the elements that do not satisfy the predicate into
/// \param p         A predicate for dividing the elements of the input sequence.
///
template <typename Range, typename OutputIterator1, typename OutputIterator2, 
            typename UnaryPredicate>
BOOST_CXX14_CONSTEXPR std::pair<OutputIterator1, OutputIterator2>
partition_copy ( const Range &r, OutputIterator1 out_true, OutputIterator2 out_false, 
                                UnaryPredicate p )
{
    return boost::algorithm::partition_copy 
                      (boost::begin(r), boost::end(r), out_true, out_false, p );
}

}} // namespace boost and algorithm

#endif  // BOOST_ALGORITHM_PARTITION_COPY_HPP
