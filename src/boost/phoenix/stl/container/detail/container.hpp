/*=============================================================================
    Copyright (c) 2004 Angus Leeming
    Copyright (c) 2004 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CONTAINER_DETAIL_CONTAINER_HPP
#define BOOST_PHOENIX_CONTAINER_DETAIL_CONTAINER_HPP

#include <utility>
#include <boost/mpl/eval_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace boost { namespace phoenix { namespace stl
{
///////////////////////////////////////////////////////////////////////////////
//
//  Metafunctions "value_type_of", "key_type_of" etc.
//
//      These metafunctions define a typedef "type" that returns the nested
//      type if it exists. If not then the typedef returns void.
//
//      For example, "value_type_of<std::vector<int> >::type" is "int" whilst
//      "value_type_of<double>::type" is "void".
//
//      I use a macro to define structs "value_type_of" etc simply to cut
//      down on the amount of code. The macro is #undef-ed immediately after
//      its final use.
//
/////////////////////////////////////////////////////////////////c//////////////
#define MEMBER_TYPE_OF(MEMBER_TYPE)                                             \
    template <typename C>                                                       \
    struct BOOST_PP_CAT(MEMBER_TYPE, _of)                                       \
    {                                                                           \
        typedef typename C::MEMBER_TYPE type;                                   \
    }

    MEMBER_TYPE_OF(allocator_type);
    MEMBER_TYPE_OF(const_iterator);
    MEMBER_TYPE_OF(const_reference);
    MEMBER_TYPE_OF(const_reverse_iterator);
    MEMBER_TYPE_OF(container_type);
    MEMBER_TYPE_OF(data_type);
    MEMBER_TYPE_OF(iterator);
    MEMBER_TYPE_OF(key_compare);
    MEMBER_TYPE_OF(key_type);
    MEMBER_TYPE_OF(reference);
    MEMBER_TYPE_OF(reverse_iterator);
    MEMBER_TYPE_OF(size_type);
    MEMBER_TYPE_OF(value_compare);
    MEMBER_TYPE_OF(value_type);

#undef MEMBER_TYPE_OF

///////////////////////////////////////////////////////////////////////////////
//
//  Const-Qualified types.
//
//      Many of the stl member functions have const and non-const
//      overloaded versions that return distinct types. For example:
//
//          iterator begin();
//          const_iterator begin() const;
//
//      The three class templates defined below,
//      const_qualified_reference_of, const_qualified_iterator_of
//      and const_qualified_reverse_iterator_of provide a means to extract
//      this return type automatically.
//
///////////////////////////////////////////////////////////////////////////////
    template <typename C>
    struct const_qualified_reference_of
    {
        typedef typename
            boost::mpl::eval_if_c<
                boost::is_const<C>::value
              , const_reference_of<C>
              , reference_of<C>
            >::type
        type;
    };

    template <typename C>
    struct const_qualified_iterator_of
    {
        typedef typename
            boost::mpl::eval_if_c<
                boost::is_const<C>::value
              , const_iterator_of<C>
              , iterator_of<C>
            >::type
        type;
    };

    template <typename C>
    struct const_qualified_reverse_iterator_of
    {
        typedef typename
            boost::mpl::eval_if_c<
                boost::is_const<C>::value
              , const_reverse_iterator_of<C>
              , reverse_iterator_of<C>
            >::type
        type;
    };

///////////////////////////////////////////////////////////////////////////////
//
//  has_mapped_type<C>
//
//      Given a container C, determine if it is a map, multimap, unordered_map,
//      or unordered_multimap by checking if it has a member type named "mapped_type".
//
///////////////////////////////////////////////////////////////////////////////
    namespace stl_impl
    {
        struct one { char a[1]; };
        struct two { char a[2]; };

        template <typename C>
        one has_mapped_type(typename C::mapped_type(*)());

        template <typename C>
        two has_mapped_type(...);
    }

    template <typename C>
    struct has_mapped_type
        : boost::mpl::bool_<
            sizeof(stl_impl::has_mapped_type<C>(0)) == sizeof(stl_impl::one)
        >
    {};

///////////////////////////////////////////////////////////////////////////////
//
//  has_key_type<C>
//
//      Given a container C, determine if it is a Associative Container
//      by checking if it has a member type named "key_type".
//
///////////////////////////////////////////////////////////////////////////////
    namespace stl_impl
    {
        template <typename C>
        one has_key_type(typename C::key_type(*)());

        template <typename C>
        two has_key_type(...);
    }

    template <typename C>
    struct has_key_type
        : boost::mpl::bool_<
            sizeof(stl_impl::has_key_type<C>(0)) == sizeof(stl_impl::one)
        >
    {};

///////////////////////////////////////////////////////////////////////////////
//
//  is_key_type_of<C, Arg>
//
//      Lazy evaluation friendly predicate.
//
///////////////////////////////////////////////////////////////////////////////

    template <typename C, typename Arg>
    struct is_key_type_of
        : boost::is_convertible<Arg, typename key_type_of<C>::type>
    {};

///////////////////////////////////////////////////////////////////////////////
//
//  map_insert_returns_pair<C>
//
//      Distinguish a map from a multimap by checking the return type
//      of its "insert" member function. A map returns a pair while
//      a multimap returns an iterator.
//
///////////////////////////////////////////////////////////////////////////////
    namespace stl_impl
    {
        //  Cool implementation of map_insert_returns_pair by Daniel Wallin.
        //  Thanks Daniel!!! I owe you a Pizza!

        template<class A, class B>
        one map_insert_returns_pair_check(std::pair<A,B> const&);

        template <typename T>
        two map_insert_returns_pair_check(T const&);

        template <typename C>
        struct map_insert_returns_pair
        {
            static typename C::value_type const& get;
            BOOST_STATIC_CONSTANT(int,
                value = sizeof(
                    map_insert_returns_pair_check(((C*)0)->insert(get))));
            typedef boost::mpl::bool_<value == sizeof(one)> type;
        };
    }

    template <typename C>
    struct map_insert_returns_pair
        : stl_impl::map_insert_returns_pair<C>::type {};

}}} // namespace boost::phoenix::stl

#endif // BOOST_PHOENIX_STL_CONTAINER_TRAITS_HPP
