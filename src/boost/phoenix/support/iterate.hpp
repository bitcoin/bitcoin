/*==============================================================================
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_SUPPORT_ITERATE_HPP
#define BOOST_PHOENIX_SUPPORT_ITERATE_HPP

#include <boost/preprocessor/iteration/iterate.hpp>

#define BOOST_PHOENIX_IS_ITERATING 0

#define BOOST_PHOENIX_ITERATE()                                                 \
    <boost/phoenix/support/detail/iterate.hpp>

#include <boost/phoenix/support/detail/iterate_define.hpp>

#endif

