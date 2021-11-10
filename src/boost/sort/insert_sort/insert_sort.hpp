//----------------------------------------------------------------------------
/// @file insert_sort.hpp
/// @brief Insertion Sort algorithm
///
/// @author Copyright (c) 2016 Francisco Jose Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_INTROSORT_DETAIL_INSERT_SORT_HPP
#define __BOOST_SORT_INTROSORT_DETAIL_INSERT_SORT_HPP

#include <functional>
#include <iterator>
#include <algorithm>
#include <utility> // std::swap
#include <boost/sort/common/util/traits.hpp>
#include <boost/sort/common/util/insert.hpp>

namespace boost
{
namespace sort
{
using common::util::compare_iter;
using common::util::value_iter;
//
//-----------------------------------------------------------------------------
//  function : insert_sort
/// @brief : Insertion sort algorithm
/// @param first: iterator to the first element of the range
/// @param last : iterator to the next element of the last in the range
/// @param comp : object for to do the comparison between the elements
/// @remarks This algorithm is O(N^2)
//-----------------------------------------------------------------------------
template < class Iter_t, typename Compare = compare_iter < Iter_t > >
static void insert_sort (Iter_t first, Iter_t last,
                         Compare comp = Compare())
{
    //--------------------------------------------------------------------
    //                   DEFINITIONS
    //--------------------------------------------------------------------
    typedef value_iter< Iter_t > value_t;

    if ((last - first) < 2) return;

    for (Iter_t it_examine = first + 1; it_examine != last; ++it_examine)
    {
        value_t Aux = std::move (*it_examine);
        Iter_t it_insertion = it_examine;

        while (it_insertion != first and comp (Aux, *(it_insertion - 1)))
        {
            *it_insertion = std::move (*(it_insertion - 1));
            --it_insertion;
        };
        *it_insertion = std::move (Aux);
    };
};

/*
//
//-----------------------------------------------------------------------------
//  function : insert_partial_sort
/// @brief : Insertion sort of elements sorted
/// @param first: iterator to the first element of the range
/// @param mid : last pointer of the sorted data, and first pointer to the
///               elements to insert
/// @param last : iterator to the next element of the last in the range
/// @param comp : object for to do the comparison between the elements
/// @remarks This algorithm is O(N^2)
//-----------------------------------------------------------------------------
template < class Iter_t, typename Compare = compare_iter < Iter_t > >
void insert_partial_sort (Iter_t first, Iter_t mid, Iter_t last,
                          Compare comp = Compare())
{
    //--------------------------------------------------------------------
    //                   DEFINITIONS
    //--------------------------------------------------------------------
    typedef value_iter< Iter_t > value_t;

    if ( mid == last ) return ;
    insert_sort ( mid, last, comp);
    if (first == mid) return ;

    // creation of the vector of elements to insert and their position in the
    // sorted part
    std::vector<Iter_t> viter ;
    std::vector<value_t> vdata ;

    for ( Iter_t alpha = mid ; alpha != last ; ++alpha)
        vdata.push_back ( std::move ( *alpha));

    Iter_t linf = first , lsup = mid ;
    for ( uint32_t i= 0 ; i < vdata.size() ; ++i)
    {   Iter_t it1 = std::upper_bound ( linf, lsup , vdata[i], comp);
        viter.push_back ( it1 );
        linf = it1 ;
    };

    // moving the elements
    viter.push_back ( mid) ;
    for ( uint32_t i = viter.size() -1 ; i!= 0 ; --i)
    {   Iter_t src = viter[i], limit = viter[i-1];
        Iter_t dest = src + ( i);
        while ( src != limit) * (--dest) = std::move ( *(--src));
        *(viter[i-1] + (i -1)) = std::move (vdata[i-1]);
    };
}
*/
//
//****************************************************************************
}; //    End namespace sort
}; //    End namespace boost
//****************************************************************************
//
#endif
