//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_MERGE_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_MERGE_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function merge
///
/// range-based version of the merge std algorithm
///
/// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
/// \pre SinglePassRange2 is a model of the SinglePassRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
///
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator>
inline OutputIterator merge(const SinglePassRange1& rng1,
                            const SinglePassRange2& rng2,
                            OutputIterator          out)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::merge(boost::begin(rng1), boost::end(rng1),
                      boost::begin(rng2), boost::end(rng2), out);
}

/// \overload
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator, class BinaryPredicate>
inline OutputIterator merge(const SinglePassRange1& rng1,
                            const SinglePassRange2& rng2,
                            OutputIterator          out,
                            BinaryPredicate         pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::merge(boost::begin(rng1), boost::end(rng1),
                      boost::begin(rng2), boost::end(rng2), out, pred);
}

    } // namespace range
    using range::merge;
} // namespace boost

#endif // include guard
