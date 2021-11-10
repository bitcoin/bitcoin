// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_DETAIL_ANY_ITERATOR_INTERFACE_HPP_INCLUDED
#define BOOST_RANGE_DETAIL_ANY_ITERATOR_INTERFACE_HPP_INCLUDED

#include <boost/range/detail/any_iterator_buffer.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost
{
    namespace range_detail
    {
        template<class T>
        struct const_reference_type_generator
        {
            typedef typename mpl::if_<
                typename is_reference<T>::type,
                typename add_const<
                    typename remove_reference<T>::type
                >::type&,
                T
            >::type type;
        };

        template<class T>
        struct reference_as_value_type_generator
        {
            typedef typename remove_reference<
                typename remove_const<T>::type
            >::type value_type;

            typedef typename mpl::if_<
                typename is_convertible<const value_type&, value_type>::type,
                value_type,
                T
            >::type type;
        };

        template<
            class Reference
          , class Buffer
        >
        struct any_incrementable_iterator_interface
        {
            typedef Reference reference;
            typedef typename const_reference_type_generator<
                Reference
            >::type const_reference;
            typedef typename reference_as_value_type_generator<
                Reference
            >::type reference_as_value_type;

            typedef Buffer buffer_type;

            virtual ~any_incrementable_iterator_interface() {}

            virtual any_incrementable_iterator_interface*
                        clone(buffer_type& buffer) const = 0;

            virtual any_incrementable_iterator_interface<const_reference, Buffer>*
                        clone_const_ref(buffer_type& buffer) const = 0;

            virtual any_incrementable_iterator_interface<reference_as_value_type, Buffer>*
                        clone_reference_as_value(buffer_type& buffer) const = 0;

            virtual void increment() = 0;
        };

        template<
            class Reference
          , class Buffer
        >
        struct any_single_pass_iterator_interface
            : any_incrementable_iterator_interface<Reference, Buffer>
        {
            typedef typename any_incrementable_iterator_interface<Reference, Buffer>::reference reference;
            typedef typename any_incrementable_iterator_interface<Reference, Buffer>::const_reference const_reference;
            typedef typename any_incrementable_iterator_interface<Reference, Buffer>::buffer_type buffer_type;
            typedef typename any_incrementable_iterator_interface<Reference, Buffer>::reference_as_value_type reference_as_value_type;

            virtual any_single_pass_iterator_interface*
                        clone(buffer_type& buffer) const = 0;

            virtual any_single_pass_iterator_interface<const_reference, Buffer>*
                        clone_const_ref(buffer_type& buffer) const = 0;

            virtual any_single_pass_iterator_interface<reference_as_value_type, Buffer>*
                        clone_reference_as_value(buffer_type& buffer) const = 0;

            virtual reference dereference() const = 0;

            virtual bool equal(const any_single_pass_iterator_interface& other) const = 0;
        };

        template<
            class Reference
          , class Buffer
        >
        struct any_forward_iterator_interface
            : any_single_pass_iterator_interface<Reference, Buffer>
        {
            typedef typename any_single_pass_iterator_interface<Reference, Buffer>::reference reference;
            typedef typename any_single_pass_iterator_interface<Reference, Buffer>::const_reference const_reference;
            typedef typename any_single_pass_iterator_interface<Reference, Buffer>::buffer_type buffer_type;
            typedef typename any_single_pass_iterator_interface<Reference, Buffer>::reference_as_value_type reference_as_value_type;

            virtual any_forward_iterator_interface*
                        clone(buffer_type& buffer) const = 0;

            virtual any_forward_iterator_interface<const_reference, Buffer>*
                        clone_const_ref(buffer_type& buffer) const = 0;

            virtual any_forward_iterator_interface<reference_as_value_type, Buffer>*
                        clone_reference_as_value(buffer_type& buffer) const = 0;
        };

        template<
            class Reference
          , class Buffer
        >
        struct any_bidirectional_iterator_interface
            : any_forward_iterator_interface<Reference, Buffer>
        {
            typedef typename any_forward_iterator_interface<Reference, Buffer>::reference reference;
            typedef typename any_forward_iterator_interface<Reference, Buffer>::const_reference const_reference;
            typedef typename any_forward_iterator_interface<Reference, Buffer>::buffer_type buffer_type;
            typedef typename any_forward_iterator_interface<Reference, Buffer>::reference_as_value_type reference_as_value_type;

            virtual any_bidirectional_iterator_interface*
                        clone(buffer_type& buffer) const = 0;

            virtual any_bidirectional_iterator_interface<const_reference, Buffer>*
                        clone_const_ref(buffer_type& buffer) const = 0;

            virtual any_bidirectional_iterator_interface<reference_as_value_type, Buffer>*
                        clone_reference_as_value(buffer_type& buffer) const = 0;

            virtual void decrement() = 0;
        };

        template<
            class Reference
          , class Difference
          , class Buffer
        >
        struct any_random_access_iterator_interface
            : any_bidirectional_iterator_interface<
                    Reference
                  , Buffer
                >
        {
            typedef typename any_bidirectional_iterator_interface<Reference, Buffer>::reference reference;
            typedef typename any_bidirectional_iterator_interface<Reference, Buffer>::const_reference const_reference;
            typedef typename any_bidirectional_iterator_interface<Reference, Buffer>::buffer_type buffer_type;
            typedef typename any_bidirectional_iterator_interface<Reference, Buffer>::reference_as_value_type reference_as_value_type;
            typedef Difference difference_type;

            virtual any_random_access_iterator_interface*
                        clone(buffer_type& buffer) const = 0;

            virtual any_random_access_iterator_interface<const_reference, Difference, Buffer>*
                        clone_const_ref(buffer_type& buffer) const = 0;

            virtual any_random_access_iterator_interface<reference_as_value_type, Difference, Buffer>*
                        clone_reference_as_value(buffer_type& buffer) const = 0;

            virtual void advance(Difference offset) = 0;

            virtual Difference distance_to(const any_random_access_iterator_interface& other) const = 0;
        };

        template<
            class Traversal
          , class Reference
          , class Difference
          , class Buffer
        >
        struct any_iterator_interface_type_generator;

        template<
            class Reference
          , class Difference
          , class Buffer
        >
        struct any_iterator_interface_type_generator<
                    incrementable_traversal_tag
                  , Reference
                  , Difference
                  , Buffer
                >
        {
            typedef any_incrementable_iterator_interface<Reference, Buffer> type;
        };

        template<
            class Reference
          , class Difference
          , class Buffer
        >
        struct any_iterator_interface_type_generator<
                    single_pass_traversal_tag
                  , Reference
                  , Difference
                  , Buffer
                >
        {
            typedef any_single_pass_iterator_interface<Reference, Buffer> type;
        };

        template<
            class Reference
          , class Difference
          , class Buffer
        >
        struct any_iterator_interface_type_generator<
                    forward_traversal_tag
                  , Reference
                  , Difference
                  , Buffer
                >
        {
            typedef any_forward_iterator_interface<Reference, Buffer> type;
        };

        template<
            class Reference
          , class Difference
          , class Buffer
        >
        struct any_iterator_interface_type_generator<
                    bidirectional_traversal_tag
                  , Reference
                  , Difference
                  , Buffer
                >
        {
            typedef any_bidirectional_iterator_interface<Reference, Buffer> type;
        };

        template<
            class Reference
          , class Difference
          , class Buffer
        >
        struct any_iterator_interface_type_generator<
                    random_access_traversal_tag
                  , Reference
                  , Difference
                  , Buffer
                >
        {
            typedef any_random_access_iterator_interface<
                        Reference
                      , Difference
                      , Buffer
                    > type;
        };

    } // namespace range_detail
} // namespace boost

#endif // include guard
