//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_ADJACENT_FIND_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_ADJACENT_FIND_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/value_type.hpp>
#include <boost/range/detail/range_return.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function adjacent_find
///
/// range-based version of the adjacent_find std algorithm
///
/// \pre ForwardRange is a model of the ForwardRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template< typename ForwardRange >
inline typename range_iterator<ForwardRange>::type
adjacent_find(ForwardRange & rng)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    return std::adjacent_find(boost::begin(rng),boost::end(rng));
}

/// \overload
template< typename ForwardRange >
inline typename range_iterator<const ForwardRange>::type
adjacent_find(const ForwardRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    return std::adjacent_find(boost::begin(rng),boost::end(rng));
}

/// \overload
template< typename ForwardRange, typename BinaryPredicate >
inline typename range_iterator<ForwardRange>::type
adjacent_find(ForwardRange & rng, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        typename range_value<ForwardRange>::type,
        typename range_value<ForwardRange>::type>));
    return std::adjacent_find(boost::begin(rng),boost::end(rng),pred);
}

/// \overload
template< typename ForwardRange, typename BinaryPredicate >
inline typename range_iterator<const ForwardRange>::type
adjacent_find(const ForwardRange& rng, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        typename range_value<const ForwardRange>::type,
        typename range_value<const ForwardRange>::type>));
    return std::adjacent_find(boost::begin(rng),boost::end(rng),pred);
}

//  range_return overloads

/// \overload
template< range_return_value re, typename ForwardRange >
inline typename range_return<ForwardRange,re>::type
adjacent_find(ForwardRange & rng)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    return range_return<ForwardRange,re>::
        pack(std::adjacent_find(boost::begin(rng),boost::end(rng)),
             rng);
}

/// \overload
template< range_return_value re, typename ForwardRange >
inline typename range_return<const ForwardRange,re>::type
adjacent_find(const ForwardRange& rng)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    return range_return<const ForwardRange,re>::
        pack(std::adjacent_find(boost::begin(rng),boost::end(rng)),
             rng);
}

/// \overload
template< range_return_value re, typename ForwardRange, typename BinaryPredicate >
inline typename range_return<ForwardRange,re>::type
adjacent_find(ForwardRange& rng, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        typename range_value<ForwardRange>::type,
        typename range_value<ForwardRange>::type>));
    return range_return<ForwardRange,re>::
        pack(std::adjacent_find(boost::begin(rng),boost::end(rng),pred),
             rng);
}

/// \overload
template< range_return_value re, typename ForwardRange, typename BinaryPredicate >
inline typename range_return<const ForwardRange,re>::type
adjacent_find(const ForwardRange& rng, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    return range_return<const ForwardRange,re>::
        pack(std::adjacent_find(boost::begin(rng),boost::end(rng),pred),
             rng);
}

    } // namespace range
    using range::adjacent_find;
} // namespace boost

#endif // include guard
