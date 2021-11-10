
#ifndef BOOST_MPL_ASSERT_HPP_INCLUDED
#define BOOST_MPL_ASSERT_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2006
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#include <boost/mpl/not.hpp>
#include <boost/mpl/aux_/value_wknd.hpp>
#include <boost/mpl/aux_/nested_type_wknd.hpp>
#include <boost/mpl/aux_/yes_no.hpp>
#include <boost/mpl/aux_/na.hpp>
#include <boost/mpl/aux_/adl_barrier.hpp>

#include <boost/mpl/aux_/config/nttp.hpp>
#include <boost/mpl/aux_/config/dtp.hpp>
#include <boost/mpl/aux_/config/gcc.hpp>
#include <boost/mpl/aux_/config/msvc.hpp>
#include <boost/mpl/aux_/config/gpu.hpp>
#include <boost/mpl/aux_/config/static_constant.hpp>
#include <boost/mpl/aux_/config/pp_counter.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>

#include <boost/preprocessor/cat.hpp>

#include <boost/config.hpp> // make sure 'size_t' is placed into 'std'
#include <cstddef>

#if BOOST_WORKAROUND(BOOST_MSVC, == 1700)
#include <boost/mpl/if.hpp>
#endif

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x610)) \
    || (BOOST_MPL_CFG_GCC != 0) \
    || BOOST_WORKAROUND(__IBMCPP__, <= 600)
#   define BOOST_MPL_CFG_ASSERT_USE_RELATION_NAMES
#endif

#if BOOST_WORKAROUND(__MWERKS__, < 0x3202) \
    || BOOST_WORKAROUND(__EDG_VERSION__, <= 238) \
    || BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x610)) \
    || BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x840))
#   define BOOST_MPL_CFG_ASSERT_BROKEN_POINTER_TO_POINTER_TO_MEMBER
#endif

// agurt, 10/nov/06: use enums for Borland (which cannot cope with static constants) 
// and GCC (which issues "unused variable" warnings when static constants are used 
// at a function scope)
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x610)) \
    || (BOOST_MPL_CFG_GCC != 0) || (BOOST_MPL_CFG_GPU != 0) || defined(__PGI)
#   define BOOST_MPL_AUX_ASSERT_CONSTANT(T, expr) enum { expr }
#else
#   define BOOST_MPL_AUX_ASSERT_CONSTANT(T, expr) BOOST_STATIC_CONSTANT(T, expr)
#endif


BOOST_MPL_AUX_ADL_BARRIER_NAMESPACE_OPEN

struct failed {};

// agurt, 24/aug/04: MSVC 7.1 workaround here and below: return/accept 
// 'assert<false>' by reference; can't apply it unconditionally -- apparently it
// degrades the quality of GCC diagnostics
#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
#   define AUX778076_ASSERT_ARG(x) x&
#else
#   define AUX778076_ASSERT_ARG(x) x
#endif

template< bool C >  struct assert        { typedef void* type; };
template<>          struct assert<false> { typedef AUX778076_ASSERT_ARG(assert) type; };

template< bool C >
int assertion_failed( typename assert<C>::type );

template< bool C >
struct assertion
{
    static int failed( assert<false> );
};

template<>
struct assertion<true>
{
    static int failed( void* );
};

struct assert_
{
#if !defined(BOOST_MPL_CFG_NO_DEFAULT_PARAMETERS_IN_NESTED_TEMPLATES)
    template< typename T1, typename T2 = na, typename T3 = na, typename T4 = na > struct types {};
#endif
    static assert_ const arg;
    enum relations { equal = 1, not_equal, greater, greater_equal, less, less_equal };
};


#if !defined(BOOST_MPL_CFG_ASSERT_USE_RELATION_NAMES)

bool operator==( failed, failed );
bool operator!=( failed, failed );
bool operator>( failed, failed );
bool operator>=( failed, failed );
bool operator<( failed, failed );
bool operator<=( failed, failed );

#if defined(__EDG_VERSION__)
template< bool (*)(failed, failed), long x, long y > struct assert_relation {};
#   define BOOST_MPL_AUX_ASSERT_RELATION(x, y, r) assert_relation<r,x,y>
#else
template< BOOST_MPL_AUX_NTTP_DECL(long, x), BOOST_MPL_AUX_NTTP_DECL(long, y), bool (*)(failed, failed) > 
struct assert_relation {};
#   define BOOST_MPL_AUX_ASSERT_RELATION(x, y, r) assert_relation<x,y,r>
#endif

#else // BOOST_MPL_CFG_ASSERT_USE_RELATION_NAMES

boost::mpl::aux::weighted_tag<1>::type operator==( assert_, assert_ );
boost::mpl::aux::weighted_tag<2>::type operator!=( assert_, assert_ );
boost::mpl::aux::weighted_tag<3>::type operator>(  assert_, assert_ );
boost::mpl::aux::weighted_tag<4>::type operator>=( assert_, assert_ );
boost::mpl::aux::weighted_tag<5>::type operator<( assert_, assert_ );
boost::mpl::aux::weighted_tag<6>::type operator<=( assert_, assert_ );

template< assert_::relations r, long x, long y > struct assert_relation {};

#endif 

#if BOOST_WORKAROUND(BOOST_MSVC, == 1700)

template<class Pred>
struct extract_assert_pred;

template<class Pred>
struct extract_assert_pred<void(Pred)> { typedef Pred type; };

template<class Pred>
struct eval_assert {
    typedef typename extract_assert_pred<Pred>::type P;
    typedef typename P::type p_type;
    typedef typename ::boost::mpl::if_c<p_type::value,
        AUX778076_ASSERT_ARG(assert<false>),
        failed ************ P::************
    >::type type;
};

template<class Pred>
struct eval_assert_not {
    typedef typename extract_assert_pred<Pred>::type P;
    typedef typename P::type p_type;
    typedef typename ::boost::mpl::if_c<!p_type::value,
        AUX778076_ASSERT_ARG(assert<false>),
        failed ************ ::boost::mpl::not_<P>::************
    >::type type;
};

template< typename T >
T make_assert_arg();

#elif !defined(BOOST_MPL_CFG_ASSERT_BROKEN_POINTER_TO_POINTER_TO_MEMBER)

template< bool > struct assert_arg_pred_impl { typedef int type; };
template<> struct assert_arg_pred_impl<true> { typedef void* type; };

template< typename P > struct assert_arg_pred
{
    typedef typename P::type p_type;
    typedef typename assert_arg_pred_impl< p_type::value >::type type;
};

template< typename P > struct assert_arg_pred_not
{
    typedef typename P::type p_type;
    BOOST_MPL_AUX_ASSERT_CONSTANT( bool, p = !p_type::value );
    typedef typename assert_arg_pred_impl<p>::type type;
};

#if defined(BOOST_GCC) && BOOST_GCC >= 80000
#define BOOST_MPL_IGNORE_PARENTHESES_WARNING
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

template< typename Pred >
failed ************ (Pred::************
      assert_arg( void (*)(Pred), typename assert_arg_pred<Pred>::type )
    );

template< typename Pred >
failed ************ (boost::mpl::not_<Pred>::************
      assert_not_arg( void (*)(Pred), typename assert_arg_pred_not<Pred>::type )
    );

#ifdef BOOST_MPL_IGNORE_PARENTHESES_WARNING
#undef BOOST_MPL_IGNORE_PARENTHESES_WARNING
#pragma GCC diagnostic pop
#endif

template< typename Pred >
AUX778076_ASSERT_ARG(assert<false>)
assert_arg( void (*)(Pred), typename assert_arg_pred_not<Pred>::type );

template< typename Pred >
AUX778076_ASSERT_ARG(assert<false>)
assert_not_arg( void (*)(Pred), typename assert_arg_pred<Pred>::type );


#else // BOOST_MPL_CFG_ASSERT_BROKEN_POINTER_TO_POINTER_TO_MEMBER
        
template< bool c, typename Pred > struct assert_arg_type_impl
{
    typedef failed      ************ Pred::* mwcw83_wknd;
    typedef mwcw83_wknd ************* type;
};

template< typename Pred > struct assert_arg_type_impl<true,Pred>
{
    typedef AUX778076_ASSERT_ARG(assert<false>) type;
};

template< typename Pred > struct assert_arg_type
    : assert_arg_type_impl< BOOST_MPL_AUX_VALUE_WKND(BOOST_MPL_AUX_NESTED_TYPE_WKND(Pred))::value, Pred >
{
};

template< typename Pred >
typename assert_arg_type<Pred>::type 
assert_arg(void (*)(Pred), int);

template< typename Pred >
typename assert_arg_type< boost::mpl::not_<Pred> >::type 
assert_not_arg(void (*)(Pred), int);

#   if !defined(BOOST_MPL_CFG_ASSERT_USE_RELATION_NAMES)
template< long x, long y, bool (*r)(failed, failed) >
typename assert_arg_type_impl< false,BOOST_MPL_AUX_ASSERT_RELATION(x,y,r) >::type
assert_rel_arg( BOOST_MPL_AUX_ASSERT_RELATION(x,y,r) );
#   else
template< assert_::relations r, long x, long y >
typename assert_arg_type_impl< false,assert_relation<r,x,y> >::type
assert_rel_arg( assert_relation<r,x,y> );
#   endif

#endif // BOOST_MPL_CFG_ASSERT_BROKEN_POINTER_TO_POINTER_TO_MEMBER

#undef AUX778076_ASSERT_ARG

BOOST_MPL_AUX_ADL_BARRIER_NAMESPACE_CLOSE

#if BOOST_WORKAROUND(BOOST_MSVC, == 1700)

// BOOST_MPL_ASSERT((pred<x,...>))

#define BOOST_MPL_ASSERT(pred) \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,BOOST_MPL_AUX_PP_COUNTER()) = sizeof( \
          boost::mpl::assertion_failed<false>( \
              boost::mpl::make_assert_arg< \
                  typename boost::mpl::eval_assert<void pred>::type \
                >() \
            ) \
        ) \
    ) \
/**/

// BOOST_MPL_ASSERT_NOT((pred<x,...>))

#define BOOST_MPL_ASSERT_NOT(pred) \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,BOOST_MPL_AUX_PP_COUNTER()) = sizeof( \
          boost::mpl::assertion_failed<false>( \
              boost::mpl::make_assert_arg< \
                  typename boost::mpl::eval_assert_not<void pred>::type \
                >() \
            ) \
        ) \
    ) \
/**/

#else

// BOOST_MPL_ASSERT((pred<x,...>))

#define BOOST_MPL_ASSERT(pred) \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,BOOST_MPL_AUX_PP_COUNTER()) = sizeof( \
          boost::mpl::assertion_failed<false>( \
              boost::mpl::assert_arg( (void (*) pred)0, 1 ) \
            ) \
        ) \
    ) \
/**/

// BOOST_MPL_ASSERT_NOT((pred<x,...>))

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
#   define BOOST_MPL_ASSERT_NOT(pred) \
enum { \
      BOOST_PP_CAT(mpl_assertion_in_line_,BOOST_MPL_AUX_PP_COUNTER()) = sizeof( \
          boost::mpl::assertion<false>::failed( \
              boost::mpl::assert_not_arg( (void (*) pred)0, 1 ) \
            ) \
        ) \
}\
/**/
#else
#   define BOOST_MPL_ASSERT_NOT(pred) \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,BOOST_MPL_AUX_PP_COUNTER()) = sizeof( \
          boost::mpl::assertion_failed<false>( \
              boost::mpl::assert_not_arg( (void (*) pred)0, 1 ) \
            ) \
        ) \
   ) \
/**/
#endif

#endif

// BOOST_MPL_ASSERT_RELATION(x, ==|!=|<=|<|>=|>, y)

#if defined(BOOST_MPL_CFG_ASSERT_USE_RELATION_NAMES)

#   if !defined(BOOST_MPL_CFG_ASSERT_BROKEN_POINTER_TO_POINTER_TO_MEMBER)
// agurt, 9/nov/06: 'enum' below is a workaround for gcc 4.0.4/4.1.1 bugs #29522 and #29518
#   define BOOST_MPL_ASSERT_RELATION_IMPL(counter, x, rel, y)      \
enum { BOOST_PP_CAT(mpl_assert_rel_value,counter) = (x rel y) }; \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,counter) = sizeof( \
        boost::mpl::assertion_failed<BOOST_PP_CAT(mpl_assert_rel_value,counter)>( \
            (boost::mpl::failed ************ ( boost::mpl::assert_relation< \
                  boost::mpl::assert_::relations( sizeof( \
                      boost::mpl::assert_::arg rel boost::mpl::assert_::arg \
                    ) ) \
                , x \
                , y \
                >::************)) 0 ) \
        ) \
    ) \
/**/
#   else
#   define BOOST_MPL_ASSERT_RELATION_IMPL(counter, x, rel, y)    \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assert_rel,counter) = sizeof( \
          boost::mpl::assert_::arg rel boost::mpl::assert_::arg \
        ) \
    ); \
BOOST_MPL_AUX_ASSERT_CONSTANT( bool, BOOST_PP_CAT(mpl_assert_rel_value,counter) = (x rel y) ); \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,counter) = sizeof( \
        boost::mpl::assertion_failed<BOOST_PP_CAT(mpl_assert_rel_value,counter)>( \
              boost::mpl::assert_rel_arg( boost::mpl::assert_relation< \
                  boost::mpl::assert_::relations(BOOST_PP_CAT(mpl_assert_rel,counter)) \
                , x \
                , y \
                >() ) \
            ) \
        ) \
    ) \
/**/
#   endif

#   define BOOST_MPL_ASSERT_RELATION(x, rel, y) \
BOOST_MPL_ASSERT_RELATION_IMPL(BOOST_MPL_AUX_PP_COUNTER(), x, rel, y) \
/**/

#else // !BOOST_MPL_CFG_ASSERT_USE_RELATION_NAMES

#   if defined(BOOST_MPL_CFG_ASSERT_BROKEN_POINTER_TO_POINTER_TO_MEMBER)
#   define BOOST_MPL_ASSERT_RELATION(x, rel, y) \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,BOOST_MPL_AUX_PP_COUNTER()) = sizeof( \
        boost::mpl::assertion_failed<(x rel y)>( boost::mpl::assert_rel_arg( \
              boost::mpl::BOOST_MPL_AUX_ASSERT_RELATION(x,y,(&boost::mpl::operator rel))() \
            ) ) \
        ) \
    ) \
/**/
#   else
#   define BOOST_MPL_ASSERT_RELATION(x, rel, y) \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,BOOST_MPL_AUX_PP_COUNTER()) = sizeof( \
        boost::mpl::assertion_failed<(x rel y)>( (boost::mpl::failed ************ ( \
            boost::mpl::BOOST_MPL_AUX_ASSERT_RELATION(x,y,(&boost::mpl::operator rel))::************))0 ) \
        ) \
    ) \
/**/
#   endif

#endif


// BOOST_MPL_ASSERT_MSG( (pred<x,...>::value), USER_PROVIDED_MESSAGE, (types<x,...>) ) 

#if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3202))
#   define BOOST_MPL_ASSERT_MSG_IMPL( counter, c, msg, types_ ) \
struct msg; \
typedef struct BOOST_PP_CAT(msg,counter) : boost::mpl::assert_ \
{ \
    using boost::mpl::assert_::types; \
    static boost::mpl::failed ************ (msg::************ assert_arg()) types_ \
    { return 0; } \
} BOOST_PP_CAT(mpl_assert_arg,counter); \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,counter) = sizeof( \
        boost::mpl::assertion<(c)>::failed( BOOST_PP_CAT(mpl_assert_arg,counter)::assert_arg() ) \
        ) \
    ) \
/**/
#else
#   define BOOST_MPL_ASSERT_MSG_IMPL( counter, c, msg, types_ )  \
struct msg; \
typedef struct BOOST_PP_CAT(msg,counter) : boost::mpl::assert_ \
{ \
    static boost::mpl::failed ************ (msg::************ assert_arg()) types_ \
    { return 0; } \
} BOOST_PP_CAT(mpl_assert_arg,counter); \
BOOST_MPL_AUX_ASSERT_CONSTANT( \
      std::size_t \
    , BOOST_PP_CAT(mpl_assertion_in_line_,counter) = sizeof( \
        boost::mpl::assertion_failed<(c)>( BOOST_PP_CAT(mpl_assert_arg,counter)::assert_arg() ) \
        ) \
    ) \
/**/
#endif

#if 0
// Work around BOOST_MPL_ASSERT_MSG_IMPL generating multiple definition linker errors on VC++8.
// #if defined(BOOST_MSVC) && BOOST_MSVC < 1500
#   include <boost/static_assert.hpp>
#   define BOOST_MPL_ASSERT_MSG( c, msg, types_ ) \
BOOST_STATIC_ASSERT_MSG( c, #msg ) \
/**/
#else
#   define BOOST_MPL_ASSERT_MSG( c, msg, types_ ) \
BOOST_MPL_ASSERT_MSG_IMPL( BOOST_MPL_AUX_PP_COUNTER(), c, msg, types_ ) \
/**/
#endif

#endif // BOOST_MPL_ASSERT_HPP_INCLUDED
