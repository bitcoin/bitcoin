//----------------------------------------------------------------------------
/// @file insert.hpp
/// @brief
///
/// @author Copyright (c) 2016 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_UTIL_INSERT_HPP
#define __BOOST_SORT_COMMON_UTIL_INSERT_HPP

//#include <boost/sort/spinsort/util/indirect.hpp>
#include <boost/sort/common/util/insert.hpp>
#include <boost/sort/common/util/traits.hpp>
#include <boost/sort/common/util/algorithm.hpp>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>
#include <cstddef>

namespace boost
{
namespace sort
{
namespace common
{
namespace util
{
namespace here = boost::sort::common::util;
//
//############################################################################
//
//          D E F I N I T I O N S    O F    F U N C T I O N S
//    
// template < class Iter1_t, class Iter2_t, typename Compare>
// void insert_sorted (Iter1_t first, Iter1_t mid, Iter1_t last,
//                     Compare comp, Iter2_t  it_aux)
//
//############################################################################
//
//-----------------------------------------------------------------------------
//  function : insert_sorted
/// @brief : Insertion sort of elements sorted
/// @param first: iterator to the first element of the range
/// @param mid : last pointer of the sorted data, and first pointer to the
///               elements to insert
/// @param last : iterator to the next element of the last in the range
/// @param comp :
/// @comments : the two ranges are sorted and in it_aux there is spave for 
///             to store temporally the elements to insert
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, typename Compare>
static void insert_sorted(Iter1_t first, Iter1_t mid, Iter1_t last,
                          Compare comp, Iter2_t it_aux)
{
    //------------------------------------------------------------------------
    //                 metaprogram
    //------------------------------------------------------------------------
    typedef value_iter<Iter1_t> value_t;
    typedef value_iter<Iter2_t> value2_t;
    static_assert (std::is_same< value_t, value2_t>::value,
                    "Incompatible iterators\n");

    //--------------------------------------------------------------------
    //                   program
    //--------------------------------------------------------------------
    if (mid == last) return;
    if (first == mid) return;

    //------------------------------------------------------------------------
    // creation of the vector of elements to insert and their position in the
    // sorted part
    // the data are inserted in it_aux
    //-----------------------------------------------------------------------
    move_forward(it_aux, mid, last);

    // search of the iterators where insert the new elements
    size_t ndata = last - mid;
    Iter1_t mv_first = mid, mv_last = mid;

    for (size_t i = ndata; i > 0; --i)
    {
        mv_last = mv_first;
        mv_first = std::upper_bound(first, mv_last, it_aux[i - 1], comp);
        Iter1_t it1 = here::move_backward(mv_last + i, mv_first, mv_last);
        *(it1 - 1) = std::move(it_aux[i - 1]);
    };
};

template<class Iter1_t, class Iter2_t, typename Compare>
static void insert_sorted_backward(Iter1_t first, Iter1_t mid, Iter1_t last,
                                   Compare comp, Iter2_t it_aux)
{
    //------------------------------------------------------------------------
    //                 metaprogram
    //------------------------------------------------------------------------
    typedef value_iter<Iter1_t> value_t;
    typedef value_iter<Iter2_t> value2_t;
    static_assert (std::is_same< value_t, value2_t>::value,
                    "Incompatible iterators\n");

    //--------------------------------------------------------------------
    //                   program
    //--------------------------------------------------------------------
    if (mid == last) return;
    if (first == mid) return;
    //------------------------------------------------------------------------
    // creation of the vector of elements to insert and their position in the
    // sorted part
    // the data are inserted in it_aux
    //-----------------------------------------------------------------------
    move_forward(it_aux, first, mid);

    // search of the iterators where insert the new elements
    size_t ndata = mid - first;
    Iter1_t mv_first = mid, mv_last = mid;

    for (size_t i = 0; i < ndata; ++i)
    {
        mv_first = mv_last;
        mv_last = std::lower_bound(mv_first, last, it_aux[i], comp);
        Iter1_t it1 = move_forward(mv_first - (ndata - i), mv_first, mv_last);
        *(it1) = std::move(it_aux[i]);
    };

};
//
//****************************************************************************
};//    End namespace util
};//    End namepspace common
};//    End namespace sort
};//    End namepspace boost
//****************************************************************************
//
#endif
