//----------------------------------------------------------------------------
/// @file block.hpp
/// @brief This file contains the internal data structures used in the
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
#ifndef __BOOST_SORT_PARALLEL_DETAIL_BLOCK_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_BLOCK_HPP

#include <boost/sort/common/range.hpp>

namespace boost
{
namespace sort
{
namespace blk_detail
{
//---------------------------------------------------------------------------
//                 USING SENTENCES
//---------------------------------------------------------------------------
using namespace boost::sort::common;
//
//---------------------------------------------------------------------------
/// @struct block_pos
/// @brief represent a pair of values, a position represented as an unsigned
///        variable ( position ), and a bool variable ( side ). They are packed
///        in a size_t variable. The Least Significant Bit is the bool variable,
///        and the others bits are the position
//----------------------------------------------------------------------------
class block_pos
{
    //------------------------------------------------------------------------
    //                   VARIABLES
    //-----------------------------------------------------------------------
    size_t num; // number which store a position and a bool side

  public:
    //----------------------------- FUNCTIONS ------------------------------
    block_pos (void) : num (0){};
    //
    //-------------------------------------------------------------------------
    //  function : block_pos
    /// @brief constructor from a position and a side
    /// @param position : position to sotre
    /// @param side : side to store
    //-------------------------------------------------------------------------
    block_pos (size_t position, bool side = false)
    {
        num = (position << 1) + ((side) ? 1 : 0);
    };
    //
    //-------------------------------------------------------------------------
    //  function : pos
    /// @brief obtain the position stored inside the block_pos
    /// @return position
    //-------------------------------------------------------------------------
    size_t pos (void) const { return (num >> 1); };
    //
    //-------------------------------------------------------------------------
    //  function : pos
    /// @brief store a position inside the block_pos
    /// @param position : value to store
    //-------------------------------------------------------------------------
    void set_pos (size_t position) { num = (position << 1) + (num & 1); };
    //
    //-------------------------------------------------------------------------
    //  function : side
    /// @brief obtain the side stored inside the block_pos
    /// @return bool value
    //-------------------------------------------------------------------------
    bool side (void) const { return ((num & 1) != 0); };
    //
    //-------------------------------------------------------------------------
    //  function : side
    /// @brief store a bool value the block_pos
    /// @param sd : bool value to store
    //-------------------------------------------------------------------------
    void set_side (bool sd) { num = (num & ~1) + ((sd) ? 1 : 0); };
}; // end struct block_pos

//
//---------------------------------------------------------------------------
/// @struct block
/// @brief represent a group of Block_size contiguous elements, beginning
///        with the pointed by first
//----------------------------------------------------------------------------
template < uint32_t Block_size, class Iter_t >
struct block
{
    //----------------------------------------------------------------------
    //                     VARIABLES
    //----------------------------------------------------------------------
    Iter_t first; // iterator to the first element of the block

    //-------------------------------------------------------------------------
    //  function : block
    /// @brief constructor from an iterator to the first element of the block
    /// @param it : iterator to the first element of the block
    //-------------------------------------------------------------------------
    block (Iter_t it) : first (it){};

    //-------------------------------------------------------------------------
    //  function : get_range
    /// @brief convert a block in a range
    /// @return range
    //-------------------------------------------------------------------------
    range< Iter_t > get_range (void)
    {
        return range_it (first, first + Block_size);
    };

}; // end struct block

//
//-------------------------------------------------------------------------
//  function : compare_block
/// @brief compare two blocks using the content of the pointed by first
/// @param block1 : first block to compare
/// @param block2 : second block to compare
/// @param cmp : comparison operator
//-------------------------------------------------------------------------
template < uint32_t Block_size, class Iter_t, class Compare >
bool compare_block (block< Block_size, Iter_t > block1,
                    block< Block_size, Iter_t > block2,
                    Compare cmp = Compare ( ))
{
    return cmp (*block1.first, *block2.first);
};
//
///---------------------------------------------------------------------------
/// @struct compare_block_pos
/// @brief This is a object for to compare two block_pos objects
//----------------------------------------------------------------------------
template < uint32_t Block_size, class Iter_t, class Compare >
struct compare_block_pos
{
    //-----------------------------------------------------------------------
    //                        VARIABLES
    //-----------------------------------------------------------------------
    Iter_t global_first; // iterator to the first element to sort
    Compare comp;        // comparison object for to compare two elements

    //-------------------------------------------------------------------------
    //  function : compare_block_pos
    /// @brief constructor
    /// @param g_first : itertor to the first element to sort
    /// @param cmp : comparison operator
    //-------------------------------------------------------------------------
    compare_block_pos (Iter_t g_first, Compare cmp)
        : global_first (g_first), comp (cmp){};
    //
    //-------------------------------------------------------------------------
    //  function : operator ()
    /// @brief compare two blocks using the content of the pointed by
    ///        global_first
    /// @param block_pos1 : first block to compare
    /// @param block_pos2 : second block to compare
    //-------------------------------------------------------------------------
    bool operator( ) (block_pos block_pos1, block_pos block_pos2) const
    {
        return comp (*(global_first + (block_pos1.pos ( ) * Block_size)),
                     *(global_first + (block_pos2.pos ( ) * Block_size)));
    };

}; // end struct compare_block_pos

//****************************************************************************
}; //    End namespace blk_detail
}; //    End namespace sort
}; //    End namespace boost
//****************************************************************************
//
#endif
