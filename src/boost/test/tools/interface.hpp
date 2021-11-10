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
//  Description : contains definition for all test tools in test toolbox
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_INTERFACE_HPP_111712GER
#define BOOST_TEST_TOOLS_INTERFACE_HPP_111712GER

// Boost.Test
#include <boost/test/unit_test_log.hpp>
#ifdef BOOST_TEST_TOOLS_DEBUGGABLE
#include <boost/test/debug.hpp>
#endif

#include <boost/test/detail/pp_variadic.hpp>

#ifdef BOOST_TEST_NO_OLD_TOOLS
#include <boost/preprocessor/seq/to_tuple.hpp>

#include <iterator>
#endif // BOOST_TEST_NO_OLD_TOOLS

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

// ************************************************************************** //
// **************               BOOST_TEST_<level>             ************** //
// ************************************************************************** //

#define BOOST_TEST_BUILD_ASSERTION( P )             \
    (::boost::test_tools::assertion::seed()->*P)    \
/**/

//____________________________________________________________________________//

// Implementation based on direct predicate evaluation
#define BOOST_TEST_TOOL_DIRECT_IMPL( P, level, M )              \
do {                                                            \
    ::boost::test_tools::assertion_result res = (P);            \
    report_assertion(                                           \
        res,                                                    \
        BOOST_TEST_LAZY_MSG( M ),                               \
        BOOST_TEST_L(__FILE__),                                 \
        static_cast<std::size_t>(__LINE__),                     \
        ::boost::test_tools::tt_detail::level,                  \
        ::boost::test_tools::tt_detail::CHECK_MSG,              \
        0 );                                                    \
} while( ::boost::test_tools::tt_detail::dummy_cond() )         \
/**/

//____________________________________________________________________________//

// Implementation based on expression template construction
#define BOOST_TEST_TOOL_ET_IMPL( P, level )                     \
do {                                                            \
    BOOST_TEST_PASSPOINT();                                     \
                                                                \
    ::boost::test_tools::tt_detail::                            \
    report_assertion(                                           \
      BOOST_TEST_BUILD_ASSERTION( P ).evaluate(),               \
      BOOST_TEST_LAZY_MSG( BOOST_TEST_STRINGIZE( P ) ),         \
      BOOST_TEST_L(__FILE__),                                   \
      static_cast<std::size_t>(__LINE__),                       \
      ::boost::test_tools::tt_detail::level,                    \
      ::boost::test_tools::tt_detail::CHECK_BUILT_ASSERTION,    \
      0 );                                                      \
} while( ::boost::test_tools::tt_detail::dummy_cond() )         \
/**/

//____________________________________________________________________________//

// Implementation based on expression template construction with extra tool arguments
#define BOOST_TEST_TOOL_ET_IMPL_EX( P, level, arg )             \
do {                                                            \
    BOOST_TEST_PASSPOINT();                                     \
                                                                \
    ::boost::test_tools::tt_detail::                            \
    report_assertion(                                           \
      ::boost::test_tools::tt_detail::assertion_evaluate(       \
        BOOST_TEST_BUILD_ASSERTION( P ) )                       \
          << arg,                                               \
      ::boost::test_tools::tt_detail::assertion_text(           \
          BOOST_TEST_LAZY_MSG( BOOST_TEST_STRINGIZE(P) ),       \
          BOOST_TEST_LAZY_MSG( arg ) ),                         \
      BOOST_TEST_L(__FILE__),                                   \
      static_cast<std::size_t>(__LINE__),                       \
      ::boost::test_tools::tt_detail::level,                    \
      ::boost::test_tools::tt_detail::assertion_type()          \
          << arg,                                               \
      0 );                                                      \
} while( ::boost::test_tools::tt_detail::dummy_cond() )         \
/**/

//____________________________________________________________________________//

#ifdef BOOST_TEST_TOOLS_UNDER_DEBUGGER

#define BOOST_TEST_TOOL_UNIV( level, P )                                    \
    BOOST_TEST_TOOL_DIRECT_IMPL( P, level, BOOST_TEST_STRINGIZE( P ) )      \
/**/

#define BOOST_TEST_TOOL_UNIV_EX( level, P, ... )                            \
    BOOST_TEST_TOOL_UNIV( level, P )                                        \
/**/

#elif defined(BOOST_TEST_TOOLS_DEBUGGABLE)

#define BOOST_TEST_TOOL_UNIV( level, P )                                    \
do {                                                                        \
    if( ::boost::debug::under_debugger() )                                  \
        BOOST_TEST_TOOL_DIRECT_IMPL( P, level, BOOST_TEST_STRINGIZE( P ) ); \
    else                                                                    \
        BOOST_TEST_TOOL_ET_IMPL( P, level );                                \
} while( ::boost::test_tools::tt_detail::dummy_cond() )                     \
/**/

#define BOOST_TEST_TOOL_UNIV_EX( level, P, ... )                            \
    BOOST_TEST_TOOL_UNIV( level, P )                                        \
/**/

#else

#define BOOST_TEST_TOOL_UNIV( level, P )                                    \
    BOOST_TEST_TOOL_ET_IMPL( P, level )                                     \
/**/

#define BOOST_TEST_TOOL_UNIV_EX( level, P, ... )                            \
    BOOST_TEST_TOOL_ET_IMPL_EX( P, level, __VA_ARGS__ )                     \
/**/

#endif

//____________________________________________________________________________//

#define BOOST_TEST_WARN( ... )              BOOST_TEST_INVOKE_IF_N_ARGS(    \
    2, BOOST_TEST_TOOL_UNIV, BOOST_TEST_TOOL_UNIV_EX, WARN, __VA_ARGS__ )   \
/**/
#define BOOST_TEST_CHECK( ... )             BOOST_TEST_INVOKE_IF_N_ARGS(    \
    2, BOOST_TEST_TOOL_UNIV, BOOST_TEST_TOOL_UNIV_EX, CHECK, __VA_ARGS__ )  \
/**/
#define BOOST_TEST_REQUIRE( ... )           BOOST_TEST_INVOKE_IF_N_ARGS(    \
    2, BOOST_TEST_TOOL_UNIV, BOOST_TEST_TOOL_UNIV_EX, REQUIRE, __VA_ARGS__ )\
/**/

#define BOOST_TEST( ... )                   BOOST_TEST_INVOKE_IF_N_ARGS(    \
    2, BOOST_TEST_TOOL_UNIV, BOOST_TEST_TOOL_UNIV_EX, CHECK, __VA_ARGS__ )  \
/**/

//____________________________________________________________________________//

#define BOOST_TEST_ERROR( M )               BOOST_CHECK_MESSAGE( false, M )
#define BOOST_TEST_FAIL( M )                BOOST_REQUIRE_MESSAGE( false, M )

//____________________________________________________________________________//

#define BOOST_TEST_IS_DEFINED( symb ) ::boost::test_tools::tt_detail::is_defined_impl( symb, BOOST_STRINGIZE(= symb) )

//____________________________________________________________________________//

#ifdef BOOST_TEST_NO_OLD_TOOLS

#ifdef BOOST_TEST_TOOLS_UNDER_DEBUGGER

#define BOOST_CHECK_THROW_IMPL(S, E, TL, Ppassed, Mpassed, Pcaught, Mcaught)\
do { try {                                                                  \
    S;                                                                      \
    BOOST_TEST_TOOL_DIRECT_IMPL( Ppassed, TL, Mpassed );                    \
} catch( E ) {                                                              \
    BOOST_TEST_TOOL_DIRECT_IMPL( Pcaught, TL, Mcaught );                    \
}} while( ::boost::test_tools::tt_detail::dummy_cond() )                    \
/**/

#elif defined(BOOST_TEST_TOOLS_DEBUGGABLE)

#define BOOST_CHECK_THROW_IMPL(S, E, TL, Ppassed, Mpassed, Pcaught, Mcaught)\
do { try {                                                                  \
    if( ::boost::debug::under_debugger() )                                  \
        BOOST_TEST_PASSPOINT();                                             \
    S;                                                                      \
    BOOST_TEST_TOOL_DIRECT_IMPL( Ppassed, TL, Mpassed );                    \
} catch( E ) {                                                              \
    BOOST_TEST_TOOL_DIRECT_IMPL( Pcaught, TL, Mcaught );                    \
}} while( ::boost::test_tools::tt_detail::dummy_cond() )                    \
/**/

#else

#define BOOST_CHECK_THROW_IMPL(S, E, TL, Ppassed, Mpassed, Pcaught, Mcaught)\
do { try {                                                                  \
    BOOST_TEST_PASSPOINT();                                                 \
    S;                                                                      \
    BOOST_TEST_TOOL_DIRECT_IMPL( Ppassed, TL, Mpassed );                    \
} catch( E ) {                                                              \
    BOOST_TEST_TOOL_DIRECT_IMPL( Pcaught, TL, Mcaught );                    \
}} while( ::boost::test_tools::tt_detail::dummy_cond() )                    \
/**/

#endif

//____________________________________________________________________________//

#define BOOST_WARN_THROW( S, E )                                            \
    BOOST_CHECK_THROW_IMPL(S, E const&, WARN,                               \
            false, "exception " BOOST_STRINGIZE(E) " is expected",          \
            true , "exception " BOOST_STRINGIZE(E) " is caught" )           \
/**/
#define BOOST_CHECK_THROW( S, E )                                           \
    BOOST_CHECK_THROW_IMPL(S, E const&, CHECK,                              \
            false, "exception " BOOST_STRINGIZE(E) " is expected",          \
            true , "exception " BOOST_STRINGIZE(E) " is caught" )           \
/**/
#define BOOST_REQUIRE_THROW( S, E )                                         \
    BOOST_CHECK_THROW_IMPL(S, E const&, REQUIRE,                            \
            false, "exception " BOOST_STRINGIZE(E) " is expected",          \
            true , "exception " BOOST_STRINGIZE(E) " is caught" )           \
/**/

//____________________________________________________________________________//

#define BOOST_WARN_EXCEPTION( S, E, P )                                     \
    BOOST_CHECK_THROW_IMPL(S, E const& ex, WARN,                            \
            false, "exception " BOOST_STRINGIZE(E) " is expected",          \
            P(ex), "incorrect exception " BOOST_STRINGIZE(E) " is caught" ) \
/**/
#define BOOST_CHECK_EXCEPTION( S, E, P )                                    \
    BOOST_CHECK_THROW_IMPL(S, E const& ex, CHECK,                           \
            false, "exception " BOOST_STRINGIZE(E) " is expected",          \
            P(ex), "incorrect exception " BOOST_STRINGIZE(E) " is caught" ) \
/**/
#define BOOST_REQUIRE_EXCEPTION( S, E, P )                                  \
    BOOST_CHECK_THROW_IMPL(S, E const& ex, REQUIRE,                         \
            false, "exception " BOOST_STRINGIZE(E) " is expected",          \
            P(ex), "incorrect exception " BOOST_STRINGIZE(E) " is caught" ) \
/**/

//____________________________________________________________________________//

#define BOOST_WARN_NO_THROW( S )                                            \
    BOOST_CHECK_THROW_IMPL(S, ..., WARN,                                    \
            true , "no exceptions thrown by " BOOST_STRINGIZE( S ),         \
            false, "exception thrown by " BOOST_STRINGIZE( S ) )            \
/**/
#define BOOST_CHECK_NO_THROW( S )                                           \
    BOOST_CHECK_THROW_IMPL(S, ..., CHECK,                                   \
            true , "no exceptions thrown by " BOOST_STRINGIZE( S ),         \
            false, "exception thrown by " BOOST_STRINGIZE( S ) )            \
/**/
#define BOOST_REQUIRE_NO_THROW( S )                                         \
    BOOST_CHECK_THROW_IMPL(S, ..., REQUIRE,                                 \
            true , "no exceptions thrown by " BOOST_STRINGIZE( S ),         \
            false, "exception thrown by " BOOST_STRINGIZE( S ) )            \
/**/

//____________________________________________________________________________//

#define BOOST_WARN_MESSAGE( P, M )          BOOST_TEST_TOOL_DIRECT_IMPL( P, WARN, M )
#define BOOST_CHECK_MESSAGE( P, M )         BOOST_TEST_TOOL_DIRECT_IMPL( P, CHECK, M )
#define BOOST_REQUIRE_MESSAGE( P, M )       BOOST_TEST_TOOL_DIRECT_IMPL( P, REQUIRE, M )

//____________________________________________________________________________//

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////   DEPRECATED TOOLS   /////////////////////////////

#define BOOST_WARN( P )                     BOOST_TEST_WARN( P )
#define BOOST_CHECK( P )                    BOOST_TEST_CHECK( P )
#define BOOST_REQUIRE( P )                  BOOST_TEST_REQUIRE( P )

//____________________________________________________________________________//

#define BOOST_ERROR( M )                    BOOST_TEST_ERROR( M )
#define BOOST_FAIL( M )                     BOOST_TEST_FAIL( M )

//____________________________________________________________________________//

#define BOOST_WARN_EQUAL( L, R )            BOOST_TEST_WARN( L == R )
#define BOOST_CHECK_EQUAL( L, R )           BOOST_TEST_CHECK( L == R )
#define BOOST_REQUIRE_EQUAL( L, R )         BOOST_TEST_REQUIRE( L == R )

#define BOOST_WARN_NE( L, R )               BOOST_TEST_WARN( L != R )
#define BOOST_CHECK_NE( L, R )              BOOST_TEST_CHECK( L != R )
#define BOOST_REQUIRE_NE( L, R )            BOOST_TEST_REQUIRE( L != R )

#define BOOST_WARN_LT( L, R )               BOOST_TEST_WARN( L < R )
#define BOOST_CHECK_LT( L, R )              BOOST_TEST_CHECK( L < R )
#define BOOST_REQUIRE_LT( L, R )            BOOST_TEST_REQUIRE( L < R )

#define BOOST_WARN_LE( L, R )               BOOST_TEST_WARN( L <= R )
#define BOOST_CHECK_LE( L, R )              BOOST_TEST_CHECK( L <= R )
#define BOOST_REQUIRE_LE( L, R )            BOOST_TEST_REQUIRE( L <= R )

#define BOOST_WARN_GT( L, R )               BOOST_TEST_WARN( L > R )
#define BOOST_CHECK_GT( L, R )              BOOST_TEST_CHECK( L > R )
#define BOOST_REQUIRE_GT( L, R )            BOOST_TEST_REQUIRE( L > R )

#define BOOST_WARN_GE( L, R )               BOOST_TEST_WARN( L >= R )
#define BOOST_CHECK_GE( L, R )              BOOST_TEST_CHECK( L >= R )
#define BOOST_REQUIRE_GE( L, R )            BOOST_TEST_REQUIRE( L >= R )

//____________________________________________________________________________//

#define BOOST_WARN_CLOSE( L, R, T )         BOOST_TEST_WARN( L == R, T % ::boost::test_tools::tolerance() )
#define BOOST_CHECK_CLOSE( L, R, T )        BOOST_TEST_CHECK( L == R, T % ::boost::test_tools::tolerance() )
#define BOOST_REQUIRE_CLOSE( L, R, T )      BOOST_TEST_REQUIRE( L == R, T % ::boost::test_tools::tolerance() )

#define BOOST_WARN_CLOSE_FRACTION(L, R, T)  BOOST_TEST_WARN( L == R, ::boost::test_tools::tolerance( T ) )
#define BOOST_CHECK_CLOSE_FRACTION(L, R, T) BOOST_TEST_CHECK( L == R, ::boost::test_tools::tolerance( T ) )
#define BOOST_REQUIRE_CLOSE_FRACTION(L,R,T) BOOST_TEST_REQUIRE( L == R, ::boost::test_tools::tolerance( T ) )

#define BOOST_WARN_SMALL( FPV, T )          BOOST_TEST_WARN( FPV == 0., ::boost::test_tools::tolerance( T ) )
#define BOOST_CHECK_SMALL( FPV, T )         BOOST_TEST_CHECK( FPV == 0., ::boost::test_tools::tolerance( T ) )
#define BOOST_REQUIRE_SMALL( FPV, T )       BOOST_TEST_REQUIRE( FPV == 0., ::boost::test_tools::tolerance( T ) )

//____________________________________________________________________________//

#define BOOST_WARN_EQUAL_COLLECTIONS( L_begin, L_end, R_begin, R_end )              \
    BOOST_TEST_WARN( ::boost::test_tools::tt_detail::make_it_pair(L_begin, L_end) ==\
                     ::boost::test_tools::tt_detail::make_it_pair(R_begin, R_end),  \
                     ::boost::test_tools::per_element() )                           \
/**/

#define BOOST_CHECK_EQUAL_COLLECTIONS( L_begin, L_end, R_begin, R_end )              \
    BOOST_TEST_CHECK( ::boost::test_tools::tt_detail::make_it_pair(L_begin, L_end) ==\
                      ::boost::test_tools::tt_detail::make_it_pair(R_begin, R_end),  \
                      ::boost::test_tools::per_element() )                           \
/**/

#define BOOST_REQUIRE_EQUAL_COLLECTIONS( L_begin, L_end, R_begin, R_end )              \
    BOOST_TEST_REQUIRE( ::boost::test_tools::tt_detail::make_it_pair(L_begin, L_end) ==\
                        ::boost::test_tools::tt_detail::make_it_pair(R_begin, R_end),  \
                        ::boost::test_tools::per_element() )                           \
/**/

//____________________________________________________________________________//

#define BOOST_WARN_BITWISE_EQUAL( L, R )    BOOST_TEST_WARN( L == R, ::boost::test_tools::bitwise() )
#define BOOST_CHECK_BITWISE_EQUAL( L, R )   BOOST_TEST_CHECK( L == R, ::boost::test_tools::bitwise() )
#define BOOST_REQUIRE_BITWISE_EQUAL( L, R ) BOOST_TEST_REQUIRE( L == R, ::boost::test_tools::bitwise() )

//____________________________________________________________________________//

#define BOOST_WARN_PREDICATE( P, ARGS )     BOOST_TEST_WARN( P BOOST_PP_SEQ_TO_TUPLE(ARGS) )
#define BOOST_CHECK_PREDICATE( P, ARGS )    BOOST_TEST_CHECK( P BOOST_PP_SEQ_TO_TUPLE(ARGS) )
#define BOOST_REQUIRE_PREDICATE( P, ARGS )  BOOST_TEST_REQUIRE( P BOOST_PP_SEQ_TO_TUPLE(ARGS) )

//____________________________________________________________________________//

#define BOOST_IS_DEFINED( symb ) ::boost::test_tools::tt_detail::is_defined_impl( #symb, BOOST_STRINGIZE(= symb) )

//____________________________________________________________________________//

#endif // BOOST_TEST_NO_OLD_TOOLS

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_INTERFACE_HPP_111712GER
