//----------------------------------------------------------------------------
/// @file merge_blocks.hpp
/// @brief contains the class merge_blocks, which is part of the
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
#ifndef __BOOST_SORT_PARALLEL_DETAIL_MERGE_BLOCKS_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_MERGE_BLOCKS_HPP

#include <atomic>
#include <boost/sort/block_indirect_sort/blk_detail/backbone.hpp>
#include <boost/sort/common/range.hpp>
#include <future>
#include <iostream>
#include <iterator>

namespace boost
{
namespace sort
{
namespace blk_detail
{
//----------------------------------------------------------------------------
//                          USING SENTENCES
//----------------------------------------------------------------------------
namespace bsc = boost::sort::common;
namespace bscu = bsc::util;
using bsc::range;
using bsc::is_mergeable;
using bsc::merge_uncontiguous;
//
///---------------------------------------------------------------------------
/// @struct merge_blocks
/// @brief This class merge the blocks. The blocks to merge are defined by two
///        ranges of positions in the index of the backbone
//----------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
struct merge_blocks
{
    //-----------------------------------------------------------------------
    //                  D E F I N I T I O N S
    //-----------------------------------------------------------------------
    typedef typename std::iterator_traits<Iter_t>::value_type value_t;
    typedef std::atomic<uint32_t> atomic_t;
    typedef range<size_t> range_pos;
    typedef range<Iter_t> range_it;
    typedef range<value_t *> range_buf;
    typedef std::function<void(void)> function_t;
    typedef backbone<Block_size, Iter_t, Compare> backbone_t;
    typedef compare_block_pos<Block_size, Iter_t, Compare> compare_block_pos_t;

    //------------------------------------------------------------------------
    //                V A R I A B L E S
    //------------------------------------------------------------------------
    // Object with the elements to sort and all internal data structures of the
    // algorithm
    backbone_t &bk;
    //
    //------------------------------------------------------------------------
    //                F U N C T I O N S
    //------------------------------------------------------------------------
    merge_blocks(backbone_t &bkb, size_t pos_index1, size_t pos_index2,
                    size_t pos_index3);

    void tail_process(std::vector<block_pos> &vblkpos1,
                    std::vector<block_pos> &vblkpos2);

    void cut_range(range_pos rng);

    void merge_range_pos(range_pos rng);

    void extract_ranges(range_pos range_input);
    //
    //------------------------------------------------------------------------
    //  function : function_merge_range_pos
    /// @brief create a function_t with a call to merge_range_pos, and insert
    ///        in the stack of the backbone
    //
    /// @param rng_input : range of positions of blocks in the index to merge
    /// @param son_counter : atomic variable which is decremented when finish
    ///                      the function. This variable is used for to know
    ///                      when are finished all the function_t created
    ///                      inside an object
    /// @param error : global indicator of error.
    ///
    //------------------------------------------------------------------------
    void function_merge_range_pos(const range_pos &rng_input, atomic_t &counter,
                    bool &error)
    {
        bscu::atomic_add(counter, 1);
        function_t f1 = [this, rng_input, &counter, &error]( ) -> void
        {
            if (not error)
            {
                try
                {
                    this->merge_range_pos (rng_input);
                }
                catch (std::bad_alloc &ba)
                {
                    error = true;
                };
            }
            bscu::atomic_sub (counter, 1);
        };
        bk.works.emplace_back(f1);
    }
    ;
    //
    //------------------------------------------------------------------------
    //  function : function_cut_range
    /// @brief create a function_t with a call to cut_range, and inser in
    ///        the stack of the backbone
    //
    /// @param rng_input : range of positions in the index to cut
    /// @param counter : atomic variable which is decremented when finish
    ///                  the function. This variable is used for to know
    ///                  when are finished all the function_t created
    ///                  inside an object
    /// @param error : global indicator of error.
    //------------------------------------------------------------------------
    void function_cut_range(const range_pos &rng_input, atomic_t &counter,
                    bool &error)
    {
        bscu::atomic_add(counter, 1);
        function_t f1 = [this, rng_input, &counter, &error]( ) -> void
        {
            if (not error)
            {
                try
                {
                    this->cut_range (rng_input);
                }
                catch (std::bad_alloc &)
                {
                    error = true;
                };
            }
            bscu::atomic_sub (counter, 1);
        };
        bk.works.emplace_back(f1);
    }


//----------------------------------------------------------------------------
};
// end struct merge_blocks
//----------------------------------------------------------------------------
//
//############################################################################
//                                                                          ##
//                                                                          ##
//            N O N     I N L I N E      F U N C T I O N S                  ##
//                                                                          ##
//                                                                          ##
//############################################################################
//
//-------------------------------------------------------------------------
//  function : merge_blocks
/// @brief make the indirect merge of the two range_pos defined by their index
///        position [pos_index1, pos_index2 ) and [ pos_index2, pos_index3 )
//
/// @param bkb : backbone with all the data to sort , and the internal data
///              structures of the algorithm
/// @param pos_index1 : first position of the first range in the index
/// @param pos_index2 : last position of the first range and first position
///                     of the second range in the index
/// @param pos_index3 : last position of the second range in the index
//-------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
merge_blocks<Block_size, Group_size, Iter_t, Compare>
::merge_blocks( backbone_t &bkb, size_t pos_index1, size_t pos_index2,
                size_t pos_index3) : bk(bkb)
{
    size_t nblock1 = pos_index2 - pos_index1;
    size_t nblock2 = pos_index3 - pos_index2;
    if (nblock1 == 0 or nblock2 == 0) return;

    //-----------------------------------------------------------------------
    // Merging of the two intervals
    //-----------------------------------------------------------------------
    std::vector<block_pos> vpos1, vpos2;
    vpos1.reserve(nblock1 + 1);
    vpos2.reserve(nblock2 + 1);

    for (size_t i = pos_index1; i < pos_index2; ++i)
    {
        vpos1.emplace_back(bk.index[i].pos(), true);
    };

    for (size_t i = pos_index2; i < pos_index3; ++i)
    {
        vpos2.emplace_back(bk.index[i].pos(), false);
    };
    //-------------------------------------------------------------------
    //  tail process
    //-------------------------------------------------------------------
    if (vpos2.back().pos() == (bk.nblock - 1)
                    and bk.range_tail.first != bk.range_tail.last)
    {
        tail_process(vpos1, vpos2);
        nblock1 = vpos1.size();
        nblock2 = vpos2.size();
    };

    compare_block_pos_t cmp_blk(bk.global_range.first, bk.cmp);
    if (bk.error) return;
    bscu::merge(vpos1.begin(), vpos1.end(), vpos2.begin(), vpos2.end(),
                    bk.index.begin() + pos_index1, cmp_blk);
    if (bk.error) return;
    // Extracting the ranges for to merge the elements
    extract_ranges(range_pos(pos_index1, pos_index1 + nblock1 + nblock2));
}


//
//-------------------------------------------------------------------------
//  function : tail_process
/// @brief make the process when the second vector of block_pos to merge is
///        the last, and have an incomplete block ( tail)
//
/// @param vblkpos1 : first vector of block_pos elements to merge
/// @param vblkpos2 : second vector of block_pos elements to merge
//-------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
void merge_blocks<Block_size, Group_size, Iter_t, Compare>
::tail_process( std::vector<block_pos> &vblkpos1,
                std::vector<block_pos> &vblkpos2 )
{
    if (vblkpos1.size() == 0 or vblkpos2.size() == 0) return;

    vblkpos2.pop_back();

    size_t posback1 = vblkpos1.back().pos();
    range_it range_back1 = bk.get_range(posback1);

    if (bsc::is_mergeable(range_back1, bk.range_tail, bk.cmp))
    {
        bsc::merge_uncontiguous(range_back1, bk.range_tail, bk.get_range_buf(),
                        bk.cmp);
        if (vblkpos1.size() > 1)
        {
            size_t pos_aux = vblkpos1[vblkpos1.size() - 2].pos();
            range_it range_aux = bk.get_range(pos_aux);

            if (bsc::is_mergeable(range_aux, range_back1, bk.cmp))
            {
                vblkpos2.emplace_back(posback1, false);
                vblkpos1.pop_back();
            };
        };
    };
}

//
//-------------------------------------------------------------------------
//  function : cut_range
/// @brief when the rng_input is greather than Group_size, this function divide
///        it in several parts creating function_t elements, which are inserted
///        in the concurrent stack of the backbone
//
/// @param rng_input : range to divide
//-------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
void merge_blocks<Block_size, Group_size, Iter_t, Compare>
::cut_range(range_pos rng_input)
{
    if (rng_input.size() < Group_size)
    {
        merge_range_pos(rng_input);
        return;
    };

    atomic_t counter(0);
    size_t npart = (rng_input.size() + Group_size - 1) / Group_size;
    size_t size_part = rng_input.size() / npart;

    size_t pos_ini = rng_input.first;
    size_t pos_last = rng_input.last;

    while (pos_ini < pos_last)
    {
        size_t pos = pos_ini + size_part;
        while (pos < pos_last
                        and bk.index[pos - 1].side() == bk.index[pos].side())
        {
            ++pos;
        };
        if (pos < pos_last)
        {
            merge_uncontiguous(bk.get_range(bk.index[pos - 1].pos()),
                            bk.get_range(bk.index[pos].pos()),
                            bk.get_range_buf(), bk.cmp);
        }
        else pos = pos_last;
        if ((pos - pos_ini) > 1)
        {
            range_pos rng_aux(pos_ini, pos);
            function_merge_range_pos(rng_aux, counter, bk.error);
        };
        pos_ini = pos;
    };
    bk.exec(counter); // wait until finish all the ranges
}


//
//-------------------------------------------------------------------------
//  function : merge_range_pos
/// @brief make the indirect merge of the blocks inside the rng_input
//
/// @param rng_input : range of positions of the blocks to merge
//-------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
void merge_blocks<Block_size, Group_size, Iter_t, Compare>
::merge_range_pos(range_pos rng_input)
{
    if (rng_input.size() < 2) return;
    range_buf rbuf = bk.get_range_buf();

    range_it rng_prev = bk.get_range(bk.index[rng_input.first].pos());
    move_forward(rbuf, rng_prev);
    range_it rng_posx(rng_prev);

    for (size_t posx = rng_input.first + 1; posx != rng_input.last; ++posx)
    {
        rng_posx = bk.get_range(bk.index[posx].pos());
        bsc::merge_flow(rng_prev, rbuf, rng_posx, bk.cmp);
        rng_prev = rng_posx;

    };
    move_forward(rng_posx, rbuf);
}
//
//-------------------------------------------------------------------------
//  function : extract_ranges
/// @brief from a big range of positions of blocks in the index. Examine which
///        are mergeable, and generate a couple of ranges for to be merged.
///        With the ranges obtained generate function_t elements and are
///        inserted in the concurrent stack.
///        When the range obtained is smaller than Group_size, generate a
///        function_t calling to merge_range_pos, when is greater, generate a
///        function_t calling to cut_range
//
/// @param rpos range_input : range of the position in the index, where must
///                           extract the ranges to merge
//-------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
void merge_blocks<Block_size, Group_size, Iter_t, Compare>
::extract_ranges(range_pos range_input)
{
    if (range_input.size() < 2) return;
    atomic_t counter(0);

    // The names with x are positions of the index
    size_t posx_ini = range_input.first;
    block_pos bp_posx_ini = bk.index[posx_ini];

    range_it rng_max = bk.get_range(bp_posx_ini.pos());
    bool side_max = bp_posx_ini.side();

    block_pos bp_posx;
    range_it rng_posx = rng_max;
    bool side_posx = side_max;

    for (size_t posx = posx_ini + 1; posx <= range_input.last; ++posx)
    {
        bool final = (posx == range_input.last);
        bool mergeable = false;

        if (not final)
        {
            bp_posx = bk.index[posx];
            rng_posx = bk.get_range(bp_posx.pos());
            side_posx = bp_posx.side();
            mergeable = (side_max != side_posx
                            and is_mergeable(rng_max, rng_posx, bk.cmp));
        };
        if (bk.error) return;
        if (final or not mergeable)
        {
            range_pos rp_final(posx_ini, posx);
            if (rp_final.size() > 1)
            {
                if (rp_final.size() > Group_size)
                {
                    function_cut_range(rp_final, counter, bk.error);
                }
                else
                {
                    function_merge_range_pos(rp_final, counter, bk.error);
                };
            };
            posx_ini = posx;
            if (not final)
            {
                rng_max = rng_posx;
                side_max = side_posx;
            };
        }
        else
        {
            if (bk.cmp(*(rng_max.back()), *(rng_posx.back())))
            {
                rng_max = rng_posx;
                side_max = side_posx;
            };
        };
    };
    bk.exec(counter);
}
//
//****************************************************************************
}; //    End namespace blk_detail
}; //    End namespace sort
}; //    End namespace boost
//****************************************************************************
//
#endif
