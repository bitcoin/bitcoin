//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ANY_RANGE_HPP_INCLUDED
#define BOOST_RANGE_ANY_RANGE_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range/detail/any_iterator.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/reference.hpp>
#include <boost/range/value_type.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace boost
{
    namespace range_detail
    {
        // If T is use_default, return the result of Default, otherwise
        // return T.
        //
        // This is an implementation artifact used to pick intelligent default
        // values when the user specified boost::use_default as a template
        // parameter.
        template<
            class T,
            class Default
        >
        struct any_range_default_help
            : mpl::eval_if<
                is_same<T, use_default>
              , Default
              , mpl::identity<T>
            >
        {
        };

        template<
            class WrappedRange
          , class Value
          , class Reference
        >
        struct any_range_value_type
        {
# ifdef BOOST_ITERATOR_REF_CONSTNESS_KILLS_WRITABILITY
            typedef typename any_range_default_help<
                    Value
                  , mpl::eval_if<
                        is_same<Reference, use_default>
                      , range_value<
                            typename remove_const<WrappedRange>
                        ::type>
                      , remove_reference<Reference>
                    >
                >::type type;
# else
            typedef typename any_range_default_help<
                Value
              , range_value<
                    typename remove_const<WrappedRange>
                ::type>
            >::type type;
# endif
        };

        template<
            class Value
          , class Traversal
          , class Reference = Value&
          , class Difference = std::ptrdiff_t
          , class Buffer = use_default
        >
        class any_range
            : public iterator_range<
                        any_iterator<
                            Value
                          , Traversal
                          , Reference
                          , Difference
                          , typename any_range_default_help<
                                Buffer
                              , mpl::identity<any_iterator_default_buffer>
                            >::type
                        >
                    >
        {
            typedef iterator_range<
                        any_iterator<
                            Value
                          , Traversal
                          , Reference
                          , Difference
                          , typename any_range_default_help<
                                Buffer
                              , mpl::identity<any_iterator_default_buffer>
                            >::type
                        >
                    > base_type;

            struct enabler {};
            struct disabler {};
        public:
            any_range()
            {
            }

            any_range(const any_range& other)
                : base_type(other)
            {
            }

            template<class WrappedRange>
            any_range(WrappedRange& wrapped_range)
            : base_type(boost::begin(wrapped_range),
                        boost::end(wrapped_range))
            {
            }

            template<class WrappedRange>
            any_range(const WrappedRange& wrapped_range)
            : base_type(boost::begin(wrapped_range),
                        boost::end(wrapped_range))
            {
            }

            template<
                class OtherValue
              , class OtherTraversal
              , class OtherReference
              , class OtherDifference
            >
            any_range(const any_range<
                                OtherValue
                              , OtherTraversal
                              , OtherReference
                              , OtherDifference
                              , Buffer
                            >& other)
            : base_type(boost::begin(other), boost::end(other))
            {
            }

            template<class Iterator>
            any_range(Iterator first, Iterator last)
                : base_type(first, last)
            {
            }
        };

        template<
            class WrappedRange
          , class Value = use_default
          , class Traversal = use_default
          , class Reference = use_default
          , class Difference = use_default
          , class Buffer = use_default
        >
        struct any_range_type_generator
        {
            BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<WrappedRange> ));
            typedef any_range<
                typename any_range_value_type<
                    WrappedRange
                  , Value
                  , typename any_range_default_help<
                        Reference
                      , range_reference<WrappedRange>
                    >::type
                >::type
              , typename any_range_default_help<
                            Traversal
                          , iterator_traversal<
                                typename range_iterator<WrappedRange>::type
                            >
                        >::type
              , typename any_range_default_help<
                    Reference
                  , range_reference<WrappedRange>
                >::type
              , typename any_range_default_help<
                    Difference
                  , range_difference<WrappedRange>
                >::type
              , typename any_range_default_help<
                    Buffer
                  , mpl::identity<any_iterator_default_buffer>
                >::type
            > type;
        };
    } // namespace range_detail

    using range_detail::any_range;
    using range_detail::any_range_type_generator;
} // namespace boost

#endif // include guard
