/*=============================================================================
    Copyright (c) 2016 Paul Fultz II
    using.hpp
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_USING_HPP
#define BOOST_HOF_GUARD_USING_HPP

#include <boost/hof/config.hpp>

#if BOOST_HOF_HAS_TEMPLATE_ALIAS
#define BOOST_HOF_USING(name, ...) using name = __VA_ARGS__
#define BOOST_HOF_USING_TYPENAME(name, ...) using name = typename __VA_ARGS__
#else
#define BOOST_HOF_USING(name, ...) struct name : std::enable_if<true, __VA_ARGS__>::type {}
#define BOOST_HOF_USING_TYPENAME(name, ...) struct name : __VA_ARGS__ {}
#endif

#endif
