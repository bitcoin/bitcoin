//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_SEARCH_N_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_SEARCH_N_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/range/value_type.hpp>
#include <iterator>
#include <algorithm>

namespace boost
{

namespace range_detail
{
    // Rationale: search_n is implemented rather than delegate to
    // the standard library implementation because some standard
    // library implementations are broken eg. MSVC.

    // search_n forward iterator version
    template<typename ForwardIterator, typename Integer, typename Value>
    inline ForwardIterator
    search_n_impl(ForwardIterator first, ForwardIterator last, Integer count,
                  const Value& value, std::forward_iterator_tag)
    {
        first = std::find(first, last, value);
        while (first != last)
        {
            typename std::iterator_traits<ForwardIterator>::difference_type n = count;
            ForwardIterator i = first;
            ++i;
            while (i != last && n != 1 && *i==value)
            {
                ++i;
                --n;
            }
            if (n == 1)
                return first;
            if (i == last)
                return last;
            first = std::find(++i, last, value);
        }
        return last;
    }

    // search_n random-access iterator version
    template<typename RandomAccessIterator, typename Integer, typename Value>
    inline RandomAccessIterator
    search_n_impl(RandomAccessIterator first, RandomAccessIterator last,
                  Integer count, const Value& value,
                  std::random_access_iterator_tag)
    {
        typedef typename std::iterator_traits<RandomAccessIterator>::difference_type difference_t;

        difference_t tail_size = last - first;
        const difference_t pattern_size = count;

        if (tail_size < pattern_size)
            return last;

        const difference_t skip_offset = pattern_size - 1;
        RandomAccessIterator look_ahead = first + skip_offset;
        tail_size -= pattern_size;

        while (1)
        {
            // look_ahead here is pointing to the last element of the
            // next possible match
            while (!(*look_ahead == value)) // skip loop...
            {
                if (tail_size < pattern_size)
                    return last; // no match
                look_ahead += pattern_size;
                tail_size -= pattern_size;
            }
            difference_t remainder = skip_offset;
            for (RandomAccessIterator back_track = look_ahead - 1;
                    *back_track == value; --back_track)
            {
                if (--remainder == 0)
                {
                    return look_ahead - skip_offset; // matched
                }
            }
            if (remainder > tail_size)
                return last; // no match
            look_ahead += remainder;
            tail_size -= remainder;
        }

        return last;
    }

    // search_n for forward iterators using a binary predicate
    // to determine a match
    template<typename ForwardIterator, typename Integer, typename Value,
             typename BinaryPredicate>
    inline ForwardIterator
    search_n_pred_impl(ForwardIterator first, ForwardIterator last,
                       Integer count, const Value& value,
                       BinaryPredicate pred, std::forward_iterator_tag)
    {
        typedef typename std::iterator_traits<ForwardIterator>::difference_type difference_t;

        while (first != last && !static_cast<bool>(pred(*first, value)))
            ++first;

        while (first != last)
        {
            difference_t n = count;
            ForwardIterator i = first;
            ++i;
            while (i != last && n != 1 && static_cast<bool>(pred(*i, value)))
            {
                ++i;
                --n;
            }
            if (n == 1)
                return first;
            if (i == last)
                return last;
            first = ++i;
            while (first != last && !static_cast<bool>(pred(*first, value)))
                ++first;
        }
        return last;
    }

    // search_n for random-access iterators using a binary predicate
    // to determine a match
    template<typename RandomAccessIterator, typename Integer,
             typename Value, typename BinaryPredicate>
    inline RandomAccessIterator
    search_n_pred_impl(RandomAccessIterator first, RandomAccessIterator last,
                       Integer count, const Value& value,
                       BinaryPredicate pred, std::random_access_iterator_tag)
    {
        typedef typename std::iterator_traits<RandomAccessIterator>::difference_type difference_t;

        difference_t tail_size = last - first;
        const difference_t pattern_size = count;

        if (tail_size < pattern_size)
            return last;

        const difference_t skip_offset = pattern_size - 1;
        RandomAccessIterator look_ahead = first + skip_offset;
        tail_size -= pattern_size;

        while (1)
        {
            // look_ahead points to the last element of the next
            // possible match
            while (!static_cast<bool>(pred(*look_ahead, value))) // skip loop
            {
                if (tail_size < pattern_size)
                    return last; // no match
                look_ahead += pattern_size;
                tail_size -= pattern_size;
            }
            difference_t remainder = skip_offset;
            for (RandomAccessIterator back_track = look_ahead - 1;
                    pred(*back_track, value); --back_track)
            {
                if (--remainder == 0)
                    return look_ahead -= skip_offset; // success
            }
            if (remainder > tail_size)
            {
                return last; // no match
            }
            look_ahead += remainder;
            tail_size -= remainder;
        }
    }

    template<typename ForwardIterator, typename Integer, typename Value>
    inline ForwardIterator
    search_n_impl(ForwardIterator first, ForwardIterator last,
                  Integer count, const Value& value)
    {
        BOOST_RANGE_CONCEPT_ASSERT((ForwardIteratorConcept<ForwardIterator>));
        BOOST_RANGE_CONCEPT_ASSERT((EqualityComparableConcept<Value>));
        BOOST_RANGE_CONCEPT_ASSERT((EqualityComparableConcept<typename std::iterator_traits<ForwardIterator>::value_type>));
        //BOOST_RANGE_CONCEPT_ASSERT((EqualityComparableConcept2<typename std::iterator_traits<ForwardIterator>::value_type, Value>));

        typedef typename std::iterator_traits<ForwardIterator>::iterator_category cat_t;

        if (count <= 0)
            return first;
        if (count == 1)
            return std::find(first, last, value);
        return range_detail::search_n_impl(first, last, count, value, cat_t());
    }

    template<typename ForwardIterator, typename Integer, typename Value,
             typename BinaryPredicate>
    inline ForwardIterator
    search_n_pred_impl(ForwardIterator first, ForwardIterator last,
                       Integer count, const Value& value,
                       BinaryPredicate pred)
    {
        BOOST_RANGE_CONCEPT_ASSERT((ForwardIteratorConcept<ForwardIterator>));
        BOOST_RANGE_CONCEPT_ASSERT((
            BinaryPredicateConcept<
                BinaryPredicate,
                typename std::iterator_traits<ForwardIterator>::value_type,
                Value>
            ));

        typedef typename std::iterator_traits<ForwardIterator>::iterator_category cat_t;

        if (count <= 0)
            return first;
        if (count == 1)
        {
            while (first != last && !static_cast<bool>(pred(*first, value)))
                ++first;
            return first;
        }
        return range_detail::search_n_pred_impl(first, last, count,
                                                value, pred, cat_t());
    }
} // namespace range_detail

namespace range {

/// \brief template function search
///
/// range-based version of the search std algorithm
///
/// \pre ForwardRange is a model of the ForwardRangeConcept
/// \pre Integer is an integral type
/// \pre Value is a model of the EqualityComparableConcept
/// \pre ForwardRange's value type is a model of the EqualityComparableConcept
/// \pre Object's of ForwardRange's value type can be compared for equality with Objects of type Value
template< class ForwardRange, class Integer, class Value >
inline BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange>::type
search_n(ForwardRange& rng, Integer count, const Value& value)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    return range_detail::search_n_impl(boost::begin(rng),boost::end(rng), count, value);
}

/// \overload
template< class ForwardRange, class Integer, class Value >
inline BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange>::type
search_n(const ForwardRange& rng, Integer count, const Value& value)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRange>));
    return range_detail::search_n_impl(boost::begin(rng), boost::end(rng), count, value);
}

/// \overload
template< class ForwardRange, class Integer, class Value,
          class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange>::type
search_n(ForwardRange& rng, Integer count, const Value& value,
         BinaryPredicate binary_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        BOOST_DEDUCED_TYPENAME range_value<ForwardRange>::type, const Value&>));
    return range_detail::search_n_pred_impl(boost::begin(rng), boost::end(rng),
        count, value, binary_pred);
}

/// \overload
template< class ForwardRange, class Integer, class Value,
          class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange>::type
search_n(const ForwardRange& rng, Integer count, const Value& value,
         BinaryPredicate binary_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        BOOST_DEDUCED_TYPENAME range_value<const ForwardRange>::type, const Value&>));
    return range_detail::search_n_pred_impl(boost::begin(rng), boost::end(rng),
        count, value, binary_pred);
}

// range_return overloads

/// \overload
template< range_return_value re, class ForwardRange, class Integer,
          class Value >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange,re>::type
search_n(ForwardRange& rng, Integer count, const Value& value)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    return range_return<ForwardRange,re>::
        pack(range_detail::search_n_impl(boost::begin(rng),boost::end(rng),
                           count, value),
             rng);
}

/// \overload
template< range_return_value re, class ForwardRange, class Integer,
          class Value >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange,re>::type
search_n(const ForwardRange& rng, Integer count, const Value& value)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRange>));
    return range_return<const ForwardRange,re>::
        pack(range_detail::search_n_impl(boost::begin(rng), boost::end(rng),
                           count, value),
             rng);
}

/// \overload
template< range_return_value re, class ForwardRange, class Integer,
          class Value, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange,re>::type
search_n(ForwardRange& rng, Integer count, const Value& value,
         BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        BOOST_DEDUCED_TYPENAME range_value<ForwardRange>::type,
        const Value&>));
    return range_return<ForwardRange,re>::
        pack(range_detail::search_n_pred_impl(boost::begin(rng),
                                              boost::end(rng),
                           count, value, pred),
             rng);
}

/// \overload
template< range_return_value re, class ForwardRange, class Integer,
          class Value, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange,re>::type
search_n(const ForwardRange& rng, Integer count, const Value& value,
         BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        BOOST_DEDUCED_TYPENAME range_value<const ForwardRange>::type,
        const Value&>));
    return range_return<const ForwardRange,re>::
        pack(range_detail::search_n_pred_impl(boost::begin(rng),
                                              boost::end(rng),
                           count, value, pred),
             rng);
}

    } // namespace range
    using range::search_n;
} // namespace boost

#endif // include guard
