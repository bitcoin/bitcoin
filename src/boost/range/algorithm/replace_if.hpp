//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_REPLACE_IF_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_REPLACE_IF_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function replace_if
///
/// range-based version of the replace_if std algorithm
///
/// \pre ForwardRange is a model of the ForwardRangeConcept
/// \pre UnaryPredicate is a model of the UnaryPredicateConcept
template< class ForwardRange, class UnaryPredicate, class Value >
inline ForwardRange&
    replace_if(ForwardRange& rng, UnaryPredicate pred,
               const Value& val)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    std::replace_if(boost::begin(rng), boost::end(rng), pred, val);
    return rng;
}

/// \overload
template< class ForwardRange, class UnaryPredicate, class Value >
inline const ForwardRange&
    replace_if(const ForwardRange& rng, UnaryPredicate pred,
               const Value& val)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    std::replace_if(boost::begin(rng), boost::end(rng), pred, val);
    return rng;
}

    } // namespace range
    using range::replace_if;
} // namespace boost

#endif // include guard
