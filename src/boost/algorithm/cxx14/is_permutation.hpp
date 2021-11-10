/*
   Copyright (c) Marshall Clow 2014.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/// \file  is_permutation.hpp
/// \brief Is a sequence a permutation of another sequence (four iterator versions)
/// \author Marshall Clow

#ifndef BOOST_ALGORITHM_IS_PERMUTATION14_HPP
#define BOOST_ALGORITHM_IS_PERMUTATION14_HPP

#include <utility>      // for std::pair
#include <functional>   // for std::equal_to
#include <iterator>

#include <boost/config.hpp>
#include <boost/algorithm/cxx11/is_permutation.hpp>
#include <boost/algorithm/cxx14/mismatch.hpp>

namespace boost { namespace algorithm {

/// \fn is_permutation ( ForwardIterator1 first, ForwardIterator1 last, 
///                      ForwardIterator2 first2, ForwardIterator2 last2 )
/// \brief Tests to see if the sequence [first,last) is a permutation of the sequence starting at first2
///
/// \param first1   The start of the input sequence
/// \param last2    One past the end of the input sequence
/// \param first2   The start of the second sequence
/// \param last1    One past the end of the second sequence
/// \note           This function is part of the C++2014 standard library.
template< class ForwardIterator1, class ForwardIterator2 >
bool is_permutation ( ForwardIterator1 first1, ForwardIterator1 last1, 
                      ForwardIterator2 first2, ForwardIterator2 last2 )
{
//  How should I deal with the idea that ForwardIterator1::value_type
//  and ForwardIterator2::value_type could be different? Define my own comparison predicate?
    std::pair<ForwardIterator1, ForwardIterator2> eq = boost::algorithm::mismatch
        ( first1, last1, first2, last2 );
    if ( eq.first == last1 && eq.second == last2)
        return true;
    return boost::algorithm::detail::is_permutation_tag (
        eq.first, last1, eq.second, last2, 
        std::equal_to<typename std::iterator_traits<ForwardIterator1>::value_type> (),
        typename std::iterator_traits<ForwardIterator1>::iterator_category (),
        typename std::iterator_traits<ForwardIterator2>::iterator_category ());
}

/// \fn is_permutation ( ForwardIterator1 first, ForwardIterator1 last, 
///                      ForwardIterator2 first2, ForwardIterator2 last2, 
///                      BinaryPredicate p )
/// \brief Tests to see if the sequence [first,last) is a permutation of the sequence starting at first2
///
/// \param first1   The start of the input sequence
/// \param last1    One past the end of the input sequence
/// \param first2   The start of the second sequence
/// \param last2    One past the end of the second sequence
/// \param pred     The predicate to compare elements with
///
/// \note           This function is part of the C++2014 standard library.
template< class ForwardIterator1, class ForwardIterator2, class BinaryPredicate >
bool is_permutation ( ForwardIterator1 first1, ForwardIterator1 last1,
                      ForwardIterator2 first2, ForwardIterator2 last2, 
                      BinaryPredicate pred )
{
    std::pair<ForwardIterator1, ForwardIterator2> eq = boost::algorithm::mismatch
        ( first1, last1, first2, last2, pred );
    if ( eq.first == last1 && eq.second == last2)
        return true;
    return boost::algorithm::detail::is_permutation_tag (
        first1, last1, first2, last2, pred, 
        typename std::iterator_traits<ForwardIterator1>::iterator_category (),
        typename std::iterator_traits<ForwardIterator2>::iterator_category ());
}

}}

#endif  // BOOST_ALGORITHM_IS_PERMUTATION14_HPP
