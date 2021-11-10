// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_DETAIL_ANY_ITERATOR_HPP_INCLUDED
#define BOOST_RANGE_DETAIL_ANY_ITERATOR_HPP_INCLUDED

#include <boost/mpl/and.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/not.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/range/detail/any_iterator_buffer.hpp>
#include <boost/range/detail/any_iterator_interface.hpp>
#include <boost/range/detail/any_iterator_wrapper.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost
{
    namespace range_detail
    {
        // metafunction to determine if T is a const reference
        template<class T>
        struct is_const_reference
        {
            typedef typename mpl::and_<
                typename is_reference<T>::type,
                typename is_const<
                    typename remove_reference<T>::type
                >::type
            >::type type;
        };

        // metafunction to determine if T is a mutable reference
        template<class T>
        struct is_mutable_reference
        {
            typedef typename mpl::and_<
                typename is_reference<T>::type,
                typename mpl::not_<
                    typename is_const<
                        typename remove_reference<T>::type
                    >::type
                >::type
            >::type type;
        };

        // metafunction to evaluate if a source 'reference' can be
        // converted to a target 'reference' as a value.
        //
        // This is true, when the target reference type is actually
        // not a reference, and the source reference is convertible
        // to the target type.
        template<class SourceReference, class TargetReference>
        struct is_convertible_to_value_as_reference
        {
            typedef typename mpl::and_<
                typename mpl::not_<
                    typename is_reference<TargetReference>::type
                >::type
              , typename is_convertible<
                    SourceReference
                  , TargetReference
                >::type
            >::type type;
        };

        template<
            class Value
          , class Traversal
          , class Reference
          , class Difference
          , class Buffer = any_iterator_default_buffer
        >
        class any_iterator;

        // metafunction to determine if SomeIterator is an
        // any_iterator.
        //
        // This is the general implementation which evaluates to false.
        template<class SomeIterator>
        struct is_any_iterator
            : mpl::bool_<false>
        {
        };

        // specialization of is_any_iterator to return true for
        // any_iterator classes regardless of template parameters.
        template<
            class Value
          , class Traversal
          , class Reference
          , class Difference
          , class Buffer
        >
        struct is_any_iterator<
            any_iterator<
                Value
              , Traversal
              , Reference
              , Difference
              , Buffer
            >
        >
            : mpl::bool_<true>
        {
        };
    } // namespace range_detail

    namespace iterators
    {
    namespace detail
    {
        // Rationale:
        // These are specialized since the iterator_facade versions lack
        // the requisite typedefs to allow wrapping to determine the types
        // if a user copy constructs from a postfix increment.

        template<
            class Value
          , class Traversal
          , class Reference
          , class Difference
          , class Buffer
        >
        class postfix_increment_proxy<
                    range_detail::any_iterator<
                        Value
                      , Traversal
                      , Reference
                      , Difference
                      , Buffer
                    >
                >
        {
            typedef range_detail::any_iterator<
                Value
              , Traversal
              , Reference
              , Difference
              , Buffer
            > any_iterator_type;

        public:
            typedef Value value_type;
            typedef typename std::iterator_traits<any_iterator_type>::iterator_category iterator_category;
            typedef Difference difference_type;
            typedef typename iterator_pointer<any_iterator_type>::type pointer;
            typedef Reference reference;

            explicit postfix_increment_proxy(any_iterator_type const& x)
                : stored_value(*x)
            {}

            value_type&
            operator*() const
            {
                return this->stored_value;
            }
        private:
            mutable value_type stored_value;
        };

        template<
            class Value
          , class Traversal
          , class Reference
          , class Difference
          , class Buffer
        >
        class writable_postfix_increment_proxy<
                    range_detail::any_iterator<
                        Value
                      , Traversal
                      , Reference
                      , Difference
                      , Buffer
                    >
                >
        {
            typedef range_detail::any_iterator<
                        Value
                      , Traversal
                      , Reference
                      , Difference
                      , Buffer
                    > any_iterator_type;
         public:
            typedef Value value_type;
            typedef typename std::iterator_traits<any_iterator_type>::iterator_category iterator_category;
            typedef Difference difference_type;
            typedef typename iterator_pointer<any_iterator_type>::type pointer;
            typedef Reference reference;

            explicit writable_postfix_increment_proxy(any_iterator_type const& x)
              : stored_value(*x)
              , stored_iterator(x)
            {}

            // Dereferencing must return a proxy so that both *r++ = o and
            // value_type(*r++) can work.  In this case, *r is the same as
            // *r++, and the conversion operator below is used to ensure
            // readability.
            writable_postfix_increment_proxy const&
            operator*() const
            {
                return *this;
            }

            // Provides readability of *r++
            operator value_type&() const
            {
                return stored_value;
            }

            // Provides writability of *r++
            template <class T>
            T const& operator=(T const& x) const
            {
                *this->stored_iterator = x;
                return x;
            }

            // This overload just in case only non-const objects are writable
            template <class T>
            T& operator=(T& x) const
            {
                *this->stored_iterator = x;
                return x;
            }

            // Provides X(r++)
            operator any_iterator_type const&() const
            {
                return stored_iterator;
            }

         private:
            mutable value_type stored_value;
            any_iterator_type stored_iterator;
        };

    } //namespace detail
    } //namespace iterators

    namespace range_detail
    {
        template<
            class Value
          , class Traversal
          , class Reference
          , class Difference
          , class Buffer
        >
        class any_iterator
            : public iterator_facade<
                        any_iterator<
                            Value
                          , Traversal
                          , Reference
                          , Difference
                          , Buffer
                        >
                    , Value
                    , Traversal
                    , Reference
                    , Difference
                >
        {
            template<
                class OtherValue
              , class OtherTraversal
              , class OtherReference
              , class OtherDifference
              , class OtherBuffer
            >
            friend class any_iterator;

            struct enabler {};
            struct disabler {};

            typedef typename any_iterator_interface_type_generator<
                Traversal
              , Reference
              , Difference
              , Buffer
            >::type abstract_base_type;

            typedef iterator_facade<
                        any_iterator<
                            Value
                          , Traversal
                          , Reference
                          , Difference
                          , Buffer
                        >
                      , Value
                      , Traversal
                      , Reference
                      , Difference
                  > base_type;

            typedef Buffer buffer_type;

        public:
            typedef typename base_type::value_type value_type;
            typedef typename base_type::reference reference;
            typedef typename base_type::difference_type difference_type;

            // Default constructor
            any_iterator()
                : m_impl(0) {}

            // Simple copy construction without conversion
            any_iterator(const any_iterator& other)
                : base_type(other)
                , m_impl(other.m_impl
                            ? other.m_impl->clone(m_buffer)
                            : 0)
            {
            }

            // Simple assignment operator without conversion
            any_iterator& operator=(const any_iterator& other)
            {
                if (this != &other)
                {
                    if (m_impl)
                        m_impl->~abstract_base_type();
                    m_buffer.deallocate();
                    m_impl = 0;
                    if (other.m_impl)
                        m_impl = other.m_impl->clone(m_buffer);
                }
                return *this;
            }

            // Implicit conversion from another any_iterator where the
            // conversion is from a non-const reference to a const reference
            template<
                class OtherValue
              , class OtherTraversal
              , class OtherReference
              , class OtherDifference
            >
            any_iterator(const any_iterator<
                                OtherValue,
                                OtherTraversal,
                                OtherReference,
                                OtherDifference,
                                Buffer
                            >& other,
                         typename ::boost::enable_if<
                            typename mpl::and_<
                                typename is_mutable_reference<OtherReference>::type,
                                typename is_const_reference<Reference>::type
                            >::type,
                            enabler
                        >::type* = 0
                    )
                : m_impl(other.m_impl
                            ? other.m_impl->clone_const_ref(m_buffer)
                         : 0
                        )
            {
            }

            // Implicit conversion from another any_iterator where the
            // reference types of the source and the target are references
            // that are either both const, or both non-const.
            template<
                class OtherValue
              , class OtherTraversal
              , class OtherReference
              , class OtherDifference
            >
            any_iterator(const any_iterator<
                                OtherValue
                              , OtherTraversal
                              , OtherReference
                              , OtherDifference
                              , Buffer
                            >& other,
                         typename ::boost::enable_if<
                            typename mpl::or_<
                                typename mpl::and_<
                                    typename is_mutable_reference<OtherReference>::type,
                                    typename is_mutable_reference<Reference>::type
                                >::type,
                                typename mpl::and_<
                                    typename is_const_reference<OtherReference>::type,
                                    typename is_const_reference<Reference>::type
                                >::type
                            >::type,
                            enabler
                        >::type* = 0
                        )
                : m_impl(other.m_impl
                            ? other.m_impl->clone(m_buffer)
                         : 0
                        )
            {
            }

            // Implicit conversion to an any_iterator that uses a value for
            // the reference type.
            template<
                class OtherValue
              , class OtherTraversal
              , class OtherReference
              , class OtherDifference
            >
            any_iterator(const any_iterator<
                                OtherValue
                              , OtherTraversal
                              , OtherReference
                              , OtherDifference
                              , Buffer
                            >& other,
                        typename ::boost::enable_if<
                            typename is_convertible_to_value_as_reference<
                                        OtherReference
                                      , Reference
                                    >::type,
                            enabler
                        >::type* = 0
                        )
                : m_impl(other.m_impl
                            ? other.m_impl->clone_reference_as_value(m_buffer)
                            : 0
                            )
            {
            }

            any_iterator clone() const
            {
                any_iterator result;
                if (m_impl)
                    result.m_impl = m_impl->clone(result.m_buffer);
                return result;
            }

            any_iterator<
                Value
              , Traversal
              , typename abstract_base_type::const_reference
              , Difference
              , Buffer
            >
            clone_const_ref() const
            {
                typedef any_iterator<
                    Value
                  , Traversal
                  , typename abstract_base_type::const_reference
                  , Difference
                  , Buffer
                > result_type;

                result_type result;

                if (m_impl)
                    result.m_impl = m_impl->clone_const_ref(result.m_buffer);

                return result;
            }

            // implicit conversion and construction from type-erasure-compatible
            // iterators
            template<class WrappedIterator>
            explicit any_iterator(
                const WrappedIterator& wrapped_iterator,
                typename disable_if<
                    typename is_any_iterator<WrappedIterator>::type
                  , disabler
                >::type* = 0
                )
            {
                typedef typename any_iterator_wrapper_type_generator<
                            WrappedIterator
                          , Traversal
                          , Reference
                          , Difference
                          , Buffer
                        >::type wrapper_type;

                void* ptr = m_buffer.allocate(sizeof(wrapper_type));
                m_impl = new(ptr) wrapper_type(wrapped_iterator);
            }

            ~any_iterator()
            {
                // manually run the destructor, the deallocation is automatically
                // handled by the any_iterator_small_buffer base class.
                if (m_impl)
                    m_impl->~abstract_base_type();
            }

        private:
            friend class ::boost::iterator_core_access;

            Reference dereference() const
            {
                BOOST_ASSERT( m_impl );
                return m_impl->dereference();
            }

            bool equal(const any_iterator& other) const
            {
                return (m_impl == other.m_impl)
                    || (m_impl && other.m_impl && m_impl->equal(*other.m_impl));
            }

            void increment()
            {
                BOOST_ASSERT( m_impl );
                m_impl->increment();
            }

            void decrement()
            {
                BOOST_ASSERT( m_impl );
                m_impl->decrement();
            }

            Difference distance_to(const any_iterator& other) const
            {
                return m_impl && other.m_impl
                    ? m_impl->distance_to(*other.m_impl)
                    : 0;
            }

            void advance(Difference offset)
            {
                BOOST_ASSERT( m_impl );
                m_impl->advance(offset);
            }

            any_iterator& swap(any_iterator& other)
            {
                BOOST_ASSERT( this != &other );
                // grab a temporary copy of the other iterator
                any_iterator tmp(other);

                // deallocate the other iterator, taking care to obey the
                // class-invariants in-case of exceptions later
                if (other.m_impl)
                {
                    other.m_impl->~abstract_base_type();
                    other.m_buffer.deallocate();
                    other.m_impl = 0;
                }

                // If this is a non-null iterator then we need to put
                // a clone of this iterators implementation into the other
                // iterator.
                // We can't just swap because of the small buffer optimization.
                if (m_impl)
                {
                    other.m_impl = m_impl->clone(other.m_buffer);
                    m_impl->~abstract_base_type();
                    m_buffer.deallocate();
                    m_impl = 0;
                }

                // assign to this instance a clone of the temporarily held
                // tmp which represents the input other parameter at the
                // start of execution of this function.
                if (tmp.m_impl)
                    m_impl = tmp.m_impl->clone(m_buffer);

                return *this;
            }

            buffer_type m_buffer;
            abstract_base_type* m_impl;
        };

    } // namespace range_detail
} // namespace boost

#endif // include guard
