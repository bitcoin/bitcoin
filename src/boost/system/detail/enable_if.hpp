#ifndef BOOST_SYSTEM_DETAIL_ENABLE_IF_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_ENABLE_IF_HPP_INCLUDED

// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0
// http://www.boost.org/LICENSE_1_0.txt

namespace boost
{

namespace system
{

namespace detail
{

template<bool C, class T = void> struct enable_if
{
    typedef T type;
};

template<class T> struct enable_if<false, T>
{
};

} // namespace detail

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_ENABLE_IF_HPP_INCLUDED
