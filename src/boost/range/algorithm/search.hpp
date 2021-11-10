//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_SEARCH_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_SEARCH_HPP_INCLUDED

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

/// \brief template function search
///
/// range-based version of the search std algorithm
///
/// \pre ForwardRange1 is a model of the ForwardRangeConcept
/// \pre ForwardRange2 is a model of the ForwardRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template< class ForwardRange1, class ForwardRange2 >
inline BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange1>::type
search(ForwardRange1& rng1, const ForwardRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));
    return std::search(boost::begin(rng1),boost::end(rng1),
                       boost::begin(rng2),boost::end(rng2));
}

/// \overload
template< class ForwardRange1, class ForwardRange2 >
inline BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange1>::type
search(const ForwardRange1& rng1, const ForwardRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));
    return std::search(boost::begin(rng1), boost::end(rng1),
                       boost::begin(rng2), boost::end(rng2));
}

/// \overload
template< class ForwardRange1, class ForwardRange2, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange1>::type
search(ForwardRange1& rng1, const ForwardRange2& rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));
    return std::search(boost::begin(rng1),boost::end(rng1),
                       boost::begin(rng2),boost::end(rng2),pred);
}

/// \overload
template< class ForwardRange1, class ForwardRange2, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange1>::type
search(const ForwardRange1& rng1, const ForwardRange2& rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));
    return std::search(boost::begin(rng1), boost::end(rng1),
                       boost::begin(rng2), boost::end(rng2), pred);
}

// range_return overloads

/// \overload
template< range_return_value re, class ForwardRange1, class ForwardRange2 >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange1,re>::type
search(ForwardRange1& rng1, const ForwardRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));
    return range_return<ForwardRange1,re>::
        pack(std::search(boost::begin(rng1),boost::end(rng1),
                         boost::begin(rng2),boost::end(rng2)),
             rng1);
}

/// \overload
template< range_return_value re, class ForwardRange1, class ForwardRange2 >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange1,re>::type
search(const ForwardRange1& rng1, const ForwardRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));
    return range_return<const ForwardRange1,re>::
        pack(std::search(boost::begin(rng1),boost::end(rng1),
                         boost::begin(rng2),boost::end(rng2)),
             rng1);
}

/// \overload
template< range_return_value re, class ForwardRange1, class ForwardRange2,
          class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange1,re>::type
search(ForwardRange1& rng1, const ForwardRange2& rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<ForwardRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));
    return range_return<ForwardRange1,re>::
        pack(std::search(boost::begin(rng1),boost::end(rng1),
                         boost::begin(rng2),boost::end(rng2),pred),
             rng1);
}

/// \overload
template< range_return_value re, class ForwardRange1, class ForwardRange2,
          class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange1,re>::type
search(const ForwardRange1& rng1, const ForwardRange2& rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));
    return range_return<const ForwardRange1,re>::
        pack(std::search(boost::begin(rng1),boost::end(rng1),
                         boost::begin(rng2),boost::end(rng2),pred),
             rng1);
}

    } // namespace range
    using range::search;
} // namespace boost

#endif // include guard
