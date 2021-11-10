//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_PARTIAL_SORT_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_PARTIAL_SORT_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function partial_sort
///
/// range-based version of the partial_sort std algorithm
///
/// \pre RandomAccessRange is a model of the RandomAccessRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template<class RandomAccessRange>
inline RandomAccessRange& partial_sort(RandomAccessRange& rng,
    BOOST_DEDUCED_TYPENAME range_iterator<RandomAccessRange>::type middle)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::partial_sort(boost::begin(rng), middle, boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange>
inline const RandomAccessRange& partial_sort(const RandomAccessRange& rng,
    BOOST_DEDUCED_TYPENAME range_iterator<const RandomAccessRange>::type middle)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::partial_sort(boost::begin(rng), middle, boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange, class BinaryPredicate>
inline RandomAccessRange& partial_sort(RandomAccessRange& rng,
    BOOST_DEDUCED_TYPENAME range_iterator<RandomAccessRange>::type middle,
    BinaryPredicate sort_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::partial_sort(boost::begin(rng), middle, boost::end(rng),
                        sort_pred);
    return rng;
}

/// \overload
template<class RandomAccessRange, class BinaryPredicate>
inline const RandomAccessRange& partial_sort(const RandomAccessRange& rng,
    BOOST_DEDUCED_TYPENAME range_iterator<const RandomAccessRange>::type middle,
    BinaryPredicate sort_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::partial_sort(boost::begin(rng), middle, boost::end(rng),
                        sort_pred);
    return rng;
}

    } // namespace range
    using range::partial_sort;
} // namespace boost

#endif // include guard
