//  Copyright Bryce Lelbach 2010
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_EXT_IS_SORTED_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_EXT_IS_SORTED_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/value_type.hpp>
#include <boost/detail/is_sorted.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function is_sorted
///
/// range-based version of the is_sorted std algorithm
///
/// \pre SinglePassRange is a model of the SinglePassRangeConcept
template<class SinglePassRange>
inline bool is_sorted(const SinglePassRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<const SinglePassRange>));
    BOOST_RANGE_CONCEPT_ASSERT((LessThanComparableConcept<BOOST_DEDUCED_TYPENAME
      range_value<const SinglePassRange>::type>));
    return ::boost::detail::is_sorted(boost::begin(rng), boost::end(rng));
}

/// \overload
template<class SinglePassRange, class BinaryPredicate>
inline bool is_sorted(const SinglePassRange& rng, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<const SinglePassRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
      BOOST_DEDUCED_TYPENAME range_value<const SinglePassRange>::type,
      BOOST_DEDUCED_TYPENAME range_value<const SinglePassRange>::type>));
    return ::boost::detail::is_sorted(boost::begin(rng), boost::end(rng), pred);
}

    } // namespace range

using range::is_sorted;

} // namespace boost

#endif // include guard
