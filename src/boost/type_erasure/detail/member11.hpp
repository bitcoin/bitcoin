// Boost.TypeErasure library
//
// Copyright 2018 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DETAIL_MEMBER11_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_MEMBER11_HPP_INCLUDED

#include <boost/config.hpp>

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES) && \
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && \
    !defined(BOOST_NO_CXX11_DECLTYPE) && \
    !BOOST_WORKAROUND(BOOST_MSVC, == 1800)

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_erasure/detail/const.hpp>
#include <boost/type_erasure/detail/macro.hpp>
#include <boost/type_erasure/rebind_any.hpp>
#include <boost/type_erasure/param.hpp>
#include <boost/type_erasure/is_placeholder.hpp>
#include <boost/type_erasure/call.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

template<class P, template<class ...> class interface, class Sig, class Concept, class Base, class ID>
using choose_member_interface = typename ::boost::mpl::if_c< ::boost::is_reference<P>::value,
    typename ::boost::mpl::if_c< ::boost::type_erasure::detail::is_non_const_ref<P>::value,
        interface<Sig, Concept, Base, const ID>,
        Base
    >::type,
    interface<Sig, Concept, Base, ID>
>::type;


struct dummy {};

template<class Sig>
struct choose_member_impl;

template<class R, class C, class... T>
struct choose_member_impl<R (C::*)(T...) const>
{
    template<class P, template<class> class M, template<class,class> class S> 
    using apply = ::boost::mpl::vector1<S<R(T...), const P> >;
};

template<class R, class C, class... T>
struct choose_member_impl<R (C::*)(T...)>
{
    template<class P, template<class> class M, template<class,class> class S> 
    using apply = M<R(P&, T...)>;
};

template<class Sig, class P, template<class> class M, template<class, class> class Self>
using choose_member_impl_t =
    typename ::boost::type_erasure::detail::choose_member_impl<
        Sig (::boost::type_erasure::detail::dummy::*)
    >::template apply<P, M, Self>;

template<class Sig, class T, class ID>
struct member_interface_chooser
{
    template<class Base, template<class, class> class C, template<class...> class M>
    using apply = Base;
};

template<class R, class T, class... A>
struct member_interface_chooser<R(A...), T, T>
{
    template<class Base, template<class, class> class C, template<class...> class M>
    using apply = ::boost::type_erasure::detail::choose_member_interface<
        ::boost::type_erasure::placeholder_of_t<Base>,
        M,
        R(A...), C<R(A...), T>, Base, T>;
};

template<class R, class T, class... A>
struct member_interface_chooser<R(A...), const T, T>
{
    template<class Base, template<class, class> class C, template<class...> class M>
    using apply = M<R(A...), C<R(A...), const T>, Base, const T>;
};

template<class Sig, class T, template<class, class> class C, template<class...> class M>
struct member_choose_interface
{
    template<class Concept, class Base, class ID>
    using apply = typename ::boost::type_erasure::detail::member_interface_chooser<Sig, T, ID>::template apply<Base, C, M>;
};

}
}
}

#define BOOST_TYPE_ERASURE_MEMBER_I(concept_name, function_name)            \
template<class Sig, class T = ::boost::type_erasure::_self>                 \
struct concept_name;                                                        \
                                                                            \
namespace boost_type_erasure_impl {                                         \
                                                                            \
template<class Sig, class T>                                                \
using concept_name ## self = concept_name<Sig, T>;                          \
                                                                            \
template<class Sig, class Concept, class Base, class ID, class Enable = void>\
struct concept_name ## _member_interface;                                   \
                                                                            \
template<class R, class... A, class Concept, class Base, class ID, class V> \
struct concept_name ## _member_interface<R(A...), Concept, Base, ID, V> : Base\
{                                                                           \
    typedef void _boost_type_erasure_has_member ## function_name;           \
    ::boost::type_erasure::rebind_any_t<Base, R> function_name(::boost::type_erasure::as_param_t<Base, A>... a)\
    { return ::boost::type_erasure::call(Concept(), *this, std::forward<A>(a)...); }\
};                                                                          \
                                                                            \
template<class R, class... A, class Concept, class Base, class ID>          \
struct concept_name ## _member_interface<R(A...), Concept, Base, ID,        \
    typename Base::_boost_type_erasure_has_member ## function_name> : Base  \
{                                                                           \
    using Base::function_name;                                              \
    ::boost::type_erasure::rebind_any_t<Base, R> function_name(::boost::type_erasure::as_param_t<Base, A>... a)\
    { return ::boost::type_erasure::call(Concept(), *this, std::forward<A>(a)...); }\
};                                                                          \
                                                                            \
template<class R, class... A, class Concept, class Base, class ID, class V> \
struct concept_name ## _member_interface<R(A...), Concept, Base, const ID, V> : Base\
{                                                                           \
    typedef void _boost_type_erasure_has_member ## function_name;           \
    ::boost::type_erasure::rebind_any_t<Base, R> function_name(::boost::type_erasure::as_param_t<Base, A>... a) const\
    { return ::boost::type_erasure::call(Concept(), *this, std::forward<A>(a)...); }\
};                                                                          \
                                                                            \
template<class R, class... A, class Concept, class Base, class ID>          \
struct concept_name ## _member_interface<R(A...), Concept, Base, const ID,  \
    typename Base::_boost_type_erasure_has_member ## function_name> : Base  \
{                                                                           \
    using Base::function_name;                                              \
    ::boost::type_erasure::rebind_any_t<Base, R> function_name(::boost::type_erasure::as_param_t<Base, A>... a) const\
    { return ::boost::type_erasure::call(Concept(), *this, std::forward<A>(a)...); }\
};                                                                          \
                                                                            \
template<class Sig>                                                         \
struct concept_name ## member;                                              \
                                                                            \
template<class R, class T0, class... T>                                     \
struct concept_name ## member<R(T0, T...)> {                                \
    static R apply(T0 t0, T... t)                                           \
    { return t0.function_name(std::forward<T>(t)...); }                     \
};                                                                          \
                                                                            \
template<class T0, class... T>                                              \
struct concept_name ## member<void(T0, T...)> {                             \
    static void apply(T0 t0, T... t)                                        \
    { t0.function_name(std::forward<T>(t)...); }                            \
};                                                                          \
                                                                            \
}                                                                           \
                                                                            \
template<class Sig, class T>                                                \
struct concept_name :                                                       \
    ::boost::type_erasure::detail::choose_member_impl_t<Sig, T,             \
        boost_type_erasure_impl::concept_name##member,                      \
        boost_type_erasure_impl::concept_name##self                         \
    >                                                                       \
{};                                                                         \
                                                                            \
template<class Sig, class T>                                                \
::boost::type_erasure::detail::member_choose_interface<Sig, T, concept_name,\
    boost_type_erasure_impl::concept_name ## _member_interface>             \
boost_type_erasure_find_interface(concept_name<Sig, T>);

#define BOOST_TYPE_ERASURE_MEMBER_SIMPLE(name, ...) \
    BOOST_TYPE_ERASURE_MEMBER_I(has_ ## name, name)

#define BOOST_TYPE_ERASURE_MEMBER_NS_I(concept_name, name)  \
    BOOST_TYPE_ERASURE_MEMBER_I(concept_name, name)

#define BOOST_TYPE_ERASURE_MEMBER_NS(concept_name, name)    \
    BOOST_TYPE_ERASURE_OPEN_NAMESPACE(concept_name)         \
    BOOST_TYPE_ERASURE_MEMBER_NS_I(BOOST_PP_SEQ_ELEM(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(concept_name)), concept_name), name) \
    BOOST_TYPE_ERASURE_CLOSE_NAMESPACE(concept_name)

#define BOOST_TYPE_ERASURE_MEMBER_NAMED(concept_name, name, ...)    \
    BOOST_PP_IF(BOOST_PP_IS_BEGIN_PARENS(concept_name),             \
        BOOST_TYPE_ERASURE_MEMBER_NS,                               \
        BOOST_TYPE_ERASURE_MEMBER_I)                                \
    (concept_name, name)

#define BOOST_TYPE_ERASURE_MEMBER_CAT(x, y) x y

#define BOOST_TYPE_ERASURE_MEMBER(name, ...)            \
    BOOST_TYPE_ERASURE_MEMBER_CAT(                      \
        BOOST_PP_IF(BOOST_VMD_IS_EMPTY(__VA_ARGS__),    \
            BOOST_TYPE_ERASURE_MEMBER_SIMPLE,           \
            BOOST_TYPE_ERASURE_MEMBER_NAMED),           \
        (name, __VA_ARGS__))

#endif

#endif
