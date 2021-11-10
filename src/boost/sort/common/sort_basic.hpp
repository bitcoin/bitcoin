//----------------------------------------------------------------------------
/// @file sort_basic.hpp
/// @brief Spin Sort algorithm
///
/// @author Copyright (c) 2016 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_SORT_BASIC_HPP
#define __BOOST_SORT_COMMON_SORT_BASIC_HPP

//#include <boost/sort/spinsort/util/indirect.hpp>
#include <boost/sort/insert_sort/insert_sort.hpp>
#include <boost/sort/common/util/traits.hpp>
#include <boost/sort/common/range.hpp>
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

//----------------------------------------------------------------------------
//                USING SENTENCES
//----------------------------------------------------------------------------
using boost::sort::insert_sort;

//-----------------------------------------------------------------------------
//  function : is_stable_sorted_forward
/// @brief examine the elements in the range first, last if they are stable
///        sorted, and return an iterator to the first element not sorted
/// @param first : iterator to the first element in the range
/// @param last : ierator after the last element of the range
/// @param comp : object for to compare two elements
/// @return iterator to the first element not stable sorted. The number of
///         elements sorted is the iterator returned minus first
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare = std::less<value_iter<Iter_t> > >
inline Iter_t is_stable_sorted_forward (Iter_t first, Iter_t last,
                                        Compare comp = Compare())
{
#ifdef __BS_DEBUG
    assert ( (last- first) >= 0);
#endif
    if ((last - first) < 2) return first;
    Iter_t it2 = first + 1;
    for (Iter_t it1 = first; it2 != last and not comp(*it2, *it1); it1 = it2++);
    return it2;
}
//-----------------------------------------------------------------------------
//  function : is_reverse_stable_sorted_forward
/// @brief examine the elements in the range first, last if they are reverse
///        stable sorted, and return an iterator to the first element not
///        reverse stable sorted
/// @param first : iterator to the first element in the range
/// @param last : ierator after the last element of the range
/// @param comp : object for to compare two elements
/// @return iterator to the first element not  reverse stable sorted. The number
///         of elements sorted is the iterator returned minus first
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare = std::less<value_iter<Iter_t> > >
inline Iter_t is_reverse_stable_sorted_forward(Iter_t first, Iter_t last,
                                               Compare comp = Compare())
{
#ifdef __BS_DEBUG
    assert ( (last- first) >= 0);
#endif
    if ((last - first) < 2) return first;
    Iter_t it2 = first + 1;
    for (Iter_t it1 = first; it2 != last and comp(*it2, *it1); it1 = it2++);
    return it2;
};
//-----------------------------------------------------------------------------
//  function : number_stable_sorted_forward
/// @brief examine the elements in the range first, last if they are stable
///        sorted, and return the number of elements sorted
/// @param first : iterator to the first element in the range
/// @param last : ierator after the last element of the range
/// @param comp : object for to compare two elements
/// @param min_process : minimal number of elements to be consideer
/// @return number of element sorted. I f the number is lower than min_process
///         return 0
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare = std::less<value_iter<Iter_t> > >
size_t number_stable_sorted_forward (Iter_t first, Iter_t last,
		                             size_t min_process,
                                     Compare comp = Compare())
{
#ifdef __BS_DEBUG
    assert ( (last- first) >= 0);
#endif
    if ((last - first) < 2) return 0;

    // sorted elements
    Iter_t it2 = first + 1;
    for (Iter_t it1 = first; it2 != last and not comp(*it2, *it1); it1 = it2++);
    size_t nsorted = size_t ( it2 - first);
    if ( nsorted != 1)
    	return (nsorted >= min_process) ? nsorted: 0;

    // reverse sorted elements
    it2 = first + 1;
    for (Iter_t it1 = first; it2 != last and comp(*it2, *it1); it1 = it2++);
    nsorted = size_t ( it2 - first);

    if ( nsorted < min_process) return 0 ;
    util::reverse ( first , it2);
    return nsorted;
};

//-----------------------------------------------------------------------------
//  function : is_stable_sorted_backward
/// @brief examine the elements in the range first, last beginning at end, and
///        if they are stablesorted, and return an iterator to the last element
///        sorted
/// @param first : iterator to the first element in the range
/// @param last : ierator after the last element of the range
/// @param comp : object for to compare two elements
/// @return iterator to the last element stable sorted. The number of
///         elements sorted is the last minus the iterator returned
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare = std::less<value_iter<Iter_t> > >
inline Iter_t is_stable_sorted_backward(Iter_t first, Iter_t last,
                                        Compare comp = Compare())
{
#ifdef __BS_DEBUG
    assert ( (last- first) >= 0);
#endif
    if ((last - first) < 2) return first;
    Iter_t itaux = last - 1;
    while (itaux != first and not comp(*itaux, *(itaux - 1))) {--itaux; };
    return itaux;
}
//-----------------------------------------------------------------------------
//  function : is_reverse_stable_sorted_backward
/// @brief examine the elements in the range first, last beginning at end, and
///        if they are stablesorted, and return an iterator to the last element
///        sorted
/// @param first : iterator to the first element in the range
/// @param last : ierator after the last element of the range
/// @param comp : object for to compare two elements
/// @return iterator to the last element stable sorted. The number of
///         elements sorted is the last minus the iterator returned
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare = std::less<value_iter<Iter_t> > >
inline Iter_t is_reverse_stable_sorted_backward (Iter_t first, Iter_t last,
                                                 Compare comp = Compare())
{
#ifdef __BS_DEBUG
    assert ( (last- first) >= 0);
#endif
    if ((last - first) < 2) return first;
    Iter_t itaux = last - 1;
    for (; itaux != first and comp(*itaux, *(itaux - 1)); --itaux);
    return itaux;
}

//-----------------------------------------------------------------------------
//  function : number_stable_sorted_backward
/// @brief examine the elements in the range first, last if they are stable
///        sorted, and return the number of elements sorted
/// @param first : iterator to the first element in the range
/// @param last : ierator after the last element of the range
/// @param comp : object for to compare two elements
/// @param min_process : minimal number of elements to be consideer
/// @return number of element sorted. I f the number is lower than min_process
///         return 0
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare = std::less<value_iter<Iter_t> > >
size_t number_stable_sorted_backward (Iter_t first, Iter_t last,
		                             size_t min_process,
                                     Compare comp = Compare())
{
#ifdef __BS_DEBUG
    assert ( (last- first) >= 0);
#endif
    if ((last - first) < 2) return 0;
    Iter_t itaux = last - 1;
    while (itaux != first and not comp(*itaux, *(itaux - 1))) {--itaux; };
    size_t nsorted = size_t ( last - itaux);
    if ( nsorted != 1)
    	return ( nsorted >= min_process)?nsorted: 0 ;

    itaux = last - 1;
    for (; itaux != first and comp(*itaux, *(itaux - 1)); --itaux);
    nsorted = size_t ( last - itaux);
    if ( nsorted < min_process) return 0 ;
    util::reverse ( itaux, last );
    return nsorted;
}
//-----------------------------------------------------------------------------
//  function : internal_sort
/// @brief this function divide r_input in two parts, sort it,and merge moving
///        the elements to range_buf
/// @param range_input : range with the elements to sort
/// @param range_buffer : range with the elements sorted
/// @param comp : object for to compare two elements
/// @param level : when is 1, sort with the insertionsort algorithm
///                if not make a recursive call splitting the ranges
//
//-----------------------------------------------------------------------------
template <class Iter1_t, class Iter2_t, class Compare>
inline void internal_sort (const range<Iter1_t> &rng1,
		                   const range<Iter2_t> &rng2,
                           Compare comp, uint32_t level, bool even = true)
{
    //-----------------------------------------------------------------------
    //                  metaprogram
    //-----------------------------------------------------------------------
    typedef value_iter<Iter1_t> value_t;
    typedef value_iter<Iter2_t> value2_t;
    static_assert (std::is_same< value_t, value2_t>::value,
                    "Incompatible iterators\n");

    //-----------------------------------------------------------------------
    //                  program
    //-----------------------------------------------------------------------
#ifdef __BS_DEBUG
    assert (rng1.size ( ) == rng2.size ( ) );
#endif
    size_t nelem = (rng1.size() + 1) >> 1;

    range<Iter1_t> rng1_left(rng1.first, rng1.first + nelem), 
                   rng1_right(rng1.first + nelem, rng1.last);

    range<Iter2_t> rng2_left(rng2.first, rng2.first + nelem), 
                   rng2_right(rng2.first + nelem, rng2.last);

    if (nelem <= 32 and (level & 1) == even)
    {
        insert_sort(rng1_left.first, rng1_left.last, comp);
        insert_sort(rng1_right.first, rng1_right.last, comp);
    }
    else
    {
        internal_sort(rng2_left, rng1_left, comp, level + 1, even);
        internal_sort(rng2_right, rng1_right, comp, level + 1, even);
    };
    merge(rng2, rng1_left, rng1_right, comp);
};
//-----------------------------------------------------------------------------
//  function : range_sort_data
/// @brief this sort elements using the range_sort function and receiving a
///        buffer of initialized memory
/// @param rng_data : range with the elements to sort
/// @param rng_aux : range of at least the same memory than rng_data used as
///                  auxiliary memory in the sorting
/// @param comp : object for to compare two elements
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Compare>
static void range_sort_data (const range<Iter1_t> & rng_data,
                             const range<Iter2_t> & rng_aux, Compare comp)
{
    //-----------------------------------------------------------------------
    //                  metaprogram
    //-----------------------------------------------------------------------
    typedef value_iter<Iter1_t> value_t;
    typedef value_iter<Iter2_t> value2_t;
    static_assert (std::is_same< value_t, value2_t>::value,
                    "Incompatible iterators\n");

    //------------------------------------------------------------------------
    //                    program
    //------------------------------------------------------------------------
#ifdef __BS_DEBUG
    assert ( rng_data.size() == rng_aux.size());
#endif
    // minimal number of element before to jump to insertionsort
    const uint32_t sort_min = 32;
    if (rng_data.size() <= sort_min)
    {
        insert_sort(rng_data.first, rng_data.last, comp);
        return;
    };

    internal_sort(rng_aux, rng_data, comp, 0, true);
};
//-----------------------------------------------------------------------------
//  function : range_sort_buffer
/// @brief this sort elements using the range_sort function and receiving a
///        buffer of initialized memory
/// @param rng_data : range with the elements to sort
/// @param rng_aux : range of at least the same memory than rng_data used as
///                  auxiliary memory in the sorting
/// @param comp : object for to compare two elements
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Compare>
static void range_sort_buffer(const range<Iter1_t> & rng_data,
                              const range<Iter2_t> & rng_aux, Compare comp)
{
    //-----------------------------------------------------------------------
    //                  metaprogram
    //-----------------------------------------------------------------------
    typedef value_iter<Iter1_t> value_t;
    typedef value_iter<Iter2_t> value2_t;
    static_assert (std::is_same< value_t, value2_t>::value,
                    "Incompatible iterators\n");

    //------------------------------------------------------------------------
    //                    program
    //------------------------------------------------------------------------
#ifdef __BS_DEBUG
    assert ( rng_data.size() == rng_aux.size());
#endif
    // minimal number of element before to jump to insertionsort
    const uint32_t sort_min = 32;
    if (rng_data.size() <= sort_min)
    {
        insert_sort(rng_data.first, rng_data.last, comp);
        move_forward(rng_aux, rng_data);
        return;
    };

    internal_sort(rng_data, rng_aux, comp, 0, false);
};
//****************************************************************************
};//    End namespace common
};//    End namespace sort
};//    End namepspace boost
//****************************************************************************
//
#endif
