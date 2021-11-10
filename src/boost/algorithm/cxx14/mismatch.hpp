/* 
   Copyright (c) Marshall Clow 2008-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/// \file  mismatch.hpp
/// \brief Find the first mismatched element in a sequence
/// \author Marshall Clow

#ifndef BOOST_ALGORITHM_MISMATCH_HPP
#define BOOST_ALGORITHM_MISMATCH_HPP

#include <utility>      // for std::pair

#include <boost/config.hpp>

namespace boost { namespace algorithm {

/// \fn mismatch ( InputIterator1 first1, InputIterator1 last1, 
///                InputIterator2 first2, InputIterator2 last2,
///                BinaryPredicate pred )
/// \return a pair of iterators pointing to the first elements in the sequence that do not match
/// 
/// \param first1    The start of the first range.
/// \param last1     One past the end of the first range.
/// \param first2    The start of the second range.
/// \param last2     One past the end of the second range.
/// \param pred      A predicate for comparing the elements of the ranges
template <class InputIterator1, class InputIterator2, class BinaryPredicate>
BOOST_CXX14_CONSTEXPR std::pair<InputIterator1, InputIterator2> mismatch (
                    InputIterator1 first1, InputIterator1 last1,
                    InputIterator2 first2, InputIterator2 last2,
                    BinaryPredicate pred )
{
    for (; first1 != last1 && first2 != last2; ++first1, ++first2)
        if ( !pred ( *first1, *first2 ))
            break;
    return std::pair<InputIterator1, InputIterator2>(first1, first2);
}

/// \fn mismatch ( InputIterator1 first1, InputIterator1 last1, 
///                InputIterator2 first2, InputIterator2 last2 )
/// \return a pair of iterators pointing to the first elements in the sequence that do not match
/// 
/// \param first1    The start of the first range.
/// \param last1     One past the end of the first range.
/// \param first2    The start of the second range.
/// \param last2     One past the end of the second range.
template <class InputIterator1, class InputIterator2>
BOOST_CXX14_CONSTEXPR std::pair<InputIterator1, InputIterator2> mismatch (
                    InputIterator1 first1, InputIterator1 last1,
                    InputIterator2 first2, InputIterator2 last2 )
{
    for (; first1 != last1 && first2 != last2; ++first1, ++first2)
        if ( *first1 != *first2 )
            break;
    return std::pair<InputIterator1, InputIterator2>(first1, first2);
}

//  There are already range-based versions of these.

}} // namespace boost and algorithm

#endif // BOOST_ALGORITHM_MISMATCH_HPP
