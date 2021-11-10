// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/config.hpp>             // BOOST_MSVC.
#include <boost/detail/workaround.hpp>  // BOOST_WORKAROUND.

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4127)    // Conditional expression is constant.
# pragma warning(disable:4130)    // Logical operation on address of string constant.
# pragma warning(disable:4224)    // Parameter previously defined as type.
# pragma warning(disable:4244)    // Conversion: possible loss of data.
# pragma warning(disable:4512)    // Assignment operator could not be generated.
# pragma warning(disable:4706)    // Assignment within conditional expression.
# if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#  pragma warning(disable:6334)   // sizeof applied to an expression with an operator.
# endif
#else
# if BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600)
#  pragma warn -8008     // Condition always true/false.
#  pragma warn -8066     // Unreachable code.
#  pragma warn -8071     // Conversion may lose significant digits.
#  pragma warn -8072     // Suspicious pointer arithmetic.
#  pragma warn -8080     // identifier declared but never used.
# endif
#endif
