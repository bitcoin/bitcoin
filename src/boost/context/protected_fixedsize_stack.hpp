
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

#if defined(BOOST_WINDOWS)
# include <boost/context/windows/protected_fixedsize_stack.hpp>
#else
# include <boost/context/posix/protected_fixedsize_stack.hpp>
#endif
