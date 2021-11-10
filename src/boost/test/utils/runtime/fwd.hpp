//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : runtime parameters forward declaration
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_FWD_HPP
#define BOOST_TEST_UTILS_RUNTIME_FWD_HPP

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <boost/test/utils/basic_cstring/io.hpp> // operator<<(boost::runtime::cstring)

// Boost
#include <boost/shared_ptr.hpp>

// STL
#include <map>

namespace boost {
namespace runtime {

typedef unit_test::const_string     cstring;

class argument;
typedef shared_ptr<argument> argument_ptr;

template<typename T> class typed_argument;

class basic_param;
typedef shared_ptr<basic_param> basic_param_ptr;

} // namespace runtime
} // namespace boost

#endif // BOOST_TEST_UTILS_RUNTIME_FWD_HPP
