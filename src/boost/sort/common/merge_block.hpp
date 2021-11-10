//----------------------------------------------------------------------------
/// @file merge_block.hpp
/// @brief This file constains the class merge_block, which is part of the
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
#ifndef __BOOST_SORT_COMMON_MERGE_BLOCK_HPP
#define __BOOST_SORT_COMMON_MERGE_BLOCK_HPP

#include <boost/sort/common/range.hpp>
#include <boost/sort/common/rearrange.hpp>
#include <boost/sort/common/util/merge.hpp>
#include <boost/sort/common/util/traits.hpp>

namespace boost
{
namespace sort
{
namespace common
{
///---------------------------------------------------------------------------
/// @struct merge_block
/// @brief This contains all the information shared betwen the classes of the
///        block indirect sort algorithm

//----------------------------------------------------------------------------
template<class Iter_t, class Compare, uint32_t Power2 = 10>
struct merge_block
{
    //-------------------------------------------------------------------------
    //                  D E F I N I T I O N S
    //-------------------------------------------------------------------------
    typedef util::value_iter<Iter_t>                    value_t;
    typedef range<size_t>                               range_pos;
    typedef range<Iter_t>                               range_it;
    typedef range<value_t *>                            range_buf;
    typedef typename std::vector<size_t>::iterator      it_index;
    typedef util::circular_buffer<value_t, Power2 + 1>  circular_t;

    //------------------------------------------------------------------------
    //                          CONSTANTS
    //------------------------------------------------------------------------
    const size_t BLOCK_SIZE = (size_t) 1 << Power2;
    const size_t LOG_BLOCK = Power2;

    //------------------------------------------------------------------------
    //                V A R I A B L E S
    //------------------------------------------------------------------------
    // range with all the element to sort
    range<Iter_t> global_range;

    // index vector of block_pos elements
    std::vector<size_t> index;

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

    // circular buffer
    circular_t * ptr_circ;

    // indicate  if the circulr buffer is owned  by the data structure
    // or is received as parameter
    bool owned;

    //
    //------------------------------------------------------------------------
    //                F U N C T I O N S
    //------------------------------------------------------------------------
    //
    //------------------------------------------------------------------------
    //  function : merge_block
    /// @brief constructor of the class
    //
    /// @param first : iterator to the first element of the range to sort
    /// @param last : iterator after the last element to the range to sort
    /// @param comp : object for to compare two elements pointed by Iter_t
    ///               iterators
    //------------------------------------------------------------------------
    merge_block (Iter_t first, Iter_t last, Compare comp,
                 circular_t *pcirc_buffer)
    : global_range(first, last), cmp(comp), ptr_circ(pcirc_buffer),
      owned(pcirc_buffer == nullptr)
    {
        assert((last - first) >= 0);
        if (first == last) return; // nothing to do

        nelem = size_t(last - first);
        nblock = (nelem + BLOCK_SIZE - 1) / BLOCK_SIZE;
        ntail = (nelem % BLOCK_SIZE);
        index.reserve(nblock + 1);

        for (size_t i = 0; i < nblock; ++i)
            index.emplace_back(i);

        range_tail.first = first + ((nblock - 1) << LOG_BLOCK);
        range_tail.last = last;
        if (owned)
        {
            ptr_circ = new circular_t;
            ptr_circ->initialize(*first);
        };
    }

    merge_block(Iter_t first, Iter_t last, Compare comp)
                    : merge_block(first, last, comp, nullptr) { };

    ~ merge_block()
    {
        if (ptr_circ != nullptr and owned)
        {
            delete ptr_circ;
            ptr_circ = nullptr;
        };
    };
    //-------------------------------------------------------------------------
    //  function : get_range
    /// @brief obtain the range in the position pos
    /// @param pos : position of the range
    /// @return range required
    //-------------------------------------------------------------------------
    range_it get_range(size_t pos) const
    {
        Iter_t it1 = global_range.first + (pos << LOG_BLOCK);
        Iter_t it2 = (pos == (nblock - 1)) ?
                        global_range.last : it1 + BLOCK_SIZE;
        return range_it(it1, it2);
    };
    //-------------------------------------------------------------------------
    //  function : get_group_range
    /// @brief obtain the range of the contiguous blocks beginning in the
    //         position pos
    /// @param pos : position of the first range
    /// @param nrange : number of ranges of the group
    /// @return range required
    //-------------------------------------------------------------------------
    range_it get_group_range(size_t pos, size_t nrange) const
    {
        Iter_t it1 = global_range.first + (pos << LOG_BLOCK);

        Iter_t it2 = ((pos + nrange) == nblock)?global_range.last: global_range.first + ((pos + nrange) << LOG_BLOCK);
        //Iter_t it2 = global_range.first + ((pos + nrange) << LOG_BLOCK);
        //if ((pos + nrange) == nblock) it2 = global_range.last;

        return range_it(it1, it2);
    };
    //-------------------------------------------------------------------------
    //  function : is_tail
    /// @brief indicate if a block is the tail
    /// @param pos : position of the block
    /// @return true : taiol  false : not tail
    //-------------------------------------------------------------------------
    bool is_tail(size_t pos) const
    {
        return (pos == (nblock - 1) and ntail != 0);
    };
    //-------------------------------------------------------------------------
    //  function :
    /// @brief
    /// @param
    /// @return
    //-------------------------------------------------------------------------
    void merge_range_pos(it_index itx_first, it_index itx_mid,
                         it_index itx_last);

    //-------------------------------------------------------------------------
    //  function : move_range_pos_backward
    /// @brief Move backward the elements of a range of blocks in a index
    /// @param itx_first : iterator to the position of the first block
    /// @param  itx_last : itertor to the position of the last block
    /// @param  npos : number of positions to move. Must be less than BLOCK_SIZE
    /// @return
    //-------------------------------------------------------------------------
    void move_range_pos_backward(it_index itx_first, it_index itx_last,
                                 size_t npos);

    //-------------------------------------------------------------------------
    //  function : rearrange_with_index
    /// @brief rearrange the blocks with the relative positions of the index
    /// @param
    /// @param
    /// @param
    /// @return
    //-------------------------------------------------------------------------
    void rearrange_with_index(void);

//---------------------------------------------------------------------------
};// end struct merge_block
//---------------------------------------------------------------------------
//
//############################################################################
//                                                                          ##
//           N O N     I N L I N E     F U N C T IO N S                     ##
//                                                                          ##
//############################################################################
//
//-------------------------------------------------------------------------
//  function :
/// @brief
/// @param
/// @return
//-------------------------------------------------------------------------
template<class Iter_t, class Compare, uint32_t Power2>
void merge_block<Iter_t, Compare, Power2>
::merge_range_pos(it_index itx_first, it_index itx_mid,it_index itx_last)
{
    assert((itx_last - itx_mid) >= 0 and (itx_mid - itx_first) >= 0);

    size_t nelemA = (itx_mid - itx_first), nelemB = (itx_last - itx_mid);
    if (nelemA == 0 or nelemB == 0) return;

    //-------------------------------------------------------------------
    // Create two index with the position of the blocks to merge
    //-------------------------------------------------------------------
    std::vector<size_t> indexA, indexB;
    indexA.reserve(nelemA + 1);
    indexB.reserve(nelemB);

    indexA.insert(indexA.begin(), itx_first, itx_mid);
    indexB.insert(indexB.begin(), itx_mid, itx_last);

    it_index itx_out = itx_first;
    it_index itxA = indexA.begin(), itxB = indexB.begin();
    range_it rngA, rngB;
    Iter_t itA = global_range.first, itB = global_range.first;
    bool validA = false, validB = false;

    while (itxA != indexA.end() and itxB != indexB.end())
    {   //----------------------------------------------------------------
        // Load valid ranges from the itxA and ItxB positions
        //----------------------------------------------------------------
        if (not validA)
        {
            rngA = get_range(*itxA);
            itA = rngA.first;
            validA = true;
        };
        if (not validB)
        {
            rngB = get_range(*itxB);
            itB = rngB.first;
            validB = true;
        };
        //----------------------------------------------------------------
        // If don't have merge betweeen the  blocks, pass directly the
        // position of the block to itx_out
        //----------------------------------------------------------------
        if (ptr_circ->size() == 0)
        {
            if (not cmp(*rngB.front(), *rngA.back()))
            {
                *(itx_out++) = *(itxA++);
                validA = false;
                continue;
            };
            if (cmp(*rngB.back(), *rngA.front()))
            {
                if (not is_tail(*itxB))
                    *(itx_out++) = *itxB;
                else ptr_circ->push_move_back(rngB.first, rngB.size());
                ++itxB;
                validB = false;
                continue;
            };
        };
        //----------------------------------------------------------------
        // Normal merge
        //----------------------------------------------------------------
        bool side = util::merge_circular(itA, rngA.last, itB, rngB.last,
                        *ptr_circ, cmp, itA, itB);
        if (side)
        {   // rngA is finished
            ptr_circ->pop_move_front(rngA.first, rngA.size());
            *(itx_out++) = *(itxA++);
            validA = false;
        }
        else
        {   // rngB is finished
            if (not is_tail(*itxB))
            {
                ptr_circ->pop_move_front(rngB.first, rngB.size());
                *(itx_out++) = *itxB;
            };
            ++itxB;
            validB = false;
        };
    }; // end while

    if (itxA == indexA.end())
    {   // the index A is finished
        rngB = get_range(*itxB);
        ptr_circ->pop_move_front(rngB.first, ptr_circ->size());
        while (itxB != indexB.end())
            *(itx_out++) = *(itxB++);
    }
    else
    {   // The list B is finished
        rngA = get_range(*itxA);
        if (ntail != 0 and indexB.back() == (nblock - 1)) // exist tail
        {   // add the tail block to indexA, and shift the element
            indexA.push_back(indexB.back());
            size_t numA = size_t(itA - rngA.first);
            ptr_circ->pop_move_back(rngA.first, numA);
            move_range_pos_backward(itxA, indexA.end(), ntail);
        };

        ptr_circ->pop_move_front(rngA.first, ptr_circ->size());
        while (itxA != indexA.end())
            *(itx_out++) = *(itxA++);
    };
};

//-------------------------------------------------------------------------
//  function : move_range_pos_backward
/// @brief Move backward the elements of a range of blocks in a index
/// @param itx_first : iterator to the position of the first block
/// @param  itx_last : itertor to the position of the last block
/// @param  npos : number of positions to move. Must be less than BLOCK_SIZE
/// @return
//-------------------------------------------------------------------------
template<class Iter_t, class Compare, uint32_t Power2>
void merge_block<Iter_t, Compare, Power2>
::move_range_pos_backward(it_index itx_first, it_index itx_last, size_t npos)
{
    assert((itx_last - itx_first) >= 0 and npos <= BLOCK_SIZE);

    //--------------------------------------------------------------------
    // Processing the last block. Must be ready fore to accept npos
    // elements from the upper block
    //--------------------------------------------------------------------
    range_it rng1 = get_range(*(itx_last - 1));
    assert(rng1.size() >= npos);
    if (rng1.size() > npos)
    {
        size_t nmove = rng1.size() - npos;
        util::move_backward(rng1.last, rng1.first, rng1.first + nmove);
    };
    //--------------------------------------------------------------------
    // Movement of elements between blocks
    //--------------------------------------------------------------------
    for (it_index itx = itx_last - 1; itx != itx_first;)
    {
        --itx;
        range_it rng2 = rng1;
        rng1 = get_range(*itx);
        Iter_t it_mid1 = rng1.last - npos, it_mid2 = rng2.first + npos;
        util::move_backward(it_mid2, it_mid1, rng1.last);
        util::move_backward(rng1.last, rng1.first, it_mid1);
    };
};
//-------------------------------------------------------------------------
//  function : rearrange_with_index
/// @brief rearrange the blocks with the relative positions of the index
/// @param
/// @param
/// @param
/// @return
//-------------------------------------------------------------------------
template<class Iter_t, class Compare, uint32_t Power2>
void merge_block<Iter_t, Compare, Power2>
::rearrange_with_index(void)
{   //--------------------------------------------------------------------
    //                     Code
    //--------------------------------------------------------------------
    size_t pos_dest, pos_src, pos_ini;
    size_t nelem = index.size();

    ptr_circ->clear();
    value_t * aux = ptr_circ->get_buffer();
    range_buf rng_buf(aux, aux + ptr_circ->NMAX);

    pos_ini = 0;
    while (pos_ini < nelem)
    {
        while (pos_ini < nelem and index[pos_ini] == pos_ini)
            ++pos_ini;
        if (pos_ini == nelem) return;
        pos_dest = pos_src = pos_ini;
        rng_buf = move_forward(rng_buf, get_range(pos_ini));
        pos_src = index[pos_ini];

        while (pos_src != pos_ini)
        {
            move_forward(get_range(pos_dest), get_range(pos_src));
            index[pos_dest] = pos_dest;
            pos_dest = pos_src;
            pos_src = index[pos_src];
        };
        move_forward(get_range(pos_dest), rng_buf);
        index[pos_dest] = pos_dest;
        ++pos_ini;
    };
};

//****************************************************************************
};//    End namespace common
};//    End namespace sort
};//    End namespace boost
//****************************************************************************
#endif
