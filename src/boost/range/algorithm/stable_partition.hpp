//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_STABLE_PARTITION_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_STABLE_PARTITION_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/detail/range_return.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function stable_partition
///
/// range-based version of the stable_partition std algorithm
///
/// \pre BidirectionalRange is a model of the BidirectionalRangeConcept
/// \pre UnaryPredicate is a model of the UnaryPredicateConcept
template<class BidirectionalRange, class UnaryPredicate>
inline BOOST_DEDUCED_TYPENAME range_iterator<BidirectionalRange>::type
stable_partition(BidirectionalRange& rng, UnaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( BidirectionalRangeConcept<BidirectionalRange> ));
    return std::stable_partition(boost::begin(rng), boost::end(rng), pred);
}

/// \overload
template<class BidirectionalRange, class UnaryPredicate>
inline BOOST_DEDUCED_TYPENAME range_iterator<const BidirectionalRange>::type
stable_partition(const BidirectionalRange& rng, UnaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( BidirectionalRangeConcept<const BidirectionalRange> ));
    return std::stable_partition(boost::begin(rng),boost::end(rng),pred);
}

// range_return overloads
template<range_return_value re, class BidirectionalRange, class UnaryPredicate>
inline BOOST_DEDUCED_TYPENAME range_return<BidirectionalRange,re>::type
stable_partition(BidirectionalRange& rng, UnaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( BidirectionalRangeConcept<BidirectionalRange> ));
    return range_return<BidirectionalRange,re>::pack(
        std::stable_partition(boost::begin(rng), boost::end(rng), pred),
        rng);
}

/// \overload
template<range_return_value re, class BidirectionalRange, class UnaryPredicate>
inline BOOST_DEDUCED_TYPENAME range_return<const BidirectionalRange,re>::type
stable_partition(const BidirectionalRange& rng, UnaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( BidirectionalRangeConcept<const BidirectionalRange> ));
    return range_return<const BidirectionalRange,re>::pack(
        std::stable_partition(boost::begin(rng),boost::end(rng),pred),
        rng);
}

    } // namespace range
    using range::stable_partition;
} // namespace boost

#endif // include guard
