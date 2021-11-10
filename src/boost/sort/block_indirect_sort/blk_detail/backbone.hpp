//----------------------------------------------------------------------------
/// @file backbone.hpp
/// @brief This file constains the class backbone, which is part of the
///        block_indirect_sort algorithm
///
/// @author Copyright (c) 2016 Francisco Jose Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_DETAIL_BACKBONE_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_BACKBONE_HPP

#include <atomic>
#include <boost/sort/pdqsort/pdqsort.hpp>
#include <boost/sort/common/util/atomic.hpp>
#include <boost/sort/common/util/algorithm.hpp>
#include <boost/sort/common/stack_cnc.hpp>
#include <future>
#include <iostream>
#include <iterator>

#include <boost/sort/block_indirect_sort/blk_detail/block.hpp>

namespace boost
{
namespace sort
{
namespace blk_detail
{

//---------------------------------------------------------------------------
//                 USING SENTENCES
//---------------------------------------------------------------------------
namespace bsc = boost::sort::common;
namespace bscu = bsc::util;
using bsc::stack_cnc;
using bsc::range;

///---------------------------------------------------------------------------
/// @struct backbone
/// @brief This contains all the information shared betwen the classes of the
///        block indirect sort algorithm

//----------------------------------------------------------------------------
template < uint32_t Block_size, class Iter_t, class Compare >
struct backbone
{
    //-------------------------------------------------------------------------
    //                  D E F I N I T I O N S
    //-------------------------------------------------------------------------
    typedef typename std::iterator_traits< Iter_t >::value_type value_t;
    typedef std::atomic< uint32_t >                             atomic_t;
    typedef range< size_t >                                     range_pos;
    typedef range< Iter_t >                                     range_it;
    typedef range< value_t * >                                  range_buf;
    typedef std::function< void(void) >                         function_t;
    typedef block< Block_size, Iter_t >                         block_t;

    //------------------------------------------------------------------------
    //                V A R I A B L E S
    //------------------------------------------------------------------------
    // range with all the element to sort
    range< Iter_t > global_range;

    // index vector of block_pos elements
    std::vector< block_pos > index;

    // Number of elements to sort
    size_t nelem;

    // Number of blocks to sort
    size_t nblock;

    // Number of elements in the last block (tail)
    size_t ntail;

    // object for to compare two elements
    Compare cmp;

    // range  of elements of the last block (tail)
    range_it range_tail;

    // thread local varible. It is a pointer to the buffer
    static thread_local value_t *buf;

    // concurrent stack where store the function_t elements
    stack_cnc< function_t > works;

    // global indicator of error
    bool error;
    //
    //------------------------------------------------------------------------
    //                F U N C T I O N S
    //------------------------------------------------------------------------
    backbone (Iter_t first, Iter_t last, Compare comp);

    //------------------------------------------------------------------------
    //  function : get_block
    /// @brief obtain the block in the position pos
    /// @param pos : position of the range
    /// @return block required
    //------------------------------------------------------------------------
    block_t get_block (size_t pos) const
    {
        return block_t (global_range.first + (pos * Block_size));
    };
    //-------------------------------------------------------------------------
    //  function : get_range
    /// @brief obtain the range in the position pos
    /// @param pos : position of the range
    /// @return range required
    //-------------------------------------------------------------------------
    range_it get_range (size_t pos) const
    {
        Iter_t it1 = global_range.first + (pos * Block_size);
        Iter_t it2 =
            (pos == (nblock - 1)) ? global_range.last : it1 + Block_size;
        return range_it (it1, it2);
    };
    //-------------------------------------------------------------------------
    //  function : get_range_buf
    /// @brief obtain the auxiliary buffer of the thread
    //-------------------------------------------------------------------------
    range_buf get_range_buf ( ) const
    {
        return range_buf (buf, buf + Block_size);
    };

    //-------------------------------------------------------------------------
    //  function : exec
    /// @brief Initialize the thread local buffer with the ptr_buf pointer,
    ///        and begin with the execution of the functions stored in works
    //
    /// @param ptr_buf : Pointer to the memory assigned to the thread_local
    ///                  buffer
    /// @param counter : atomic counter for to invoke to the exec function
    ///                  with only 1 parameter
    //-------------------------------------------------------------------------
    void exec (value_t *ptr_buf, atomic_t &counter)
    {
        buf = ptr_buf;
        exec (counter);
    };

    void exec (atomic_t &counter);

//---------------------------------------------------------------------------
}; // end struct backbone
//---------------------------------------------------------------------------
//
//############################################################################
//                                                                          ##
//                                                                          ##
//            N O N     I N L I N E      F U N C T I O N S                  ##
//                                                                          ##
//                                                                          ##
//############################################################################
//
// initialization of the thread_local pointer to the auxiliary buffer
template < uint32_t Block_size, class Iter_t, class Compare >
thread_local typename std::iterator_traits< Iter_t >
::value_type *backbone< Block_size, Iter_t, Compare >::buf = nullptr;

//------------------------------------------------------------------------
//  function : backbone
/// @brief constructor of the class
//
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param comp : object for to compare two elements pointed by Iter_t
///               iterators
//------------------------------------------------------------------------
template < uint32_t Block_size, class Iter_t, class Compare >
backbone< Block_size, Iter_t, Compare >
::backbone (Iter_t first, Iter_t last, Compare comp)
: global_range (first, last), cmp (comp), error (false)
{
    assert ((last - first) >= 0);
    if (first == last) return; // nothing to do

    nelem = size_t (last - first);
    nblock = (nelem + Block_size - 1) / Block_size;
    ntail = (nelem % Block_size);
    index.reserve (nblock + 1);

    for (size_t i = 0; i < nblock; ++i) index.emplace_back (block_pos (i));

    range_tail.first =
        (ntail == 0) ? last : (first + ((nblock - 1) * Block_size));
    range_tail.last = last;
};
//
//-------------------------------------------------------------------------
//  function : exec
/// @brief execute the function_t stored in works, until counter is zero
//
/// @param counter : atomic counter. When 0 exits the function
//-------------------------------------------------------------------------
template < uint32_t Block_size, class Iter_t, class Compare >
void backbone< Block_size, Iter_t, Compare >::exec (atomic_t &counter)
{
    function_t func_exec;
    while (bscu::atomic_read (counter) != 0)
    {
        if (works.pop_move_back (func_exec)) func_exec ( );
        else std::this_thread::yield ( );
    };
};
//
//****************************************************************************
}; //    End namespace blk_detail
}; //    End namespace sort
}; //    End namespace boost
//****************************************************************************
#endif
