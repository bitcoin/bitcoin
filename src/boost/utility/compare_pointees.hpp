// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com
//
#ifndef BOOST_UTILITY_COMPARE_POINTEES_25AGO2003_HPP
#define BOOST_UTILITY_COMPARE_POINTEES_25AGO2003_HPP

#include<functional>

namespace boost {

// template<class OP> bool equal_pointees(OP const& x, OP const& y);
// template<class OP> struct equal_pointees_t;
//
// Being OP a model of OptionalPointee (either a pointer or an optional):
//
// If both x and y have valid pointees, returns the result of (*x == *y)
// If only one has a valid pointee, returns false.
// If none have valid pointees, returns true.
// No-throw
template<class OptionalPointee>
inline
bool equal_pointees ( OptionalPointee const& x, OptionalPointee const& y )
{
  return (!x) != (!y) ? false : ( !x ? true : (*x) == (*y) ) ;
}

template<class OptionalPointee>
struct equal_pointees_t
{
  typedef bool result_type;
  typedef OptionalPointee first_argument_type;
  typedef OptionalPointee second_argument_type;

  bool operator() ( OptionalPointee const& x, OptionalPointee const& y ) const
    { return equal_pointees(x,y) ; }
} ;

// template<class OP> bool less_pointees(OP const& x, OP const& y);
// template<class OP> struct less_pointees_t;
//
// Being OP a model of OptionalPointee (either a pointer or an optional):
//
// If y has not a valid pointee, returns false.
// ElseIf x has not a valid pointee, returns true.
// ElseIf both x and y have valid pointees, returns the result of (*x < *y)
// No-throw
template<class OptionalPointee>
inline
bool less_pointees ( OptionalPointee const& x, OptionalPointee const& y )
{
  return !y ? false : ( !x ? true : (*x) < (*y) ) ;
}

template<class OptionalPointee>
struct less_pointees_t
{
  typedef bool result_type;
  typedef OptionalPointee first_argument_type;
  typedef OptionalPointee second_argument_type;

  bool operator() ( OptionalPointee const& x, OptionalPointee const& y ) const
    { return less_pointees(x,y) ; }
} ;

} // namespace boost

#endif

