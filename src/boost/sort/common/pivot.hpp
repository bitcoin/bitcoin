//----------------------------------------------------------------------------
/// @file pivot.hpp
/// @brief This file contains the description of several low level algorithms
///
/// @author Copyright (c) 2010 2015 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_PIVOT_HPP
#define __BOOST_SORT_COMMON_PIVOT_HPP

#include <cstdint>

namespace boost
{
namespace sort
{
namespace common
{
//
//##########################################################################
//                                                                        ##
//                    G L O B A L     V A R I B L E S                     ##
//                                                                        ##
//##########################################################################
//
//-----------------------------------------------------------------------------
//  function : mid3
/// @brief : return the iterator to the mid value of the three values passsed
///          as parameters
//
/// @param iter_1 : iterator to the first value
/// @param iter_2 : iterator to the second value
/// @param iter_3 : iterator to the third value
/// @param comp : object for to compare two values
/// @return iterator to mid value
//-----------------------------------------------------------------------------
template < typename Iter_t, typename Compare >
inline Iter_t mid3 (Iter_t iter_1, Iter_t iter_2, Iter_t iter_3, Compare comp)
{
	if (comp (*iter_2, *iter_1)) std::swap ( *iter_2, *iter_1);
	if (comp (*iter_3, *iter_2))
	{	std::swap ( *iter_3, *iter_2);
		if (comp (*iter_2, *iter_1)) std::swap ( *iter_2, *iter_1);
	};
	return iter_2;
};
//
//-----------------------------------------------------------------------------
//  function : pivot3
/// @brief : receive a range between first and last, calcule the mid iterator
///          with the first, the previous to the last, and the central
///          position. With this mid iterator swap with the first position
//
/// @param first : iterator to the first element
/// @param last : iterator to the last element
/// @param comp : object for to compare two elements
//-----------------------------------------------------------------------------
template < class Iter_t, class Compare >
inline void pivot3 (Iter_t first, Iter_t last, Compare comp)
{
    auto N2 = (last - first) >> 1;
    Iter_t it_val = mid3 (first + 1, first + N2, last - 1, comp);
    std::swap (*first, *it_val);
};

//
//-----------------------------------------------------------------------------
//  function : mid9
/// @brief : return the iterator to the mid value of the nine values passsed
///          as parameters
//
/// @param iter_1 : iterator to the first value
/// @param iter_2 : iterator to the second value
/// @param iter_3 : iterator to the third value
/// @param iter_4 : iterator to the fourth value
/// @param iter_5 : iterator to the fifth value
/// @param iter_6 : iterator to the sixth value
/// @param iter_7 : iterator to the seventh value
/// @param iter_8 : iterator to the eighth value
/// @param iter_9 : iterator to the ninth value
/// @return iterator to the mid value
//-----------------------------------------------------------------------------
template < class Iter_t, class Compare >
inline Iter_t mid9 (Iter_t iter_1, Iter_t iter_2, Iter_t iter_3, Iter_t iter_4,
                    Iter_t iter_5, Iter_t iter_6, Iter_t iter_7, Iter_t iter_8,
                    Iter_t iter_9, Compare comp)
{
    return mid3 (mid3 (iter_1, iter_2, iter_3, comp),
                 mid3 (iter_4, iter_5, iter_6, comp),
                 mid3 (iter_7, iter_8, iter_9, comp), comp);
};
//
//-----------------------------------------------------------------------------
//  function : pivot9
/// @brief : receive a range between first and last, obtain 9 values between
///          the elements  including the first and the previous to the last.
///          Obtain the iterator to the mid value and swap with the first
///          position
//
/// @param first : iterator to the first element
/// @param last : iterator to the last element
/// @param comp : object for to compare two elements
//-----------------------------------------------------------------------------
template < class Iter_t, class Compare >
inline void pivot9 (Iter_t first, Iter_t last, Compare comp)
{
    size_t cupo = (last - first) >> 3;
    Iter_t itaux = mid9 (first + 1, first + cupo, first + 2 * cupo,
                         first + 3 * cupo, first + 4 * cupo, first + 5 * cupo,
                         first + 6 * cupo, first + 7 * cupo, last - 1, comp);
    std::swap (*first, *itaux);
};
//****************************************************************************
};//    End namespace common
};//    End namespace sort
};//    End namespace boost
//****************************************************************************
#endif
