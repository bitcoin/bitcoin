//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_BINARY_SEARCH_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_BINARY_SEARCH_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function binary_search
///
/// range-based version of the binary_search std algorithm
///
/// \pre ForwardRange is a model of the ForwardRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template<class ForwardRange, class Value>
inline bool binary_search(const ForwardRange& rng, const Value& val)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return std::binary_search(boost::begin(rng), boost::end(rng), val);
}

/// \overload
template<class ForwardRange, class Value, class BinaryPredicate>
inline bool binary_search(const ForwardRange& rng, const Value& val,
                          BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return std::binary_search(boost::begin(rng), boost::end(rng), val, pred);
}

    } // namespace range
    using range::binary_search;
} // namespace boost

#endif // include guard
