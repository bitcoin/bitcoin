//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_SWAP_RANGES_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_SWAP_RANGES_HPP_INCLUDED

#include <boost/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/iterator.hpp>
#include <algorithm>

namespace boost
{
    namespace range_detail
    {
        template<class Iterator1, class Iterator2>
        void swap_ranges_impl(Iterator1 it1, Iterator1 last1,
                              Iterator2 it2, Iterator2 last2,
                              single_pass_traversal_tag,
                              single_pass_traversal_tag)
        {
            ignore_unused_variable_warning(last2);
            for (; it1 != last1; ++it1, ++it2)
            {
                BOOST_ASSERT( it2 != last2 );
                std::iter_swap(it1, it2);
            }
        }

        template<class Iterator1, class Iterator2>
        void swap_ranges_impl(Iterator1 it1, Iterator1 last1,
                              Iterator2 it2, Iterator2 last2,
                              random_access_traversal_tag,
                              random_access_traversal_tag)
        {
            ignore_unused_variable_warning(last2);
            BOOST_ASSERT( last2 - it2 >= last1 - it1 );
            std::swap_ranges(it1, last1, it2);
        }

        template<class Iterator1, class Iterator2>
        void swap_ranges_impl(Iterator1 first1, Iterator1 last1,
                              Iterator2 first2, Iterator2 last2)
        {
            swap_ranges_impl(first1, last1, first2, last2,
                BOOST_DEDUCED_TYPENAME iterator_traversal<Iterator1>::type(),
                BOOST_DEDUCED_TYPENAME iterator_traversal<Iterator2>::type());
        }
    } // namespace range_detail

    namespace range
    {

/// \brief template function swap_ranges
///
/// range-based version of the swap_ranges std algorithm
///
/// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
/// \pre SinglePassRange2 is a model of the SinglePassRangeConcept
template< class SinglePassRange1, class SinglePassRange2 >
inline SinglePassRange2&
swap_ranges(SinglePassRange1& range1, SinglePassRange2& range2)
{
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<SinglePassRange1>));
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<SinglePassRange2>));

    boost::range_detail::swap_ranges_impl(
        boost::begin(range1), boost::end(range1),
        boost::begin(range2), boost::end(range2));

    return range2;
}

/// \overload
template< class SinglePassRange1, class SinglePassRange2 >
inline SinglePassRange2&
swap_ranges(const SinglePassRange1& range1, SinglePassRange2& range2)
{
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<const SinglePassRange1>));
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<SinglePassRange2>));

    boost::range_detail::swap_ranges_impl(
        boost::begin(range1), boost::end(range1),
        boost::begin(range2), boost::end(range2));

    return range2;
}

/// \overload
template< class SinglePassRange1, class SinglePassRange2 >
inline const SinglePassRange2&
swap_ranges(SinglePassRange1& range1, const SinglePassRange2& range2)
{
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<SinglePassRange1>));
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<const SinglePassRange2>));

    boost::range_detail::swap_ranges_impl(
        boost::begin(range1), boost::end(range1),
        boost::begin(range2), boost::end(range2));

    return range2;
}

/// \overload
template< class SinglePassRange1, class SinglePassRange2 >
inline const SinglePassRange2&
swap_ranges(const SinglePassRange1& range1, const SinglePassRange2& range2)
{
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<const SinglePassRange1>));
    BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<const SinglePassRange2>));

    boost::range_detail::swap_ranges_impl(
        boost::begin(range1), boost::end(range1),
        boost::begin(range2), boost::end(range2));

    return range2;
}

    } // namespace range
    using range::swap_ranges;
} // namespace boost

#endif // include guard
