/*
Copyright 2017-2018 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_DETECTED_HPP_INCLUDED
#define BOOST_TT_DETECTED_HPP_INCLUDED

#include <boost/type_traits/detail/detector.hpp>
#include <boost/type_traits/nonesuch.hpp>

namespace boost {

template<template<class...> class Op, class... Args>
using detected_t = typename
    detail::detector<nonesuch, void, Op, Args...>::type;

} /* boost */

#endif
