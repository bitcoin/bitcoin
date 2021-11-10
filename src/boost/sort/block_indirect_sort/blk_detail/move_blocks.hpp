//----------------------------------------------------------------------------
/// @file move_blocks.hpp
/// @brief contains the class move_blocks, which is part of the
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
#ifndef __BOOST_SORT_PARALLEL_DETAIL_MOVE_BLOCKS_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_MOVE_BLOCKS_HPP

#include <atomic>
#include <boost/sort/block_indirect_sort/blk_detail/backbone.hpp>
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
//
///---------------------------------------------------------------------------
/// @struct move_blocks
/// @brief This class move the blocks, trnasforming a logical sort by an index,
///        in physical sort
//----------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
struct move_blocks
{
    //-------------------------------------------------------------------------
    //                  D E F I N I T I O N S
    //-------------------------------------------------------------------------
    typedef move_blocks<Block_size, Group_size, Iter_t, Compare> this_type;
    typedef typename std::iterator_traits<Iter_t>::value_type value_t;
    typedef std::atomic<uint32_t> atomic_t;
    typedef bsc::range<size_t> range_pos;
    typedef bsc::range<Iter_t> range_it;
    typedef bsc::range<value_t *> range_buf;
    typedef std::function<void(void)> function_t;
    typedef backbone<Block_size, Iter_t, Compare> backbone_t;

    //------------------------------------------------------------------------
    //                V A R I A B L E S
    //------------------------------------------------------------------------
    // Object with the elements to sort and all internal data structures of the
    // algorithm
    backbone_t &bk;

    //------------------------------------------------------------------------
    //                F U N C T I O N S
    //------------------------------------------------------------------------
    move_blocks(backbone_t &bkb);

    void move_sequence(const std::vector<size_t> &init_sequence);

    void move_long_sequence(const std::vector<size_t> &init_sequence);
    //
    //------------------------------------------------------------------------
    //  function : function_move_sequence
    /// @brief create a function_t with a call to move_sequence, and insert
    ///        in the stack of the backbone
    ///
    /// @param sequence :sequence of positions for to move the blocks
    /// @param counter : atomic variable which is decremented when finish
    ///                  the function. This variable is used for to know
    ///                  when are finished all the function_t created
    ///                  inside an object
    /// @param error : global indicator of error.
    //------------------------------------------------------------------------
    void function_move_sequence(std::vector<size_t> &sequence,
                                atomic_t &counter, bool &error)
    {
        bscu::atomic_add(counter, 1);
        function_t f1 = [this, sequence, &counter, &error]( ) -> void
        {
            if (not error)
            {
                try
                {
                    this->move_sequence (sequence);
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

    //
    //------------------------------------------------------------------------
    //  function : function_move_long_sequence
    /// @brief create a function_t with a call to move_long_sequence, and
    ///        insert in the stack of the backbone
    //
    /// @param sequence :sequence of positions for to move the blocks
    /// @param counter : atomic variable which is decremented when finish
    ///                  the function. This variable is used for to know
    ///                  when are finished all the function_t created
    ///                  inside an object
    /// @param error : global indicator of error.
    //------------------------------------------------------------------------
    void function_move_long_sequence(std::vector<size_t> &sequence,
                                     atomic_t &counter, bool &error)
    {
        bscu::atomic_add(counter, 1);
        function_t f1 = [this, sequence, &counter, &error]( ) -> void
        {
            if (not error)
            {
                try
                {
                    this->move_long_sequence (sequence);
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
    ;
//---------------------------------------------------------------------------
}; // end of struct move_blocks
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
//-------------------------------------------------------------------------
//  function : move_blocks
/// @brief constructor of the class for to move the blocks to their true
///        position obtained from the index
//
/// @param bkb : backbone with the index and the blocks
//-------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
move_blocks<Block_size, Group_size, Iter_t, Compare>
::move_blocks(backbone_t &bkb) : bk(bkb)
{
    std::vector<std::vector<size_t> > vsequence;
    vsequence.reserve(bk.index.size() >> 1);
    std::vector<size_t> sequence;
    atomic_t counter(0);

    size_t pos_index_ini = 0, pos_index_src = 0, pos_index_dest = 0;
    while (pos_index_ini < bk.index.size())
    {
        while (pos_index_ini < bk.index.size()
                        and bk.index[pos_index_ini].pos() == pos_index_ini)
        {
            ++pos_index_ini;
        };

        if (pos_index_ini == bk.index.size()) break;

        sequence.clear();
        pos_index_src = pos_index_dest = pos_index_ini;
        sequence.push_back(pos_index_ini);

        while (bk.index[pos_index_dest].pos() != pos_index_ini)
        {
            pos_index_src = bk.index[pos_index_dest].pos();
            sequence.push_back(pos_index_src);

            bk.index[pos_index_dest].set_pos(pos_index_dest);
            pos_index_dest = pos_index_src;
        };

        bk.index[pos_index_dest].set_pos(pos_index_dest);
        vsequence.push_back(sequence);

        if (sequence.size() < Group_size)
        {
            function_move_sequence(vsequence.back(), counter, bk.error);
        }
        else
        {
            function_move_long_sequence(vsequence.back(), counter, bk.error);
        };
    };
    bk.exec(counter);
}
;
//
//-------------------------------------------------------------------------
//  function : move_sequence
/// @brief move the blocks, following the positions of the init_sequence
//
/// @param init_sequence : vector with the positions from and where move the
///                        blocks
//-------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
void move_blocks<Block_size, Group_size, Iter_t, Compare>
::move_sequence(const std::vector<size_t> &init_sequence)
{
    range_buf rbuf = bk.get_range_buf();
    size_t pos_range2 = init_sequence[0];

    range_it range2 = bk.get_range(pos_range2);
    move_forward(rbuf, range2);

    for (size_t i = 1; i < init_sequence.size(); ++i)
    {
        pos_range2 = init_sequence[i];
        range_it range1(range2);
        range2 = bk.get_range(pos_range2);
        move_forward(range1, range2);
    };
    move_forward(range2, rbuf);
};
//
//-------------------------------------------------------------------------
//  function : move_long_sequence
/// @brief move the blocks, following the positions of the init_sequence.
///        if the sequence is greater than Group_size, it is divided in small
///        sequences, creating function_t elements, for to be inserted in the
///        concurrent stack
//
/// @param init_sequence : vector with the positions from and where move the
///                        blocks
//-------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
void move_blocks<Block_size, Group_size, Iter_t, Compare>
::move_long_sequence(const std::vector<size_t> &init_sequence)
{
    if (init_sequence.size() < Group_size) return move_sequence(init_sequence);

    size_t npart = (init_sequence.size() + Group_size - 1) / Group_size;
    size_t size_part = init_sequence.size() / npart;
    atomic_t son_counter(0);

    std::vector<size_t> sequence;
    sequence.reserve(size_part);

    std::vector<size_t> index_seq;
    index_seq.reserve(npart);

    auto it_pos = init_sequence.begin();
    for (size_t i = 0; i < (npart - 1); ++i, it_pos += size_part)
    {
        sequence.assign(it_pos, it_pos + size_part);
        index_seq.emplace_back(*(it_pos + size_part - 1));
        function_move_sequence(sequence, son_counter, bk.error);
    };

    sequence.assign(it_pos, init_sequence.end());
    index_seq.emplace_back(init_sequence.back());
    function_move_sequence(sequence, son_counter, bk.error);

    bk.exec(son_counter);
    if (bk.error) return;
    move_long_sequence(index_seq);
}

//
//****************************************************************************
}; //    End namespace blk_detail
}; //    End namespace sort
}; //    End namespace boost
//****************************************************************************
//
#endif
