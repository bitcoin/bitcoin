/* boost random/detail/enable_warnings.hpp header file
 *
 * Copyright Steven Watanabe 2009
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 */

// No #include guard.  This header is intended to be included multiple times.

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#if defined(BOOST_GCC) && BOOST_GCC >= 40600
#pragma GCC diagnostic pop
#endif
