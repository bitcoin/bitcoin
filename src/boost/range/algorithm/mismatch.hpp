//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_MISMATCH_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_MISMATCH_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/difference_type.hpp>
#include <algorithm>

namespace boost
{
    namespace range_detail
    {
        template< class SinglePassTraversalReadableIterator1,
                  class SinglePassTraversalReadableIterator2 >
        inline std::pair<SinglePassTraversalReadableIterator1,
                         SinglePassTraversalReadableIterator2>
        mismatch_impl(SinglePassTraversalReadableIterator1 first1,
                      SinglePassTraversalReadableIterator1 last1,
                      SinglePassTraversalReadableIterator2 first2,
                      SinglePassTraversalReadableIterator2 last2)
        {
            while (first1 != last1 && first2 != last2 && *first1 == *first2)
            {
                ++first1;
                ++first2;
            }
            return std::pair<SinglePassTraversalReadableIterator1,
                             SinglePassTraversalReadableIterator2>(first1, first2);
        }

        template< class SinglePassTraversalReadableIterator1,
                  class SinglePassTraversalReadableIterator2,
                  class BinaryPredicate >
        inline std::pair<SinglePassTraversalReadableIterator1,
                         SinglePassTraversalReadableIterator2>
        mismatch_impl(SinglePassTraversalReadableIterator1 first1,
                      SinglePassTraversalReadableIterator1 last1,
                      SinglePassTraversalReadableIterator2 first2,
                      SinglePassTraversalReadableIterator2 last2,
                      BinaryPredicate pred)
        {
            while (first1 != last1 && first2 != last2 && pred(*first1, *first2))
            {
                ++first1;
                ++first2;
            }
            return std::pair<SinglePassTraversalReadableIterator1,
                             SinglePassTraversalReadableIterator2>(first1, first2);
        }
    } // namespace range_detail

    namespace range
    {
/// \brief template function mismatch
///
/// range-based version of the mismatch std algorithm
///
/// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
/// \pre SinglePassRange2 is a model of the SinglePassRangeConcept
/// \pre BinaryPredicate is a model of the BinaryPredicateConcept
template< class SinglePassRange1, class SinglePassRange2 >
inline std::pair<
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange2>::type >
mismatch(SinglePassRange1& rng1, const SinglePassRange2 & rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));

    return ::boost::range_detail::mismatch_impl(
        ::boost::begin(rng1), ::boost::end(rng1),
        ::boost::begin(rng2), ::boost::end(rng2));
}

/// \overload
template< class SinglePassRange1, class SinglePassRange2 >
inline std::pair<
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange2>::type >
mismatch(const SinglePassRange1& rng1, const SinglePassRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));

    return ::boost::range_detail::mismatch_impl(
        ::boost::begin(rng1), ::boost::end(rng1),
        ::boost::begin(rng2), ::boost::end(rng2));
}

/// \overload
template< class SinglePassRange1, class SinglePassRange2 >
inline std::pair<
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange2>::type >
mismatch(SinglePassRange1& rng1, SinglePassRange2 & rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange2> ));

    return ::boost::range_detail::mismatch_impl(
        ::boost::begin(rng1), ::boost::end(rng1),
        ::boost::begin(rng2), ::boost::end(rng2));
}

/// \overload
template< class SinglePassRange1, class SinglePassRange2 >
inline std::pair<
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange2>::type >
mismatch(const SinglePassRange1& rng1, SinglePassRange2& rng2)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange2> ));

    return ::boost::range_detail::mismatch_impl(
        ::boost::begin(rng1), ::boost::end(rng1),
        ::boost::begin(rng2), ::boost::end(rng2));
}


/// \overload
template< class SinglePassRange1, class SinglePassRange2, class BinaryPredicate >
inline std::pair<
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange2>::type >
mismatch(SinglePassRange1& rng1, const SinglePassRange2& rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));

    return ::boost::range_detail::mismatch_impl(
        ::boost::begin(rng1), ::boost::end(rng1),
        ::boost::begin(rng2), ::boost::end(rng2), pred);
}

/// \overload
template< class SinglePassRange1, class SinglePassRange2, class BinaryPredicate >
inline std::pair<
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange2>::type >
mismatch(const SinglePassRange1& rng1, const SinglePassRange2& rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));

    return ::boost::range_detail::mismatch_impl(
        ::boost::begin(rng1), ::boost::end(rng1),
        ::boost::begin(rng2), ::boost::end(rng2), pred);
}

/// \overload
template< class SinglePassRange1, class SinglePassRange2, class BinaryPredicate >
inline std::pair<
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange2>::type >
mismatch(SinglePassRange1& rng1, SinglePassRange2& rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange2> ));

    return ::boost::range_detail::mismatch_impl(
        ::boost::begin(rng1), ::boost::end(rng1),
        ::boost::begin(rng2), ::boost::end(rng2), pred);
}

/// \overload
template< class SinglePassRange1, class SinglePassRange2, class BinaryPredicate >
inline std::pair<
    BOOST_DEDUCED_TYPENAME range_iterator<const SinglePassRange1>::type,
    BOOST_DEDUCED_TYPENAME range_iterator<SinglePassRange2>::type >
mismatch(const SinglePassRange1& rng1, SinglePassRange2& rng2, BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange2> ));

    return ::boost::range_detail::mismatch_impl(
        ::boost::begin(rng1), ::boost::end(rng1),
        ::boost::begin(rng2), ::boost::end(rng2), pred);
}

    } // namespace range
    using range::mismatch;
} // namespace boost

#endif // include guard
