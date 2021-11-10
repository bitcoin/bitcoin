//----------------------------------------------------------------------------
/// @file heap_sort.hpp
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
#ifndef __BOOST_SORT_INTROSORT_DETAIL_HEAP_SORT_HPP
#define __BOOST_SORT_INTROSORT_DETAIL_HEAP_SORT_HPP

#include <cassert>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <utility> // for std::swap
#include <boost/sort/common/util/traits.hpp>

namespace boost
{
namespace sort
{
namespace heap_detail
{
namespace bscu = boost::sort::common::util;
//
//---------------------------------------------------------------------------
//  struct : heap_sort
/// @brief : Heap sort algorithm
/// @remarks This algorithm is O(NLogN)
//---------------------------------------------------------------------------
template < class Iter_t, class Compare >
struct heap_sort
{
    typedef bscu::value_iter<Iter_t> value_t;

    //
    //------------------------------------------------------------------------
    //  function : sort3
    /// @brief Sort and signal the changes of three values
    /// @param val_0 : first value to compare
    /// @param val_1 : second value to compare
    /// @param val_2 : third value to compare
    /// @param [out] bool_0 : if true indicates val_0 had been changed
    /// @param [out] bool_1 : if true indicates val_1 had been changed
    /// @param [out] bool_2 : if true indicates val_2 had been changed
    /// @return if true , some value had changed
    /// @remarks
    //------------------------------------------------------------------------
    bool sort3 (value_t &val_0, value_t &val_1, value_t &val_2, bool &bool_0,
                bool &bool_1, bool &bool_2)
    {
        bool_0 = bool_1 = bool_2 = false;
        int value = 0;
        if (val_0 < val_1) value += 4;
        if (val_1 < val_2) value += 2;
        if (val_0 < val_2) value += 1;

        switch (value)
        {
            case 0: break;

            case 2:
                std::swap (val_1, val_2);
                bool_1 = bool_2 = true;
                break;

            case 3:
                if (not(val_0 > val_1)) {
                    std::swap (val_0, val_2);
                    bool_0 = bool_2 = true;
                }
                else
                {
                    auto aux = std::move (val_2);
                    val_2 = std::move (val_1);
                    val_1 = std::move (val_0);
                    val_0 = std::move (aux);
                    bool_0 = bool_1 = bool_2 = true;
                };
                break;

            case 4:
                std::swap (val_0, val_1);
                bool_0 = bool_1 = true;
                break;

            case 5:
                if (val_1 > val_2) {
                    auto aux = std::move (val_0);
                    val_0 = std::move (val_1);
                    val_1 = std::move (val_2);
                    val_2 = std::move (aux);
                    bool_0 = bool_1 = bool_2 = true;
                }
                else
                {
                    std::swap (val_0, val_2);
                    bool_0 = bool_2 = true;
                };
                break;

            case 7:
                std::swap (val_0, val_2);
                bool_0 = bool_2 = true;
                break;

            default: abort ( );
        };
        return (bool_0 or bool_1 or bool_2);
    };
    //
    //-----------------------------------------------------------------------
    //  function : make_heap
    /// @brief Make the heap for to extract the sorted elements
    /// @param first : iterator to the first element of the range
    /// @param nelem : number of lements of the range
    /// @param comp : object for to compare two elements
    /// @remarks This algorithm is O(NLogN)
    //------------------------------------------------------------------------
    void make_heap (Iter_t first, size_t nelem, Compare comp)
    {
        size_t pos_father, pos_son;
        Iter_t iter_father = first, iter_son = first;
        bool sw = false;

        for (size_t i = 1; i < nelem; ++i)
        {
            pos_father = i;
            iter_father = first + i;
            sw = false;
            do
            {
                iter_son = iter_father;
                pos_son = pos_father;
                pos_father = (pos_son - 1) >> 1;
                iter_father = first + pos_father;
                if ((sw = comp (*iter_father, *iter_son)))
                    std::swap (*iter_father, *iter_son);
            } while (sw and pos_father != 0);
        };
    };
    //
    //------------------------------------------------------------------------
    //  function : heap_sort
    /// @brief : Heap sort algorithm
    /// @param first: iterator to the first element of the range
    /// @param last : iterator to the next element of the last in the range
    /// @param comp : object for to do the comparison between the elements
    /// @remarks This algorithm is O(NLogN)
    //------------------------------------------------------------------------
    heap_sort (Iter_t first, Iter_t last, Compare comp)
    {
        assert ((last - first) >= 0);
        size_t nelem = last - first;
        if (nelem < 2) return;

        //--------------------------------------------------------------------
        // Creating the initial heap
        //--------------------------------------------------------------------
        make_heap (first, nelem, comp);

        //--------------------------------------------------------------------
        //  Sort the heap
        //--------------------------------------------------------------------
        size_t pos_father, pos_son;
        Iter_t iter_father = first, iter_son = first;

        bool sw = false;
        for (size_t i = 1; i < nelem; ++i)
        {
            std::swap (*first, *(first + (nelem - i)));
            pos_father = 0;
            pos_son = 1;
            iter_father = first;
            sw = true;
            while (sw and pos_son < (nelem - i))
            {
                // if the father have two sons must select the bigger
                iter_son = first + pos_son;
                if ((pos_son + 1) < (nelem - i) and
                    comp (*iter_son, *(iter_son + 1)))
                {
                    ++pos_son;
                    ++iter_son;
                };
                if ((sw = comp (*iter_father, *iter_son)))
                    std::swap (*iter_father, *iter_son);
                pos_father = pos_son;
                iter_father = iter_son;
                pos_son = (pos_father << 1) + 1;
            };
        };
    };
}; // End class heap_sort
}; // end namespace heap_sort

namespace bscu = boost::sort::common::util;

template < class Iter_t, typename Compare = bscu::compare_iter < Iter_t > >
void heap_sort (Iter_t first, Iter_t last, Compare comp = Compare())
{
	heap_detail::heap_sort<Iter_t, Compare> ( first, last, comp);
}
//
//****************************************************************************
}; //    End namespace sort
}; //    End namespace boost
//****************************************************************************
//
#endif
