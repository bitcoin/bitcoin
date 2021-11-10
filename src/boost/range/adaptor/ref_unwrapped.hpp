// Boost.Range library
//
//  Copyright Robin Eckert 2015.
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_REF_UNWRAPPED_HPP
#define BOOST_RANGE_ADAPTOR_REF_UNWRAPPED_HPP

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/reference.hpp>
#include <boost/range/concepts.hpp>
#include <boost/type_traits/declval.hpp>
#include <utility>

#if !defined(BOOST_NO_CXX11_DECLTYPE)

namespace boost
{
    namespace range_detail
    {
        struct ref_unwrapped_forwarder {};

        template<class SinglePassRange>
        struct unwrap_ref
        {
            typedef BOOST_DEDUCED_TYPENAME
                          range_reference<SinglePassRange>::type argument_type;

            typedef decltype( boost::declval<argument_type>().get() ) result_type;

            result_type operator()( argument_type &&r ) const
            {
                return r.get();
            }
        };


        template<class SinglePassRange>
        class unwrap_ref_range
            : public transformed_range<unwrap_ref<SinglePassRange>,
                                       SinglePassRange>
        {
            typedef transformed_range<unwrap_ref<SinglePassRange>,
                                           SinglePassRange> base;
        public:
            typedef unwrap_ref<SinglePassRange> transform_fn_type;
            typedef SinglePassRange source_range_type;

            unwrap_ref_range(transform_fn_type fn, source_range_type &rng)
                : base(fn, rng)
            {
            }

            unwrap_ref_range(const base &other) : base(other) {}
        };

        template<class SinglePassRange>
        inline unwrap_ref_range<SinglePassRange>
        operator|(SinglePassRange& r, ref_unwrapped_forwarder)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<SinglePassRange>));

            return operator|( r,
                boost::adaptors::transformed(unwrap_ref<SinglePassRange>()));
        }

    }

    using range_detail::unwrap_ref_range;

    namespace adaptors
    {
        namespace
        {
            const range_detail::ref_unwrapped_forwarder ref_unwrapped =
                                       range_detail::ref_unwrapped_forwarder();
        }

        template<class SinglePassRange>
        inline unwrap_ref_range<SinglePassRange>
        ref_unwrap(SinglePassRange& rng)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<SinglePassRange>));

            return unwrap_ref_range<SinglePassRange>(
                range_detail::unwrap_ref<SinglePassRange>(), rng );
        }
    } // 'adaptors'

}

#endif

#endif
