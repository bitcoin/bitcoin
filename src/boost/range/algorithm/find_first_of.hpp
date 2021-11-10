//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_FIND_FIRST_OF_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_FIND_FIRST_OF_HPP_INCLUDED

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

/// \brief template function find_first_of
///
/// range-based version of the find_first_of std algorithm
///
/// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
/// \pre ForwardRange2 is a model of the ForwardRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template< class SinglePassRange1, class ForwardRange2 >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<SinglePassRange1>,
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type
>::type
find_first_of(SinglePassRange1 & rng1, ForwardRange2 const & rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));

    return std::find_first_of(boost::begin(rng1),boost::end(rng1),
                              boost::begin(rng2),boost::end(rng2));
}

/// \overload
template< class SinglePassRange1, class ForwardRange2 >
inline BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange1>::type
find_first_of(const SinglePassRange1& rng1, const ForwardRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));

    return std::find_first_of(boost::begin(rng1),boost::end(rng1),
                              boost::begin(rng2),boost::end(rng2));
}

/// \overload
template< class SinglePassRange1, class ForwardRange2, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<SinglePassRange1>,
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type
>::type
find_first_of(SinglePassRange1 & rng1, ForwardRange2 const & rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));

    return std::find_first_of(boost::begin(rng1),boost::end(rng1),
                              boost::begin(rng2),boost::end(rng2),pred);
}

/// \overload
template< class SinglePassRange1, class ForwardRange2, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange1>::type
find_first_of(const SinglePassRange1& rng1, const ForwardRange2& rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));

    return std::find_first_of(boost::begin(rng1),boost::end(rng1),
                              boost::begin(rng2),boost::end(rng2),pred);
}

// range return overloads
/// \overload
template< range_return_value re, class SinglePassRange1, class ForwardRange2 >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<SinglePassRange1>,
    BOOST_DEDUCED_TYPENAME range_return<SinglePassRange1,re>::type
>::type
find_first_of(SinglePassRange1& rng1, const ForwardRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));

    return range_return<SinglePassRange1,re>::
        pack(std::find_first_of(boost::begin(rng1), boost::end(rng1),
                                boost::begin(rng2), boost::end(rng2)),
             rng1);
}

/// \overload
template< range_return_value re, class SinglePassRange1, class ForwardRange2 >
inline BOOST_DEDUCED_TYPENAME range_return<const SinglePassRange1,re>::type
find_first_of(const SinglePassRange1& rng1, const ForwardRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));

    return range_return<const SinglePassRange1,re>::
        pack(std::find_first_of(boost::begin(rng1), boost::end(rng1),
                                boost::begin(rng2), boost::end(rng2)),
             rng1);
}

/// \overload
template< range_return_value re, class SinglePassRange1, class ForwardRange2,
          class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME disable_if<
    is_const<SinglePassRange1>,
    BOOST_DEDUCED_TYPENAME range_return<SinglePassRange1,re>::type
>::type
find_first_of(SinglePassRange1 & rng1, const ForwardRange2& rng2,
              BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));

    return range_return<SinglePassRange1,re>::
        pack(std::find_first_of(boost::begin(rng1), boost::end(rng1),
                                boost::begin(rng2), boost::end(rng2), pred),
             rng1);
}

/// \overload
template< range_return_value re, class SinglePassRange1, class ForwardRange2,
          class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<const SinglePassRange1,re>::type
find_first_of(const SinglePassRange1 & rng1, const ForwardRange2& rng2,
              BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( ForwardRangeConcept<const ForwardRange2> ));

    return range_return<const SinglePassRange1,re>::
        pack(std::find_first_of(boost::begin(rng1), boost::end(rng1),
                                boost::begin(rng2), boost::end(rng2), pred),
             rng1);
}

    } // namespace range
    using range::find_first_of;
} // namespace boost

#endif // include guard
