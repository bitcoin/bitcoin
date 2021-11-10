//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief test tools compatibility header
///
/// This file is used to select the test tools implementation and includes all the necessary headers
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_HPP_111812GER
#define BOOST_TEST_TOOLS_HPP_111812GER

#include <boost/config.hpp>

// brings some compiler configuration like BOOST_PP_VARIADICS
#include <boost/test/detail/config.hpp>

#include <boost/preprocessor/config/config.hpp>

#if    defined(BOOST_NO_CXX11_VARIADIC_MACROS) \
    || defined(BOOST_NO_CXX11_AUTO_DECLARATIONS) \
    || defined(BOOST_NO_CXX11_DECLTYPE)
#  define BOOST_TEST_MACRO_LIMITED_SUPPORT
#endif

// Boost.Test
// #define BOOST_TEST_NO_OLD_TOOLS

#if     defined(BOOST_TEST_MACRO_LIMITED_SUPPORT) \
    &&  (   !BOOST_PP_VARIADICS \
         || !(__cplusplus >= 201103L) && defined(BOOST_NO_CXX11_VARIADIC_MACROS))
#  define BOOST_TEST_NO_NEW_TOOLS
#endif

// #define BOOST_TEST_TOOLS_UNDER_DEBUGGER
// #define BOOST_TEST_TOOLS_DEBUGGABLE

#include <boost/test/tools/context.hpp>

#ifndef BOOST_TEST_NO_OLD_TOOLS
#  include <boost/test/tools/old/interface.hpp>
#  include <boost/test/tools/old/impl.hpp>

#  include <boost/test/tools/detail/print_helper.hpp>
#endif

#ifndef BOOST_TEST_NO_NEW_TOOLS
#  include <boost/test/tools/interface.hpp>
#  include <boost/test/tools/assertion.hpp>
#  include <boost/test/tools/fpc_op.hpp>
#  include <boost/test/tools/collection_comparison_op.hpp>
#  include <boost/test/tools/cstring_comparison_op.hpp>

#  include <boost/test/tools/detail/fwd.hpp>
#  include <boost/test/tools/detail/print_helper.hpp>
#  include <boost/test/tools/detail/it_pair.hpp>

#  include <boost/test/tools/detail/bitwise_manip.hpp>
#  include <boost/test/tools/detail/tolerance_manip.hpp>
#  include <boost/test/tools/detail/per_element_manip.hpp>
#  include <boost/test/tools/detail/lexicographic_manip.hpp>
#endif

#endif // BOOST_TEST_TOOLS_HPP_111812GER
