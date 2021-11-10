//  boost/cstdlib.hpp header  ------------------------------------------------//

//  Copyright Beman Dawes 2001.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/utility/cstdlib.html for documentation.

//  Revision History
//   26 Feb 01  Initial version (Beman Dawes)

#ifndef BOOST_CSTDLIB_HPP
#define BOOST_CSTDLIB_HPP

#include <cstdlib>

namespace boost
{
   //  The intent is to propose the following for addition to namespace std
   //  in the C++ Standard Library, and to then deprecate EXIT_SUCCESS and
   //  EXIT_FAILURE.  As an implementation detail, this header defines the
   //  new constants in terms of EXIT_SUCCESS and EXIT_FAILURE.  In a new
   //  standard, the constants would be implementation-defined, although it
   //  might be worthwhile to "suggest" (which a standard is allowed to do)
   //  values of 0 and 1 respectively.

   //  Rationale for having multiple failure values: some environments may
   //  wish to distinguish between different classes of errors.
   //  Rationale for choice of values: programs often use values < 100 for
   //  their own error reporting.  Values > 255 are sometimes reserved for
   //  system detected errors.  200/201 were suggested to minimize conflict.

   const int exit_success = EXIT_SUCCESS;  // implementation-defined value
   const int exit_failure = EXIT_FAILURE;  // implementation-defined value
   const int exit_exception_failure = 200; // otherwise uncaught exception
   const int exit_test_failure = 201;      // report_error or
                                           //  report_critical_error called.
}

#endif

