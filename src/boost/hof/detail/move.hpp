/*=============================================================================
    Copyright (c) 2014 Paul Fultz II
    move.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_MOVE_H
#define BOOST_HOF_GUARD_MOVE_H

#include <utility>

namespace boost { namespace hof {

template<typename T>
constexpr typename std::remove_reference<T>::type&&
move(T&& x) noexcept
{ 
    return static_cast<typename std::remove_reference<T>::type&&>(x); 
}

}} // namespace boost::hof

#endif
