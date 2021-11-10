// Copyright 2009-2014 Neil Groves.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Copyright 2006 Thorsten Ottosen.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Copyright 2004 Eric Niebler.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//   Contains range-based versions of the numeric std algorithms
//
#if defined(_MSC_VER)
    #pragma once
#endif

#ifndef BOOST_RANGE_NUMERIC_HPP
#define BOOST_RANGE_NUMERIC_HPP

#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/category.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/distance.hpp>
#include <boost/range/size.hpp>
#include <numeric>


namespace boost
{
    template<class SinglePassRange, class Value>
    inline Value accumulate(const SinglePassRange& rng, Value init)
    {
        BOOST_RANGE_CONCEPT_ASSERT((
            SinglePassRangeConcept<const SinglePassRange>));

        return std::accumulate(boost::begin(rng), boost::end(rng), init);
    }

    template<class SinglePassRange, class Value, class BinaryOperation>
    inline Value accumulate(const SinglePassRange& rng, Value init,
                            BinaryOperation op)
    {
        BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange> ));

        return std::accumulate(boost::begin(rng), boost::end(rng), init, op);
    }

    namespace range_detail
    {
        template<class SinglePassRange1, class SinglePassRange2>
        inline bool inner_product_precondition(
            const SinglePassRange1&,
            const SinglePassRange2&,
            std::input_iterator_tag,
            std::input_iterator_tag)
        {
            return true;
        }

        template<class SinglePassRange1, class SinglePassRange2>
        inline bool inner_product_precondition(
            const SinglePassRange1& rng1,
            const SinglePassRange2& rng2,
            std::forward_iterator_tag,
            std::forward_iterator_tag)
        {
            return boost::size(rng2) >= boost::size(rng1);
        }

    } // namespace range_detail

    template<
        class SinglePassRange1,
        class SinglePassRange2,
        class Value
    >
    inline Value inner_product(
        const SinglePassRange1& rng1,
        const SinglePassRange2& rng2,
        Value                   init)
    {
        BOOST_RANGE_CONCEPT_ASSERT((
            SinglePassRangeConcept<const SinglePassRange1>));

        BOOST_RANGE_CONCEPT_ASSERT((
            SinglePassRangeConcept<const SinglePassRange2>));

        BOOST_ASSERT(
            range_detail::inner_product_precondition(
                    rng1, rng2,
                    typename range_category<const SinglePassRange1>::type(),
                    typename range_category<const SinglePassRange2>::type()));

        return std::inner_product(
            boost::begin(rng1), boost::end(rng1),
            boost::begin(rng2), init);
    }

    template<
        class SinglePassRange1,
        class SinglePassRange2,
        class Value,
        class BinaryOperation1,
        class BinaryOperation2
    >
    inline Value inner_product(
        const SinglePassRange1& rng1,
        const SinglePassRange2& rng2,
        Value                   init,
        BinaryOperation1        op1,
        BinaryOperation2        op2)
    {
        BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange1>));

        BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange2>));

        BOOST_ASSERT(
            range_detail::inner_product_precondition(
                rng1, rng2,
                typename range_category<const SinglePassRange1>::type(),
                typename range_category<const SinglePassRange2>::type()));

        return std::inner_product(
            boost::begin(rng1), boost::end(rng1),
            boost::begin(rng2), init, op1, op2);
    }

    template<class SinglePassRange, class OutputIterator>
    inline OutputIterator partial_sum(const SinglePassRange& rng,
                                      OutputIterator result)
    {
        BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

        return std::partial_sum(boost::begin(rng), boost::end(rng), result);
    }

    template<class SinglePassRange, class OutputIterator, class BinaryOperation>
    inline OutputIterator partial_sum(
        const SinglePassRange&  rng,
        OutputIterator          result,
        BinaryOperation         op)
    {
        BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

        return std::partial_sum(boost::begin(rng), boost::end(rng), result, op);
    }

    template<class SinglePassRange, class OutputIterator>
    inline OutputIterator adjacent_difference(
        const SinglePassRange&  rng,
        OutputIterator          result)
    {
        BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

        return std::adjacent_difference(boost::begin(rng), boost::end(rng),
                                        result);
    }

    template<class SinglePassRange, class OutputIterator, class BinaryOperation>
    inline OutputIterator adjacent_difference(
        const SinglePassRange&  rng,
        OutputIterator          result,
        BinaryOperation         op)
    {
        BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

        return std::adjacent_difference(boost::begin(rng), boost::end(rng),
                                        result, op);
    }

} // namespace boost

#endif
