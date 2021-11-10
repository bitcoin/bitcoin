//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 81247 $
//
//  Description : contains definition for all test tools in old test toolbox
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_OLD_INTERFACE_HPP_111712GER
#define BOOST_TEST_TOOLS_OLD_INTERFACE_HPP_111712GER

// Boost
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>

#include <boost/core/ignore_unused.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

// ************************************************************************** //
// **************                    TOOL BOX                  ************** //
// ************************************************************************** //

// In macros below following argument abbreviations are used:
// P - predicate
// M - message
// S - statement
// E - exception
// L - left argument
// R - right argument
// TL - tool level
// CT - check type
// ARGS - arguments list (as PP sequence)

// frwd_type:
// 0 - args exists and need to be forwarded; call check_frwd
// 1 - args exists, but do not need to be forwarded; call report_assertion directly
// 2 - no arguments; call report_assertion directly

#define BOOST_TEST_TOOL_PASS_PRED0( P, ARGS ) P
#define BOOST_TEST_TOOL_PASS_PRED1( P, ARGS ) P BOOST_PP_SEQ_TO_TUPLE(ARGS)
#define BOOST_TEST_TOOL_PASS_PRED2( P, ARGS ) P

#define BOOST_TEST_TOOL_PASS_ARG( r, _, arg ) , arg, BOOST_STRINGIZE( arg )
#define BOOST_TEST_TOOL_PASS_ARG_DSCR( r, _, arg ) , BOOST_STRINGIZE( arg )

#define BOOST_TEST_TOOL_PASS_ARGS0( ARGS ) \
    BOOST_PP_SEQ_FOR_EACH( BOOST_TEST_TOOL_PASS_ARG, _, ARGS )
#define BOOST_TEST_TOOL_PASS_ARGS1( ARGS ) \
    , BOOST_PP_SEQ_SIZE(ARGS) BOOST_PP_SEQ_FOR_EACH( BOOST_TEST_TOOL_PASS_ARG_DSCR, _, ARGS )
#define BOOST_TEST_TOOL_PASS_ARGS2( ARGS ) \
    , 0

#define BOOST_TEST_TOOL_IMPL( frwd_type, P, assertion_descr, TL, CT, ARGS )     \
do {                                                                            \
    BOOST_TEST_PASSPOINT();                                                     \
    ::boost::test_tools::tt_detail::                                            \
    BOOST_PP_IF( frwd_type, report_assertion, check_frwd ) (                    \
        BOOST_JOIN( BOOST_TEST_TOOL_PASS_PRED, frwd_type )( P, ARGS ),          \
        BOOST_TEST_LAZY_MSG( assertion_descr ),                                 \
        BOOST_TEST_L(__FILE__),                                                 \
        static_cast<std::size_t>(__LINE__),                                     \
        ::boost::test_tools::tt_detail::TL,                                     \
        ::boost::test_tools::tt_detail::CT                                      \
        BOOST_JOIN( BOOST_TEST_TOOL_PASS_ARGS, frwd_type )( ARGS ) );           \
} while( ::boost::test_tools::tt_detail::dummy_cond() )                         \
/**/

//____________________________________________________________________________//

#define BOOST_WARN( P )                     BOOST_TEST_TOOL_IMPL( 2, \
    (P), BOOST_TEST_STRINGIZE( P ), WARN, CHECK_PRED, _ )
#define BOOST_CHECK( P )                    BOOST_TEST_TOOL_IMPL( 2, \
    (P), BOOST_TEST_STRINGIZE( P ), CHECK, CHECK_PRED, _ )
#define BOOST_REQUIRE( P )                  BOOST_TEST_TOOL_IMPL( 2, \
    (P), BOOST_TEST_STRINGIZE( P ), REQUIRE, CHECK_PRED, _ )

//____________________________________________________________________________//

#define BOOST_WARN_MESSAGE( P, M )          BOOST_TEST_TOOL_IMPL( 2, (P), M, WARN, CHECK_MSG, _ )
#define BOOST_CHECK_MESSAGE( P, M )         BOOST_TEST_TOOL_IMPL( 2, (P), M, CHECK, CHECK_MSG, _ )
#define BOOST_REQUIRE_MESSAGE( P, M )       BOOST_TEST_TOOL_IMPL( 2, (P), M, REQUIRE, CHECK_MSG, _ )

//____________________________________________________________________________//

#define BOOST_ERROR( M )                    BOOST_CHECK_MESSAGE( false, M )
#define BOOST_FAIL( M )                     BOOST_REQUIRE_MESSAGE( false, M )

//____________________________________________________________________________//

#define BOOST_CHECK_THROW_IMPL( S, E, P, postfix, TL )                                  \
do {                                                                                    \
    try {                                                                               \
        BOOST_TEST_PASSPOINT();                                                         \
        S;                                                                              \
        BOOST_TEST_TOOL_IMPL( 2, false, "exception " BOOST_STRINGIZE(E) " expected but not raised", \
                              TL, CHECK_MSG, _ );                                       \
    } catch( E const& ex ) {                                                            \
        boost::ignore_unused( ex );                                                     \
        BOOST_TEST_TOOL_IMPL( 2, P,                                                     \
                              "exception \"" BOOST_STRINGIZE( E )"\" raised as expected" postfix,   \
                              TL, CHECK_MSG, _  );                                      \
    }                                                                                   \
} while( ::boost::test_tools::tt_detail::dummy_cond() )                                 \
/**/

//____________________________________________________________________________//

#define BOOST_WARN_THROW( S, E )            BOOST_CHECK_THROW_IMPL( S, E, true, "", WARN )
#define BOOST_CHECK_THROW( S, E )           BOOST_CHECK_THROW_IMPL( S, E, true, "", CHECK )
#define BOOST_REQUIRE_THROW( S, E )         BOOST_CHECK_THROW_IMPL( S, E, true, "", REQUIRE )

//____________________________________________________________________________//

#define BOOST_WARN_EXCEPTION( S, E, P )     BOOST_CHECK_THROW_IMPL( S, E, P( ex ), \
              ": validation on the raised exception through predicate \"" BOOST_STRINGIZE(P) "\"", WARN )
#define BOOST_CHECK_EXCEPTION( S, E, P )    BOOST_CHECK_THROW_IMPL( S, E, P( ex ), \
              ": validation on the raised exception through predicate \"" BOOST_STRINGIZE(P) "\"", CHECK )
#define BOOST_REQUIRE_EXCEPTION( S, E, P )  BOOST_CHECK_THROW_IMPL( S, E, P( ex ), \
              ": validation on the raised exception through predicate \"" BOOST_STRINGIZE(P) "\"", REQUIRE )

//____________________________________________________________________________//

#define BOOST_CHECK_NO_THROW_IMPL( S, TL )                                              \
do {                                                                                    \
    try {                                                                               \
        S;                                                                              \
        BOOST_TEST_TOOL_IMPL( 2, true, "no exceptions thrown by " BOOST_STRINGIZE( S ), \
                              TL, CHECK_MSG, _ );                                       \
    } catch( ... ) {                                                                    \
        BOOST_TEST_TOOL_IMPL( 2, false, "unexpected exception thrown by " BOOST_STRINGIZE( S ),    \
                              TL, CHECK_MSG, _ );                                       \
    }                                                                                   \
} while( ::boost::test_tools::tt_detail::dummy_cond() )                                 \
/**/

#define BOOST_WARN_NO_THROW( S )            BOOST_CHECK_NO_THROW_IMPL( S, WARN )
#define BOOST_CHECK_NO_THROW( S )           BOOST_CHECK_NO_THROW_IMPL( S, CHECK )
#define BOOST_REQUIRE_NO_THROW( S )         BOOST_CHECK_NO_THROW_IMPL( S, REQUIRE )

//____________________________________________________________________________//

#define BOOST_WARN_EQUAL( L, R )            BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::equal_impl_frwd(), "", WARN, CHECK_EQUAL, (L)(R) )
#define BOOST_CHECK_EQUAL( L, R )           BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::equal_impl_frwd(), "", CHECK, CHECK_EQUAL, (L)(R) )
#define BOOST_REQUIRE_EQUAL( L, R )         BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::equal_impl_frwd(), "", REQUIRE, CHECK_EQUAL, (L)(R) )

//____________________________________________________________________________//

#define BOOST_WARN_NE( L, R )               BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::ne_impl(), "", WARN, CHECK_NE, (L)(R) )
#define BOOST_CHECK_NE( L, R )              BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::ne_impl(), "", CHECK, CHECK_NE, (L)(R) )
#define BOOST_REQUIRE_NE( L, R )            BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::ne_impl(), "", REQUIRE, CHECK_NE, (L)(R) )

//____________________________________________________________________________//

#define BOOST_WARN_LT( L, R )               BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::lt_impl(), "", WARN, CHECK_LT, (L)(R) )
#define BOOST_CHECK_LT( L, R )              BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::lt_impl(), "", CHECK, CHECK_LT, (L)(R) )
#define BOOST_REQUIRE_LT( L, R )            BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::lt_impl(), "", REQUIRE, CHECK_LT, (L)(R) )

//____________________________________________________________________________//

#define BOOST_WARN_LE( L, R )               BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::le_impl(), "", WARN, CHECK_LE, (L)(R) )
#define BOOST_CHECK_LE( L, R )              BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::le_impl(), "", CHECK, CHECK_LE, (L)(R) )
#define BOOST_REQUIRE_LE( L, R )            BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::le_impl(), "", REQUIRE, CHECK_LE, (L)(R) )

//____________________________________________________________________________//

#define BOOST_WARN_GT( L, R )               BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::gt_impl(), "", WARN, CHECK_GT, (L)(R) )
#define BOOST_CHECK_GT( L, R )              BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::gt_impl(), "", CHECK, CHECK_GT, (L)(R) )
#define BOOST_REQUIRE_GT( L, R )            BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::gt_impl(), "", REQUIRE, CHECK_GT, (L)(R) )

//____________________________________________________________________________//

#define BOOST_WARN_GE( L, R )               BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::ge_impl(), "", WARN, CHECK_GE, (L)(R) )
#define BOOST_CHECK_GE( L, R )              BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::ge_impl(), "", CHECK, CHECK_GE, (L)(R) )
#define BOOST_REQUIRE_GE( L, R )            BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::tt_detail::ge_impl(), "", REQUIRE, CHECK_GE, (L)(R) )

//____________________________________________________________________________//

#define BOOST_WARN_CLOSE( L, R, T )         BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::check_is_close_t(), "", WARN, CHECK_CLOSE, (L)(R)(::boost::math::fpc::percent_tolerance(T)) )
#define BOOST_CHECK_CLOSE( L, R, T )        BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::check_is_close_t(), "", CHECK, CHECK_CLOSE, (L)(R)(::boost::math::fpc::percent_tolerance(T)) )
#define BOOST_REQUIRE_CLOSE( L, R, T )      BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::check_is_close_t(), "", REQUIRE, CHECK_CLOSE, (L)(R)(::boost::math::fpc::percent_tolerance(T)) )

//____________________________________________________________________________//

#define BOOST_WARN_CLOSE_FRACTION(L, R, T)  BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::check_is_close_t(), "", WARN, CHECK_CLOSE_FRACTION, (L)(R)(T) )
#define BOOST_CHECK_CLOSE_FRACTION(L, R, T) BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::check_is_close_t(), "", CHECK, CHECK_CLOSE_FRACTION, (L)(R)(T) )
#define BOOST_REQUIRE_CLOSE_FRACTION(L,R,T) BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::check_is_close_t(), "", REQUIRE, CHECK_CLOSE_FRACTION, (L)(R)(T) )

//____________________________________________________________________________//

#define BOOST_WARN_SMALL( FPV, T )          BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::check_is_small_t(), "", WARN, CHECK_SMALL, (FPV)(T) )
#define BOOST_CHECK_SMALL( FPV, T )         BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::check_is_small_t(), "", CHECK, CHECK_SMALL, (FPV)(T) )
#define BOOST_REQUIRE_SMALL( FPV, T )       BOOST_TEST_TOOL_IMPL( 0, \
    ::boost::test_tools::check_is_small_t(), "", REQUIRE, CHECK_SMALL, (FPV)(T) )

//____________________________________________________________________________//

#define BOOST_WARN_PREDICATE( P, ARGS )     BOOST_TEST_TOOL_IMPL( 0, \
    P, BOOST_TEST_STRINGIZE( P ), WARN, CHECK_PRED_WITH_ARGS, ARGS )
#define BOOST_CHECK_PREDICATE( P, ARGS )    BOOST_TEST_TOOL_IMPL( 0, \
    P, BOOST_TEST_STRINGIZE( P ), CHECK, CHECK_PRED_WITH_ARGS, ARGS )
#define BOOST_REQUIRE_PREDICATE( P, ARGS )  BOOST_TEST_TOOL_IMPL( 0, \
    P, BOOST_TEST_STRINGIZE( P ), REQUIRE, CHECK_PRED_WITH_ARGS, ARGS )

//____________________________________________________________________________//

#define BOOST_WARN_EQUAL_COLLECTIONS( L_begin, L_end, R_begin, R_end )                  \
    BOOST_TEST_TOOL_IMPL( 1, ::boost::test_tools::tt_detail::equal_coll_impl(),         \
        "", WARN, CHECK_EQUAL_COLL, (L_begin)(L_end)(R_begin)(R_end) )                  \
/**/
#define BOOST_CHECK_EQUAL_COLLECTIONS( L_begin, L_end, R_begin, R_end )                 \
    BOOST_TEST_TOOL_IMPL( 1, ::boost::test_tools::tt_detail::equal_coll_impl(),         \
        "", CHECK, CHECK_EQUAL_COLL, (L_begin)(L_end)(R_begin)(R_end) )                 \
/**/
#define BOOST_REQUIRE_EQUAL_COLLECTIONS( L_begin, L_end, R_begin, R_end )               \
    BOOST_TEST_TOOL_IMPL( 1, ::boost::test_tools::tt_detail::equal_coll_impl(),         \
        "", REQUIRE, CHECK_EQUAL_COLL, (L_begin)(L_end)(R_begin)(R_end) )               \
/**/

//____________________________________________________________________________//

#define BOOST_WARN_BITWISE_EQUAL( L, R )    BOOST_TEST_TOOL_IMPL( 1, \
    ::boost::test_tools::tt_detail::bitwise_equal_impl(), "", WARN, CHECK_BITWISE_EQUAL, (L)(R) )
#define BOOST_CHECK_BITWISE_EQUAL( L, R )   BOOST_TEST_TOOL_IMPL( 1, \
    ::boost::test_tools::tt_detail::bitwise_equal_impl(), "", CHECK, CHECK_BITWISE_EQUAL, (L)(R) )
#define BOOST_REQUIRE_BITWISE_EQUAL( L, R ) BOOST_TEST_TOOL_IMPL( 1, \
    ::boost::test_tools::tt_detail::bitwise_equal_impl(), "", REQUIRE, CHECK_BITWISE_EQUAL, (L)(R) )

//____________________________________________________________________________//

#define BOOST_IS_DEFINED( symb ) ::boost::test_tools::tt_detail::is_defined_impl( #symb, BOOST_STRINGIZE(= symb) )

//____________________________________________________________________________//

#ifdef BOOST_TEST_NO_NEW_TOOLS

#define BOOST_TEST_WARN( P )                BOOST_WARN( P )
#define BOOST_TEST_CHECK( P )               BOOST_CHECK( P )
#define BOOST_TEST_REQUIRE( P )             BOOST_REQUIRE( P )

#define BOOST_TEST( P )                     BOOST_CHECK( P )

#endif

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_OLD_INTERFACE_HPP_111712GER
