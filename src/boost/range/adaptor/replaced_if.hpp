// Boost.Range library
//
//  Copyright Neil Groves 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_REPLACED_IF_IMPL_HPP_INCLUDED
#define BOOST_RANGE_ADAPTOR_REPLACED_IF_IMPL_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>
#include <boost/range/concepts.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/optional/optional.hpp>

namespace boost
{
    namespace range_detail
    {
        template< class Pred, class Value >
        class replace_value_if
        {
        public:
            typedef const Value& result_type;
            typedef const Value& first_argument_type;

            // Rationale:
            // required to allow the iterator to be default constructible.
            replace_value_if()
            {
            }

            replace_value_if(const Pred& pred, const Value& to)
                : m_impl(data(pred, to))
            {
            }

            const Value& operator()(const Value& x) const
            {
                return m_impl->m_pred(x) ? m_impl->m_to : x;
            }

        private:
            struct data
            {
                data(const Pred& p, const Value& t)
                    : m_pred(p), m_to(t)
                {
                }

                Pred  m_pred;
                Value m_to;
            };
            boost::optional<data> m_impl;
        };

        template< class Pred, class R >
        class replaced_if_range :
            public boost::iterator_range<
                boost::transform_iterator<
                    replace_value_if< Pred, BOOST_DEDUCED_TYPENAME range_value<R>::type >,
                    BOOST_DEDUCED_TYPENAME range_iterator<R>::type > >
        {
        private:
            typedef replace_value_if< Pred, BOOST_DEDUCED_TYPENAME range_value<R>::type > Fn;

            typedef boost::iterator_range<
                boost::transform_iterator<
                    replace_value_if< Pred, BOOST_DEDUCED_TYPENAME range_value<R>::type >,
                    BOOST_DEDUCED_TYPENAME range_iterator<R>::type > > base_t;

        public:
            typedef BOOST_DEDUCED_TYPENAME range_value<R>::type value_type;

            replaced_if_range( R& r, const Pred& pred, value_type to )
                : base_t( make_transform_iterator( boost::begin(r), Fn(pred, to) ),
                          make_transform_iterator( boost::end(r), Fn(pred, to) ) )
            { }
        };

        template< class Pred, class T >
        class replace_if_holder
        {
        public:
            replace_if_holder( const Pred& pred, const T& to )
                : m_pred(pred), m_to(to)
            { }

            const Pred& pred() const { return m_pred; }
            const T& to() const { return m_to; }

        private:
            Pred m_pred;
            T m_to;
        };

        template< class Pred, class SinglePassRange, class Value >
        inline replaced_if_range<Pred, SinglePassRange>
        operator|(SinglePassRange& r, const replace_if_holder<Pred, Value>& f)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<SinglePassRange>));

            return replaced_if_range<Pred, SinglePassRange>(
                r, f.pred(), f.to());
        }

        template< class Pred, class SinglePassRange, class Value >
        inline replaced_if_range<Pred, const SinglePassRange>
        operator|(const SinglePassRange& r, const replace_if_holder<Pred, Value>& f)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

            return replaced_if_range<Pred, const SinglePassRange>(
                r, f.pred(), f.to());
        }
    } // 'range_detail'

    using range_detail::replaced_if_range;

    namespace adaptors
    {
        namespace
        {
            const range_detail::forwarder2TU<range_detail::replace_if_holder>
                replaced_if =
                    range_detail::forwarder2TU<range_detail::replace_if_holder>();
        }

        template< class Pred, class SinglePassRange, class Value >
        inline replaced_if_range<Pred, SinglePassRange>
        replace_if(SinglePassRange& rng, Pred pred, Value to)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<SinglePassRange>));

            return range_detail::replaced_if_range<Pred, SinglePassRange>(
                rng, pred, to);
        }

        template< class Pred, class SinglePassRange, class Value >
        inline replaced_if_range<Pred, const SinglePassRange>
        replace_if(const SinglePassRange& rng, Pred pred, Value to)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

            return range_detail::replaced_if_range<Pred, const SinglePassRange>(
                rng, pred, to);
        }
    } // 'adaptors'

} // 'boost'

#endif // include guard
