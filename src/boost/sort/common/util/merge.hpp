//----------------------------------------------------------------------------
/// @file merge.hpp
/// @brief low level merge functions
///
/// @author Copyright (c) 2016 Francisco Jose Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_UTIL_MERGE_HPP
#define __BOOST_SORT_COMMON_UTIL_MERGE_HPP

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>

#include <boost/sort/common/util/algorithm.hpp>
#include <boost/sort/common/util/traits.hpp>
#include <boost/sort/common/util/circular_buffer.hpp>

namespace boost
{
namespace sort
{
namespace common
{
namespace util
{
namespace here = boost::sort::common::util;
//----------------------------------------------------------------------------
//
//           F U N C T I O N S    I N    T H E     F I L E
//----------------------------------------------------------------------------
//
// template < class Iter1_t, class Iter2_t, class Compare >
// Iter2_t merge (Iter1_t buf1, const Iter1_t end_buf1, Iter1_t buf2,
//                const Iter1_t end_buf2, Iter2_t buf_out, Compare comp)
//
// template < class Iter_t, class Value_t, class Compare >
// Value_t *merge_construct (Iter_t first1, const Iter_t last1, Iter_t first2,
//                           const Iter_t last2, Value_t *it_out, Compare comp)
//
// template < class Iter1_t, class Iter2_t, class Compare >
// Iter2_t merge_half (Iter1_t buf1, const Iter1_t end_buf1, Iter2_t buf2,
//                     const Iter2_t end_buf2, Iter2_t buf_out, Compare comp)
//
// template < class Iter1_t, class Iter2_t, class Compare >
// Iter2_t merge_half_backward (Iter1_t buf1,  Iter1_t end_buf1,
//                              Iter2_t buf2, Iter2_t end_buf2,
//                              Iter1_t end_buf_out, Compare comp)
//
// template < class Iter1_t, class Iter2_t, class Iter3_t, class Compare >
// bool merge_uncontiguous (Iter1_t src1, const Iter1_t end_src1,
//                          Iter2_t src2, const Iter2_t end_src2,
//                          Iter3_t aux, Compare comp)
//
// template < class Iter1_t, class Iter2_t, class Compare >
// bool merge_contiguous (Iter1_t src1, Iter1_t src2, Iter1_t end_src2,
//                        Iter2_t buf, Compare comp)
//
// template < class Iter_t, class Circular ,class Compare >
// bool merge_circular  (Iter_t buf1, Iter_t end_buf1,
//                       Iter_t buf2, Iter_t end_buf2,
//                       Circular &circ, Compare comp, Iter_t &it_aux)
//
//----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//  function : merge
/// @brief Merge two contiguous buffers pointed by buf1 and buf2, and put
///        in the buffer pointed by buf_out
///
/// @param buf1 : iterator to the first element in the first buffer
/// @param end_buf1 : final iterator of first buffer
/// @param buf2 : iterator to the first iterator to the second buffer
/// @param end_buf2 : final iterator of the second buffer
/// @param buf_out : buffer where move the elements merged
/// @param comp : comparison object
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Iter3_t, class Compare>
static Iter3_t merge(Iter1_t buf1, const Iter1_t end_buf1, Iter2_t buf2,
                     const Iter2_t end_buf2, Iter3_t buf_out, Compare comp)
{
    //-------------------------------------------------------------------------
    //                       Metaprogramming
    //------------------------------------------------------------------------- 
    typedef value_iter<Iter1_t> value1_t;
    typedef value_iter<Iter2_t> value2_t;
    typedef value_iter<Iter3_t> value3_t;
    static_assert (std::is_same< value1_t, value2_t >::value,
                    "Incompatible iterators\n");
    static_assert (std::is_same< value3_t, value2_t >::value,
                    "Incompatible iterators\n");

    //-------------------------------------------------------------------------
    //                       Code
    //-------------------------------------------------------------------------
    const size_t MIN_CHECK = 1024;

    if (size_t((end_buf1 - buf1) + (end_buf2 - buf2)) >= MIN_CHECK)
    {
        if (buf1 == end_buf1) return move_forward(buf_out, buf2, end_buf2);
        if (buf2 == end_buf2) return move_forward(buf_out, buf1, end_buf1);

        if (not comp(*buf2, *(end_buf1 - 1)))
        {
            Iter3_t mid = move_forward(buf_out, buf1, end_buf1);
            return move_forward(mid, buf2, end_buf2);
        };

        if (comp(*(end_buf2 - 1), *buf1))
        {
            Iter3_t mid = move_forward(buf_out, buf2, end_buf2);
            return move_forward(mid, buf1, end_buf1);
        };
    };
    while ((buf1 != end_buf1) and (buf2 != end_buf2))
    {
        *(buf_out++) = (not comp(*buf2, *buf1)) ?
                        std::move(*(buf1++)) : std::move(*(buf2++));
    };

    return (buf1 == end_buf1) ?
                    move_forward(buf_out, buf2, end_buf2) :
                    move_forward(buf_out, buf1, end_buf1);
}
;
//
//-----------------------------------------------------------------------------
//  function : merge_construct
/// @brief Merge two contiguous buffers pointed by first1 and first2, and put
///        in the uninitialized buffer pointed by it_out
///
/// @param first1 : iterator to the first element in the first buffer
/// @param last1 : last iterator of the first buffer
/// @param first2 : iterator to the first element to the second buffer
/// @param last2 : final iterator of the second buffer
/// @param it_out : uninitialized buffer where move the elements merged
/// @param comp : comparison object
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Value_t, class Compare>
static Value_t *merge_construct(Iter1_t first1, const Iter1_t last1,
                                Iter2_t first2, const Iter2_t last2,
                                Value_t *it_out, Compare comp)
{
    //-------------------------------------------------------------------------
    //                       Metaprogramming
    //------------------------------------------------------------------------- 
    typedef value_iter<Iter1_t> type1;
    typedef value_iter<Iter2_t> type2;
    static_assert (std::is_same< Value_t, type1 >::value,
                    "Incompatible iterators\n");
    static_assert (std::is_same< Value_t, type2 >::value,
                    "Incompatible iterators\n");

    //-------------------------------------------------------------------------
    //                       Code
    //-------------------------------------------------------------------------
    const size_t MIN_CHECK = 1024;

    if (size_t((last1 - first1) + (last2 - first2)) >= MIN_CHECK)
    {
        if (first1 == last1) return move_construct(it_out, first2, last2);
        if (first2 == last2) return move_construct(it_out, first1, last1);

        if (not comp(*first2, *(last1 - 1)))
        {
            Value_t* mid = move_construct(it_out, first1, last1);
            return move_construct(mid, first2, last2);
        };

        if (comp(*(last2 - 1), *first1))
        {
            Value_t* mid = move_construct(it_out, first2, last2);
            return move_construct(mid, first1, last1);
        };
    };
    while (first1 != last1 and first2 != last2)
    {
        construct_object((it_out++),
                        (not comp(*first2, *first1)) ?
                                        std::move(*(first1++)) :
                                        std::move(*(first2++)));
    };
    return (first1 == last1) ?
                    move_construct(it_out, first2, last2) :
                    move_construct(it_out, first1, last1);
};
//
//---------------------------------------------------------------------------
//  function : merge_half
/// @brief : Merge two buffers. The first buffer is in a separate memory.
///          The second buffer have a empty space before buf2 of the same size
///          than the (end_buf1 - buf1)
///
/// @param buf1 : iterator to the first element of the first buffer
/// @param end_buf1 : iterator to the last element of the first buffer
/// @param buf2 : iterator to the first element of the second buffer
/// @param end_buf2 : iterator to the last element of the second buffer
/// @param buf_out : iterator to the first element to the buffer where put
///                  the result
/// @param comp : object for Compare two elements of the type pointed
///                by the Iter1_t and Iter2_t
//---------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Compare>
static Iter2_t merge_half(Iter1_t buf1, const Iter1_t end_buf1, Iter2_t buf2,
                          const Iter2_t end_buf2, Iter2_t buf_out, Compare comp)
{
    //-------------------------------------------------------------------------
    //                         Metaprogramming
    //------------------------------------------------------------------------- 
    typedef value_iter<Iter1_t> value1_t;
    typedef value_iter<Iter2_t> value2_t;
    static_assert (std::is_same< value1_t, value2_t >::value,
                    "Incompatible iterators\n");

    //-------------------------------------------------------------------------
    //                         Code
    //-------------------------------------------------------------------------
#ifdef __BS_DEBUG
    assert ( (buf2 - buf_out) == ( end_buf1 - buf1));
#endif
    const size_t MIN_CHECK = 1024;

    if (size_t((end_buf1 - buf1) + (end_buf2 - buf2)) >= MIN_CHECK)
    {
        if (buf1 == end_buf1) return end_buf2;
        if (buf2 == end_buf2) return move_forward(buf_out, buf1, end_buf1);

        if (not comp(*buf2, *(end_buf1 - 1)))
        {
            move_forward(buf_out, buf1, end_buf1);
            return end_buf2;
        };

        if (comp(*(end_buf2 - 1), *buf1))
        {
            Iter2_t mid = move_forward(buf_out, buf2, end_buf2);
            return move_forward(mid, buf1, end_buf1);
        };
    };
    while ((buf1 != end_buf1) and (buf2 != end_buf2))
    {
        *(buf_out++) = (not comp(*buf2, *buf1)) ?
                        std::move(*(buf1++)) : std::move(*(buf2++));
    };
    return (buf2 == end_buf2)? move_forward(buf_out, buf1, end_buf1) : end_buf2;
};

//
//---------------------------------------------------------------------------
//  function : merge_half_backward
/// @brief : Merge two buffers. The first buffer is in a separate memory.
///          The second buffer have a empty space before buf2 of the same size
///          than the (end_buf1 - buf1)
///
/// @param buf1 : iterator to the first element of the first buffer
/// @param end_buf1 : iterator to the last element of the first buffer
/// @param buf2 : iterator to the first element of the second buffer
/// @param end_buf2 : iterator to the last element of the second buffer
/// @param buf_out : iterator to the first element to the buffer where put
///                  the result
/// @param comp : object for Compare two elements of the type pointed
///                by the Iter1_t and Iter2_t
//---------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Compare>
static Iter2_t merge_half_backward(Iter1_t buf1, Iter1_t end_buf1, Iter2_t buf2,
                                   Iter2_t end_buf2, Iter1_t end_buf_out,
                                   Compare comp)
{
    //-------------------------------------------------------------------------
    //                         Metaprogramming
    //-------------------------------------------------------------------------
    typedef value_iter<Iter1_t> value1_t;
    typedef value_iter<Iter2_t> value2_t;
    static_assert (std::is_same< value1_t, value2_t >::value,
                    "Incompatible iterators\n");

    //-------------------------------------------------------------------------
    //                         Code
    //-------------------------------------------------------------------------
#ifdef __BS_DEBUG
    assert ((end_buf_out - end_buf1) == (end_buf2 - buf2) );
#endif
    const size_t MIN_CHECK = 1024;

    if (size_t((end_buf1 - buf1) + (end_buf2 - buf2)) >= MIN_CHECK)
    {
        if (buf2 == end_buf2) return buf1;
        if (buf1 == end_buf1)
            return here::move_backward(end_buf_out, buf2, end_buf2);

        if (not comp(*buf2, *(end_buf1 - 1)))
        {
            here::move_backward(end_buf_out, buf2, end_buf2);
            return buf1;
        };

        if (comp(*(end_buf2 - 1), *buf1))
        {
            Iter1_t mid = here::move_backward(end_buf_out, buf1, end_buf1);
            return here::move_backward(mid, buf2, end_buf2);
        };
    };
    while ((buf1 != end_buf1) and (buf2 != end_buf2))
    {
        *(--end_buf_out) =
                        (not comp(*(end_buf2 - 1), *(end_buf1 - 1))) ?
                                        std::move(*(--end_buf2)):
                                        std::move(*(--end_buf1));
    };
    return (buf1 == end_buf1) ?
                    here::move_backward(end_buf_out, buf2, end_buf2) : buf1;
};

//
//-----------------------------------------------------------------------------
//  function : merge_uncontiguous
/// @brief : merge two uncontiguous buffers, placing the results in the buffers
///          Use an auxiliary buffer pointed by aux
///
/// @param src1 : iterator to the first element of the first buffer
/// @param end_src1 : last iterator  of the first buffer
/// @param src2 : iterator to the first element of the second buffer
/// @param end_src2 : last iterator  of the second buffer
/// @param aux  : iterator to the first element of the auxiliary buffer
/// @param comp : object for to Compare elements
/// @return true : not changes done,  false : changes in the buffers
/// @remarks
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Iter3_t, class Compare>
static bool merge_uncontiguous(Iter1_t src1, const Iter1_t end_src1,
                               Iter2_t src2, const Iter2_t end_src2,
                               Iter3_t aux, Compare comp)
{
    //-------------------------------------------------------------------------
    //                    Metaprogramming
    //------------------------------------------------------------------------- 
    typedef value_iter<Iter1_t> type1;
    typedef value_iter<Iter2_t> type2;
    typedef value_iter<Iter3_t> type3;
    static_assert (std::is_same< type1, type2 >::value,
                    "Incompatible iterators\n");
    static_assert (std::is_same< type3, type2 >::value,
                    "Incompatible iterators\n");

    //-------------------------------------------------------------------------
    //                    Code
    //-------------------------------------------------------------------------
    if (src1 == end_src1 or src2 == end_src2
                    or not comp(*src2, *(end_src1 - 1))) return true;

    while (src1 != end_src1 and not comp(*src2, *src1))
        ++src1;

    Iter3_t const end_aux = aux + (end_src1 - src1);
    Iter2_t src2_first = src2;
    move_forward(aux, src1, end_src1);

    while ((src1 != end_src1) and (src2 != end_src2))
    {
        *(src1++) = std::move((not comp(*src2, *aux)) ? *(aux++) : *(src2++));
    }

    if (src2 == end_src2)
    {
        while (src1 != end_src1)
            *(src1++) = std::move(*(aux++));
        move_forward(src2_first, aux, end_aux);
    }
    else
    {
        merge_half(aux, end_aux, src2, end_src2, src2_first, comp);
    };
    return false;
};

//
//-----------------------------------------------------------------------------
//  function : merge_contiguous
/// @brief : merge two contiguous buffers,using an auxiliary buffer pointed
///          by buf. The results are in src1 and src2
///
/// @param src1: iterator to the first position of the first buffer
/// @param src2: final iterator of the first buffer and first iterator
///              of the second buffer
/// @param end_src2 : final iterator of the second buffer
/// @param buf  : iterator to buffer used as auxiliary memory
/// @param comp : object for to Compare elements
/// @return true : not changes done,  false : changes in the buffers
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Compare>
static bool merge_contiguous(Iter1_t src1, Iter1_t src2, Iter1_t end_src2,
                             Iter2_t buf, Compare comp)
{
    //-------------------------------------------------------------------------
    //                      Metaprogramming
    //------------------------------------------------------------------------- 
    typedef value_iter<Iter1_t> type1;
    typedef value_iter<Iter2_t> type2;
    static_assert (std::is_same< type1, type2 >::value,
                    "Incompatible iterators\n");

    //-------------------------------------------------------------------------
    //                         Code
    //-------------------------------------------------------------------------
    if (src1 == src2 or src2 == end_src2 or not comp(*src2, *(src2 - 1)))
        return true;

    Iter1_t end_src1 = src2;
    while (src1 != end_src1 and not comp(*src2, *src1))
        ++src1;

    if (src1 == end_src1) return false;

    size_t nx = end_src1 - src1;
    move_forward(buf, src1, end_src1);
    merge_half(buf, buf + nx, src2, end_src2, src1, comp);
    return false;
};
//
//-----------------------------------------------------------------------------
//  function : merge_circular
/// @brief : merge two buffers,using a circular buffer
///          This function don't check the parameters
/// @param buf1: iterator to the first position of the first buffer
/// @param end_buf1: iterator after the last element of the first buffer
/// @param buf2: iterator to the first element of the secind buffer
/// @param end_buf2: iterator to the first element of the secind buffer
/// @param circ : circular buffer
/// @param comp : comparison object
/// @return true : finished buf1,  false : finished buf2
/// @comments : be carefully because the iterators buf1 and buf2 are modified
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t, class Circular, class Compare>
static bool merge_circular(Iter1_t buf1, Iter1_t end_buf1, Iter2_t buf2,
                           Iter2_t end_buf2, Circular &circ, Compare comp,
                           Iter1_t &it1_out, Iter2_t &it2_out)
{
    //-------------------------------------------------------------------------
    //                      Metaprogramming
    //-------------------------------------------------------------------------
    typedef value_iter<Iter1_t> type1;
    typedef value_iter<Iter2_t> type2;
    static_assert (std::is_same< type1, type2 >::value,
                    "Incompatible iterators\n");
    typedef typename Circular::value_t type3;
    static_assert (std::is_same<type1, type3>::value,
                    "Incompatible iterators\n");

    //-------------------------------------------------------------------------
    //                      Code
    //-------------------------------------------------------------------------
#ifdef __BS_DEBUG
    assert ( circ.free_size() >= size_t ((end_buf1-buf1) + (end_buf2-buf2)));
#endif

    if (not comp(*buf2, *(end_buf1 - 1)))
    {
        circ.push_move_back(buf1, (end_buf1 - buf1));
        it1_out = end_buf1;
        it2_out = buf2;
        return true;
    };
    if (comp(*(end_buf2 - 1), *buf1))
    {
        circ.push_move_back(buf2, (end_buf2 - buf2));
        it1_out = buf1;
        it2_out = end_buf2;
        return false;
    }
    while (buf1 != end_buf1 and buf2 != end_buf2)
    {
        circ.push_back(comp(*buf2, *buf1) ? std::move(*(buf2++))
                                          : std::move(*(buf1++)));
    };
    it2_out = buf2;
    it1_out = buf1;
    bool ret = (buf1 == end_buf1);
    return ret;
};
//
//****************************************************************************
};//    End namespace util
};//    End namespace common
};//    End namespace sort
};//    End namespace boost
//****************************************************************************
//
#endif
