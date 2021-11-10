// Boost.Range library
//
//  Copyright Neil Groves 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_REPLACED_IMPL_HPP_INCLUDED
#define BOOST_RANGE_ADAPTOR_REPLACED_IMPL_HPP_INCLUDED

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
        template< class Value >
        class replace_value
        {
        public:
            typedef const Value& result_type;
            typedef const Value& first_argument_type;

            // Rationale:
            // The default constructor is required to allow the transform
            // iterator to properly model the iterator concept.
            replace_value()
            {
            }

            replace_value(const Value& from, const Value& to)
                :   m_impl(data(from, to))
            {
            }

            const Value& operator()(const Value& x) const
            {
                return (x == m_impl->m_from) ? m_impl->m_to : x;
            }

        private:
            struct data
            {
                data(const Value& from, const Value& to)
                    : m_from(from)
                    , m_to(to)
                {
                }

                Value m_from;
                Value m_to;
            };
            boost::optional<data> m_impl;
        };

        template< class R >
        class replaced_range :
            public boost::iterator_range<
                boost::transform_iterator<
                    replace_value< BOOST_DEDUCED_TYPENAME range_value<R>::type >,
                    BOOST_DEDUCED_TYPENAME range_iterator<R>::type > >
        {
        private:
            typedef replace_value< BOOST_DEDUCED_TYPENAME range_value<R>::type > Fn;

            typedef boost::iterator_range<
                boost::transform_iterator<
                    replace_value< BOOST_DEDUCED_TYPENAME range_value<R>::type >,
                    BOOST_DEDUCED_TYPENAME range_iterator<R>::type > > base_t;

        public:
            typedef BOOST_DEDUCED_TYPENAME range_value<R>::type value_type;

            replaced_range( R& r, value_type from, value_type to )
                : base_t( make_transform_iterator( boost::begin(r), Fn(from, to) ),
                          make_transform_iterator( boost::end(r), Fn(from, to) ) )
            { }
        };

        template< class T >
        class replace_holder : public holder2<T>
        {
        public:
            replace_holder( const T& from, const T& to )
                : holder2<T>(from, to)
            { }
        private:
            // not assignable
            void operator=(const replace_holder&);
        };

        template< class SinglePassRange, class Value >
        inline replaced_range<SinglePassRange>
        operator|(SinglePassRange& r, const replace_holder<Value>& f)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<SinglePassRange>));

            return replaced_range<SinglePassRange>(r, f.val1, f.val2);
        }

        template< class SinglePassRange, class Value >
        inline replaced_range<const SinglePassRange>
        operator|(const SinglePassRange& r, const replace_holder<Value>& f)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

            return replaced_range<const SinglePassRange>(r, f.val1, f.val2);
        }
    } // 'range_detail'

    using range_detail::replaced_range;

    namespace adaptors
    {
        namespace
        {
            const range_detail::forwarder2<range_detail::replace_holder>
                replaced =
                    range_detail::forwarder2<range_detail::replace_holder>();
        }

        template< class SinglePassRange, class Value >
        inline replaced_range<SinglePassRange>
        replace(SinglePassRange& rng, Value from, Value to)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<SinglePassRange>));

            return replaced_range<SinglePassRange>(rng, from, to);
        }

        template< class SinglePassRange, class Value >
        inline replaced_range<const SinglePassRange>
        replace(const SinglePassRange& rng, Value from, Value to)
        {
            BOOST_RANGE_CONCEPT_ASSERT((
                SinglePassRangeConcept<const SinglePassRange>));

            return replaced_range<const SinglePassRange>(rng, from ,to);
        }

    } // 'adaptors'
} // 'boost'

#endif // include guard
