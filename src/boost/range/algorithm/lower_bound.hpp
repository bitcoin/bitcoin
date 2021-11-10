//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_LOWER_BOUND_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_LOWER_BOUND_HPP_INCLUDED

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

/// \brief template function lower_bound
///
/// range-based version of the lower_bound std algorithm
///
/// \pre ForwardRange is a model of the ForwardRangeConcept
template< class ForwardRange, class Value >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<ForwardRange>,
    BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange>::type
>::type
lower_bound( ForwardRange& rng, const Value& val )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    return std::lower_bound(boost::begin(rng), boost::end(rng), val);
}

/// \overload
template< class ForwardRange, class Value >
inline BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange>::type
lower_bound( const ForwardRange& rng, const Value& val )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return std::lower_bound(boost::begin(rng), boost::end(rng), val);
}

/// \overload
template< class ForwardRange, class Value, class SortPredicate >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<ForwardRange>,
    BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange>::type
>::type
lower_bound( ForwardRange& rng, const Value& val, SortPredicate pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    return std::lower_bound(boost::begin(rng), boost::end(rng), val, pred);
}

/// \overload
template< class ForwardRange, class Value, class SortPredicate >
inline BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange>::type
lower_bound( const ForwardRange& rng, const Value& val, SortPredicate pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return std::lower_bound(boost::begin(rng), boost::end(rng), val, pred);
}

/// \overload
template< range_return_value re, class ForwardRange, class Value >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<ForwardRange>,
    BOOST_DEDUCED_TYPENAME range_return<ForwardRange,re>::type
>::type
lower_bound( ForwardRange& rng, const Value& val )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    return range_return<ForwardRange,re>::
        pack(std::lower_bound(boost::begin(rng), boost::end(rng), val),
             rng);
}

/// \overload
template< range_return_value re, class ForwardRange, class Value >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange,re>::type
lower_bound( const ForwardRange& rng, const Value& val )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return range_return<const ForwardRange,re>::
        pack(std::lower_bound(boost::begin(rng), boost::end(rng), val),
             rng);
}

/// \overload
template< range_return_value re, class ForwardRange, class Value, class SortPredicate >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<ForwardRange>,
    BOOST_DEDUCED_TYPENAME range_return<ForwardRange,re>::type
>::type
lower_bound( ForwardRange& rng, const Value& val, SortPredicate pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange> ));
    return range_return<ForwardRange,re>::
        pack(std::lower_bound(boost::begin(rng), boost::end(rng), val, pred),
             rng);
}

/// \overload
template< range_return_value re, class ForwardRange, class Value, class SortPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange,re>::type
lower_bound( const ForwardRange& rng, const Value& val, SortPredicate pred )
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange> ));
    return range_return<const ForwardRange,re>::
        pack(std::lower_bound(boost::begin(rng), boost::end(rng), val, pred),
             rng);
}

    } // namespace range
    using range::lower_bound;
} // namespace boost

#endif // include guard
