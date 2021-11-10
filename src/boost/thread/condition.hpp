#ifndef BOOST_THREAD_CONDITION_HPP
#define BOOST_THREAD_CONDITION_HPP
//  (C) Copyright 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#if defined BOOST_THREAD_PROVIDES_CONDITION

#include <boost/thread/condition_variable.hpp>

namespace boost
{
    typedef condition_variable_any condition;
}

#endif
#endif
