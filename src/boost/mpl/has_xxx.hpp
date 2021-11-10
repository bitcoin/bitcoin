
#ifndef BOOST_MPL_HAS_XXX_HPP_INCLUDED
#define BOOST_MPL_HAS_XXX_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2002-2006
// Copyright David Abrahams 2002-2003
// Copyright Daniel Walker 2007
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#include <boost/mpl/bool.hpp>
#include <boost/mpl/aux_/na_spec.hpp>
#include <boost/mpl/aux_/type_wrapper.hpp>
#include <boost/mpl/aux_/yes_no.hpp>
#include <boost/mpl/aux_/config/gcc.hpp>
#include <boost/mpl/aux_/config/has_xxx.hpp>
#include <boost/mpl/aux_/config/msvc_typename.hpp>
#include <boost/mpl/aux_/config/msvc.hpp>
#include <boost/mpl/aux_/config/static_constant.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>

#include <boost/preprocessor/array/elem.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>

#if BOOST_WORKAROUND( BOOST_BORLANDC, BOOST_TESTED_AT(0x590) )
# include <boost/type_traits/is_class.hpp>
#endif

#if !defined(BOOST_MPL_CFG_NO_HAS_XXX)

#   if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)

// agurt, 11/sep/02: MSVC-specific version (< 7.1), based on a USENET 
// newsgroup's posting by John Madsen (comp.lang.c++.moderated, 
// 1999-11-12 19:17:06 GMT); the code is _not_ standard-conforming, but 
// it works way more reliably than the SFINAE-based implementation

// Modified dwa 8/Oct/02 to handle reference types.

#   include <boost/mpl/if.hpp>
#   include <boost/mpl/bool.hpp>

namespace boost { namespace mpl { namespace aux {

struct has_xxx_tag;

#if BOOST_WORKAROUND(BOOST_MSVC, == 1300)
template< typename U > struct msvc_incomplete_array
{
    typedef char (&type)[sizeof(U) + 1];
};
#endif

template< typename T >
struct msvc_is_incomplete
{
    // MSVC is capable of some kinds of SFINAE.  If U is an incomplete
    // type, it won't pick the second overload
    static char tester(...);

#if BOOST_WORKAROUND(BOOST_MSVC, == 1300)
    template< typename U >
    static typename msvc_incomplete_array<U>::type tester(type_wrapper<U>);
#else
    template< typename U >
    static char (& tester(type_wrapper<U>) )[sizeof(U)+1];
#endif 
    
    BOOST_STATIC_CONSTANT(bool, value = 
          sizeof(tester(type_wrapper<T>())) == 1
        );
};

template<>
struct msvc_is_incomplete<int>
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};

}}}

#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF_(trait, name, default_) \
template< typename T, typename name = ::boost::mpl::aux::has_xxx_tag > \
struct BOOST_PP_CAT(trait,_impl) : T \
{ \
    static boost::mpl::aux::no_tag \
    test(void(*)(::boost::mpl::aux::has_xxx_tag)); \
    \
    static boost::mpl::aux::yes_tag test(...); \
    \
    BOOST_STATIC_CONSTANT(bool, value = \
          sizeof(test(static_cast<void(*)(name)>(0))) \
            != sizeof(boost::mpl::aux::no_tag) \
        ); \
    typedef boost::mpl::bool_<value> type; \
}; \
\
template< typename T, typename fallback_ = boost::mpl::bool_<default_> > \
struct trait \
    : boost::mpl::if_c< \
          boost::mpl::aux::msvc_is_incomplete<T>::value \
        , boost::mpl::bool_<false> \
        , BOOST_PP_CAT(trait,_impl)<T> \
        >::type \
{ \
}; \
\
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, void) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, bool) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, char) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, signed char) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, unsigned char) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, signed short) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, unsigned short) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, signed int) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, unsigned int) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, signed long) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, unsigned long) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, float) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, double) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, long double) \
/**/

#   define BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, T) \
template<> struct trait<T> \
{ \
    BOOST_STATIC_CONSTANT(bool, value = false); \
    typedef boost::mpl::bool_<false> type; \
}; \
/**/

#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, unused) \
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF_(trait, name, unused) \
    BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, wchar_t) \
/**/
#else
#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, unused) \
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF_(trait, name, unused) \
/**/
#endif


// SFINAE-based implementations below are derived from a USENET newsgroup's 
// posting by Rani Sharoni (comp.lang.c++.moderated, 2002-03-17 07:45:09 PST)

#   elif BOOST_WORKAROUND(BOOST_MSVC, <= 1400) \
      || (BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1800)) && defined(__CUDACC__)) \
      || BOOST_WORKAROUND(__IBMCPP__, <= 700)

// MSVC 7.1 & MSVC 8.0 & VACPP

// agurt, 15/jun/05: replace overload-based SFINAE implementation with SFINAE
// applied to partial specialization to fix some apparently random failures 
// (thanks to Daniel Wallin for researching this!)

#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, default_) \
template< typename T > \
struct BOOST_PP_CAT(trait, _msvc_sfinae_helper) \
{ \
    typedef void type; \
};\
\
template< typename T, typename U = void > \
struct BOOST_PP_CAT(trait,_impl_) \
{ \
    BOOST_STATIC_CONSTANT(bool, value = false); \
    typedef boost::mpl::bool_<value> type; \
}; \
\
template< typename T > \
struct BOOST_PP_CAT(trait,_impl_)< \
      T \
    , typename BOOST_PP_CAT(trait, _msvc_sfinae_helper)< typename T::name >::type \
    > \
{ \
    BOOST_STATIC_CONSTANT(bool, value = true); \
    typedef boost::mpl::bool_<value> type; \
}; \
\
template< typename T, typename fallback_ = boost::mpl::bool_<default_> > \
struct trait \
    : BOOST_PP_CAT(trait,_impl_)<T> \
{ \
}; \
/**/

#   elif BOOST_WORKAROUND( BOOST_BORLANDC, BOOST_TESTED_AT(0x590) )

#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_BCB_DEF(trait, trait_tester, name, default_) \
template< typename T, bool IS_CLASS > \
struct trait_tester \
{ \
    BOOST_STATIC_CONSTANT( bool,  value = false ); \
}; \
template< typename T > \
struct trait_tester< T, true > \
{ \
    struct trait_tester_impl \
    { \
        template < class U > \
        static int  resolve( boost::mpl::aux::type_wrapper<U> const volatile * \
                           , boost::mpl::aux::type_wrapper<typename U::name >* = 0 ); \
        static char resolve( ... ); \
    }; \
    typedef boost::mpl::aux::type_wrapper<T> t_; \
    BOOST_STATIC_CONSTANT( bool, value = ( sizeof( trait_tester_impl::resolve( static_cast< t_ * >(0) ) ) == sizeof(int) ) ); \
}; \
template< typename T, typename fallback_ = boost::mpl::bool_<default_> > \
struct trait           \
{                      \
    BOOST_STATIC_CONSTANT( bool, value = (trait_tester< T, boost::is_class< T >::value >::value) );     \
    typedef boost::mpl::bool_< trait< T, fallback_ >::value > type; \
};

#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, default_) \
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_BCB_DEF( trait \
                                         , BOOST_PP_CAT(trait,_tester)      \
                                         , name       \
                                         , default_ ) \
/**/

#   else // other SFINAE-capable compilers

#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, default_) \
template< typename T, typename fallback_ = boost::mpl::bool_<default_> > \
struct trait \
{ \
    struct gcc_3_2_wknd \
    { \
        template< typename U > \
        static boost::mpl::aux::yes_tag test( \
              boost::mpl::aux::type_wrapper<U> const volatile* \
            , boost::mpl::aux::type_wrapper<BOOST_MSVC_TYPENAME U::name>* = 0 \
            ); \
    \
        static boost::mpl::aux::no_tag test(...); \
    }; \
    \
    typedef boost::mpl::aux::type_wrapper<T> t_; \
    BOOST_STATIC_CONSTANT(bool, value = \
          sizeof(gcc_3_2_wknd::test(static_cast<t_*>(0))) \
            == sizeof(boost::mpl::aux::yes_tag) \
        ); \
    typedef boost::mpl::bool_<value> type; \
}; \
/**/

#   endif // BOOST_WORKAROUND(BOOST_MSVC, <= 1300)


#else // BOOST_MPL_CFG_NO_HAS_XXX

// placeholder implementation

#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, default_) \
template< typename T, typename fallback_ = boost::mpl::bool_<default_> > \
struct trait \
{ \
    BOOST_STATIC_CONSTANT(bool, value = fallback_::value); \
    typedef fallback_ type; \
}; \
/**/

#endif

#define BOOST_MPL_HAS_XXX_TRAIT_DEF(name) \
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(BOOST_PP_CAT(has_,name), name, false) \
/**/


#if !defined(BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE)

// Create a boolean Metafunction to detect a nested template
// member. This implementation is based on a USENET newsgroup's
// posting by Aleksey Gurtovoy (comp.lang.c++.moderated, 2002-03-19),
// Rani Sharoni's USENET posting cited above, the non-template has_xxx
// implementations above, and discussion on the Boost mailing list.

#   if !defined(BOOST_MPL_HAS_XXX_NO_WRAPPED_TYPES)
#     if BOOST_WORKAROUND(BOOST_MSVC, <= 1400)
#       define BOOST_MPL_HAS_XXX_NO_WRAPPED_TYPES 1
#     else
#       define BOOST_MPL_HAS_XXX_NO_WRAPPED_TYPES 0
#     endif
#   endif

#   if !defined(BOOST_MPL_HAS_XXX_NO_EXPLICIT_TEST_FUNCTION)
#     if (defined(BOOST_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS))
#       define BOOST_MPL_HAS_XXX_NO_EXPLICIT_TEST_FUNCTION 1
#     else
#       define BOOST_MPL_HAS_XXX_NO_EXPLICIT_TEST_FUNCTION 0
#     endif
#   endif

#   if !defined(BOOST_MPL_HAS_XXX_NEEDS_TEMPLATE_SFINAE)
#     if BOOST_WORKAROUND(BOOST_MSVC, <= 1400)
#       define BOOST_MPL_HAS_XXX_NEEDS_TEMPLATE_SFINAE 1
#     else
#       define BOOST_MPL_HAS_XXX_NEEDS_TEMPLATE_SFINAE 0
#     endif
#   endif

// NOTE: Many internal implementation macros take a Boost.Preprocessor
// array argument called args which is of the following form.
//           ( 4, ( trait, name, max_arity, default_ ) )

#   define BOOST_MPL_HAS_MEMBER_INTROSPECTION_NAME(args) \
      BOOST_PP_CAT(BOOST_PP_ARRAY_ELEM(0, args) , _introspect) \
    /**/

#   define BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME(args, n) \
      BOOST_PP_CAT(BOOST_PP_CAT(BOOST_PP_ARRAY_ELEM(0, args) , _substitute), n) \
    /**/

#   define BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args) \
      BOOST_PP_CAT(BOOST_PP_ARRAY_ELEM(0, args) , _test) \
    /**/

// Thanks to Guillaume Melquiond for pointing out the need for the
// "substitute" template as an argument to the overloaded test
// functions to get SFINAE to work for member templates with the
// correct name but different number of arguments.
#   define BOOST_MPL_HAS_MEMBER_MULTI_SUBSTITUTE(z, n, args) \
      template< \
          template< BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(n), typename V) > class V \
       > \
      struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME(args, n) { \
      }; \
    /**/

#   define BOOST_MPL_HAS_MEMBER_SUBSTITUTE(args, substitute_macro) \
      BOOST_PP_REPEAT( \
          BOOST_PP_ARRAY_ELEM(2, args) \
        , BOOST_MPL_HAS_MEMBER_MULTI_SUBSTITUTE \
        , args \
      ) \
    /**/

#   if !BOOST_MPL_HAS_XXX_NO_EXPLICIT_TEST_FUNCTION
#     define BOOST_MPL_HAS_MEMBER_REJECT(args, member_macro) \
        template< typename V > \
        static boost::mpl::aux::no_tag \
        BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)(...); \
      /**/
#   else
#     define BOOST_MPL_HAS_MEMBER_REJECT(args, member_macro) \
        static boost::mpl::aux::no_tag \
        BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)(...); \
      /**/
#   endif

#   if !BOOST_MPL_HAS_XXX_NO_WRAPPED_TYPES
#     define BOOST_MPL_HAS_MEMBER_MULTI_ACCEPT(z, n, args) \
        template< typename V > \
        static boost::mpl::aux::yes_tag \
        BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)( \
            boost::mpl::aux::type_wrapper< V > const volatile* \
          , BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME(args, n) < \
                V::template BOOST_PP_ARRAY_ELEM(1, args) \
            >* = 0 \
        ); \
      /**/
#     define BOOST_MPL_HAS_MEMBER_ACCEPT(args, member_macro) \
        BOOST_PP_REPEAT( \
            BOOST_PP_ARRAY_ELEM(2, args) \
          , BOOST_MPL_HAS_MEMBER_MULTI_ACCEPT \
          , args \
        ) \
      /**/
#   else
#     define BOOST_MPL_HAS_MEMBER_ACCEPT(args, member_macro) \
        template< typename V > \
        static boost::mpl::aux::yes_tag \
        BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)( \
            V const volatile* \
          , member_macro(args, V, T)* = 0 \
        ); \
      /**/
#   endif

#   if !BOOST_MPL_HAS_XXX_NO_EXPLICIT_TEST_FUNCTION
#     define BOOST_MPL_HAS_MEMBER_TEST(args) \
          sizeof(BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)< U >(0)) \
              == sizeof(boost::mpl::aux::yes_tag) \
      /**/
#   else
#     if !BOOST_MPL_HAS_XXX_NO_WRAPPED_TYPES
#       define BOOST_MPL_HAS_MEMBER_TEST(args) \
          sizeof( \
              BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)( \
                  static_cast< boost::mpl::aux::type_wrapper< U >* >(0) \
              ) \
          ) == sizeof(boost::mpl::aux::yes_tag) \
        /**/
#     else
#       define BOOST_MPL_HAS_MEMBER_TEST(args) \
          sizeof( \
              BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)( \
                  static_cast< U* >(0) \
              ) \
          ) == sizeof(boost::mpl::aux::yes_tag) \
        /**/
#     endif
#   endif

#   define BOOST_MPL_HAS_MEMBER_INTROSPECT( \
               args, substitute_macro, member_macro \
           ) \
      template< typename U > \
      struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_NAME(args) { \
          BOOST_MPL_HAS_MEMBER_SUBSTITUTE(args, substitute_macro) \
          BOOST_MPL_HAS_MEMBER_REJECT(args, member_macro) \
          BOOST_MPL_HAS_MEMBER_ACCEPT(args, member_macro) \
          BOOST_STATIC_CONSTANT( \
              bool, value = BOOST_MPL_HAS_MEMBER_TEST(args) \
          ); \
          typedef boost::mpl::bool_< value > type; \
      }; \
    /**/

#   define BOOST_MPL_HAS_MEMBER_IMPLEMENTATION( \
               args, introspect_macro, substitute_macro, member_macro \
           ) \
      template< \
          typename T \
        , typename fallback_ \
              = boost::mpl::bool_< BOOST_PP_ARRAY_ELEM(3, args) > \
      > \
      class BOOST_PP_ARRAY_ELEM(0, args) { \
          introspect_macro(args, substitute_macro, member_macro) \
      public: \
          static const bool value \
              = BOOST_MPL_HAS_MEMBER_INTROSPECTION_NAME(args)< T >::value; \
          typedef typename BOOST_MPL_HAS_MEMBER_INTROSPECTION_NAME(args)< \
              T \
          >::type type; \
      }; \
    /**/

// BOOST_MPL_HAS_MEMBER_WITH_FUNCTION_SFINAE expands to the full
// implementation of the function-based metafunction. Compile with -E
// to see the preprocessor output for this macro.
#   define BOOST_MPL_HAS_MEMBER_WITH_FUNCTION_SFINAE( \
               args, substitute_macro, member_macro \
           ) \
      BOOST_MPL_HAS_MEMBER_IMPLEMENTATION( \
          args \
        , BOOST_MPL_HAS_MEMBER_INTROSPECT \
        , substitute_macro \
        , member_macro \
      ) \
    /**/

#   if BOOST_MPL_HAS_XXX_NEEDS_TEMPLATE_SFINAE

#     if !defined(BOOST_MPL_HAS_XXX_NEEDS_NAMESPACE_LEVEL_SUBSTITUTE)
#       if BOOST_WORKAROUND(BOOST_MSVC, <= 1400)
#         define BOOST_MPL_HAS_XXX_NEEDS_NAMESPACE_LEVEL_SUBSTITUTE 1
#       endif
#     endif

#     if !BOOST_MPL_HAS_XXX_NEEDS_NAMESPACE_LEVEL_SUBSTITUTE
#       define BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME_WITH_TEMPLATE_SFINAE( \
                   args, n \
               ) \
          BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME(args, n) \
        /**/
#     else
#       define BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME_WITH_TEMPLATE_SFINAE( \
                   args, n \
               ) \
          BOOST_PP_CAT( \
              boost_mpl_has_xxx_ \
            , BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME(args, n) \
          ) \
        /**/
#     endif

#     define BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_TAG_NAME( \
                 args \
             ) \
        BOOST_PP_CAT( \
            BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME_WITH_TEMPLATE_SFINAE( \
                args, 0 \
            ) \
          , _tag \
        ) \
      /**/

#     define BOOST_MPL_HAS_MEMBER_MULTI_SUBSTITUTE_WITH_TEMPLATE_SFINAE( \
                 z, n, args \
             ) \
        template< \
             template< BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(n), typename U) > class U \
        > \
        struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME_WITH_TEMPLATE_SFINAE( \
                args, n \
               ) { \
            typedef \
                BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_TAG_NAME(args) \
                type; \
        }; \
      /**/

#     define BOOST_MPL_HAS_MEMBER_SUBSTITUTE_WITH_TEMPLATE_SFINAE( \
                 args, substitute_macro \
             ) \
        typedef void \
            BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_TAG_NAME(args); \
        BOOST_PP_REPEAT( \
            BOOST_PP_ARRAY_ELEM(2, args) \
          , BOOST_MPL_HAS_MEMBER_MULTI_SUBSTITUTE_WITH_TEMPLATE_SFINAE \
          , args \
        ) \
      /**/

#     define BOOST_MPL_HAS_MEMBER_REJECT_WITH_TEMPLATE_SFINAE( \
                 args, member_macro \
             ) \
        template< \
            typename U \
          , typename V \
                = BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_TAG_NAME(args) \
        > \
        struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args) { \
            BOOST_STATIC_CONSTANT(bool, value = false); \
            typedef boost::mpl::bool_< value > type; \
        }; \
      /**/

#     define BOOST_MPL_HAS_MEMBER_MULTI_ACCEPT_WITH_TEMPLATE_SFINAE( \
                 z, n, args \
             ) \
        template< typename U > \
        struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)< \
            U \
          , typename \
                BOOST_MPL_HAS_MEMBER_INTROSPECTION_SUBSTITUTE_NAME_WITH_TEMPLATE_SFINAE( \
                    args, n \
                )< \
                    BOOST_MSVC_TYPENAME U::BOOST_PP_ARRAY_ELEM(1, args)< > \
                >::type \
        > { \
            BOOST_STATIC_CONSTANT(bool, value = true); \
            typedef boost::mpl::bool_< value > type; \
        }; \
      /**/

#     define BOOST_MPL_HAS_MEMBER_ACCEPT_WITH_TEMPLATE_SFINAE( \
                 args, member_macro \
             ) \
        BOOST_PP_REPEAT( \
            BOOST_PP_ARRAY_ELEM(2, args) \
          , BOOST_MPL_HAS_MEMBER_MULTI_ACCEPT_WITH_TEMPLATE_SFINAE \
          , args \
        ) \
      /**/

#     define BOOST_MPL_HAS_MEMBER_INTROSPECT_WITH_TEMPLATE_SFINAE( \
                 args, substitute_macro, member_macro \
             ) \
        BOOST_MPL_HAS_MEMBER_REJECT_WITH_TEMPLATE_SFINAE(args, member_macro) \
        BOOST_MPL_HAS_MEMBER_ACCEPT_WITH_TEMPLATE_SFINAE(args, member_macro) \
        template< typename U > \
        struct BOOST_MPL_HAS_MEMBER_INTROSPECTION_NAME(args) \
            : BOOST_MPL_HAS_MEMBER_INTROSPECTION_TEST_NAME(args)< U > { \
        }; \
      /**/
 
// BOOST_MPL_HAS_MEMBER_WITH_TEMPLATE_SFINAE expands to the full
// implementation of the template-based metafunction. Compile with -E
// to see the preprocessor output for this macro.
//
// Note that if BOOST_MPL_HAS_XXX_NEEDS_NAMESPACE_LEVEL_SUBSTITUTE is
// defined BOOST_MPL_HAS_MEMBER_SUBSTITUTE_WITH_TEMPLATE_SFINAE needs
// to be expanded at namespace level before
// BOOST_MPL_HAS_MEMBER_WITH_TEMPLATE_SFINAE can be used.
#     define BOOST_MPL_HAS_MEMBER_WITH_TEMPLATE_SFINAE( \
                 args, substitute_macro, member_macro \
             ) \
        BOOST_MPL_HAS_MEMBER_SUBSTITUTE_WITH_TEMPLATE_SFINAE( \
            args, substitute_macro \
        ) \
        BOOST_MPL_HAS_MEMBER_IMPLEMENTATION( \
            args \
          , BOOST_MPL_HAS_MEMBER_INTROSPECT_WITH_TEMPLATE_SFINAE \
          , substitute_macro \
          , member_macro \
        ) \
      /**/

#   endif // BOOST_MPL_HAS_XXX_NEEDS_TEMPLATE_SFINAE

// Note: In the current implementation the parameter and access macros
// are no longer expanded.
#   if !BOOST_WORKAROUND(BOOST_MSVC, <= 1400)
#     define BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF(trait, name, default_) \
        BOOST_MPL_HAS_MEMBER_WITH_FUNCTION_SFINAE( \
            ( 4, ( trait, name, BOOST_MPL_LIMIT_METAFUNCTION_ARITY, default_ ) ) \
          , BOOST_MPL_HAS_MEMBER_TEMPLATE_SUBSTITUTE_PARAMETER \
          , BOOST_MPL_HAS_MEMBER_TEMPLATE_ACCESS \
        ) \
      /**/
#   else
#     define BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF(trait, name, default_) \
        BOOST_MPL_HAS_MEMBER_WITH_TEMPLATE_SFINAE( \
            ( 4, ( trait, name, BOOST_MPL_LIMIT_METAFUNCTION_ARITY, default_ ) ) \
          , BOOST_MPL_HAS_MEMBER_TEMPLATE_SUBSTITUTE_PARAMETER \
          , BOOST_MPL_HAS_MEMBER_TEMPLATE_ACCESS \
        ) \
      /**/
#   endif

#else // BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE

// placeholder implementation

#   define BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF(trait, name, default_) \
      template< typename T \
              , typename fallback_ = boost::mpl::bool_< default_ > > \
      struct trait { \
          BOOST_STATIC_CONSTANT(bool, value = fallback_::value); \
          typedef fallback_ type; \
      }; \
    /**/

#endif // BOOST_MPL_CFG_NO_HAS_XXX_TEMPLATE

#   define BOOST_MPL_HAS_XXX_TEMPLATE_DEF(name) \
      BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF( \
          BOOST_PP_CAT(has_, name), name, false \
      ) \
    /**/

#endif // BOOST_MPL_HAS_XXX_HPP_INCLUDED
