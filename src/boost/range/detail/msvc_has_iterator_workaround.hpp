// Boost.Range library
//
//  Copyright Eric Niebler 2014. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_DETAIL_MSVC_HAS_ITERATOR_WORKAROUND_HPP
#define BOOST_RANGE_DETAIL_MSVC_HAS_ITERATOR_WORKAROUND_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#ifndef BOOST_RANGE_MUTABLE_ITERATOR_HPP
# error This file should only be included from <boost/range/mutable_iterator.hpp>
#endif

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1600)
namespace boost
{
namespace cb_details
{
    template <class Buff, class Traits>
    struct iterator;
}

namespace python
{
    template <class Container
              , class NextPolicies /*= objects::default_iterator_call_policies*/>
    struct iterator;
}

namespace type_erasure
{
    template<
        class Traversal,
        class T                 /*= _self*/,
        class Reference         /*= ::boost::use_default*/,
        class DifferenceType    /*= ::std::ptrdiff_t*/,
        class ValueType         /*= typename deduced<iterator_value_type<T> >::type*/
    >
    struct iterator;
}

namespace unordered { namespace iterator_detail
{
    template <typename Node>
    struct iterator;
}}

namespace container { namespace container_detail
{
    template<class IIterator, bool IsConst>
    class iterator;
}}

namespace spirit { namespace lex { namespace lexertl
{
    template <typename Functor>
    class iterator;
}}}

namespace range_detail
{
    template <class Buff, class Traits>
    struct has_iterator< ::boost::cb_details::iterator<Buff, Traits> >
      : mpl::false_
    {};

    template <class Buff, class Traits>
    struct has_iterator< ::boost::cb_details::iterator<Buff, Traits> const>
      : mpl::false_
    {};

    template <class Container, class NextPolicies>
    struct has_iterator< ::boost::python::iterator<Container, NextPolicies> >
      : mpl::false_
    {};

    template <class Container, class NextPolicies>
    struct has_iterator< ::boost::python::iterator<Container, NextPolicies> const>
      : mpl::false_
    {};

    template<class Traversal, class T, class Reference, class DifferenceType, class ValueType>
    struct has_iterator< ::boost::type_erasure::iterator<Traversal, T, Reference, DifferenceType, ValueType> >
      : mpl::false_
    {};

    template<class Traversal, class T, class Reference, class DifferenceType, class ValueType>
    struct has_iterator< ::boost::type_erasure::iterator<Traversal, T, Reference, DifferenceType, ValueType> const>
      : mpl::false_
    {};

    template <typename Node>
    struct has_iterator< ::boost::unordered::iterator_detail::iterator<Node> >
      : mpl::false_
    {};

    template <typename Node>
    struct has_iterator< ::boost::unordered::iterator_detail::iterator<Node> const>
      : mpl::false_
    {};

    template<class IIterator, bool IsConst>
    struct has_iterator< ::boost::container::container_detail::iterator<IIterator, IsConst> >
      : mpl::false_
    {};

    template<class IIterator, bool IsConst>
    struct has_iterator< ::boost::container::container_detail::iterator<IIterator, IsConst> const>
      : mpl::false_
    {};

    template <typename Functor>
    struct has_iterator< ::boost::spirit::lex::lexertl::iterator<Functor> >
      : mpl::false_
    {};

    template <typename Functor>
    struct has_iterator< ::boost::spirit::lex::lexertl::iterator<Functor> const>
      : mpl::false_
    {};
}
}
#endif
#endif
