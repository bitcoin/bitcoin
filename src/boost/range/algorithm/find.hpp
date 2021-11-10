//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_FIND_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_FIND_HPP_INCLUDED

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

/// \brief template function find
///
/// range-based version of the find std algorithm
///
/// \pre SinglePassRange is a model of the SinglePassRangeConcept
template< class SinglePassRange, class Value >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<SinglePassRange>,
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange>::type
>::type
find( SinglePassRange& rng, const Value& val )
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange> ));
    return std::find(boost::begin(rng), boost::end(rng), val);
}

/// \overload
template< class SinglePassRange, class Value >
inline BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange>::type
find( const SinglePassRange& rng, const Value& val )
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange> ));
    return std::find(boost::begin(rng), boost::end(rng), val);
}

// range_return overloads

/// \overload
template< range_return_value re, class SinglePassRange, class Value >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<SinglePassRange>,
    BOOST_DEDUCED_TYPENAME range_return<SinglePassRange,re>::type
>::type
find( SinglePassRange& rng, const Value& val )
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange> ));
    return range_return<SinglePassRange,re>::
        pack(std::find(boost::begin(rng), boost::end(rng), val),
             rng);
}

/// \overload
template< range_return_value re, class SinglePassRange, class Value >
inline BOOST_DEDUCED_TYPENAME range_return<const SinglePassRange,re>::type
find( const SinglePassRange& rng, const Value& val )
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange> ));
    return range_return<const SinglePassRange,re>::
        pack(std::find(boost::begin(rng), boost::end(rng), val),
             rng);
}

    } // namespace range
    using range::find;
}

#endif // include guard
