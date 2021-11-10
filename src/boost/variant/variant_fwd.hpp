//-----------------------------------------------------------------------------
// boost variant/variant_fwd.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003 Eric Friedman, Itay Maman
// Copyright (c) 2013-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_VARIANT_FWD_HPP
#define BOOST_VARIANT_VARIANT_FWD_HPP

#include <boost/variant/detail/config.hpp>

#include <boost/blank_fwd.hpp>
#include <boost/mpl/arg.hpp>
#include <boost/mpl/limits/arity.hpp>
#include <boost/mpl/aux_/na.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/enum.hpp>
#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/enum_shifted_params.hpp>
#include <boost/preprocessor/repeat.hpp>

///////////////////////////////////////////////////////////////////////////////
// macro BOOST_VARIANT_NO_TYPE_SEQUENCE_SUPPORT
//
// Defined if variant does not support make_variant_over (see below). 
//
#if defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE)
#   define BOOST_VARIANT_NO_TYPE_SEQUENCE_SUPPORT
#endif

///////////////////////////////////////////////////////////////////////////////
// macro BOOST_VARIANT_NO_FULL_RECURSIVE_VARIANT_SUPPORT
//
// Defined if make_recursive_variant cannot be supported as documented.
//
// Note: Currently, MPL lambda facility is used as workaround if defined, and
// so only types declared w/ MPL lambda workarounds will work.
//

#include <boost/variant/detail/substitute_fwd.hpp>

#if defined(BOOST_VARIANT_DETAIL_NO_SUBSTITUTE) \
  && !defined(BOOST_VARIANT_NO_FULL_RECURSIVE_VARIANT_SUPPORT)
#   define BOOST_VARIANT_NO_FULL_RECURSIVE_VARIANT_SUPPORT
#endif


///////////////////////////////////////////////////////////////////////////////
// macro BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES
//

/* 
    GCC before 4.0 had no variadic tempaltes; 
    GCC 4.6 has incomplete implementation of variadic templates.

    MSVC2015 Update 1 has variadic templates, but they have issues.

    NOTE: Clang compiler defines __GNUC__
*/
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) \
  || (!defined(__clang__) && defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ < 7)) \
  || (defined(_MSC_VER) && (_MSC_VER <= 1900)) \
  || defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE) \
  || defined (BOOST_VARIANT_NO_TYPE_SEQUENCE_SUPPORT)

#ifndef BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES
#   define BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES
#endif

#endif

#if !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)
#include <boost/preprocessor/seq/size.hpp>

#define BOOST_VARIANT_CLASS_OR_TYPENAME_TO_SEQ_class class)(
#define BOOST_VARIANT_CLASS_OR_TYPENAME_TO_SEQ_typename typename)(

#define BOOST_VARIANT_CLASS_OR_TYPENAME_TO_VARIADIC_class class...
#define BOOST_VARIANT_CLASS_OR_TYPENAME_TO_VARIADIC_typename typename...

#define ARGS_VARIADER_1(x) x ## N...
#define ARGS_VARIADER_2(x) BOOST_VARIANT_CLASS_OR_TYPENAME_TO_VARIADIC_ ## x ## N

#define BOOST_VARIANT_MAKE_VARIADIC(sequence, x)        BOOST_VARIANT_MAKE_VARIADIC_I(BOOST_PP_SEQ_SIZE(sequence), x)
#define BOOST_VARIANT_MAKE_VARIADIC_I(argscount, x)     BOOST_VARIANT_MAKE_VARIADIC_II(argscount, x)
#define BOOST_VARIANT_MAKE_VARIADIC_II(argscount, orig) ARGS_VARIADER_ ## argscount(orig)

///////////////////////////////////////////////////////////////////////////////
// BOOST_VARIANT_ENUM_PARAMS and BOOST_VARIANT_ENUM_SHIFTED_PARAMS
//
// Convenience macro for enumeration of variant params.
// When variadic templates are available expands:
//      BOOST_VARIANT_ENUM_PARAMS(class Something)      => class Something0, class... SomethingN
//      BOOST_VARIANT_ENUM_PARAMS(typename Something)   => typename Something0, typename... SomethingN
//      BOOST_VARIANT_ENUM_PARAMS(Something)            => Something0, SomethingN...
//      BOOST_VARIANT_ENUM_PARAMS(Something)            => Something0, SomethingN...
//      BOOST_VARIANT_ENUM_SHIFTED_PARAMS(class Something)      => class... SomethingN
//      BOOST_VARIANT_ENUM_SHIFTED_PARAMS(typename Something)   => typename... SomethingN
//      BOOST_VARIANT_ENUM_SHIFTED_PARAMS(Something)            => SomethingN...
//      BOOST_VARIANT_ENUM_SHIFTED_PARAMS(Something)            => SomethingN...
//
// Rationale: Cleaner, simpler code for clients of variant library. Minimal 
// code modifications to move from C++03 to C++11.
//
// With BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES defined
// will be used BOOST_VARIANT_ENUM_PARAMS and BOOST_VARIANT_ENUM_SHIFTED_PARAMS from below `#else`
//

#define BOOST_VARIANT_ENUM_PARAMS(x) \
    x ## 0, \
    BOOST_VARIANT_MAKE_VARIADIC( (BOOST_VARIANT_CLASS_OR_TYPENAME_TO_SEQ_ ## x), x) \
    /**/

#define BOOST_VARIANT_ENUM_SHIFTED_PARAMS(x) \
    BOOST_VARIANT_MAKE_VARIADIC( (BOOST_VARIANT_CLASS_OR_TYPENAME_TO_SEQ_ ## x), x) \
    /**/

#else // defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)

///////////////////////////////////////////////////////////////////////////////
// macro BOOST_VARIANT_LIMIT_TYPES
//
// Implementation-defined preprocessor symbol describing the actual
// length of variant's pseudo-variadic template parameter list.
//
#include <boost/mpl/limits/list.hpp>
#define BOOST_VARIANT_LIMIT_TYPES \
    BOOST_MPL_LIMIT_LIST_SIZE

///////////////////////////////////////////////////////////////////////////////
// macro BOOST_VARIANT_RECURSIVE_VARIANT_MAX_ARITY
//
// Exposes maximum allowed arity of class templates with recursive_variant
// arguments. That is,
//   make_recursive_variant< ..., T<[1], recursive_variant_, ... [N]> >.
//
#include <boost/mpl/limits/arity.hpp>
#define BOOST_VARIANT_RECURSIVE_VARIANT_MAX_ARITY \
    BOOST_MPL_LIMIT_METAFUNCTION_ARITY

///////////////////////////////////////////////////////////////////////////////
// macro BOOST_VARIANT_ENUM_PARAMS
//
// Convenience macro for enumeration of BOOST_VARIANT_LIMIT_TYPES params.
//
// Rationale: Cleaner, simpler code for clients of variant library.
//
#define BOOST_VARIANT_ENUM_PARAMS( param )  \
    BOOST_PP_ENUM_PARAMS(BOOST_VARIANT_LIMIT_TYPES, param)

///////////////////////////////////////////////////////////////////////////////
// macro BOOST_VARIANT_ENUM_SHIFTED_PARAMS
//
// Convenience macro for enumeration of BOOST_VARIANT_LIMIT_TYPES-1 params.
//
#define BOOST_VARIANT_ENUM_SHIFTED_PARAMS( param )  \
    BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_VARIANT_LIMIT_TYPES, param)

#endif // BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES workaround


namespace boost {

namespace detail { namespace variant {

///////////////////////////////////////////////////////////////////////////////
// (detail) class void_ and class template convert_void
// 
// Provides the mechanism by which void(NN) types are converted to
// mpl::void_ (and thus can be passed to mpl::list).
//
// Rationale: This is particularly needed for the using-declarations
// workaround (below), but also to avoid associating mpl namespace with
// variant in argument dependent lookups (which used to happen because of
// defaulting of template parameters to mpl::void_).
//

struct void_;

template <typename T>
struct convert_void
{
    typedef T type;
};

template <>
struct convert_void< void_ >
{
    typedef mpl::na type;
};

///////////////////////////////////////////////////////////////////////////////
// (workaround) BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE
//
// Needed to work around compilers that don't support using-declaration
// overloads. (See the variant::initializer workarounds below.)
//

#if defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE)
// (detail) tags voidNN -- NN defined on [0, BOOST_VARIANT_LIMIT_TYPES)
//
// Defines void types that are each unique and specializations of
// convert_void that yields mpl::na for each voidNN type.
//

#define BOOST_VARIANT_DETAIL_DEFINE_VOID_N(z,N,_)          \
    struct BOOST_PP_CAT(void,N);                           \
                                                           \
    template <>                                            \
    struct convert_void< BOOST_PP_CAT(void,N) >            \
    {                                                      \
        typedef mpl::na type;                              \
    };                                                     \
    /**/

BOOST_PP_REPEAT(
      BOOST_VARIANT_LIMIT_TYPES
    , BOOST_VARIANT_DETAIL_DEFINE_VOID_N
    , _
    )

#undef BOOST_VARIANT_DETAIL_DEFINE_VOID_N

#endif // BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE workaround

}} // namespace detail::variant

#if !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)
#   define BOOST_VARIANT_AUX_DECLARE_PARAMS BOOST_VARIANT_ENUM_PARAMS(typename T)
#else // defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)

///////////////////////////////////////////////////////////////////////////////
// (detail) macro BOOST_VARIANT_AUX_DECLARE_PARAM
//
// Template parameter list for variant and recursive_variant declarations.
//

#if !defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE)

#   define BOOST_VARIANT_AUX_DECLARE_PARAMS_IMPL(z, N, T) \
    typename BOOST_PP_CAT(T,N) = detail::variant::void_ \
    /**/

#else // defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE)

#   define BOOST_VARIANT_AUX_DECLARE_PARAMS_IMPL(z, N, T) \
    typename BOOST_PP_CAT(T,N) = BOOST_PP_CAT(detail::variant::void,N) \
    /**/

#endif // BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE workaround

#define BOOST_VARIANT_AUX_DECLARE_PARAMS \
    BOOST_PP_ENUM( \
          BOOST_VARIANT_LIMIT_TYPES \
        , BOOST_VARIANT_AUX_DECLARE_PARAMS_IMPL \
        , T \
        ) \
    /**/

#endif // BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES workaround

///////////////////////////////////////////////////////////////////////////////
// class template variant (concept inspired by Andrei Alexandrescu)
//
// Efficient, type-safe bounded discriminated union.
//
// Preconditions:
//  - Each type must be unique.
//  - No type may be const-qualified.
//
// Proper declaration form:
//   variant<types>    (where types is a type-sequence)
// or
//   variant<T0,T1,...,Tn>  (where T0 is NOT a type-sequence)
//
template < BOOST_VARIANT_AUX_DECLARE_PARAMS > class variant;

///////////////////////////////////////////////////////////////////////////////
// metafunction make_recursive_variant
//
// Exposes a boost::variant with recursive_variant_ tags (below) substituted
// with the variant itself (wrapped as needed with boost::recursive_wrapper).
//
template < BOOST_VARIANT_AUX_DECLARE_PARAMS > struct make_recursive_variant;

#undef BOOST_VARIANT_AUX_DECLARE_PARAMS_IMPL
#undef BOOST_VARIANT_AUX_DECLARE_PARAMS

///////////////////////////////////////////////////////////////////////////////
// type recursive_variant_
//
// Tag type indicates where recursive variant substitution should occur.
//
#if !defined(BOOST_VARIANT_NO_FULL_RECURSIVE_VARIANT_SUPPORT)
    struct recursive_variant_ {};
#else
    typedef mpl::arg<1> recursive_variant_;
#endif

///////////////////////////////////////////////////////////////////////////////
// metafunction make_variant_over
//
// Result is a variant w/ types of the specified type sequence.
//
template <typename Types> struct make_variant_over;

///////////////////////////////////////////////////////////////////////////////
// metafunction make_recursive_variant_over
//
// Result is a recursive variant w/ types of the specified type sequence.
//
template <typename Types> struct make_recursive_variant_over;

} // namespace boost

#endif // BOOST_VARIANT_VARIANT_FWD_HPP
