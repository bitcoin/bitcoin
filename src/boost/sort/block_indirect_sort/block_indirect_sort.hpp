//----------------------------------------------------------------------------
/// @file block_indirect_sort.hpp
/// @brief block indirect sort algorithm
///
/// @author Copyright (c) 2016 Francisco Jose Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_DETAIL_BLOCK_INDIRECT_SORT_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_BLOCK_INDIRECT_SORT_HPP

#include <atomic>
#include <boost/sort/block_indirect_sort/blk_detail/merge_blocks.hpp>
#include <boost/sort/block_indirect_sort/blk_detail/move_blocks.hpp>
#include <boost/sort/block_indirect_sort/blk_detail/parallel_sort.hpp>
#include <boost/sort/pdqsort/pdqsort.hpp>
#include <boost/sort/common/util/traits.hpp>
#include <boost/sort/common/util/algorithm.hpp>
#include <future>
#include <iterator>

// This value is the minimal number of threads for to use the
// block_indirect_sort algorithm
#define BOOST_NTHREAD_BORDER 6

namespace boost
{
namespace sort
{
namespace blk_detail
{
//---------------------------------------------------------------------------
//         USING SENTENCES
//---------------------------------------------------------------------------
namespace bs = boost::sort;
namespace bsc = bs::common;
namespace bscu = bsc::util;
using bscu::compare_iter;
using bscu::value_iter;
using bsc::range;
using bsc::destroy;
using bsc::initialize;
using bscu::nbits64;
using bs::pdqsort;
using bscu::enable_if_string;
using bscu::enable_if_not_string;
using bscu::tmsb;
//
///---------------------------------------------------------------------------
/// @struct block_indirect_sort
/// @brief This class is the entry point of the block indirect sort. The code
///        of this algorithm is divided in several classes:
///        bis/block.hpp : basic structures used in the algorithm
///        bis/backbone.hpp : data used by all the classes
///        bis/merge_blocks.hpp : merge the internal blocks
///        bis/move_blocks.hpp : move the blocks, and obtain all the elements
///                              phisicaly sorted
///        bis/parallel_sort.hpp : make the parallel sort of each part in the
///                                initial division of the data
///
//----------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t,
                class Compare = compare_iter<Iter_t> >
struct block_indirect_sort
{
    //------------------------------------------------------------------------
    //                  D E F I N I T I O N S
    //------------------------------------------------------------------------
    typedef typename std::iterator_traits<Iter_t>::value_type value_t;
    typedef std::atomic<uint32_t> atomic_t;
    typedef range<size_t> range_pos;
    typedef range<Iter_t> range_it;
    typedef range<value_t *> range_buf;
    typedef std::function<void(void)> function_t;

    // classes used in the internal operations of the algorithm
    typedef block_pos block_pos_t;
    typedef block<Block_size, Iter_t> block_t;
    typedef backbone<Block_size, Iter_t, Compare> backbone_t;
    typedef parallel_sort<Block_size, Iter_t, Compare> parallel_sort_t;

    typedef merge_blocks<Block_size, Group_size, Iter_t, Compare> merge_blocks_t;
    typedef move_blocks<Block_size, Group_size, Iter_t, Compare> move_blocks_t;
    typedef compare_block_pos<Block_size, Iter_t, Compare> compare_block_pos_t;
    //
    //------------------------------------------------------------------------
    //       V A R I A B L E S   A N D  C O N S T A N T S
    //------------------------------------------------------------------------
    // contains the data and the internal data structures of the algorithm for
    // to be shared between the classes which are part of the algorithm
    backbone_t bk;
    // atomic counter for to detect the end of the works created inside
    // the object
    atomic_t counter;
    // pointer to the uninitialized memory used for the thread buffers
    value_t *ptr;
    // indicate if the memory pointed by ptr is initialized
    bool construct;
    // range from extract the buffers for the threads
    range_buf rglobal_buf;
    // number of threads to use
    uint32_t nthread;
    //
    //------------------------------------------------------------------------
    //                F U N C T I O N S
    //------------------------------------------------------------------------

    block_indirect_sort(Iter_t first, Iter_t last, Compare cmp, uint32_t nthr);

    block_indirect_sort(Iter_t first, Iter_t last) :
                        block_indirect_sort(first, last, Compare(),
                        std::thread::hardware_concurrency()) { }


    block_indirect_sort(Iter_t first, Iter_t last, Compare cmp) :
                        block_indirect_sort(first, last, cmp,
                        std::thread::hardware_concurrency()) { }


    block_indirect_sort(Iter_t first, Iter_t last, uint32_t nthread) :
                        block_indirect_sort(first, last, Compare(), nthread){}


    //
    //------------------------------------------------------------------------
    //  function :destroy_all
    /// @brief destructor all the data structures of the class (if the memory
    ///        is constructed, is destroyed) and  return the uninitialized
    ///        memory
    //------------------------------------------------------------------------
    void destroy_all(void)
    {
        if (ptr != nullptr)
        {
            if (construct)
            {
                destroy(rglobal_buf);
                construct = false;
            };
            std::return_temporary_buffer(ptr);
            ptr = nullptr;
        };
    }
    //
    //------------------------------------------------------------------------
    //  function :~block_indirect_sort
    /// @brief destructor of the class (if the memory is constructed, is
    ///        destroyed) and  return the uninitialized memory
    //------------------------------------------------------------------------
    ~block_indirect_sort(void)
    {
        destroy_all();
    }

    void split_range(size_t pos_index1, size_t pos_index2,
                    uint32_t level_thread);

    void start_function(void);

//-------------------------------------------------------------------------
}; // End class block_indirect_sort
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
//  function : block_indirect_sort
/// @brief begin with the execution of the functions stored in works
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param comp : object for to compare two elements pointed by Iter_t
///               iterators
/// @param nthr : Number of threads to use in the process.When this value
///               is lower than 2, the sorting is done with 1 thread
//-------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
block_indirect_sort<Block_size, Group_size, Iter_t, Compare>
::block_indirect_sort(Iter_t first, Iter_t last, Compare cmp, uint32_t nthr)
: bk(first, last, cmp), counter(0), ptr(nullptr), construct(false),
  nthread(nthr)
{
    try
    {
        assert((last - first) >= 0);
        size_t nelem = size_t(last - first);
        if (nelem == 0) return;

        //------------------- check if sort -----------------------------------
        bool sorted = true;
        for (Iter_t it1 = first, it2 = first + 1; it2 != last and (sorted =
                        not bk.cmp(*it2, *it1)); it1 = it2++);
        if (sorted) return;

        //------------------- check if reverse sort ---------------------------
        sorted = true;
        for (Iter_t it1 = first, it2 = first + 1; it2 != last and (sorted =
                        not bk.cmp(*it1, *it2)); it1 = it2++);

        if (sorted)
        {
            size_t nelem2 = nelem >> 1;
            Iter_t it1 = first, it2 = last - 1;
            for (size_t i = 0; i < nelem2; ++i)
            {
                std::swap(*(it1++), *(it2--));
            };
            return;
        };

        //---------------- check if only single thread -----------------------
        size_t nthreadmax = nelem / (Block_size * Group_size) + 1;
        if (nthread > nthreadmax) nthread = (uint32_t) nthreadmax;

        uint32_t nbits_size = (nbits64(sizeof(value_t)) >> 1);
        if (nbits_size > 5) nbits_size = 5;
        size_t max_per_thread = 1 << (18 - nbits_size);

        if (nelem < (max_per_thread) or nthread < 2)
        {
            //intro_sort (first, last, bk.cmp);
            pdqsort(first, last, bk.cmp);
            return;
        };

        //----------- creation of the temporary buffer --------------------
        ptr = std::get_temporary_buffer<value_t>(Block_size * nthread).first;
        if (ptr == nullptr)
        {
            bk.error = true;
            throw std::bad_alloc();
        };

        rglobal_buf = range_buf(ptr, ptr + (Block_size * nthread));
        initialize(rglobal_buf, *first);
        construct = true;

        // creation of the buffers for the threads
        std::vector<value_t *> vbuf(nthread);
        for (uint32_t i = 0; i < nthread; ++i)
        {
            vbuf[i] = ptr + (i * Block_size);
        };

        // Insert the first work in the stack
        bscu::atomic_write(counter, 1);
        function_t f1 = [&]( )
        {
            start_function ( );
            bscu::atomic_sub (counter, 1);
        };
        bk.works.emplace_back(f1);

        //---------------------------------------------------------------------
        //                    PROCESS
        //---------------------------------------------------------------------
        std::vector<std::future<void> > vfuture(nthread);

        // The function launched with the futures is "execute the functions of
        // the stack until this->counter is zero
        // vbuf[i] is the memory from the main thread for to configure the
        // thread local buffer
        for (uint32_t i = 0; i < nthread; ++i)
        {
            auto f1 = [=, &vbuf]( )
            {   bk.exec (vbuf[i], this->counter);};
            vfuture[i] = std::async(std::launch::async, f1);
        };
        for (uint32_t i = 0; i < nthread; ++i)
            vfuture[i].get();
        if (bk.error) throw std::bad_alloc();
    }
    catch (std::bad_alloc &)
    {
        destroy_all();
        throw;
    }
};
//
//-----------------------------------------------------------------------------
//  function : split_rage
/// @brief this function splits a range of positions in the index, and
///        depending of the size, sort directly or make to a recursive call
///        to split_range
/// @param pos_index1 : first position in the index
/// @param pos_index2 : position after the last in the index
/// @param level_thread : depth of the call. When 0 sort the blocks
//-----------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
void block_indirect_sort<Block_size, Group_size, Iter_t, Compare>
::split_range(size_t pos_index1, size_t pos_index2, uint32_t level_thread)
{
    size_t nblock = pos_index2 - pos_index1;

    //-------------------------------------------------------------------------
    // In the blocks not sorted, the physical position is the logical position
    //-------------------------------------------------------------------------
    Iter_t first = bk.get_block(pos_index1).first;
    Iter_t last = bk.get_range(pos_index2 - 1).last;

    if (nblock < Group_size)
    {
        pdqsort(first, last, bk.cmp);
        return;
    };

    size_t pos_index_mid = pos_index1 + (nblock >> 1);
    atomic_t son_counter(1);

    //-------------------------------------------------------------------------
    // Insert in the stack the work for the second part, and the actual thread,
    // execute the first part
    //-------------------------------------------------------------------------
    if (level_thread != 0)
    {
        auto f1 = [=, &son_counter]( )
        {
            split_range (pos_index_mid, pos_index2, level_thread - 1);
            bscu::atomic_sub (son_counter, 1);
        };
        bk.works.emplace_back(f1);
        if (bk.error) return;
        split_range(pos_index1, pos_index_mid, level_thread - 1);
    }
    else
    {
        Iter_t mid = first + ((nblock >> 1) * Block_size);
        auto f1 = [=, &son_counter]( )
        {
            parallel_sort_t (bk, mid, last);
            bscu::atomic_sub (son_counter, 1);
        };
        bk.works.emplace_back(f1);
        if (bk.error) return;
        parallel_sort_t(bk, first, mid);
    };
    bk.exec(son_counter);
    if (bk.error) return;
    merge_blocks_t(bk, pos_index1, pos_index_mid, pos_index2);
};

//
//-----------------------------------------------------------------------------
//  function : start_function
/// @brief this function init the process. When the number of threads is lower
///        than a predefined value, sort the elements with a parallel pdqsort.
//-----------------------------------------------------------------------------
template<uint32_t Block_size, uint32_t Group_size, class Iter_t, class Compare>
void block_indirect_sort<Block_size, Group_size, Iter_t, Compare>
::start_function(void)
{
    if (nthread < BOOST_NTHREAD_BORDER)
    {
        parallel_sort_t(bk, bk.global_range.first, bk.global_range.last);
    }
    else
    {
        size_t level_thread = nbits64(nthread - 1) - 1;
        split_range(0, bk.nblock, level_thread - 1);
        if (bk.error) return;
        move_blocks_t k(bk);
    };
};

///---------------------------------------------------------------------------
//  function block_indirect_sort_call
/// @brief This class is select the block size in the block_indirect_sort
///        algorithm depending of the type and size of the data to sort
///
//----------------------------------------------------------------------------
template <class Iter_t, class Compare,
         enable_if_string<value_iter<Iter_t>> * = nullptr>
inline void block_indirect_sort_call(Iter_t first, Iter_t last, Compare cmp,
                uint32_t nthr)
{
    block_indirect_sort<128, 128, Iter_t, Compare>(first, last, cmp, nthr);
};

template<size_t Size>
struct block_size
{
    static constexpr const uint32_t BitsSize =
                    (Size == 0) ? 0 : (Size > 256) ? 9 : tmsb[Size - 1];
    static constexpr const uint32_t sz[10] =
    { 4096, 4096, 4096, 4096, 2048, 1024, 768, 512, 256, 128 };
    static constexpr const uint32_t data = sz[BitsSize];
};
//
///---------------------------------------------------------------------------
/// @struct block_indirect_sort_call
/// @brief This class is select the block size in the block_indirect_sort
///        algorithm depending of the type and size of the data to sort
///
//----------------------------------------------------------------------------
template <class Iter_t, class Compare,
          enable_if_not_string<value_iter<Iter_t>> * = nullptr>
inline void block_indirect_sort_call (Iter_t first, Iter_t last, Compare cmp,
                                      uint32_t nthr)
{
    block_indirect_sort<block_size<sizeof (value_iter<Iter_t> )>::data, 64,
                        Iter_t, Compare> (first, last, cmp, nthr);
};

//
//****************************************************************************
}; //    End namespace blk_detail
//****************************************************************************
//
namespace bscu = boost::sort::common::util;
//
//############################################################################
//                                                                          ##
//                                                                          ##
//               B L O C K _ I N D I R E C T _ S O R T                      ##
//                                                                          ##
//                                                                          ##
//############################################################################
//
//-----------------------------------------------------------------------------
//  function : block_indirect_sort
/// @brief invocation of block_indirtect_sort with 2 parameters
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
//-----------------------------------------------------------------------------
template<class Iter_t>
void block_indirect_sort(Iter_t first, Iter_t last)
{
    typedef bscu::compare_iter<Iter_t> Compare;
    blk_detail::block_indirect_sort_call (first, last, Compare(),
                                          std::thread::hardware_concurrency());
}

//
//-----------------------------------------------------------------------------
//  function : block_indirect_sort
/// @brief invocation of block_indirtect_sort with 3 parameters. The third is 
///        the number of threads
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param nthread : Number of threads to use in the process. When this value
///                  is lower than 2, the sorting is done with 1 thread
//-----------------------------------------------------------------------------
template<class Iter_t>
void block_indirect_sort(Iter_t first, Iter_t last, uint32_t nthread)
{
    typedef bscu::compare_iter<Iter_t> Compare;
    blk_detail::block_indirect_sort_call(first, last, Compare(), nthread);
}
//
//-----------------------------------------------------------------------------
//  function : block_indirect_sort
/// @brief invocation of block_indirtect_sort with 3 parameters. The third is 
///        the comparison object
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param comp : object for to compare two elements pointed by Iter_t
///               iterators
//-----------------------------------------------------------------------------
template <class Iter_t, class Compare,
          bscu::enable_if_not_integral<Compare> * = nullptr>
void block_indirect_sort(Iter_t first, Iter_t last, Compare comp)
{
    blk_detail::block_indirect_sort_call (first, last, comp,
                                      std::thread::hardware_concurrency());
}

//
//-----------------------------------------------------------------------------
//  function : block_indirect_sort
/// @brief invocation of block_indirtect_sort with 4 parameters. 
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param comp : object for to compare two elements pointed by Iter_t
///               iterators
/// @param nthread : Number of threads to use in the process. When this value
///                  is lower than 2, the sorting is done with 1 thread
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare>
void block_indirect_sort (Iter_t first, Iter_t last, Compare comp,
                          uint32_t nthread)
{
    blk_detail::block_indirect_sort_call(first, last, comp, nthread);
}
//
//****************************************************************************
}; //    End namespace sort
}; //    End namespace boost
//****************************************************************************
//
#endif
