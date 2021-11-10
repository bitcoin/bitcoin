//----------------------------------------------------------------------------
/// @file flat_stable_sort.hpp
/// @brief Flat stable sort algorithm
///
/// @author Copyright (c) 2017 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_FLAT_STABLE_SORT_HPP
#define __BOOST_SORT_FLAT_STABLE_SORT_HPP

#include <boost/sort/insert_sort/insert_sort.hpp>
#include <boost/sort/common/util/insert.hpp>
#include <boost/sort/common/merge_block.hpp>
#include <boost/sort/common/sort_basic.hpp>
#include <boost/sort/common/range.hpp>
#include <boost/sort/common/util/traits.hpp>
#include <boost/sort/common/indirect.hpp>

#include <cstdlib>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

namespace boost
{
namespace sort
{
namespace flat_internal
{
namespace bsc = boost::sort::common;
namespace bscu = boost::sort::common::util;
//---------------------------------------------------------------------------
/// @struct flat_stable_sort
/// @brief  This class implement s stable sort algorithm with 1 thread, with
///         an auxiliary memory of N/2 elements
//----------------------------------------------------------------------------
template <class Iter_t, typename Compare = bscu::compare_iter<Iter_t>,
           uint32_t Power2 = 10>
class flat_stable_sort: public bsc::merge_block<Iter_t, Compare, Power2>
{
    //------------------------------------------------------------------------
    //               DEFINITIONS AND CONSTANTS
    //------------------------------------------------------------------------
    typedef bsc::merge_block<Iter_t, Compare, Power2> merge_block_t;

    //-------------------------------------------------------------------------
    //                  D E F I N I T I O N S
    //-------------------------------------------------------------------------
    typedef typename merge_block_t::value_t value_t;
    typedef typename merge_block_t::range_pos range_pos;
    typedef typename merge_block_t::range_it range_it;
    typedef typename merge_block_t::range_buf range_buf;
    typedef typename merge_block_t::it_index it_index;
    typedef typename merge_block_t::circular_t circular_t;

    //------------------------------------------------------------------------
    //                          CONSTANTS
    //------------------------------------------------------------------------
    using merge_block_t::BLOCK_SIZE;
    using merge_block_t::LOG_BLOCK;

    using merge_block_t::index;
    using merge_block_t::cmp;
    using merge_block_t::ptr_circ;

    using merge_block_t::get_range;
    using merge_block_t::get_group_range;
    using merge_block_t::merge_range_pos;
    using merge_block_t::move_range_pos_backward;
    using merge_block_t::rearrange_with_index;

public:
    //------------------------------------------------------------------------
    //                   PUBLIC FUNCTIONS
    //-------------------------------------------------------------------------
    flat_stable_sort(Iter_t first, Iter_t last, Compare comp,
                     circular_t *ptr_circ)
                    : merge_block_t(first, last, comp, ptr_circ)
    {
        divide(index.begin(), index.end());
        rearrange_with_index();
    };

    flat_stable_sort(Iter_t first, Iter_t last, Compare comp = Compare())
                    : flat_stable_sort(first, last, comp, nullptr) { };

    void divide(it_index itx_first, it_index itx_last);

    void sort_small(it_index itx_first, it_index itx_last);

    bool is_sorted_forward(it_index itx_first, it_index itx_last);

    bool is_sorted_backward(it_index itx_first, it_index itx_last);
};
//----------------------------------------------------------------------------
//  End of class flat_stable_sort
//----------------------------------------------------------------------------
//
//------------------------------------------------------------------------
//  function :
/// @brief :
/// @param Pos :
/// @return
//------------------------------------------------------------------------
template <class Iter_t, typename Compare, uint32_t Power2>
void flat_stable_sort <Iter_t, Compare, Power2>
::divide(it_index itx_first, it_index itx_last)
{
    size_t nblock = size_t(itx_last - itx_first);
    if (nblock < 5)
    {   sort_small(itx_first, itx_last);
        return;
    };
    if ( nblock > 7)
    {   if (is_sorted_forward(itx_first, itx_last)) return;
        if (is_sorted_backward(itx_first, itx_last)) return;
    };
    size_t nblock1 = (nblock + 1) >> 1;
    divide(itx_first, itx_first + nblock1);
    divide(itx_first + nblock1, itx_last);
    merge_range_pos(itx_first, itx_first + nblock1, itx_last);
};
//
//------------------------------------------------------------------------
//  function : sort_small
/// @brief :
/// @param
/// @param
/// @param
//------------------------------------------------------------------------
template <class Iter_t, typename Compare, uint32_t Power2>
void flat_stable_sort <Iter_t, Compare, Power2>
::sort_small(it_index itx_first, it_index itx_last)
{
    size_t nblock = size_t(itx_last - itx_first);
    assert(nblock > 0 and nblock < 5);
    value_t *paux = ptr_circ->get_buffer();
    range_it rng_data = get_group_range(*itx_first, nblock);

    if (nblock < 3)
    {
        range_buf rng_aux(paux, paux + rng_data.size());
        range_sort_data(rng_data, rng_aux, cmp);
        return;
    };

    //--------------------------------------------------------------------
    // division of range_data in two ranges for be sorted and merged
    //--------------------------------------------------------------------
    size_t nblock1 = (nblock + 1) >> 1;
    range_it rng_data1 = get_group_range(*itx_first, nblock1);
    range_it rng_data2(rng_data1.last, rng_data.last);
    range_buf rng_aux1(paux, paux + rng_data1.size());
    range_buf rng_aux2(paux, paux + rng_data2.size());

    range_sort_data(rng_data2, rng_aux2, cmp);
    range_sort_buffer(rng_data1, rng_aux1, cmp);
    merge_half(rng_data, rng_aux1, rng_data2, cmp);
};
//
//------------------------------------------------------------------------
//  function : is_sorted_forward
/// @brief : return if the data are ordered,
/// @param itx_first : iterator to the first block in the index
/// @param itx_last : iterator to the last block in the index
/// @return : true : the data are ordered false : not ordered
//------------------------------------------------------------------------
template <class Iter_t, typename Compare, uint32_t Power2>
bool flat_stable_sort <Iter_t, Compare, Power2>
::is_sorted_forward(it_index itx_first, it_index itx_last)
{
    size_t nblock = size_t(itx_last - itx_first);
    range_it rng = get_group_range(*itx_first, nblock);
    size_t nelem = rng.size();
    size_t min_process = (std::max)(BLOCK_SIZE, (nelem >> 3));

    size_t nsorted1 = bsc::number_stable_sorted_forward (rng.first, rng.last,
                                                         min_process, cmp);
    if (nsorted1 == nelem) return true;
    if (nsorted1 == 0) return false;

    size_t nsorted2 = nelem - nsorted1;
    Iter_t itaux = rng.first + nsorted1;
    if (nsorted2 <= (BLOCK_SIZE << 1))
    {
        flat_stable_sort(itaux, rng.last, cmp, ptr_circ);
        bscu::insert_sorted(rng.first, itaux, rng.last, cmp,
                            ptr_circ->get_buffer());
    }
    else
    {   // Adjust the size of the sorted data to a number of blocks
        size_t mask = ~(BLOCK_SIZE - 1);
        size_t nsorted1_adjust = nsorted1 & mask;
        flat_stable_sort(rng.first + nsorted1_adjust, rng.last, cmp,
                         ptr_circ);
        size_t nblock1 = nsorted1_adjust >> Power2;
        merge_range_pos(itx_first, itx_first + nblock1, itx_last);
    };
    return true;
};
//
//------------------------------------------------------------------------
//  function : is_sorted_backward
/// @brief : return if the data are ordered,
/// @param itx_first : iterator to the first block in the index
/// @param itx_last : iterator to the last block in the index
/// @return : true : the data are ordered false : not ordered
//------------------------------------------------------------------------
template <class Iter_t, typename Compare, uint32_t Power2>
bool flat_stable_sort <Iter_t, Compare, Power2>
::is_sorted_backward(it_index itx_first, it_index itx_last)
{
    size_t nblock = size_t(itx_last - itx_first);
    range_it rng = get_group_range(*itx_first, nblock);

    size_t nelem = rng.size();
    size_t min_process = (std::max)(BLOCK_SIZE, (nelem >> 3));

    size_t nsorted2 = bsc::number_stable_sorted_backward(rng.first, rng.last,
                                                         min_process, cmp);
    if (nsorted2 == nelem) return true;
    if (nsorted2 == 0 ) return false;
    Iter_t itaux = rng.last - nsorted2;
    size_t nsorted1 = nelem - nsorted2;

    if (nsorted1 <= (BLOCK_SIZE << 1))
    {
        flat_stable_sort(rng.first, itaux, cmp, ptr_circ);
        bscu::insert_sorted_backward(rng.first, itaux, rng.last, cmp,
                                     ptr_circ->get_buffer());
    }
    else
    {   // Adjust the size of nsorted2 for to be a number of blocks
        size_t nblock1 = (nsorted1 + BLOCK_SIZE - 1) >> Power2;
        size_t nsorted1_adjust = (nblock1 << Power2);
        flat_stable_sort(rng.first, rng.first + nsorted1_adjust, cmp,
                         ptr_circ);
        merge_range_pos(itx_first, itx_first + nblock1, itx_last);
    };
    return true;
};
//****************************************************************************
};// End namespace flat_internal
//****************************************************************************
//
namespace bscu = boost::sort::common::util;
namespace flat = boost::sort::flat_internal;
//
///---------------------------------------------------------------------------
//  function flat_stable_sort
/// @brief This class is select the block size in the block_indirect_sort
///        algorithm depending of the type and size of the data to sort
///
//----------------------------------------------------------------------------
template <class Iter_t, class Compare = bscu::compare_iter<Iter_t>,
           bscu::enable_if_string<value_iter<Iter_t> > * = nullptr>
inline void flat_stable_sort (Iter_t first, Iter_t last,
                                 Compare cmp = Compare())
{
    flat::flat_stable_sort<Iter_t, Compare, 6> (first, last, cmp);
};

template<size_t Size>
struct block_size_fss
{
    static constexpr const uint32_t BitsSize =
                    (Size == 0) ? 0 : (Size > 128) ? 7 : bscu::tmsb[Size - 1];
    static constexpr const uint32_t sz[10] =
    { 10, 10, 10, 9, 8, 7, 6, 6 };
    static constexpr const uint32_t data = sz[BitsSize];
};

//
///---------------------------------------------------------------------------
//  function flat_stable_sort
/// @brief This class is select the block size in the flat_stable_sort
///        algorithm depending of the type and size of the data to sort
///
//----------------------------------------------------------------------------
template <class Iter_t, class Compare = bscu::compare_iter<Iter_t>,
           bscu::enable_if_not_string<value_iter<Iter_t> >* = nullptr>
inline void flat_stable_sort (Iter_t first, Iter_t last,
                                 Compare cmp = Compare())
{
    flat::flat_stable_sort<Iter_t, Compare,
                           block_size_fss<sizeof(value_iter<Iter_t> )>::data>
        (first, last, cmp);
};

template<class Iter_t, class Compare = compare_iter<Iter_t> >
inline void indirect_flat_stable_sort (Iter_t first, Iter_t last,
                                           Compare comp = Compare())
{
    typedef typename std::vector<Iter_t>::iterator itx_iter;
    typedef common::less_ptr_no_null<Iter_t, Compare> itx_comp;
    common::indirect_sort ( flat_stable_sort<itx_iter, itx_comp>,
                            first, last, comp);
};

//****************************************************************************
};//    End namespace sort
};//    End namepspace boost
//****************************************************************************
//
#endif
