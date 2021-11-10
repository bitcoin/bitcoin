//-----------------------------------------------------------------------------
// boost variant/detail/forced_return.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003 Eric Friedman
// Copyright (c) 2015-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_DETAIL_FORCED_RETURN_HPP
#define BOOST_VARIANT_DETAIL_FORCED_RETURN_HPP

#include <boost/config.hpp>
#include <boost/assert.hpp>


#ifdef BOOST_MSVC
# pragma warning( push )
# pragma warning( disable : 4702 ) // unreachable code
#endif

namespace boost { namespace detail { namespace variant {

///////////////////////////////////////////////////////////////////////////////
// (detail) function template forced_return
//
// Logical error to permit invocation at runtime, but (artificially) satisfies
// compile-time requirement of returning a result value.
//
template <typename T>
BOOST_NORETURN inline T
forced_return()
{
    // logical error: should never be here! (see above)
    BOOST_ASSERT(false);

    T (*dummy)() = 0;
    (void)dummy;
    BOOST_UNREACHABLE_RETURN(dummy());
}

}}} // namespace boost::detail::variant


#ifdef BOOST_MSVC
# pragma warning( pop )
#endif

#endif // BOOST_VARIANT_DETAIL_FORCED_RETURN_HPP
