// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#if defined(BOOST_MSVC)
# pragma warning(pop)
#else
# if BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600)
#  pragma warn .8008     // Condition always true/false.
#  pragma warn .8066     // Unreachable code.
#  pragma warn .8071     // Conversion may lose significant digits.
#  pragma warn .8072     // Suspicious pointer arithmetic.
#  pragma warn .8080     // identifier declared but never used.
# endif
#endif
