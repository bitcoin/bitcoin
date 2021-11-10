// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_FILTERED_HPP
#define BOOST_RANGE_ADAPTOR_FILTERED_HPP

#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/detail/default_constructible_unary_fn.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/concepts.hpp>
#include <boost/iterator/filter_iterator.hpp>

namespace boost
{
    namespace range_detail
    {
        template< class P, class R >
        struct filtered_range :
            boost::iterator_range<
                boost::filter_iterator<
                    typename default_constructible_unary_fn_gen<P, bool>::type,
                    typename range_iterator<R>::type
                >
            >
        {
        private:
            typedef boost::iterator_range<
                boost::filter_iterator<
                    typename default_constructible_unary_fn_gen<P, bool>::type,
                    typename range_iterator<R>::type
                >
            > base;
        public:
            typedef typename default_constructible_unary_fn_gen<P, bool>::type
                pred_t;

            filtered_range(P p, R& r)
            : base(make_filter_iterator(pred_t(p),
                                        boost::begin(r), boost::end(r)),
                   make_filter_iterator(pred_t(p),
                                        boost::end(r), boost::end(r)))
            { }
        };

        template< class T >
        struct filter_holder : holder<T>
        {
            filter_holder( T r ) : holder<T>(r)
            { }
        };

        template< class SinglePassRange, class Predicate >
        inline filtered_range<Predicate, SinglePassRange>
        operator|(SinglePassRange& r,
                  const filter_holder<Predicate>& f)
        {
            BOOST_RANGE_CONCEPT_ASSERT((SinglePassRangeConcept<SinglePassRange>));
            return filtered_range<Predicate, SinglePassRange>( f.val, r );
        }

        template< class SinglePassRange, class Predicate >
        inline filtered_range<Predicate, const SinglePassRange>
        operator|(const SinglePassRange& r,
                  const filter_holder<Predicate>& f )
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));
            return filtered_range<Predicate, const SinglePassRange>( f.val, r );
        }

    } // 'range_detail'

    // Unusual use of 'using' is intended to bring filter_range into the boost namespace
    // while leaving the mechanics of the '|' operator in range_detail and maintain
    // argument dependent lookup.
    // filter_range logically needs to be in the boost namespace to allow user of
    // the library to define the return type for filter()
    using range_detail::filtered_range;

    namespace adaptors
    {
        namespace
        {
            const range_detail::forwarder<range_detail::filter_holder>
                    filtered =
                       range_detail::forwarder<range_detail::filter_holder>();
        }

        template<class SinglePassRange, class Predicate>
        inline filtered_range<Predicate, SinglePassRange>
        filter(SinglePassRange& rng, Predicate filter_pred)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<SinglePassRange>));

            return range_detail::filtered_range<
                Predicate, SinglePassRange>( filter_pred, rng );
        }

        template<class SinglePassRange, class Predicate>
        inline filtered_range<Predicate, const SinglePassRange>
        filter(const SinglePassRange& rng, Predicate filter_pred)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

            return range_detail::filtered_range<
                Predicate, const SinglePassRange>( filter_pred, rng );
        }
    } // 'adaptors'

}

#endif
