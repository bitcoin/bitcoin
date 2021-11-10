// Copyright (C) 2016-2018 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_YAP_CONFIG_HPP_INCLUDED
#define BOOST_YAP_CONFIG_HPP_INCLUDED


#ifndef BOOST_NO_CONSTEXPR_IF
/** Indicates whether the compiler supports constexpr if.

    If the user does not define any value for this, we assume that the
    compiler does not have the necessary support.  Note that this is a
    temporary hack; this should eventually be a Boost-wide macro. */
#define BOOST_NO_CONSTEXPR_IF
#elif BOOST_NO_CONSTEXPR_IF == 0
#undef BOOST_NO_CONSTEXPR_IF
#endif

#endif
