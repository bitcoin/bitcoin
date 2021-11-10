//----------------------------------------------------------------------------
/// @file merge_four.hpp
/// @brief This file have the functions for to merge 4 buffers
///
/// @author Copyright (c) 2016 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_DETAIL_UTIL_MERGE_FOUR_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_UTIL_MERGE_FOUR_HPP

#include <boost/sort/common/util/traits.hpp>
#include <boost/sort/common/range.hpp>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

namespace boost
{
namespace sort
{
namespace common
{

//
//############################################################################
//                                                                          ##
//                       F U S I O N     O F                                ##
//                                                                          ##
//              F O U R     E L E M E N T S    R A N G E                    ##
//                                                                          ##
//############################################################################
//

//-----------------------------------------------------------------------------
//  function : less_range
/// @brief Compare the elements pointed by it1 and it2, and if they
///        are equals, compare their position, doing a stable comparison
///
/// @param it1 : iterator to the first element
/// @param pos1 : position of the object pointed by it1
/// @param it2 : iterator to the second element
/// @param pos2 : position of the element pointed by it2
/// @param comp : comparison object
/// @return result of the comparison
//-----------------------------------------------------------------------------
template<class Iter_t, class Compare = typename util::compare_iter<Iter_t> >
inline bool less_range(Iter_t it1, uint32_t pos1, Iter_t it2, uint32_t pos2,
                       Compare comp = Compare())
{
    return (comp(*it1, *it2)) ? true :
           (pos2 < pos1) ? false : not (comp(*it2, *it1));
};

//-----------------------------------------------------------------------------
//  function : full_merge4
/// @brief Merge four ranges
///
/// @param dest: range where move the elements merged. Their size must be
///              greater or equal than the sum of the sizes of the ranges
///              in vrange_input
/// @param vrange_input : array of ranges to merge
/// @param nrange_input : number of ranges in vrange_input
/// @param comp : comparison object
/// @return range with all the elements moved with the size adjusted
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Compare>
range<Iter1_t> full_merge4(const range<Iter1_t> &rdest,
                           range<Iter2_t> vrange_input[4],
                           uint32_t nrange_input, Compare comp)
{
    typedef range<Iter1_t> range1_t;
    typedef util::value_iter<Iter1_t> type1;
    typedef util::value_iter<Iter2_t> type2;
    static_assert (std::is_same< type1, type2 >::value,
                    "Incompatible iterators\n");

    size_t ndest = 0;
    uint32_t i = 0;
    while (i < nrange_input)
    {
        if (vrange_input[i].size() != 0)
        {
            ndest += vrange_input[i++].size();
        }
        else
        {
            for (uint32_t k = i + 1; k < nrange_input; ++k)
            {
                vrange_input[k - 1] = vrange_input[k];
            };
            --nrange_input;
        };
    };

    if (nrange_input == 0) return range1_t(rdest.first, rdest.first);
    if (nrange_input == 1) return move_forward(rdest, vrange_input[0]);
    if (nrange_input == 2)
    {
        return merge(rdest, vrange_input[0], vrange_input[1], comp);
    };

    //------------------------------------------------------------------------
    // Initial sort
    //------------------------------------------------------------------------
    uint32_t pos[4] =
    { 0, 1, 2, 3 }, npos = nrange_input;

    //-----------------------------------------------------------------------
    // thanks to Steven Ross by their suggestion about the optimal
    // sorting networks
    //-----------------------------------------------------------------------
    if (less_range(vrange_input[pos[1]].first, pos[1],
                    vrange_input[pos[0]].first, pos[0], comp))
    {
        std::swap(pos[0], pos[1]);
    };
    if (npos == 4 and less_range(vrange_input[pos[3]].first, pos[3],
                                 vrange_input[pos[2]].first, pos[2], comp))
    {
        std::swap(pos[3], pos[2]);
    };
    if (less_range (vrange_input[pos[2]].first, pos[2],
                    vrange_input[pos[0]].first, pos[0], comp))
    {
        std::swap(pos[0], pos[2]);
    };
    if (npos == 4
                    and less_range (vrange_input[pos[3]].first, pos[3],
                                    vrange_input[pos[1]].first, pos[1], comp))
    {
        std::swap(pos[1], pos[3]);
    };
    if (less_range (vrange_input[pos[2]].first, pos[2],
                    vrange_input[pos[1]].first, pos[1], comp))
    {
        std::swap(pos[1], pos[2]);
    };

    Iter1_t it_dest = rdest.first;
    while (npos > 2)
    {
        *(it_dest++) = std::move(*(vrange_input[pos[0]].first++));
        if (vrange_input[pos[0]].size() == 0)
        {
            pos[0] = pos[1];
            pos[1] = pos[2];
            pos[2] = pos[3];
            --npos;
        }
        else
        {
            if (less_range(vrange_input[pos[1]].first, pos[1],
                            vrange_input[pos[0]].first, pos[0], comp))
            {
                std::swap(pos[0], pos[1]);
                if (less_range(vrange_input[pos[2]].first, pos[2],
                                vrange_input[pos[1]].first, pos[1], comp))
                {
                    std::swap(pos[1], pos[2]);
                    if (npos == 4
                                    and less_range(vrange_input[pos[3]].first,
                                                    pos[3],
                                                    vrange_input[pos[2]].first,
                                                    pos[2], comp))
                    {
                        std::swap(pos[2], pos[3]);
                    };
                };
            };
        };
    };

    range1_t raux1(rdest.first, it_dest), raux2(it_dest, rdest.last);
    if (pos[0] < pos[1])
    {
        return concat(raux1,merge(raux2, vrange_input[pos[0]], 
                                  vrange_input[pos[1]], comp));
    }
    else
    {
        return concat(raux1, merge (raux2, vrange_input[pos[1]], 
                                    vrange_input[pos[0]], comp));
    };
};

//-----------------------------------------------------------------------------
//  function : uninit_full_merge4
/// @brief Merge four ranges and put the result in uninitialized memory
///
/// @param dest: range where create and move the elements merged. Their
///              size must be greater or equal than the sum of the sizes
///              of the ranges in the array R
/// @param vrange_input : array of ranges to merge
/// @param nrange_input : number of ranges in vrange_input
/// @param comp : comparison object
/// @return range with all the elements move with the size adjusted
//-----------------------------------------------------------------------------
template<class Value_t, class Iter_t, class Compare>
range<Value_t *> uninit_full_merge4(const range<Value_t *> &dest,
                                    range<Iter_t> vrange_input[4],
                                    uint32_t nrange_input, Compare comp)
{
    typedef util::value_iter<Iter_t> type1;
    static_assert (std::is_same< type1, Value_t >::value,
                    "Incompatible iterators\n");

    size_t ndest = 0;
    uint32_t i = 0;
    while (i < nrange_input)
    {
        if (vrange_input[i].size() != 0)
        {
            ndest += vrange_input[i++].size();
        }
        else
        {
            for (uint32_t k = i + 1; k < nrange_input; ++k)
            {
                vrange_input[k - 1] = vrange_input[k];
            };
            --nrange_input;
        };
    };
    if (nrange_input == 0) return range<Value_t *>(dest.first, dest.first);
    if (nrange_input == 1) return move_construct(dest, vrange_input[0]);
    if (nrange_input == 2)
    {
        return merge_construct(dest, vrange_input[0], vrange_input[1], comp);
    };

    //------------------------------------------------------------------------
    // Initial sort
    //------------------------------------------------------------------------
    uint32_t pos[4] = { 0, 1, 2, 3 }, npos = nrange_input;

    //-----------------------------------------------------------------------
    // thanks to Steven Ross by their suggestion about the optimal
    // sorting networks
    //-----------------------------------------------------------------------
    if (less_range(vrange_input[pos[1]].first, pos[1],
                    vrange_input[pos[0]].first, pos[0], comp))
    {
        std::swap(pos[0], pos[1]);
    };
    if (npos == 4  and less_range(vrange_input[pos[3]].first, pos[3],
                                  vrange_input[pos[2]].first, pos[2], comp))
    {
        std::swap(pos[3], pos[2]);
    };
    if (less_range(vrange_input[pos[2]].first, pos[2],
                    vrange_input[pos[0]].first, pos[0], comp))
    {
        std::swap(pos[0], pos[2]);
    };
    if (npos == 4 and less_range(vrange_input[pos[3]].first, pos[3],
                                 vrange_input[pos[1]].first, pos[1], comp))
    {
        std::swap(pos[1], pos[3]);
    };
    if (less_range(vrange_input[pos[2]].first, pos[2],
                    vrange_input[pos[1]].first, pos[1], comp))
    {
        std::swap(pos[1], pos[2]);
    };

    Value_t *it_dest = dest.first;
    while (npos > 2)
    {
        util::construct_object(&(*(it_dest++)),
                        std::move(*(vrange_input[pos[0]].first++)));
        if (vrange_input[pos[0]].size() == 0)
        {
            pos[0] = pos[1];
            pos[1] = pos[2];
            pos[2] = pos[3];
            --npos;
        }
        else
        {
            if (less_range (vrange_input[pos[1]].first, pos[1],
                            vrange_input[pos[0]].first, pos[0], comp))
            {
                std::swap(pos[0], pos[1]);
                if (less_range (vrange_input[pos[2]].first, pos[2],
                                vrange_input[pos[1]].first, pos[1], comp))
                {
                    std::swap(pos[1], pos[2]);
                    if (npos == 4 and less_range(vrange_input[pos[3]].first,
                                                 pos[3],
                                                 vrange_input[pos[2]].first,
                                                 pos[2], comp))
                    {
                        std::swap(pos[2], pos[3]);
                    };
                };
            };
        };
    }; // end while (npos > 2)

    range<Value_t *> raux1(dest.first, it_dest), raux2(it_dest, dest.last);
    if (pos[0] < pos[1])
    {
        return concat(raux1,
                      merge_construct(raux2, vrange_input[pos[0]],
                                      vrange_input[pos[1]], comp));
    }
    else
    {
        return concat(raux1,
                      merge_construct(raux2, vrange_input[pos[1]],
                                      vrange_input[pos[0]], comp));
    };
};

//****************************************************************************
};//    End namespace common
};//    End namespace sort
};//    End namespace boost
//****************************************************************************
//
#endif
