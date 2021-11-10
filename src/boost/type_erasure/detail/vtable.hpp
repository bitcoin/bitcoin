// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#if !defined(BOOST_PP_IS_ITERATING)

#ifndef BOOST_TYPE_ERASURE_DETAIL_VTABLE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_VTABLE_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/expr_if.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/type_erasure/detail/rebind_placeholders.hpp>
#include <boost/type_erasure/config.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_CONSTEXPR) && !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)

template<class... T>
struct stored_arg_pack;

template<class It, class End, class... T>
struct make_arg_pack_impl
{
    typedef typename make_arg_pack_impl<
        typename ::boost::mpl::next<It>::type,
        End,
        T...,
        typename ::boost::mpl::deref<It>::type
    >::type type;
};

template<class End, class... T>
struct make_arg_pack_impl<End, End, T...>
{
    typedef stored_arg_pack<T...> type;
};

template<class Seq>
struct make_arg_pack
{
    typedef typename make_arg_pack_impl<
        typename ::boost::mpl::begin<Seq>::type,
        typename ::boost::mpl::end<Seq>::type
    >::type type;
};

template<class Args>
struct make_vtable_impl;

template<class Seq>
struct make_vtable
{
    typedef typename ::boost::type_erasure::detail::make_vtable_impl<
        typename ::boost::type_erasure::detail::make_arg_pack<Seq>::type
    >::type type;
};

template<class Table, class Args>
struct make_vtable_init_impl;

template<class Seq, class Table>
struct make_vtable_init
{
    typedef typename make_vtable_init_impl<
        Table,
        typename ::boost::type_erasure::detail::make_arg_pack<Seq>::type
    >::type type;
};

template<class T>
struct vtable_entry
{
    typename T::type value;
    vtable_entry() = default;
    constexpr vtable_entry(typename T::type arg) : value(arg) {}
};

template<class... T>
struct compare_vtable;

template<>
struct compare_vtable<> {
    template<class S>
    static bool apply(const S& /*s1*/, const S& /*s2*/)
    {
        return true;
    }
};

template<class T0, class... T>
struct compare_vtable<T0, T...> {
    template<class S>
    static bool apply(const S& s1, const S& s2)
    {
        return static_cast<const vtable_entry<T0>&>(s1).value ==
            static_cast<const vtable_entry<T0>&>(s2).value &&
            compare_vtable<T...>::apply(s1, s2);
    }
};

template<class... T>
struct vtable_storage : vtable_entry<T>...
{
    vtable_storage() = default;

    constexpr vtable_storage(typename T::type... arg)
        : vtable_entry<T>(arg)... {}

    template<class Bindings, class Src>
    void convert_from(const Src& src)
    {
        *this = vtable_storage(
            src.lookup(
                (typename ::boost::type_erasure::detail::rebind_placeholders<
                T, Bindings
            >::type*)0)...);
    }

    bool operator==(const vtable_storage& other) const
    { return compare_vtable<T...>::apply(*this, other); }

    template<class U>
    typename U::type lookup(U*) const
    {
        return static_cast<const vtable_entry<U>*>(this)->value;
    }
};

// Provide this specialization manually.
// gcc 4.7.2 fails to instantiate the primary template.
template<>
struct vtable_storage<>
{
    vtable_storage() = default;

    template<class Bindings, class Src>
    void convert_from(const Src& /*src*/) {}

    bool operator==(const vtable_storage& /*other*/) const
    { return true; }
};

template<class... T>
struct make_vtable_impl<stored_arg_pack<T...> >
{
    typedef vtable_storage<T...> type;
};

template<class Table, class... T>
struct vtable_init
{
    static constexpr Table value = Table(T::value...);
};

template<class Table, class... T>
constexpr Table vtable_init<Table, T...>::value;

template<class Table, class... T>
struct make_vtable_init_impl<Table, stored_arg_pack<T...> >
{
    typedef vtable_init<Table, T...> type;
};

#else

template<int N>
struct make_vtable_impl;

template<class Seq>
struct make_vtable
{
    typedef typename make_vtable_impl<
        (::boost::mpl::size<Seq>::value)>::template apply<Seq>::type type;
};

template<int N>
struct make_vtable_init_impl;

template<class Seq, class Table>
struct make_vtable_init
{
    typedef typename make_vtable_init_impl<
        (::boost::mpl::size<Seq>::value)>::template apply<Seq, Table>::type type;
};

#define BOOST_PP_FILENAME_1 <boost/type_erasure/detail/vtable.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_TYPE_ERASURE_MAX_FUNCTIONS)
#include BOOST_PP_ITERATE()

#endif

}
}
}

#endif

#else

#define N BOOST_PP_ITERATION()

#define BOOST_TYPE_ERASURE_VTABLE_ENTRY(z, n, data)                     \
    typename BOOST_PP_CAT(T, n)::type BOOST_PP_CAT(t, n);               \
    typename BOOST_PP_CAT(T, n)::type lookup(BOOST_PP_CAT(T, n)*) const \
    {                                                                   \
        return BOOST_PP_CAT(t, n);                                      \
    }

#define BOOST_TYPE_ERASURE_VTABLE_COMPARE(z, n, data)                   \
    && BOOST_PP_CAT(t, n) == other.BOOST_PP_CAT(t, n)

#define BOOST_TYPE_ERASURE_VTABLE_INIT(z, n, data)  \
    BOOST_PP_CAT(data, n)::value

#define BOOST_TYPE_ERASURE_AT(z, n, data)       \
    typename ::boost::mpl::at_c<data, n>::type

#define BOOST_TYPE_ERASURE_CONVERT_ELEMENT(z, n, data)                  \
    BOOST_PP_CAT(t, n) = src.lookup(                                    \
        (typename ::boost::type_erasure::detail::rebind_placeholders<   \
            BOOST_PP_CAT(T, n), Bindings                                \
        >::type*)0                                                      \
    );

#if N != 0
template<BOOST_PP_ENUM_PARAMS(N, class T)>
#else
template<class D = void>
#endif
struct BOOST_PP_CAT(vtable_storage, N)
{
    BOOST_PP_REPEAT(N, BOOST_TYPE_ERASURE_VTABLE_ENTRY, ~)

    template<class Bindings, class Src>
    void convert_from(const Src& BOOST_PP_EXPR_IF(N, src))
    {
        BOOST_PP_REPEAT(N, BOOST_TYPE_ERASURE_CONVERT_ELEMENT, ~)
    }

    bool operator==(const BOOST_PP_CAT(vtable_storage, N)& BOOST_PP_EXPR_IF(N, other)) const
    { return true BOOST_PP_REPEAT(N, BOOST_TYPE_ERASURE_VTABLE_COMPARE, ~); }
};

template<>
struct make_vtable_impl<N>
{
    template<class Seq>
    struct apply
    {
        typedef ::boost::type_erasure::detail::BOOST_PP_CAT(vtable_storage, N)<
            BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_AT, Seq)
        > type;
    };
};
template<class Table BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
struct BOOST_PP_CAT(vtable_init, N)
{
    static const Table value;
};

template<class Table BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>
const Table BOOST_PP_CAT(vtable_init, N)<
    Table BOOST_PP_ENUM_TRAILING_PARAMS(N, T)>::value =
{
    BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_VTABLE_INIT, T)
};

template<>
struct make_vtable_init_impl<N>
{
    template<class Seq, class Table>
    struct apply
    {
        typedef ::boost::type_erasure::detail::BOOST_PP_CAT(vtable_init, N)<
            Table
            BOOST_PP_ENUM_TRAILING(N, BOOST_TYPE_ERASURE_AT, Seq)
        > type;
    };
};

#undef BOOST_TYPE_ERASURE_CONVERT_ELEMENT
#undef BOOST_TYPE_ERASURE_AT
#undef BOOST_TYPE_ERASURE_VTABLE_INIT
#undef BOOST_TYPE_ERASURE_VTABLE_COMPARE
#undef BOOST_TYPE_ERASURE_VTABLE_ENTRY
#undef N

#endif
