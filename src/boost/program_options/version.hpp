// Copyright Vladimir Prus 2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROGRAM_OPTIONS_VERSION_HPP_VP_2004_04_05
#define BOOST_PROGRAM_OPTIONS_VERSION_HPP_VP_2004_04_05

/** The version of the source interface.
    The value will be incremented whenever a change is made which might
    cause compilation errors for existing code.
*/
#ifdef BOOST_PROGRAM_OPTIONS_VERSION
#error BOOST_PROGRAM_OPTIONS_VERSION already defined
#endif
#define BOOST_PROGRAM_OPTIONS_VERSION 2

// Signal that implicit options will use values from next
// token, if available.
#define BOOST_PROGRAM_OPTIONS_IMPLICIT_VALUE_NEXT_TOKEN 1

#endif
