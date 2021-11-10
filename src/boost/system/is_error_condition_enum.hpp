#ifndef BOOST_SYSTEM_IS_ERROR_CONDITION_ENUM_HPP_INCLUDED
#define BOOST_SYSTEM_IS_ERROR_CONDITION_ENUM_HPP_INCLUDED

//  Copyright Beman Dawes 2006, 2007
//  Copyright Christoper Kohlhoff 2007
//  Copyright Peter Dimov 2017, 2018
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/system

namespace boost
{

namespace system
{

class error_condition;

template<class T> struct is_error_condition_enum
{
    static const bool value = false;
};

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_IS_ERROR_CONDITION_ENUM_HPP_INCLUDED
