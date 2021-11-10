// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_FREE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_FREE_HPP_INCLUDED

#include <boost/detail/workaround.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/next.hpp>
#include <boost/type_erasure/detail/macro.hpp>
#include <boost/type_erasure/detail/const.hpp>
#include <boost/type_erasure/config.hpp>
#include <boost/type_erasure/derived.hpp>
#include <boost/type_erasure/rebind_any.hpp>
#include <boost/type_erasure/param.hpp>
#include <boost/type_erasure/is_placeholder.hpp>
#include <boost/type_erasure/call.hpp>
#include <boost/type_erasure/concept_interface.hpp>

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || \
    defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || \
    defined(BOOST_TYPE_ERASURE_DOXYGEN) || \
    BOOST_WORKAROUND(BOOST_MSVC, == 1800)

namespace boost {
namespace type_erasure {
namespace detail {

template<BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_TYPE_ERASURE_MAX_ARITY, class T, void)>
struct first_placeholder {
    typedef typename ::boost::mpl::eval_if<is_placeholder<T0>,
        ::boost::mpl::identity<T0>,
        first_placeholder<BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_TYPE_ERASURE_MAX_ARITY, T)>
    >::type type;
};

template<>
struct first_placeholder<> {};

template<BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_TYPE_ERASURE_MAX_ARITY, class T, void)>
struct first_placeholder_index :
    ::boost::mpl::eval_if<is_placeholder<T0>,
        ::boost::mpl::int_<0>,
        ::boost::mpl::next<first_placeholder_index<BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_TYPE_ERASURE_MAX_ARITY, T)> >
    >::type
{};

}
}
}

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_QUALIFIED_ID(seq, N) \
    BOOST_TYPE_ERASURE_QUALIFIED_NAME(seq)<R(BOOST_PP_ENUM_PARAMS(N, T))>

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_UNQUALIFIED_PARAM_TYPE(z, n, data) \
    typename ::boost::remove_cv<typename ::boost::remove_reference<BOOST_PP_CAT(T, n)>::type>::type

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_PARAM_TYPE(z, n, data)                      \
    typename ::boost::mpl::eval_if_c<(_boost_type_erasure_free_p_idx::value == n), \
        ::boost::type_erasure::detail::maybe_const_this_param<BOOST_PP_CAT(T, n), Base>,    \
        ::boost::type_erasure::as_param<Base, BOOST_PP_CAT(T, n)>           \
    >::type BOOST_PP_CAT(t, n)

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_FORWARD_I(z, n, data) ::std::forward<BOOST_PP_CAT(T, n)>(BOOST_PP_CAT(t, n))
/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_FORWARD(n) BOOST_PP_ENUM(n, BOOST_TYPE_ERASURE_FREE_FORWARD_I, ~)
/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_FORWARD_PARAM_I(z, n, data) \
    ::std::forward<typename ::boost::mpl::eval_if_c<(_boost_type_erasure_free_p_idx::value == n), \
        ::boost::type_erasure::detail::maybe_const_this_param<BOOST_PP_CAT(T, n), Base>,    \
        ::boost::type_erasure::as_param<Base, BOOST_PP_CAT(T, n)>           \
    >::type>(BOOST_PP_CAT(t, n))

#else

#define BOOST_TYPE_ERASURE_FREE_FORWARD(n) BOOST_PP_ENUM_PARAMS(n, t)
#define BOOST_TYPE_ERASURE_FREE_FORWARD_PARAM_I(z, n, data) BOOST_PP_CAT(t, n)

#endif

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_II(qual_name, concept_name, function_name, N)  \
    BOOST_TYPE_ERASURE_OPEN_NAMESPACE(qual_name)                        \
                                                                        \
    template<class Sig>                                                 \
    struct concept_name;                                                \
                                                                        \
    template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T)>         \
    struct concept_name<R(BOOST_PP_ENUM_PARAMS(N, T))> {                \
        static R apply(BOOST_PP_ENUM_BINARY_PARAMS(N, T, t))            \
        { return function_name(BOOST_TYPE_ERASURE_FREE_FORWARD(N)); }   \
    };                                                                  \
                                                                        \
    template<BOOST_PP_ENUM_PARAMS(N, class T)>                          \
    struct concept_name<void(BOOST_PP_ENUM_PARAMS(N, T))> {             \
        static void apply(BOOST_PP_ENUM_BINARY_PARAMS(N, T, t))         \
        { function_name(BOOST_TYPE_ERASURE_FREE_FORWARD(N)); }          \
    };                                                                  \
                                                                        \
    BOOST_TYPE_ERASURE_CLOSE_NAMESPACE(qual_name)                       \
                                                                        \
    namespace boost {                                                   \
    namespace type_erasure {                                            \
                                                                        \
    template<class R BOOST_PP_ENUM_TRAILING_PARAMS(N, class T), class Base> \
    struct concept_interface<                                           \
        BOOST_TYPE_ERASURE_FREE_QUALIFIED_ID(qual_name, N),             \
        Base,                                                           \
        typename ::boost::type_erasure::detail::first_placeholder<      \
            BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_FREE_UNQUALIFIED_PARAM_TYPE, ~)>::type  \
    > : Base {                                                          \
        typedef typename ::boost::type_erasure::detail::first_placeholder_index<    \
            BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_FREE_UNQUALIFIED_PARAM_TYPE, ~)>::type  \
            _boost_type_erasure_free_p_idx;                             \
        friend typename ::boost::type_erasure::rebind_any<Base, R>::type function_name(  \
            BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_FREE_PARAM_TYPE, ~))    \
        {                                                               \
            return ::boost::type_erasure::call(                         \
                BOOST_TYPE_ERASURE_FREE_QUALIFIED_ID(qual_name, N)()    \
                BOOST_PP_ENUM_TRAILING(N, BOOST_TYPE_ERASURE_FREE_FORWARD_PARAM_I, ~)); \
        }                                                               \
    };                                                                  \
                                                                        \
    }                                                                   \
    }

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_I(namespace_name, concept_name, function_name, N)\
    BOOST_TYPE_ERASURE_FREE_II(namespace_name, concept_name, function_name, N)

#ifdef BOOST_TYPE_ERASURE_DOXYGEN

/**
 * \brief Defines a primitive concept for a free function.
 *
 * \param concept_name is the name of the concept to declare.
 *        If it is omitted it defaults to <code>has_ ## function_name</code>
 * \param function_name is the name of the function.
 *
 * The declaration of the concept is
 * \code
 * template<class Sig>
 * struct concept_name;
 * \endcode
 * where Sig is a function type giving the
 * signature of the function.
 *
 * This macro can only be used at namespace scope.
 *
 * Example:
 *
 * \code
 * BOOST_TYPE_ERASURE_FREE(to_string)
 * typedef has_to_string<std::string(_self const&)> to_string_concept;
 * \endcode
 *
 * In C++03, the macro can only be used in the global namespace and
 * is defined as:
 *
 * \code
 * #define BOOST_TYPE_ERASURE_FREE(qualified_name, function_name, N)
 * \endcode
 *
 * Example:
 *
 * \code
 * BOOST_TYPE_ERASURE_FREE((boost)(has_to_string), to_string, 1)
 * \endcode
 *
 * For backwards compatibility, this form is always accepted.
 */
#define BOOST_TYPE_ERASURE_FREE(concept_name, function_name)

#else

#define BOOST_TYPE_ERASURE_FREE(qualified_name, function_name, N)                           \
    BOOST_TYPE_ERASURE_FREE_I(                                                              \
        qualified_name,                                                                     \
        BOOST_PP_SEQ_ELEM(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(qualified_name)), qualified_name), \
        function_name,                                                                      \
        N)

#endif

#else

namespace boost {
namespace type_erasure {
    
template<int... N>
struct index_list {};

namespace detail {
    
template<class... T>
struct first_placeholder;

template<class T0, class... T>
struct first_placeholder<T0, T...> {
    typedef typename ::boost::mpl::eval_if<is_placeholder<T0>,
        ::boost::mpl::identity<T0>,
        first_placeholder<T...>
    >::type type;
};

template<>
struct first_placeholder<> {};

template<class... T>
struct first_placeholder_index;

template<class T0, class... T>
struct first_placeholder_index<T0, T...> :
    ::boost::mpl::eval_if<is_placeholder<T0>,
        ::boost::mpl::int_<0>,
        ::boost::mpl::next<first_placeholder_index<T...> >
    >::type
{};

template<class Sig>
struct transform_free_signature;

template<class T, int N>
struct push_back_index;

template<int... N, int X>
struct push_back_index<index_list<N...>, X>
{
    typedef index_list<N..., X> type;
};

template<int N>
struct make_index_list {
    typedef typename push_back_index<
        typename make_index_list<N-1>::type,
        N-1
    >::type type;
};

template<>
struct make_index_list<0> {
    typedef index_list<> type;
};

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES) && \
    !defined(BOOST_NO_CXX11_DECLTYPE)

template<int N>
using make_index_list_t = typename ::boost::type_erasure::detail::make_index_list<N>::type;

#if BOOST_WORKAROUND(BOOST_MSVC, == 1900)

template<class... T>
struct first_placeholder_index_ :
    ::boost::type_erasure::detail::first_placeholder_index<
        ::boost::remove_cv_t< ::boost::remove_reference_t<T> >...
    >
{};
template<class... T>
using first_placeholder_index_t =
    typename ::boost::type_erasure::detail::first_placeholder_index_<T...>::type;

#else

template<class... T>
using first_placeholder_index_t =
    typename ::boost::type_erasure::detail::first_placeholder_index<
        ::boost::remove_cv_t< ::boost::remove_reference_t<T> >...
    >::type;

#endif

template<class Base, class Tn, int I, class... T>
using free_param_t =
    typename ::boost::mpl::eval_if_c<(::boost::type_erasure::detail::first_placeholder_index_t<T...>::value == I),
            ::boost::type_erasure::detail::maybe_const_this_param<Tn, Base>, \
            ::boost::type_erasure::as_param<Base, Tn>
    >::type;

template<class Sig, class ID>
struct free_interface_chooser
{
    template<class Base, template<class> class C, template<class...> class F>
    using apply = Base;
};

template<class R, class... A>
struct free_interface_chooser<
    R(A...),
    typename ::boost::type_erasure::detail::first_placeholder<
        ::boost::remove_cv_t< ::boost::remove_reference_t<A> >...>::type>
{
    template<class Base, template<class> class C, template<class...> class F>
    using apply = F<R(A...), Base,
        ::boost::type_erasure::detail::make_index_list_t<sizeof...(A)> >;
};

template<class Sig, template<class> class C, template<class...> class F>
struct free_choose_interface {
    template<class Concept, class Base, class ID>
    using apply = typename free_interface_chooser<Sig, ID>::template apply<Base, C, F>;
};

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_I(concept_name, function_name)              \
template<class Sig>                                                         \
struct concept_name;                                                        \
                                                                            \
namespace boost_type_erasure_impl {                                         \
                                                                            \
template<class Sig, class Base, class Idx>                                  \
struct concept_name ## _free_interface;                                     \
template<class R, class... T, class Base, int... I>                         \
struct concept_name ## _free_interface<R(T...), Base, ::boost::type_erasure::index_list<I...> > : Base {\
    friend ::boost::type_erasure::rebind_any_t<Base, R>                     \
    function_name(                                                          \
        ::boost::type_erasure::detail::free_param_t<Base, T, I, T...>... t) \
    {                                                                       \
        return ::boost::type_erasure::call(                                 \
            concept_name<R(T...)>(),                                        \
            std::forward< ::boost::type_erasure::detail::free_param_t<Base, T, I, T...> >(t)...);\
    }                                                                       \
};                                                                          \
                                                                            \
template<class Sig>                                                         \
struct concept_name ## free;                                                \
                                                                            \
template<class R, class... T>                                               \
struct concept_name ## free<R(T...)> {                                      \
    static R apply(T... t)                                                  \
    { return function_name(std::forward<T>(t)...); }                        \
};                                                                          \
                                                                            \
template<class... T>                                                        \
struct concept_name ## free<void(T...)> {                                   \
    static void apply(T... t)                                               \
    { function_name(std::forward<T>(t)...); }                               \
};                                                                          \
                                                                            \
}                                                                           \
                                                                            \
template<class Sig>                                                         \
struct concept_name :                                                       \
    boost_type_erasure_impl::concept_name##free<Sig>                        \
{};                                                                         \
                                                                            \
template<class Sig>                                                         \
::boost::type_erasure::detail::free_choose_interface<Sig, concept_name,     \
    boost_type_erasure_impl::concept_name ## _free_interface>               \
boost_type_erasure_find_interface(concept_name<Sig>);

#define BOOST_TYPE_ERASURE_FREE_SIMPLE(name, ...) \
    BOOST_TYPE_ERASURE_FREE_I(has_ ## name, name)

#define BOOST_TYPE_ERASURE_FREE_NS_I(concept_name, name)  \
    BOOST_TYPE_ERASURE_FREE_I(concept_name, name)

#define BOOST_TYPE_ERASURE_FREE_NS(concept_name, name)      \
    BOOST_TYPE_ERASURE_OPEN_NAMESPACE(concept_name)         \
    BOOST_TYPE_ERASURE_FREE_NS_I(BOOST_PP_SEQ_ELEM(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(concept_name)), concept_name), name) \
    BOOST_TYPE_ERASURE_CLOSE_NAMESPACE(concept_name)

#define BOOST_TYPE_ERASURE_FREE_NAMED(concept_name, name, ...)  \
    BOOST_PP_IF(BOOST_PP_IS_BEGIN_PARENS(concept_name),         \
        BOOST_TYPE_ERASURE_FREE_NS,                             \
        BOOST_TYPE_ERASURE_FREE_I)                              \
    (concept_name, name)

#define BOOST_TYPE_ERASURE_FREE_CAT(x, y) x y

#define BOOST_TYPE_ERASURE_FREE(name, ...)              \
    BOOST_TYPE_ERASURE_FREE_CAT(                        \
        BOOST_PP_IF(BOOST_VMD_IS_EMPTY(__VA_ARGS__),    \
            BOOST_TYPE_ERASURE_FREE_SIMPLE,             \
            BOOST_TYPE_ERASURE_FREE_NAMED),             \
        (name, __VA_ARGS__))

#else

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_II(qual_name, concept_name, function_name)  \
    BOOST_TYPE_ERASURE_OPEN_NAMESPACE(qual_name)                        \
                                                                        \
    template<class Sig>                                                 \
    struct concept_name;                                                \
                                                                        \
    template<class R, class... T>                                       \
    struct concept_name<R(T...)> {                                      \
        static R apply(T... t)                                          \
        { return function_name(std::forward<T>(t)...); }                \
    };                                                                  \
                                                                        \
    template<class... T>                                                \
    struct concept_name<void(T...)> {                                   \
        static void apply(T... t)                                       \
        { function_name(std::forward<T>(t)...); }                       \
    };                                                                  \
                                                                        \
    BOOST_TYPE_ERASURE_CLOSE_NAMESPACE(qual_name)                       \
                                                                        \
    namespace boost {                                                   \
    namespace type_erasure {                                            \
                                                                        \
    template<class Sig, class Base, class Idx>                          \
    struct inject ## concept_name;                                      \
    template<class R, class... T, class Base, int... I>                 \
    struct inject ## concept_name<R(T...), Base, index_list<I...> > : Base {\
        typedef typename ::boost::type_erasure::detail::first_placeholder_index<    \
            typename ::boost::remove_cv<                                \
                typename ::boost::remove_reference<T>::type             \
            >::type...                                                  \
        >::type _boost_type_erasure_free_p_idx;                         \
        friend typename ::boost::type_erasure::rebind_any<Base, R>::type\
        function_name(                                                  \
            typename ::boost::mpl::eval_if_c<(_boost_type_erasure_free_p_idx::value == I), \
                ::boost::type_erasure::detail::maybe_const_this_param<T, Base>, \
                ::boost::type_erasure::as_param<Base, T>                \
            >::type... t)                                               \
       {                                                                \
            return ::boost::type_erasure::call(                         \
                BOOST_TYPE_ERASURE_QUALIFIED_NAME(qual_name)<R(T...)>(),\
                std::forward<typename ::boost::mpl::eval_if_c<(_boost_type_erasure_free_p_idx::value == I), \
                    ::boost::type_erasure::detail::maybe_const_this_param<T, Base>, \
                    ::boost::type_erasure::as_param<Base, T>            \
                >::type>(t)...);                                        \
        }                                                               \
    };                                                                  \
                                                                        \
    template<class R, class... T, class Base>                           \
    struct concept_interface<                                           \
        BOOST_TYPE_ERASURE_QUALIFIED_NAME(qual_name)<R(T...)>,          \
        Base,                                                           \
        typename ::boost::type_erasure::detail::first_placeholder<      \
            typename ::boost::remove_cv<typename ::boost::remove_reference<T>::type>::type...>::type  \
    > :  inject ## concept_name<R(T...), Base, typename ::boost::type_erasure::detail::make_index_list<sizeof...(T)>::type>\
    {};                                                                 \
                                                                        \
    }                                                                   \
    }

    
/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_FREE_I(namespace_name, concept_name, function_name)              \
    BOOST_TYPE_ERASURE_FREE_II(namespace_name, concept_name, function_name)

#define BOOST_TYPE_ERASURE_FREE(qualified_name, function_name, ...)                         \
    BOOST_TYPE_ERASURE_FREE_I(                                                              \
        qualified_name,                                                                     \
        BOOST_PP_SEQ_ELEM(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(qualified_name)), qualified_name), \
        function_name)

#endif

}
}
}

#endif

#endif
