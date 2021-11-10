// Boost.TypeErasure library
//
// Copyright 2012-2013 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_MEMBER_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_MEMBER_HPP_INCLUDED

#include <boost/type_erasure/detail/member11.hpp>

#ifndef BOOST_TYPE_ERASURE_MEMBER

#include <boost/detail/workaround.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/dec.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/type_erasure/detail/macro.hpp>
#include <boost/type_erasure/detail/const.hpp>
#include <boost/type_erasure/rebind_any.hpp>
#include <boost/type_erasure/placeholder.hpp>
#include <boost/type_erasure/call.hpp>
#include <boost/type_erasure/concept_interface.hpp>

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_ARG(z, n, data)  \
    typename ::boost::type_erasure::as_param<Base, BOOST_PP_CAT(A, n)>::type BOOST_PP_CAT(a, n)

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || \
    defined(BOOST_TYPE_ERASURE_DOXYGEN) || \
    BOOST_WORKAROUND(BOOST_MSVC, == 1800)

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(seq, N) \
    BOOST_TYPE_ERASURE_QUALIFIED_NAME(seq)<R(BOOST_PP_ENUM_PARAMS(N, A)), T>

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_TPL_ARG_LIST(N, X) BOOST_PP_ENUM_TRAILING_PARAMS(N, X)
/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_ENUM_PARAMS(N, X) BOOST_PP_ENUM_PARAMS(N, X)

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_FORWARD(z, n, data) ::std::forward<BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 0, data), n)>(BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 1, data), n))
/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_FORWARD_PARAMS(N, X, x) BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_MEMBER_FORWARD, (X, x))
/** INTERNAL ONLY*/
#define BOOST_TYPE_ERASURE_FORWARD_REBIND1(z, n, data) ::std::forward<typename ::boost::type_erasure::as_param<Base, BOOST_PP_CAT(A, n)>::type>(BOOST_PP_CAT(a, n))
/** INTERNAL ONLY*/
#define BOOST_TYPE_ERASURE_MEMBER_FORWARD_REBIND(N) BOOST_PP_ENUM_TRAILING(N, BOOST_TYPE_ERASURE_FORWARD_REBIND1, ~)

#else

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_FORWARD_PARAMS(N, X, x) BOOST_PP_ENUM_PARAMS(N, x)
/** INTERNAL ONLY*/
#define BOOST_TYPE_ERASURE_MEMBER_FORWARD_REBIND(N) BOOST_PP_ENUM_TRAILING_PARAMS(N, a)

#endif

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_ENUM_TRAILING_PARAMS(N, X) BOOST_PP_ENUM_TRAILING_PARAMS(N, X)
/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_ENUM_TRAILING_BINARY_PARAMS(N, X, x) BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, X, x)
/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_ENUM_ARGS(N) BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_MEMBER_ARG, ~)

#ifdef BOOST_TYPE_ERASURE_DOXYGEN

/**
 * \brief Defines a primitive concept for a member function.
 *
 * \param concept_name is the name of the concept to declare.
 *        If it is omitted it defaults to <code>has_ ## member</code>
 * \param member is the name of the member function.
 *
 * The declaration of the concept is
 * \code
 * template<class Sig, class T = _self>
 * struct concept_name;
 * \endcode
 * where @c Sig is a function type giving the
 * signature of the member function, and @c T is the
 * object type.  @c T may be const-qualified for
 * const member functions.  @c concept_name<R(A...) const, T>
 * is an alias for @c concept_name<R(A...), const T>.
 *
 * This macro can only be used at namespace scope.
 *
 * Example:
 *
 * \code
 * namespace boost {
 * BOOST_TYPE_ERASURE_MEMBER(push_back)
 * }
 * typedef boost::has_push_back<void(int)> push_back_concept;
 * \endcode
 *
 * The concept defined by this function may be specialized to
 * provide a concept_map.  The class object will be passed by
 * reference as the first parameter.
 *
 * \code
 * template<>
 * struct has_push_back<void(int), std::list<int> > {
 *   static void apply(std::list<int>& l, int i) { l.push_back(i); }
 * };
 * \endcode
 *
 * In C++03, the macro can only be used in the global namespace and
 * is defined as:
 *
 * \code
 * #define BOOST_TYPE_ERASURE_MEMBER(qualified_name, member, N)
 * \endcode
 *
 * Example:
 *
 * \code
 * BOOST_TYPE_ERASURE_MEMBER((boost)(has_push_back), push_back, 1)
 * typedef boost::has_push_back<void(int), _self> push_back_concept;
 * \endcode
 *
 * For backwards compatibility, this form is always accepted.
 */
#define BOOST_TYPE_ERASURE_MEMBER(concept_name, member) /**/

#else

#define BOOST_TYPE_ERASURE_MEMBER(qualified_name, member, N)                                \
    BOOST_TYPE_ERASURE_MEMBER_I(                                                            \
        qualified_name,                                                                     \
        BOOST_PP_SEQ_ELEM(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(qualified_name)), qualified_name), \
        member,                                                                             \
        N)

#endif

#else

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(seq, N) \
    BOOST_TYPE_ERASURE_QUALIFIED_NAME(seq)<R(A...), T>

#define BOOST_TYPE_ERASURE_MEMBER_TPL_ARG_LIST(N, X) , class... A
#define BOOST_TYPE_ERASURE_MEMBER_ENUM_PARAMS(N, X) X...
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#   define BOOST_TYPE_ERASURE_MEMBER_FORWARD_PARAMS(N, X, x) ::std::forward<X>(x)...
#   define BOOST_TYPE_ERASURE_MEMBER_FORWARD_REBIND(N) , ::std::forward<typename ::boost::type_erasure::as_param<Base, A>::type>(a)...
#else
#   define BOOST_TYPE_ERASURE_MEMBER_FORWARD_PARAMS(N, X, x) x...
#   define BOOST_TYPE_ERASURE_MEMBER_FORWARD_REBIND(N) , a...
#endif
#define BOOST_TYPE_ERASURE_MEMBER_ENUM_TRAILING_PARAMS(N, X) , X...
#define BOOST_TYPE_ERASURE_MEMBER_ENUM_TRAILING_BINARY_PARAMS(N, X, x) , X... x
#define BOOST_TYPE_ERASURE_MEMBER_ENUM_ARGS(N) typename ::boost::type_erasure::as_param<Base, A>::type... a


#define BOOST_TYPE_ERASURE_MEMBER(qualified_name, member, ...)                              \
    BOOST_TYPE_ERASURE_MEMBER_I(                                                            \
        qualified_name,                                                                     \
        BOOST_PP_SEQ_ELEM(BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(qualified_name)), qualified_name), \
        member,                                                                             \
        N)

#endif

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_II(qual_name, concept_name, member, N)                        \
    BOOST_TYPE_ERASURE_OPEN_NAMESPACE(qual_name)                                                \
    template<class Sig, class T = ::boost::type_erasure::_self>                                 \
    struct concept_name;                                                                        \
    template<class R BOOST_TYPE_ERASURE_MEMBER_TPL_ARG_LIST(N, class A), class T>               \
    struct concept_name<R(BOOST_TYPE_ERASURE_MEMBER_ENUM_PARAMS(N, A)), T> {                    \
        static R apply(T& t BOOST_TYPE_ERASURE_MEMBER_ENUM_TRAILING_BINARY_PARAMS(N, A, a))     \
        { return t.member(BOOST_TYPE_ERASURE_MEMBER_FORWARD_PARAMS(N, A, a)); }                 \
    };                                                                                          \
    template<class T BOOST_TYPE_ERASURE_MEMBER_TPL_ARG_LIST(N, class A)>                        \
    struct concept_name<void(BOOST_TYPE_ERASURE_MEMBER_ENUM_PARAMS(N, A)), T> {                 \
        static void apply(T& t BOOST_TYPE_ERASURE_MEMBER_ENUM_TRAILING_BINARY_PARAMS(N, A, a))  \
        { t.member(BOOST_TYPE_ERASURE_MEMBER_FORWARD_PARAMS(N, A, a)); }                        \
    };                                                                                          \
    BOOST_TYPE_ERASURE_CLOSE_NAMESPACE(qual_name)                                               \
    namespace boost {                                                                           \
    namespace type_erasure {                                                                    \
    template<                                                                                   \
        class R BOOST_TYPE_ERASURE_MEMBER_TPL_ARG_LIST(N, class A),                             \
        class T, class Base, class Enable>                                                      \
    struct concept_interface<                                                                   \
        BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(qual_name, N),                                   \
        Base,                                                                                   \
        typename ::boost::enable_if<                                                            \
            ::boost::type_erasure::detail::should_be_non_const<T, Base>,                        \
            typename ::boost::remove_const<T>::type                                             \
        >::type,                                                                                \
        Enable                                                                                  \
    > : Base                                                                                    \
    {                                                                                           \
        typedef void BOOST_PP_CAT(_boost_type_erasure_has_member, member);                      \
        typename rebind_any<Base, R>::type member(                                              \
            BOOST_TYPE_ERASURE_MEMBER_ENUM_ARGS(N))                                             \
        {                                                                                       \
            return ::boost::type_erasure::call(                                                 \
                BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(qual_name, N)(),                         \
                *this BOOST_TYPE_ERASURE_MEMBER_FORWARD_REBIND(N));                             \
        }                                                                                       \
    };                                                                                          \
    template<                                                                                   \
        class R BOOST_TYPE_ERASURE_MEMBER_TPL_ARG_LIST(N, class A),                             \
        class T, class Base, class Enable>                                                      \
    struct concept_interface<                                                                   \
        BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(qual_name, N),                                   \
        Base,                                                                                   \
        typename ::boost::enable_if<                                                            \
            ::boost::type_erasure::detail::should_be_const<T, Base>,                            \
            typename ::boost::remove_const<T>::type                                             \
        >::type,                                                                                \
        Enable                                                                                  \
    > : Base                                                                                    \
    {                                                                                           \
        typedef void BOOST_PP_CAT(_boost_type_erasure_has_member, member);                      \
        typename rebind_any<Base, R>::type member(                                              \
            BOOST_TYPE_ERASURE_MEMBER_ENUM_ARGS(N)) const                                       \
        {                                                                                       \
            return ::boost::type_erasure::call(                                                 \
                BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(qual_name, N)(),                         \
                *this BOOST_TYPE_ERASURE_MEMBER_FORWARD_REBIND(N));                             \
        }                                                                                       \
    };                                                                                          \
    template<class R BOOST_TYPE_ERASURE_MEMBER_TPL_ARG_LIST(N, class A), class T, class Base>   \
    struct concept_interface<                                                                   \
        BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(qual_name, N),                                   \
        Base,                                                                                   \
        typename ::boost::enable_if<                                                            \
            ::boost::type_erasure::detail::should_be_non_const<T, Base>,                        \
            typename ::boost::remove_const<T>::type                                             \
        >::type,                                                                                \
        typename Base::BOOST_PP_CAT(_boost_type_erasure_has_member, member)> : Base             \
    {                                                                                           \
        using Base::member;                                                                     \
        typename rebind_any<Base, R>::type member(                                              \
            BOOST_TYPE_ERASURE_MEMBER_ENUM_ARGS(N))                                             \
        {                                                                                       \
            return ::boost::type_erasure::call(                                                 \
                BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(qual_name, N)(),                         \
                *this BOOST_TYPE_ERASURE_MEMBER_FORWARD_REBIND(N));                             \
        }                                                                                       \
    };                                                                                          \
    template<class R BOOST_TYPE_ERASURE_MEMBER_TPL_ARG_LIST(N, class A), class T, class Base>   \
    struct concept_interface<                                                                   \
        BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(qual_name, N),                                   \
        Base,                                                                                   \
        typename ::boost::enable_if<                                                            \
            ::boost::type_erasure::detail::should_be_const<T, Base>,                            \
            typename ::boost::remove_const<T>::type                                             \
        >::type,                                                                                \
        typename Base::BOOST_PP_CAT(_boost_type_erasure_has_member, member)> : Base             \
    {                                                                                           \
        using Base::member;                                                                     \
        typename rebind_any<Base, R>::type member(                                              \
            BOOST_TYPE_ERASURE_MEMBER_ENUM_ARGS(N)) const                                       \
        {                                                                                       \
            return ::boost::type_erasure::call(                                                 \
                BOOST_TYPE_ERASURE_MEMBER_QUALIFIED_ID(qual_name, N)(),                         \
                *this BOOST_TYPE_ERASURE_MEMBER_FORWARD_REBIND(N));                             \
        }                                                                                       \
    };                                                                                          \
    }}

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_MEMBER_I(namespace_name, concept_name, member, N)\
    BOOST_TYPE_ERASURE_MEMBER_II(namespace_name, concept_name, member, N)

#endif

#endif
