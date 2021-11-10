//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_SET_ALGORITHM_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_SET_ALGORITHM_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function includes
///
/// range-based version of the includes std algorithm
///
/// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
/// \pre SinglePassRange2 is a model of the SinglePassRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template<class SinglePassRange1, class SinglePassRange2>
inline bool includes(const SinglePassRange1& rng1,
                     const SinglePassRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::includes(boost::begin(rng1),boost::end(rng1),
                         boost::begin(rng2),boost::end(rng2));
}

/// \overload
template<class SinglePassRange1, class SinglePassRange2,
         class BinaryPredicate>
inline bool includes(const SinglePassRange1& rng1,
                     const SinglePassRange2& rng2,
                     BinaryPredicate         pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::includes(boost::begin(rng1), boost::end(rng1),
                         boost::begin(rng2), boost::end(rng2), pred);
}

/// \brief template function set_union
///
/// range-based version of the set_union std algorithm
///
/// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
/// \pre SinglePassRange2 is a model of the SinglePassRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator>
inline OutputIterator set_union(const SinglePassRange1& rng1,
                                const SinglePassRange2& rng2,
                                OutputIterator          out)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::set_union(boost::begin(rng1), boost::end(rng1),
                          boost::begin(rng2), boost::end(rng2), out);
}

/// \overload
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator, class BinaryPredicate>
inline OutputIterator set_union(const SinglePassRange1& rng1,
                                const SinglePassRange2& rng2,
                                OutputIterator          out,
                                BinaryPredicate         pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::set_union(boost::begin(rng1), boost::end(rng1),
                          boost::begin(rng2), boost::end(rng2), out, pred);
}

/// \brief template function set_intersection
///
/// range-based version of the set_intersection std algorithm
///
/// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
/// \pre SinglePassRange2 is a model of the SinglePassRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator>
inline OutputIterator set_intersection(const SinglePassRange1& rng1,
                                       const SinglePassRange2& rng2,
                                       OutputIterator          out)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::set_intersection(boost::begin(rng1), boost::end(rng1),
                                 boost::begin(rng2), boost::end(rng2), out);
}

/// \overload
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator, class BinaryPredicate>
inline OutputIterator set_intersection(const SinglePassRange1& rng1,
                                       const SinglePassRange2& rng2,
                                       OutputIterator          out,
                                       BinaryPredicate         pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::set_intersection(boost::begin(rng1), boost::end(rng1),
                                 boost::begin(rng2), boost::end(rng2),
                                 out, pred);
}

/// \brief template function set_difference
///
/// range-based version of the set_difference std algorithm
///
/// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
/// \pre SinglePassRange2 is a model of the SinglePassRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator>
inline OutputIterator set_difference(const SinglePassRange1& rng1,
                                     const SinglePassRange2& rng2,
                                     OutputIterator out)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::set_difference(boost::begin(rng1), boost::end(rng1),
                               boost::begin(rng2), boost::end(rng2), out);
}

/// \overload
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator, class BinaryPredicate>
inline OutputIterator set_difference(const SinglePassRange1& rng1,
                                     const SinglePassRange2& rng2,
                                     OutputIterator          out,
                                     BinaryPredicate         pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::set_difference(
        boost::begin(rng1), boost::end(rng1),
        boost::begin(rng2), boost::end(rng2), out, pred);
}

/// \brief template function set_symmetric_difference
///
/// range-based version of the set_symmetric_difference std algorithm
///
/// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
/// \pre SinglePassRange2 is a model of the SinglePassRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator>
inline OutputIterator
set_symmetric_difference(const SinglePassRange1& rng1,
                         const SinglePassRange2& rng2,
                         OutputIterator          out)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::set_symmetric_difference(boost::begin(rng1), boost::end(rng1),
                                         boost::begin(rng2), boost::end(rng2), out);
}

/// \overload
template<class SinglePassRange1, class SinglePassRange2,
         class OutputIterator, class BinaryPredicate>
inline OutputIterator
set_symmetric_difference(const SinglePassRange1& rng1,
                         const SinglePassRange2& rng2,
                         OutputIterator          out,
                         BinaryPredicate         pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));
    return std::set_symmetric_difference(
        boost::begin(rng1), boost::end(rng1),
        boost::begin(rng2), boost::end(rng2), out, pred);
}

    } // namespace range
    using range::includes;
    using range::set_union;
    using range::set_intersection;
    using range::set_difference;
    using range::set_symmetric_difference;
} // namespace boost

#endif // include guard
