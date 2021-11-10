//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief contains wrappers, which allows to build Boost.Test with no exception
// ***************************************************************************

#ifndef BOOST_TEST_DETAIL_THROW_EXCEPTION_HPP
#define BOOST_TEST_DETAIL_THROW_EXCEPTION_HPP

// Boost
#include <boost/config.hpp> // BOOST_NO_EXCEPTIONS

#ifdef BOOST_NO_EXCEPTIONS
// C RUNTIME
#include <stdlib.h>

#endif

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace ut_detail {

#ifdef BOOST_NO_EXCEPTIONS

template<typename E>
BOOST_NORETURN inline void
throw_exception(E const& /*e*/) { abort(); }

#define BOOST_TEST_I_TRY
#define BOOST_TEST_I_CATCH( T, var ) for(T const& var = *(T*)0; false;)
#define BOOST_TEST_I_CATCH0( T ) if(0)
#define BOOST_TEST_I_CATCHALL() if(0)
#define BOOST_TEST_I_RETHROW

#else

template<typename E>
BOOST_NORETURN inline void
throw_exception(E const& e) { throw e; }

#define BOOST_TEST_I_TRY try
#define BOOST_TEST_I_CATCH( T, var ) catch( T const& var )
#define BOOST_TEST_I_CATCH0( T ) catch( T const& )
#define BOOST_TEST_I_CATCHALL() catch(...)
#define BOOST_TEST_I_RETHROW throw
#endif

//____________________________________________________________________________//

#define BOOST_TEST_I_THROW( E ) unit_test::ut_detail::throw_exception( E )
#define BOOST_TEST_I_ASSRT( cond, ex ) if( cond ) {} else BOOST_TEST_I_THROW( ex )


} // namespace ut_detail
} // namespace unit_test
} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DETAIL_THROW_EXCEPTION_HPP
