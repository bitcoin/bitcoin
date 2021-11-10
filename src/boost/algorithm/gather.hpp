/* 
    Copyright 2008 Adobe Systems Incorporated

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

 Revision history:
   January 2008 mtc Version for Adobe Source Library
   January 2013 mtc Version for Boost.Algorithm

*/

/**************************************************************************************************/

/*!
\author Marshall Clow
\date    January 2008
*/

#ifndef BOOST_ALGORITHM_GATHER_HPP
#define BOOST_ALGORITHM_GATHER_HPP

#include <algorithm>                // for std::stable_partition
#include <functional>
#include <utility>                  // for std::make_pair

#include <boost/config.hpp>
#include <boost/bind/bind.hpp>      // for boost::bind
#include <boost/range/begin.hpp>    // for boost::begin(range)
#include <boost/range/end.hpp>      // for boost::end(range)


/**************************************************************************************************/
/*!
    \defgroup gather gather
    \ingroup mutating_algorithm

    \c gather() takes a collection of elements defined by a pair of iterators and moves
    the ones satisfying a predicate to them to a position (called the pivot) within
    the sequence. The algorithm is stable. The result is a pair of iterators that
    contains the items that satisfy the predicate.

    Given an sequence containing:
    <pre>
    0 1 2 3 4 5 6 7 8 9
    </pre>

    a call to gather ( arr, arr + 10, arr + 4, IsEven ()) will result in:

    <pre>
    1 3 0 2 4 6 8 5 7 9
        |---|-----|
      first |  second
          pivot
    </pre>


    The problem is broken down into two basic steps, namely, moving the items before the pivot
    and then moving the items from the pivot to the end. These "moves" are done with calls to
    stable_partition.

    \par Storage Requirements:

    The algorithm uses stable_partition, which will attempt to allocate temporary memory,
    but will work in-situ if there is none available.

    \par Time Complexity:

    If there is sufficient memory available, the run time is linear in <code>N</code>.
    If there is not any memory available, then the run time is <code>O(N log N)</code>.
*/

/**************************************************************************************************/

namespace boost { namespace algorithm {

/**************************************************************************************************/

/*!
    \ingroup gather
    \brief iterator-based gather implementation
*/

template <
    typename BidirectionalIterator,  // models BidirectionalIterator
    typename Pred>                   // models UnaryPredicate
std::pair<BidirectionalIterator, BidirectionalIterator> gather 
        ( BidirectionalIterator first, BidirectionalIterator last, BidirectionalIterator pivot, Pred pred )
{
//  The first call partitions everything up to (but not including) the pivot element,
//  while the second call partitions the rest of the sequence.
    using namespace boost::placeholders;
    return std::make_pair (
        std::stable_partition ( first, pivot, !boost::bind<bool> ( pred, _1 )),
        std::stable_partition ( pivot, last,   boost::bind<bool> ( pred, _1 )));
}

/**************************************************************************************************/

/*!
    \ingroup gather
    \brief range-based gather implementation
*/

template <
    typename BidirectionalRange,    //
    typename Pred>                  // Pred models UnaryPredicate
std::pair<
    typename boost::range_iterator<const BidirectionalRange>::type,
    typename boost::range_iterator<const BidirectionalRange>::type>
gather (
    const BidirectionalRange &range,
    typename boost::range_iterator<const BidirectionalRange>::type pivot,
    Pred pred )
{
    return boost::algorithm::gather ( boost::begin ( range ), boost::end ( range ), pivot, pred );
}

/**************************************************************************************************/

}}  // namespace

/**************************************************************************************************/

#endif

