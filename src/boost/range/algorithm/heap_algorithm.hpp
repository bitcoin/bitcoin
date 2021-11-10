//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_HEAP_ALGORITHM_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_HEAP_ALGORITHM_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function push_heap
///
/// range-based version of the push_heap std algorithm
///
/// \pre RandomAccessRange is a model of the RandomAccessRangeConcept
/// \pre Compare is a model of the BinaryPredicateConcept
template<class RandomAccessRange>
inline RandomAccessRange& push_heap(RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::push_heap(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange>
inline const RandomAccessRange& push_heap(const RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::push_heap(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange, class Compare>
inline RandomAccessRange& push_heap(RandomAccessRange& rng, Compare comp_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::push_heap(boost::begin(rng), boost::end(rng), comp_pred);
    return rng;
}

/// \overload
template<class RandomAccessRange, class Compare>
inline const RandomAccessRange& push_heap(const RandomAccessRange& rng, Compare comp_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::push_heap(boost::begin(rng), boost::end(rng), comp_pred);
    return rng;
}

/// \brief template function pop_heap
///
/// range-based version of the pop_heap std algorithm
///
/// \pre RandomAccessRange is a model of the RandomAccessRangeConcept
/// \pre Compare is a model of the BinaryPredicateConcept
template<class RandomAccessRange>
inline RandomAccessRange& pop_heap(RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::pop_heap(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange>
inline const RandomAccessRange& pop_heap(const RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::pop_heap(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange, class Compare>
inline RandomAccessRange& pop_heap(RandomAccessRange& rng, Compare comp_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::pop_heap(boost::begin(rng), boost::end(rng), comp_pred);
    return rng;
}

/// \overload
template<class RandomAccessRange, class Compare>
inline const RandomAccessRange& pop_heap(const RandomAccessRange& rng, Compare comp_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::pop_heap(boost::begin(rng), boost::end(rng), comp_pred);
    return rng;
}

/// \brief template function make_heap
///
/// range-based version of the make_heap std algorithm
///
/// \pre RandomAccessRange is a model of the RandomAccessRangeConcept
/// \pre Compare is a model of the BinaryPredicateConcept
template<class RandomAccessRange>
inline RandomAccessRange& make_heap(RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::make_heap(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange>
inline const RandomAccessRange& make_heap(const RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::make_heap(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange, class Compare>
inline RandomAccessRange& make_heap(RandomAccessRange& rng, Compare comp_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::make_heap(boost::begin(rng), boost::end(rng), comp_pred);
    return rng;
}

/// \overload
template<class RandomAccessRange, class Compare>
inline const RandomAccessRange& make_heap(const RandomAccessRange& rng, Compare comp_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::make_heap(boost::begin(rng), boost::end(rng), comp_pred);
    return rng;
}

/// \brief template function sort_heap
///
/// range-based version of the sort_heap std algorithm
///
/// \pre RandomAccessRange is a model of the RandomAccessRangeConcept
/// \pre Compare is a model of the BinaryPredicateConcept
template<class RandomAccessRange>
inline RandomAccessRange& sort_heap(RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::sort_heap(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange>
inline const RandomAccessRange& sort_heap(const RandomAccessRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::sort_heap(boost::begin(rng), boost::end(rng));
    return rng;
}

/// \overload
template<class RandomAccessRange, class Compare>
inline RandomAccessRange& sort_heap(RandomAccessRange& rng, Compare comp_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<RandomAccessRange> ));
    std::sort_heap(boost::begin(rng), boost::end(rng), comp_pred);
    return rng;
}

/// \overload
template<class RandomAccessRange, class Compare>
inline const RandomAccessRange& sort_heap(const RandomAccessRange& rng, Compare comp_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( RandomAccessRangeConcept<const RandomAccessRange> ));
    std::sort_heap(boost::begin(rng), boost::end(rng), comp_pred);
    return rng;
}

    } // namespace range
    using range::push_heap;
    using range::pop_heap;
    using range::make_heap;
    using range::sort_heap;
} // namespace boost

#endif // include guard
