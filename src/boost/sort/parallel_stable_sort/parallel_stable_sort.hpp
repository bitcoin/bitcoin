//----------------------------------------------------------------------------
/// @file parallel_stable_sort.hpp
/// @brief This file contains the class parallel_stable_sort
///
/// @author Copyright (c) 2016 Francisco Jose Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_DETAIL_PARALLEL_STABLE_SORT_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_PARALLEL_STABLE_SORT_HPP

#include <ciso646>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>
#include <boost/sort/sample_sort/sample_sort.hpp>
#include <boost/sort/common/util/traits.hpp>


namespace boost
{
namespace sort
{
namespace stable_detail
{

//---------------------------------------------------------------------------
//                    USING SENTENCES
//---------------------------------------------------------------------------
namespace bsc = boost::sort::common;
namespace bss = boost::sort::spin_detail;
using bsc::range;
using bsc::merge_half;
using boost::sort::sample_detail::sample_sort;
//
///---------------------------------------------------------------------------
/// @struct parallel_stable_sort
/// @brief This a structure for to implement a parallel stable sort, exception
///        safe
//----------------------------------------------------------------------------
template <class Iter_t, class Compare = compare_iter <Iter_t> >
struct parallel_stable_sort
{
    //-------------------------------------------------------------------------
    //                      DEFINITIONS
    //-------------------------------------------------------------------------
    typedef value_iter<Iter_t> value_t;

    //-------------------------------------------------------------------------
    //                     VARIABLES
    //-------------------------------------------------------------------------
    // Number of elements to sort
    size_t nelem;
    // Pointer to the auxiliary memory needed for the algorithm
    value_t *ptr;
    // Minimal number of elements for to be sorted in parallel mode
    const size_t nelem_min = 1 << 16;

    //------------------------------------------------------------------------
    //                F U N C T I O N S
    //------------------------------------------------------------------------
    parallel_stable_sort (Iter_t first, Iter_t last)
    : parallel_stable_sort (first, last, Compare(),
                            std::thread::hardware_concurrency()) { };

    parallel_stable_sort (Iter_t first, Iter_t last, Compare cmp)
    : parallel_stable_sort (first, last, cmp,
                            std::thread::hardware_concurrency()) { };

    parallel_stable_sort (Iter_t first, Iter_t last, uint32_t num_thread)
    : parallel_stable_sort (first, last, Compare(), num_thread) { };

    parallel_stable_sort (Iter_t first, Iter_t last, Compare cmp,
                          uint32_t num_thread);

    //
    //-----------------------------------------------------------------------------
    //  function : destroy_all
    /// @brief The utility is to destroy the temporary buffer used in the
    ///        sorting process
    //-----------------------------------------------------------------------------
    void destroy_all()
    {
        if (ptr != nullptr) std::return_temporary_buffer(ptr);
    };
    //
    //-----------------------------------------------------------------------------
    //  function :~parallel_stable_sort
    /// @brief destructor of the class. The utility is to destroy the temporary
    ///        buffer used in the sorting process
    //-----------------------------------------------------------------------------
    ~parallel_stable_sort() {destroy_all(); } ;
};
// end struct parallel_stable_sort

//
//############################################################################
//                                                                          ##
//                                                                          ##
//            N O N     I N L I N E      F U N C T I O N S                  ##
//                                                                          ##
//                                                                          ##
//############################################################################
//
//-----------------------------------------------------------------------------
//  function : parallel_stable_sort
/// @brief constructor of the class
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param comp : object for to compare two elements pointed by Iter_t
///                    iterators
/// @param nthread : Number of threads to use in the process. When this value
///                  is lower than 2, the sorting is done with 1 thread
//-----------------------------------------------------------------------------
template <class Iter_t, class Compare>
parallel_stable_sort <Iter_t, Compare>
::parallel_stable_sort (Iter_t first, Iter_t last, Compare comp,
                        uint32_t nthread) : nelem(0), ptr(nullptr)
{
    range<Iter_t> range_initial(first, last);
    assert(range_initial.valid());

    nelem = range_initial.size();
    size_t nptr = (nelem + 1) >> 1;

    if (nelem < nelem_min or nthread < 2)
    {
        bss::spinsort<Iter_t, Compare>
            (range_initial.first, range_initial.last, comp);
        return;
    };

    //------------------- check if sort --------------------------------------
    bool sw = true;
    for (Iter_t it1 = first, it2 = first + 1;
         it2 != last and (sw = not comp(*it2, *it1)); it1 = it2++);
    if (sw) return;

    //------------------- check if reverse sort ---------------------------
    sw = true;
    for (Iter_t it1 = first, it2 = first + 1;
         it2 != last and (sw = comp(*it2, *it1)); it1 = it2++);
    if (sw)
    {
        size_t nelem2 = nelem >> 1;
        Iter_t it1 = first, it2 = last - 1;
        for (size_t i = 0; i < nelem2; ++i)
            std::swap(*(it1++), *(it2--));
        return;
    };

    ptr = std::get_temporary_buffer<value_t>(nptr).first;
    if (ptr == nullptr) throw std::bad_alloc();

    //---------------------------------------------------------------------
    //     Parallel Process
    //---------------------------------------------------------------------
    range<Iter_t> range_first(range_initial.first, range_initial.first + nptr);

    range<Iter_t> range_second(range_initial.first + nptr, range_initial.last);

    range<value_t *> range_buffer(ptr, ptr + nptr);

    try
    {
        sample_sort<Iter_t, Compare>
            (range_initial.first, range_initial.first + nptr,
             comp, nthread, range_buffer);
    } catch (std::bad_alloc &)
    {
        destroy_all();
        throw std::bad_alloc();
    };

    try
    {
        sample_sort<Iter_t, Compare>
            (range_initial.first + nptr,
             range_initial.last, comp, nthread, range_buffer);
    } catch (std::bad_alloc &)
    {
        destroy_all();
        throw std::bad_alloc();
    };

    range_buffer = move_construct(range_buffer, range_first);
    range_initial = merge_half(range_initial, range_buffer, range_second, comp);
    destroy (range_buffer);


    
}; // end of constructor

//
//****************************************************************************
};//    End namespace stable_detail
//****************************************************************************
//

//---------------------------------------------------------------------------
//                    USING SENTENCES
//---------------------------------------------------------------------------
namespace bsc = boost::sort::common;
namespace bscu = bsc::util;
namespace bss = boost::sort::spin_detail;
using bsc::range;
using bsc::merge_half;
//
//############################################################################
//                                                                          ##
//                                                                          ##
//            P A R A L L E L _ S T A B L E _ S O R T                       ##
//                                                                          ##
//                                                                          ##
//############################################################################
//
//-----------------------------------------------------------------------------
//  function : parallel_stable_sort
/// @brief : parallel stable sort with 2 parameters
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
//-----------------------------------------------------------------------------
template<class Iter_t>
void parallel_stable_sort(Iter_t first, Iter_t last)
{
    typedef bscu::compare_iter<Iter_t> Compare;
    stable_detail::parallel_stable_sort<Iter_t, Compare>(first, last);
};
//
//-----------------------------------------------------------------------------
//  function : parallel_stable_sort
/// @brief parallel stable sort with 3 parameters. The third is the number 
///        of threads
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param nthread : Number of threads to use in the process. When this value
///                  is lower than 2, the sorting is done with 1 thread
//-----------------------------------------------------------------------------
template<class Iter_t>
void parallel_stable_sort(Iter_t first, Iter_t last, uint32_t nthread)
{
    typedef bscu::compare_iter<Iter_t> Compare;
    stable_detail::parallel_stable_sort<Iter_t, Compare>(first, last, nthread);
};
//
//-----------------------------------------------------------------------------
//  function : parallel_stable_sort
/// @brief : parallel stable sort with 3 parameters. The thisrd is the 
///          comparison object
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param comp : object for to compare two elements pointed by Iter_t
///               iterators
//-----------------------------------------------------------------------------
template <class Iter_t, class Compare,
          bscu::enable_if_not_integral<Compare> * = nullptr>
void parallel_stable_sort(Iter_t first, Iter_t last, Compare comp)
{
    stable_detail::parallel_stable_sort<Iter_t, Compare>(first, last, comp);
};

//
//-----------------------------------------------------------------------------
//  function : parallel_stable_sort
/// @brief : parallel stable sort with 3 parameters. 
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param comp : object for to compare two elements pointed by Iter_t
///               iterators
/// @param nthread : Number of threads to use in the process. When this value
///                  is lower than 2, the sorting is done with 1 thread
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare>
void parallel_stable_sort (Iter_t first, Iter_t last, Compare comp,
                          uint32_t nthread)
{
    stable_detail::parallel_stable_sort<Iter_t, Compare>
                                                (first, last, comp, nthread);
}
//
//****************************************************************************
};//    End namespace sort
};//    End namespace boost
//****************************************************************************
//
#endif
