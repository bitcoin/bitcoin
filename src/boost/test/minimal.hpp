//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief Deprecated implementation of simple minimal testing
/// @deprecated
/// To convert to Unit Test Framework simply rewrite:
/// @code
/// #include <boost/test/minimal.hpp>
///
/// int test_main( int, char *[] )
/// {
///   ...
/// }
/// @endcode
/// as
/// @code
/// #include <boost/test/included/unit_test.hpp>
///
/// BOOST_AUTO_TEST_CASE(test_main)
/// {
///   ...
/// }
/// @endcode
// ***************************************************************************

#ifndef BOOST_TEST_MINIMAL_HPP_071894GER
#define BOOST_TEST_MINIMAL_HPP_071894GER

#include <boost/config/header_deprecated.hpp>
BOOST_HEADER_DEPRECATED( "<boost/test/included/unit_test.hpp>" )
#if defined(BOOST_ALLOW_DEPRECATED_HEADERS)
BOOST_PRAGMA_MESSAGE( "Boost.Test minimal is deprecated. Please convert to the header only variant of Boost.Test." )
#endif

#define BOOST_CHECK(exp)       \
  ( (exp)                      \
      ? static_cast<void>(0)   \
      : boost::minimal_test::report_error(#exp,__FILE__,__LINE__, BOOST_CURRENT_FUNCTION) )

#define BOOST_REQUIRE(exp)     \
  ( (exp)                      \
      ? static_cast<void>(0)   \
      : boost::minimal_test::report_critical_error(#exp,__FILE__,__LINE__,BOOST_CURRENT_FUNCTION))

#define BOOST_ERROR( msg_ )    \
        boost::minimal_test::report_error( (msg_),__FILE__,__LINE__, BOOST_CURRENT_FUNCTION, true )
#define BOOST_FAIL( msg_ )     \
        boost::minimal_test::report_critical_error( (msg_),__FILE__,__LINE__, BOOST_CURRENT_FUNCTION, true )

//____________________________________________________________________________//

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/impl/execution_monitor.ipp>
#include <boost/test/impl/debug.ipp>
#include <boost/test/utils/class_properties.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>

// Boost
#include <boost/cstdlib.hpp>            // for exit codes
#include <boost/current_function.hpp>   // for BOOST_CURRENT_FUNCTION

// STL
#include <iostream>                     // std::cerr, std::endl
#include <string>                       // std::string

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

int test_main( int argc, char* argv[] );  // prototype for users test_main()

namespace boost {
namespace minimal_test {

typedef boost::unit_test::const_string const_string;

inline unit_test::counter_t& errors_counter() { static unit_test::counter_t ec = 0; return ec; }

inline void
report_error( const char* msg, const char* file, int line, const_string func_name, bool is_msg = false )
{
    ++errors_counter();
    std::cerr << file << "(" << line << "): ";

    if( is_msg )
        std::cerr << msg;
    else
        std::cerr << "test " << msg << " failed";

    if( func_name != "(unknown)" )
        std::cerr << " in function: '" << func_name << "'";

    std::cerr << std::endl;
}

inline void
report_critical_error( const char* msg, const char* file, int line, const_string func_name, bool is_msg = false )
{
    report_error( msg, file, line, func_name, is_msg );

    throw boost::execution_aborted();
}

class caller {
public:
    // constructor
    caller( int argc, char** argv )
    : m_argc( argc ), m_argv( argv ) {}

    // execution monitor hook implementation
    int operator()() { return test_main( m_argc, m_argv ); }

private:
    // Data members
    int         m_argc;
    char**      m_argv;
}; // monitor

} // namespace minimal_test
} // namespace boost

//____________________________________________________________________________//

int BOOST_TEST_CALL_DECL main( int argc, char* argv[] )
{
    using namespace boost::minimal_test;

    try {
        ::boost::execution_monitor ex_mon;
        int run_result = ex_mon.execute( caller( argc, argv ) );

        BOOST_CHECK( run_result == 0 || run_result == boost::exit_success );
    }
    catch( boost::execution_exception const& exex ) {
        if( exex.code() != boost::execution_exception::no_error )
            BOOST_ERROR( (std::string( "exception \"" ) + exex.what() + "\" caught").c_str() );
        std::cerr << "\n**** Testing aborted.";
    }

    if( boost::minimal_test::errors_counter() != 0 ) {
        std::cerr << "\n**** " << errors_counter()
                  << " error" << (errors_counter() > 1 ? "s" : "" ) << " detected\n";

        return boost::exit_test_failure;
    }

    std::cout << "\n**** no errors detected\n";

    return boost::exit_success;
}

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_MINIMAL_HPP_071894GER
