// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_ITERATOR_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_ITERATOR_HPP_INCLUDED

#include <iterator>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/if.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_erasure/operators.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/deduced.hpp>
#include <boost/type_erasure/is_placeholder.hpp>

namespace boost {

namespace type_erasure {

/** INTERNAL ONLY */
template<>
struct is_placeholder< ::boost::use_default> : ::boost::mpl::false_ {};

namespace detail {

template<class T>
struct iterator_value_type_impl
{
    typedef typename ::std::iterator_traits<T>::value_type type;
};

}

/** INTERNAL ONLY */
template<class T>
struct iterator_value_type
{
    typedef typename ::boost::mpl::eval_if<
        ::boost::type_erasure::is_placeholder<T>,
        ::boost::mpl::identity<void>,
        ::boost::type_erasure::detail::iterator_value_type_impl<T>
    >::type type;
};

template<
    class Traversal,
    class T = _self,
    class Reference = ::boost::use_default,
    class DifferenceType = ::std::ptrdiff_t,
    class ValueType = typename deduced<iterator_value_type<T> >::type
>
struct iterator;

#ifdef BOOST_TYPE_ERASURE_DOXYGEN

/**
 * The @ref iterator concept can be used for any iterator category.
 *
 * \tparam Traversal must be one of @c boost::incrementable_traversal_tag,
 *         @c boost::single_pass_traversal_tag, @c boost::forward_traversal_tag,
 *         @c boost::bidirectional_traversal_tag, and @c boost::random_access_traversal_tag.
 * \tparam T The placeholder representing the iterator.
 * \tparam Reference The reference type.  If it is boost::use_default, then
 *         reference will be value_type&.
 * \tparam DifferenceType The iterator's difference type.
 *
 * The value_type of the iterator is deduced.  To force it to be
 * a specific type, use the @ref same_type concept.
 *
 * Example:
 *
 * \code
 * mpl::vector<
 *   iterator<boost::forward_traversal_tag>,
 *   same_type<iterator<boost::forward_traversal_tag>::value_type, int> > int_it;
 * \endcode
 */
template<
    class Traversal,
    class T = _self,
    class Reference = boost::use_default,
    class DifferenceType = std::ptrdiff_t
>
struct iterator
{
    typedef detail::unspecified value_type;
    typedef Reference reference;
    typedef DifferenceType difference_type;
};

template<
    class T = _self,
    class Reference = boost::use_default,
    class DifferenceType = std::ptrdiff_t
>
struct forward_iterator :
    iterator<boost::forward_traversal_tag, T, Reference, DifferenceType>
{};

template<
    class T = _self,
    class Reference = boost::use_default,
    class DifferenceType = std::ptrdiff_t
>
struct bidirectional_iterator :
    iterator<boost::bidirectional_traversal_tag, T, Reference, DifferenceType>
{};

template<
    class T = _self,
    class Reference = boost::use_default,
    class DifferenceType = std::ptrdiff_t
>
struct random_access_iterator :
    iterator<boost::random_access_traversal_tag, T, Reference, DifferenceType>
{
};

#else

/** INTERNAL ONLY */
template<class Reference, class ValueType>
struct iterator_reference
{
    typedef Reference type;
};

/** INTERNAL ONLY */
template<class ValueType>
struct iterator_reference< ::boost::use_default, ValueType>
{
    typedef ValueType& type;
};

template<class T, class Reference, class DifferenceType, class ValueType>
struct iterator< ::boost::no_traversal_tag, T, Reference, DifferenceType, ValueType> :
    boost::mpl::vector<
        copy_constructible<T>,
        constructible<T()>,
        equality_comparable<T>,
        dereferenceable<typename iterator_reference<Reference, ValueType>::type, T>,
        assignable<T>
    >
{
    typedef ValueType value_type;
    typedef typename iterator_reference<Reference, ValueType>::type reference;
    typedef DifferenceType difference_type;
};

template<class T, class Reference, class DifferenceType, class ValueType>
struct iterator< ::boost::incrementable_traversal_tag, T, Reference, DifferenceType, ValueType> :
    boost::mpl::vector<
        iterator< ::boost::no_traversal_tag, T, Reference, DifferenceType>,
        incrementable<T>
    >
{
    typedef ValueType value_type;
    typedef typename iterator_reference<Reference, ValueType>::type reference;
    typedef DifferenceType difference_type;
};

template<class T, class Reference, class DifferenceType, class ValueType>
struct iterator< ::boost::single_pass_traversal_tag, T, Reference, DifferenceType, ValueType> :
    iterator< ::boost::incrementable_traversal_tag, T, Reference, DifferenceType, ValueType>
{};

template<class T, class Reference, class DifferenceType, class ValueType>
struct iterator< ::boost::forward_traversal_tag, T, Reference, DifferenceType, ValueType> :
    iterator< ::boost::incrementable_traversal_tag, T, Reference, DifferenceType, ValueType>
{};

template<class T, class Reference, class DifferenceType, class ValueType>
struct iterator< ::boost::bidirectional_traversal_tag, T, Reference, DifferenceType, ValueType> :
    boost::mpl::vector<
        iterator< ::boost::incrementable_traversal_tag, T, Reference, DifferenceType, ValueType>,
        decrementable<T>
    >
{
    typedef ValueType value_type;
    typedef typename iterator_reference<Reference, ValueType>::type reference;
    typedef DifferenceType difference_type;
};

template<class T, class Reference, class DifferenceType, class ValueType>
struct iterator< ::boost::random_access_traversal_tag, T, Reference, DifferenceType, ValueType> :
    boost::mpl::vector<
        iterator< ::boost::bidirectional_traversal_tag, T, Reference, DifferenceType, ValueType>,
        addable<T, DifferenceType, T>,
        addable<DifferenceType, T, T>,
        subtractable<T, DifferenceType, T>,
        subtractable<T, T, DifferenceType>,
        subscriptable<typename iterator_reference<Reference, ValueType>::type, T, DifferenceType>
    >
{
    typedef ValueType value_type;
    typedef typename iterator_reference<Reference, ValueType>::type reference;
    typedef DifferenceType difference_type;
};

template<
    class T = _self,
    class Reference = ::boost::use_default,
    class DifferenceType = ::std::ptrdiff_t,
    class ValueType = typename deduced<iterator_value_type<T> >::type
>
struct forward_iterator :
    iterator< ::boost::forward_traversal_tag, T, Reference, DifferenceType, ValueType>
{};

template<
    class T = _self,
    class Reference = ::boost::use_default,
    class DifferenceType = ::std::ptrdiff_t,
    class ValueType = typename deduced<iterator_value_type<T> >::type
>
struct bidirectional_iterator :
    iterator< ::boost::bidirectional_traversal_tag, T, Reference, DifferenceType, ValueType>
{};

template<
    class T = _self,
    class Reference = ::boost::use_default,
    class DifferenceType = ::std::ptrdiff_t,
    class ValueType = typename deduced<iterator_value_type<T> >::type
>
struct random_access_iterator :
    iterator< ::boost::random_access_traversal_tag, T, Reference, DifferenceType, ValueType>
{
};

#endif

/// \cond show_operators

template<class T, class Reference, class DifferenceType, class ValueType, class Base>
struct concept_interface<iterator< ::boost::no_traversal_tag, T, Reference, DifferenceType, ValueType>, Base, T>
    : Base
{
    typedef typename rebind_any<Base, ValueType>::type value_type;
    typedef typename rebind_any<
        Base, 
        typename iterator_reference<Reference, ValueType>::type
    >::type reference;
    typedef DifferenceType difference_type;
    typedef typename ::boost::mpl::if_< ::boost::is_reference<reference>,
        typename ::boost::remove_reference<reference>::type*,
        value_type*
    >::type pointer;
};

template<class T, class Reference, class DifferenceType, class ValueType, class Base>
struct concept_interface<iterator< ::boost::forward_traversal_tag, T, Reference, DifferenceType, ValueType>, Base, T>
    : Base
{
    typedef std::forward_iterator_tag iterator_category;
};

template<class T, class Reference, class DifferenceType, class ValueType, class Base>
struct concept_interface<forward_iterator<T, Reference, DifferenceType, ValueType>, Base, T>
    : Base
{
    typedef std::forward_iterator_tag iterator_category;
};

template<class T, class Reference, class DifferenceType, class ValueType, class Base>
struct concept_interface<iterator< ::boost::bidirectional_traversal_tag, T, Reference, DifferenceType, ValueType>, Base, T>
    : Base
{
    typedef std::bidirectional_iterator_tag iterator_category;
};

template<class T, class Reference, class DifferenceType, class ValueType, class Base>
struct concept_interface<bidirectional_iterator<T, Reference, DifferenceType, ValueType>, Base, T>
    : Base
{
    typedef std::bidirectional_iterator_tag iterator_category;
};

template<class T, class Reference, class DifferenceType, class ValueType, class Base>
struct concept_interface<iterator< ::boost::random_access_traversal_tag, T, Reference, DifferenceType, ValueType>, Base, T>
    : Base
{
    typedef std::random_access_iterator_tag iterator_category;
};

template<class T, class Reference, class DifferenceType, class ValueType, class Base>
struct concept_interface<random_access_iterator<T, Reference, DifferenceType, ValueType>, Base, T>
    : Base
{
    typedef std::random_access_iterator_tag iterator_category;
};

/// \endcond

}
}

#endif
