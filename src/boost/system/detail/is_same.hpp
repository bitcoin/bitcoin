#ifndef BOOST_SYSTEM_DETAIL_IS_SAME_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_IS_SAME_HPP_INCLUDED

// Copyright 2021 Peter Dimov
// Distributed under the Boost Software License, Version 1.0
// http://www.boost.org/LICENSE_1_0.txt

namespace boost
{

namespace system
{

namespace detail
{

template<class T1, class T2> struct is_same
{
    enum _vt { value = 0 };
};

template<class T> struct is_same<T, T>
{
    enum _vt { value = 1 };
};

} // namespace detail

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_IS_SAME_HPP_INCLUDED
