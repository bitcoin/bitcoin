//----------------------------------------------------------------------------
/// @file sample_sort.hpp
/// @brief contains the class sample_sort
///
/// @author Copyright (c) 2016 Francisco Jose Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_DETAIL_SAMPLE_SORT_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_SAMPLE_SORT_HPP

#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

#include <algorithm>
#include <boost/sort/spinsort/spinsort.hpp>
#include <boost/sort/common/indirect.hpp>
#include <boost/sort/common/util/atomic.hpp>
#include <boost/sort/common/merge_four.hpp>
#include <boost/sort/common/merge_vector.hpp>
#include <boost/sort/common/range.hpp>

namespace boost
{
namespace sort
{
namespace sample_detail
{
//---------------------------------------------------------------------------
//                    USING SENTENCES
//---------------------------------------------------------------------------
namespace bsc = boost::sort::common;
namespace bss = boost::sort::spin_detail;
namespace bscu = boost::sort::common::util;
using bsc::range;
using bscu::atomic_add;
using bsc::merge_vector4;
using bsc::uninit_merge_level4;
using bsc::less_ptr_no_null;

//
///---------------------------------------------------------------------------
/// @struct sample_sort
/// @brief This a structure for to implement a sample sort, exception
///        safe
/// @tparam
/// @remarks
//----------------------------------------------------------------------------
template<class Iter_t, class Compare>
struct sample_sort
{
    //------------------------------------------------------------------------
    //                     DEFINITIONS
    //------------------------------------------------------------------------
    typedef value_iter<Iter_t> value_t;
    typedef range<Iter_t> range_it;
    typedef range<value_t *> range_buf;
    typedef sample_sort<Iter_t, Compare> this_t;

    //------------------------------------------------------------------------
    //                VARIABLES AND CONSTANTS
    //------------------------------------------------------------------------
    // minimun numbers of elements for to be sortd in parallel mode
    static const uint32_t thread_min = (1 << 16);

    // Number of threads to use in the algorithm
    // Number of intervals for to do the internal division of the data
    uint32_t nthread, ninterval;

    // Bool variables indicating if the auxiliary memory is constructed
    // and indicating in the auxiliary memory had been obtained inside the
    /// algorithm or had been received as a parameter
    bool construct = false, owner = false;

    // Comparison object for to compare two elements
    Compare comp;

    // Range with all the elements to sort
    range_it global_range;

    // range with the auxiliary memory
    range_buf global_buf;

    // vector of futures
    std::vector<std::future<void>> vfuture;

    // vector of vectors which contains the ranges to merge obtained in the
    // subdivision
    std::vector<std::vector<range_it>> vv_range_it;

    // each vector of ranges of the vv_range_it, need their corresponding buffer
    // for to do the merge
    std::vector<std::vector<range_buf>> vv_range_buf;

    // Initial vector of ranges
    std::vector<range_it> vrange_it_ini;

    // Initial vector of buffers
    std::vector<range_buf> vrange_buf_ini;

    // atomic counter for to know when are finished the function_t created
    // inside a function
    std::atomic<uint32_t> njob;

    // Indicate if an error in the algorithm for to undo all
    bool error;

    //------------------------------------------------------------------------
    //                       FUNCTIONS OF THE STRUCT
    //------------------------------------------------------------------------
    void initial_configuration(void);

    sample_sort (Iter_t first, Iter_t last, Compare cmp, uint32_t num_thread,
                 value_t *paux, size_t naux);

    sample_sort(Iter_t first, Iter_t last)
    : sample_sort (first, last, Compare(), std::thread::hardware_concurrency(),
                   nullptr, 0) { };

    sample_sort(Iter_t first, Iter_t last, Compare cmp)
    : sample_sort(first, last, cmp, std::thread::hardware_concurrency(),
                  nullptr, 0) { };

    sample_sort(Iter_t first, Iter_t last, uint32_t num_thread)
    : sample_sort(first, last, Compare(), num_thread, nullptr, 0) { };

    sample_sort(Iter_t first, Iter_t last, Compare cmp, uint32_t num_thread)
    : sample_sort(first, last, cmp, num_thread, nullptr, 0) { };

    sample_sort(Iter_t first, Iter_t last, Compare cmp, uint32_t num_thread,
                range_buf range_buf_initial)
    : sample_sort(first, last, cmp, num_thread,
                  range_buf_initial.first, range_buf_initial.size()) { };

    void destroy_all(void);
    //
    //-----------------------------------------------------------------------------
    //  function :~sample_sort
    /// @brief destructor of the class. The utility is to destroy the temporary
    ///        buffer used in the sorting process
    //-----------------------------------------------------------------------------
    ~sample_sort(void) { destroy_all(); };
    //
    //-----------------------------------------------------------------------
    //  function : execute first
    /// @brief this a function to assign to each thread in the first merge
    //-----------------------------------------------------------------------
    void execute_first(void)
    {
        uint32_t job = 0;
        while ((job = atomic_add(njob, 1)) < ninterval)
        {
            uninit_merge_level4(vrange_buf_ini[job], vv_range_it[job],
                            vv_range_buf[job], comp);
        };
    };
    //
    //-----------------------------------------------------------------------
    //  function : execute
    /// @brief this is a function to assignt each thread the final merge
    //-----------------------------------------------------------------------
    void execute(void)
    {
        uint32_t job = 0;
        while ((job = atomic_add(njob, 1)) < ninterval)
        {
            merge_vector4(vrange_buf_ini[job], vrange_it_ini[job],
                            vv_range_buf[job], vv_range_it[job], comp);
        };
    };
    //
    //-----------------------------------------------------------------------
    //  function : first merge
    /// @brief Implement the merge of the initially sparse ranges
    //-----------------------------------------------------------------------
    void first_merge(void)
    { //---------------------------------- begin --------------------------
        njob = 0;

        for (uint32_t i = 0; i < nthread; ++i)
        {
            vfuture[i] = std::async(std::launch::async, &this_t::execute_first,
                            this);
        };
        for (uint32_t i = 0; i < nthread; ++i)
            vfuture[i].get();
    };
    //
    //-----------------------------------------------------------------------
    //  function : final merge
    /// @brief Implement the final merge of the ranges
    //-----------------------------------------------------------------------
    void final_merge(void)
    { //---------------------------------- begin --------------------------
        njob = 0;

        for (uint32_t i = 0; i < nthread; ++i)
        {
            vfuture[i] = std::async(std::launch::async, &this_t::execute, this);
        };
        for (uint32_t i = 0; i < nthread; ++i)
            vfuture[i].get();
    };
    //----------------------------------------------------------------------------
};
//                    End class sample_sort
//----------------------------------------------------------------------------
//
//############################################################################
//                                                                          ##
//              N O N    I N L I N E      F U N C T I O N S                 ##
//                                                                          ##
//                                                                          ##
//############################################################################
//
//-----------------------------------------------------------------------------
//  function : sample_sort
/// @brief constructor of the class
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param cmp : object for to compare two elements pointed by Iter_t iterators
/// @param num_thread : Number of threads to use in the process. When this value
///                     is lower than 2, the sorting is done with 1 thread
/// @param paux : pointer to the auxiliary memory. If nullptr, the memory is
///               created inside the class
/// @param naux : number of elements of the memory pointed by paux
//-----------------------------------------------------------------------------
template<class Iter_t, typename Compare>
sample_sort<Iter_t, Compare>
::sample_sort (Iter_t first, Iter_t last, Compare cmp, uint32_t num_thread,
               value_t *paux, size_t naux)
: nthread(num_thread), owner(false), comp(cmp), global_range(first, last),
  global_buf(nullptr, nullptr), error(false)
{
    assert((last - first) >= 0);
    size_t nelem = size_t(last - first);
    construct = false;
    njob = 0;
    vfuture.resize(nthread);

    // Adjust when have many threads and only a few elements
    while (nelem > thread_min and (nthread * nthread) > (nelem >> 3))
    {
        nthread /= 2;
    };
    ninterval = (nthread << 3);

    if (nthread < 2 or nelem <= (thread_min))
    {
        bss::spinsort<Iter_t, Compare>(first, last, comp);
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

    if (paux != nullptr)
    {
        assert(naux != 0);
        global_buf.first = paux;
        global_buf.last = paux + naux;
        owner = false;
    }
    else
    {
        value_t *ptr = std::get_temporary_buffer<value_t>(nelem).first;
        if (ptr == nullptr) throw std::bad_alloc();
        owner = true;
        global_buf = range_buf(ptr, ptr + nelem);
    };
    //------------------------------------------------------------------------
    //                    PROCESS
    //------------------------------------------------------------------------
    try
    {
        initial_configuration();
    } catch (std::bad_alloc &)
    {
        error = true;
    };
    if (not error)
    {
        first_merge();
        construct = true;
        final_merge();
    };
    if (error)
    {
        destroy_all();
        throw std::bad_alloc();
    };
}
;
//
//-----------------------------------------------------------------------------
//  function : destroy_all
/// @brief destructor of the class. The utility is to destroy the temporary
///        buffer used in the sorting process
//-----------------------------------------------------------------------------
template<class Iter_t, typename Compare>
void sample_sort<Iter_t, Compare>::destroy_all(void)
{
    if (construct)
    {
        destroy(global_buf);
        construct = false;
    }
    if (global_buf.first != nullptr and owner)
        std::return_temporary_buffer(global_buf.first);
}
//
//-----------------------------------------------------------------------------
//  function : initial_configuration
/// @brief Create the internal data structures, and obtain the inital set of
///        ranges to merge
//-----------------------------------------------------------------------------
template<class Iter_t, typename Compare>
void sample_sort<Iter_t, Compare>::initial_configuration(void)
{
    std::vector<range_it> vmem_thread;
    std::vector<range_buf> vbuf_thread;
    size_t nelem = global_range.size();

    //------------------------------------------------------------------------
    size_t cupo = nelem / nthread;
    Iter_t it_first = global_range.first;
    value_t *buf_first = global_buf.first;
    vmem_thread.reserve(nthread + 1);
    vbuf_thread.reserve(nthread + 1);

    for (uint32_t i = 0; i < (nthread - 1); ++i, it_first += cupo, buf_first +=
                    cupo)
    {
        vmem_thread.emplace_back(it_first, it_first + cupo);
        vbuf_thread.emplace_back(buf_first, buf_first + cupo);
    };

    vmem_thread.emplace_back(it_first, global_range.last);
    vbuf_thread.emplace_back(buf_first, global_buf.last);

    //------------------------------------------------------------------------
    // Sorting of the ranges
    //------------------------------------------------------------------------
    std::vector<std::future<void>> vfuture(nthread);

    for (uint32_t i = 0; i < nthread; ++i)
    {
        auto func = [=]()
        {
            bss::spinsort<Iter_t, Compare> (vmem_thread[i].first,
                            vmem_thread[i].last, comp,
                            vbuf_thread[i]);
        };
        vfuture[i] = std::async(std::launch::async, func);
    };

    for (uint32_t i = 0; i < nthread; ++i)
        vfuture[i].get();

    //------------------------------------------------------------------------
    // Obtain the vector of milestones
    //------------------------------------------------------------------------
    std::vector<Iter_t> vsample;
    vsample.reserve(nthread * (ninterval - 1));

    for (uint32_t i = 0; i < nthread; ++i)
    {
        size_t distance = vmem_thread[i].size() / ninterval;
        for (size_t j = 1, pos = distance; j < ninterval; ++j, pos += distance)
        {
            vsample.push_back(vmem_thread[i].first + pos);
        };
    };
    typedef less_ptr_no_null<Iter_t, Compare> compare_ptr;
    typedef typename std::vector<Iter_t>::iterator it_to_it;

    bss::spinsort<it_to_it, compare_ptr>(vsample.begin(), vsample.end(),
                    compare_ptr(comp));

    //------------------------------------------------------------------------
    // Create the final milestone vector
    //------------------------------------------------------------------------
    std::vector<Iter_t> vmilestone;
    vmilestone.reserve(ninterval);

    for (uint32_t pos = nthread >> 1; pos < vsample.size(); pos += nthread)
    {
        vmilestone.push_back(vsample[pos]);
    };

    //------------------------------------------------------------------------
    // Creation of the first vector of ranges
    //------------------------------------------------------------------------
    std::vector<std::vector<range<Iter_t>>>vv_range_first (nthread);

    for (uint32_t i = 0; i < nthread; ++i)
    {
        Iter_t itaux = vmem_thread[i].first;

        for (uint32_t k = 0; k < (ninterval - 1); ++k)
        {
            Iter_t it2 = std::upper_bound(itaux, vmem_thread[i].last,
                            *vmilestone[k], comp);

            vv_range_first[i].emplace_back(itaux, it2);
            itaux = it2;
        };
        vv_range_first[i].emplace_back(itaux, vmem_thread[i].last);
    };

    //------------------------------------------------------------------------
    // Copy in buffer and  creation of the final matrix of ranges
    //------------------------------------------------------------------------
    vv_range_it.resize(ninterval);
    vv_range_buf.resize(ninterval);
    vrange_it_ini.reserve(ninterval);
    vrange_buf_ini.reserve(ninterval);

    for (uint32_t i = 0; i < ninterval; ++i)
    {
        vv_range_it[i].reserve(nthread);
        vv_range_buf[i].reserve(nthread);
    };

    Iter_t it = global_range.first;
    value_t *it_buf = global_buf.first;

    for (uint32_t k = 0; k < ninterval; ++k)
    {
        size_t nelem_interval = 0;

        for (uint32_t i = 0; i < nthread; ++i)
        {
            size_t nelem_range = vv_range_first[i][k].size();
            if (nelem_range != 0)
            {
                vv_range_it[k].push_back(vv_range_first[i][k]);
            };
            nelem_interval += nelem_range;
        };

        vrange_it_ini.emplace_back(it, it + nelem_interval);
        vrange_buf_ini.emplace_back(it_buf, it_buf + nelem_interval);

        it += nelem_interval;
        it_buf += nelem_interval;
    };
}
;
//
//****************************************************************************
}
;
//    End namespace sample_detail
//****************************************************************************
//
namespace bscu = boost::sort::common::util;
//
//############################################################################
//                                                                          ##
//                                                                          ##
//                       S A M P L E _ S O R T                              ##
//                                                                          ##
//                                                                          ##
//############################################################################
//
//-----------------------------------------------------------------------------
//  function : sample_sort
/// @brief parallel sample sort  algorithm (stable sort)
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
//-----------------------------------------------------------------------------
template<class Iter_t>
void sample_sort(Iter_t first, Iter_t last)
{
    typedef compare_iter<Iter_t> Compare;
    sample_detail::sample_sort<Iter_t, Compare>(first, last);
};
//
//-----------------------------------------------------------------------------
//  function : sample_sort
/// @brief parallel sample sort  algorithm (stable sort)
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param nthread : Number of threads to use in the process. When this value
///                  is lower than 2, the sorting is done with 1 thread
//-----------------------------------------------------------------------------
template<class Iter_t>
void sample_sort(Iter_t first, Iter_t last, uint32_t nthread)
{
    typedef compare_iter<Iter_t> Compare;
    sample_detail::sample_sort<Iter_t, Compare>(first, last, nthread);
};
//
//-----------------------------------------------------------------------------
//  function : sample_sort
/// @brief parallel sample sort  algorithm (stable sort)
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param comp : object for to compare two elements pointed by Iter_t
///               iterators
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare, bscu::enable_if_not_integral<Compare> * =
                nullptr>
void sample_sort(Iter_t first, Iter_t last, Compare comp)
{
    sample_detail::sample_sort<Iter_t, Compare>(first, last, comp);
};
//
//-----------------------------------------------------------------------------
//  function : sample_sort
/// @brief parallel sample sort  algorithm (stable sort)
///
/// @param first : iterator to the first element of the range to sort
/// @param last : iterator after the last element to the range to sort
/// @param comp : object for to compare two elements pointed by Iter_t
///               iterators
/// @param nthread : Number of threads to use in the process. When this value
///                  is lower than 2, the sorting is done with 1 thread
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare>
void sample_sort(Iter_t first, Iter_t last, Compare comp, uint32_t nthread)
{
    sample_detail::sample_sort<Iter_t, Compare>(first, last, comp, nthread);
};
//
//****************************************************************************
};//    End namespace sort
};//    End namespace boost
//****************************************************************************
//
#endif
