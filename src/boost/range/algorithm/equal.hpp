// Boost.Range library
//
//  Copyright Neil Groves 2009.
//  Use, modification and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_EQUAL_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_EQUAL_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/range/concepts.hpp>
#include <iterator>

namespace boost
{
    namespace range_detail
    {
        // An implementation of equality comparison that is optimized for iterator
        // traversal categories less than RandomAccessTraversal.
        template< class SinglePassTraversalReadableIterator1,
                  class SinglePassTraversalReadableIterator2,
                  class IteratorCategoryTag1,
                  class IteratorCategoryTag2 >
        inline bool equal_impl( SinglePassTraversalReadableIterator1 first1,
                                SinglePassTraversalReadableIterator1 last1,
                                SinglePassTraversalReadableIterator2 first2,
                                SinglePassTraversalReadableIterator2 last2,
                                IteratorCategoryTag1,
                                IteratorCategoryTag2 )
        {
            for (;;)
            {
                // If we have reached the end of the left range then this is
                // the end of the loop. They are equal if and only if we have
                // simultaneously reached the end of the right range.
                if (first1 == last1)
                    return first2 == last2;

                // If we have reached the end of the right range at this line
                // it indicates that the right range is shorter than the left
                // and hence the result is false.
                if (first2 == last2)
                    return false;

                // continue looping if and only if the values are equal
                if (*first1 != *first2)
                    break;

                ++first1;
                ++first2;
            }

            // Reaching this line in the algorithm indicates that a value
            // inequality has been detected.
            return false;
        }

        template< class SinglePassTraversalReadableIterator1,
                  class SinglePassTraversalReadableIterator2,
                  class IteratorCategoryTag1,
                  class IteratorCategoryTag2,
                  class BinaryPredicate >
        inline bool equal_impl( SinglePassTraversalReadableIterator1 first1,
                                SinglePassTraversalReadableIterator1 last1,
                                SinglePassTraversalReadableIterator2 first2,
                                SinglePassTraversalReadableIterator2 last2,
                                BinaryPredicate                      pred,
                                IteratorCategoryTag1,
                                IteratorCategoryTag2 )
        {
            for (;;)
            {
                // If we have reached the end of the left range then this is
                // the end of the loop. They are equal if and only if we have
                // simultaneously reached the end of the right range.
                if (first1 == last1)
                    return first2 == last2;

                // If we have reached the end of the right range at this line
                // it indicates that the right range is shorter than the left
                // and hence the result is false.
                if (first2 == last2)
                    return false;

                // continue looping if and only if the values are equal
                if (!pred(*first1, *first2))
                    break;

                ++first1;
                ++first2;
            }

            // Reaching this line in the algorithm indicates that a value
            // inequality has been detected.
            return false;
        }

        // An implementation of equality comparison that is optimized for
        // random access iterators.
        template< class RandomAccessTraversalReadableIterator1,
                  class RandomAccessTraversalReadableIterator2 >
        inline bool equal_impl( RandomAccessTraversalReadableIterator1 first1,
                                RandomAccessTraversalReadableIterator1 last1,
                                RandomAccessTraversalReadableIterator2 first2,
                                RandomAccessTraversalReadableIterator2 last2,
                                std::random_access_iterator_tag,
                                std::random_access_iterator_tag )
        {
            return ((last1 - first1) == (last2 - first2))
                && std::equal(first1, last1, first2);
        }

        template< class RandomAccessTraversalReadableIterator1,
                  class RandomAccessTraversalReadableIterator2,
                  class BinaryPredicate >
        inline bool equal_impl( RandomAccessTraversalReadableIterator1 first1,
                                RandomAccessTraversalReadableIterator1 last1,
                                RandomAccessTraversalReadableIterator2 first2,
                                RandomAccessTraversalReadableIterator2 last2,
                                BinaryPredicate                        pred,
                                std::random_access_iterator_tag,
                                std::random_access_iterator_tag )
        {
            return ((last1 - first1) == (last2 - first2))
                && std::equal(first1, last1, first2, pred);
        }

        template< class SinglePassTraversalReadableIterator1,
                  class SinglePassTraversalReadableIterator2 >
        inline bool equal( SinglePassTraversalReadableIterator1 first1,
                           SinglePassTraversalReadableIterator1 last1,
                           SinglePassTraversalReadableIterator2 first2,
                           SinglePassTraversalReadableIterator2 last2 )
        {
            BOOST_DEDUCED_TYPENAME std::iterator_traits< SinglePassTraversalReadableIterator1 >::iterator_category tag1;
            BOOST_DEDUCED_TYPENAME std::iterator_traits< SinglePassTraversalReadableIterator2 >::iterator_category tag2;

            return equal_impl(first1, last1, first2, last2, tag1, tag2);
        }

        template< class SinglePassTraversalReadableIterator1,
                  class SinglePassTraversalReadableIterator2,
                  class BinaryPredicate >
        inline bool equal( SinglePassTraversalReadableIterator1 first1,
                           SinglePassTraversalReadableIterator1 last1,
                           SinglePassTraversalReadableIterator2 first2,
                           SinglePassTraversalReadableIterator2 last2,
                           BinaryPredicate                      pred )
        {
            BOOST_DEDUCED_TYPENAME std::iterator_traits< SinglePassTraversalReadableIterator1 >::iterator_category tag1;
            BOOST_DEDUCED_TYPENAME std::iterator_traits< SinglePassTraversalReadableIterator2 >::iterator_category tag2;

            return equal_impl(first1, last1, first2, last2, pred, tag1, tag2);
        }

    } // namespace range_detail

    namespace range
    {

        /// \brief template function equal
        ///
        /// range-based version of the equal std algorithm
        ///
        /// \pre SinglePassRange1 is a model of the SinglePassRangeConcept
        /// \pre SinglePassRange2 is a model of the SinglePassRangeConcept
        /// \pre BinaryPredicate is a model of the BinaryPredicateConcept
        template< class SinglePassRange1, class SinglePassRange2 >
        inline bool equal( const SinglePassRange1& rng1, const SinglePassRange2& rng2 )
        {
            BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
            BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));

            return ::boost::range_detail::equal(
                ::boost::begin(rng1), ::boost::end(rng1),
                ::boost::begin(rng2), ::boost::end(rng2) );
        }

        /// \overload
        template< class SinglePassRange1, class SinglePassRange2, class BinaryPredicate >
        inline bool equal( const SinglePassRange1& rng1, const SinglePassRange2& rng2,
                           BinaryPredicate pred )
        {
            BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange1> ));
            BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange2> ));

            return ::boost::range_detail::equal(
                ::boost::begin(rng1), ::boost::end(rng1),
                ::boost::begin(rng2), ::boost::end(rng2),
                pred);
        }

    } // namespace range
    using ::boost::range::equal;
} // namespace boost

#endif // include guard
