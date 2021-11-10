/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/// \file  find_if_not.hpp
/// \brief Find the first element in a sequence that does not satisfy a predicate.
/// \author Marshall Clow

#ifndef BOOST_ALGORITHM_FIND_IF_NOT_HPP
#define BOOST_ALGORITHM_FIND_IF_NOT_HPP

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost { namespace algorithm {

/// \fn find_if_not(InputIterator first, InputIterator last, Predicate p)
/// \brief Finds the first element in the sequence that does not satisfy the predicate.
/// \return         The iterator pointing to the desired element.
/// 
/// \param first    The start of the input sequence
/// \param last     One past the end of the input sequence
/// \param p        A predicate for testing the elements of the range
/// \note           This function is part of the C++2011 standard library.
template<typename InputIterator, typename Predicate> 
BOOST_CXX14_CONSTEXPR InputIterator find_if_not ( InputIterator first, InputIterator last, Predicate p )
{
    for ( ; first != last; ++first )
        if ( !p(*first))
            break;
    return first;
}

/// \fn find_if_not ( const Range &r, Predicate p )
/// \brief Finds the first element in the sequence that does not satisfy the predicate.
/// \return         The iterator pointing to the desired element.
/// 
/// \param r        The input range
/// \param p        A predicate for testing the elements of the range
///
template<typename Range, typename Predicate>
BOOST_CXX14_CONSTEXPR typename boost::range_iterator<const Range>::type find_if_not ( const Range &r, Predicate p )
{
    return boost::algorithm::find_if_not (boost::begin (r), boost::end(r), p);
}

}}
#endif  // BOOST_ALGORITHM_FIND_IF_NOT_HPP
