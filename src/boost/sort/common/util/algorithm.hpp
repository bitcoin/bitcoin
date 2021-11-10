//----------------------------------------------------------------------------
/// @file algorithm.hpp
/// @brief low level functions of create, destroy, move and merge functions
///
/// @author Copyright (c) 2017 Francisco Jose Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_UTIL_ALGORITHM_HPP
#define __BOOST_SORT_COMMON_UTIL_ALGORITHM_HPP

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>
#include <boost/sort/common/util/traits.hpp>

namespace boost
{
namespace sort
{
namespace common
{
namespace util
{
//
//###########################################################################
//
//                       I M P O R T A N T
//
// The functions of this file are for internal use only
// All the operations are done with move operations, because the copy
// operations are unnecesary
//
//###########################################################################
//
//----------------------------------------------------------------------------
//
//         F U N C T I O N S   I N   T H E   F I L E
//
//----------------------------------------------------------------------------
//
// static inline uint32_t nbits32 (uint32_t num) noexcept
//
// static inline uint32_t nbits64 (uint64_t num)
//
// template < class Value_t, class... Args >
// inline void construct_object (Value_t *ptr, Args &&... args)
//
// template < class Value_t >
// inline void destroy_object (Value_t *ptr)
//
// template < class Iter_t, class Value_t = value_iter<Iter_t> >
// void initialize (Iter_t first, Iter_t last, Value_t && val)
//
// template < class Iter1_t, class Iter2_t >
// Iter2_t move_forward (Iter2_t it_dest, Iter1_t first, Iter1_t last)
//
// template < class Iter1_t, class Iter2_t >
// Iter2_t move_backward (Iter2_t it_dest, Iter1_t first, Iter1_t last)
//
// template < class Iter_t, class Value_t = value_iter< Iter_t > >
// Value_t * move_construct (Value_t *ptr, Iter_t first, Iter_t last)
//
// template < class Iter_t >
// void destroy (Iter_t first, const Iter_t last)
//
// template < class Iter_t >
// void reverse (Iter_t first, const Iter_t last)
//
//----------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
//
//                    G L O B A L     V A R I B L E S
//
//--------------------------------------------------------------------------
//
// this array represent the number of bits needed for to represent the
// first 256 numbers
static constexpr const uint32_t tmsb[256] =
{ 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8,
                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };
//
//---------------------------------------------------------------------------
//
//                           F U N C T I O N S
//
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
//  function : nbits32
/// @brief Obtain the number of bits of a number equal or greater than num
/// @param num : Number to examine
/// @return Number of bits
//---------------------------------------------------------------------------
static inline uint32_t nbits32 (uint32_t num) noexcept
{
    int Pos = (num & 0xffff0000U) ? 16 : 0;
    if ((num >> Pos) & 0xff00U) Pos += 8;
    return (tmsb[num >> Pos] + Pos);
}
//
//---------------------------------------------------------------------------
//  function : nbits64
/// @brief Obtain the number of bits of a number equal or greater than num
/// @param num : Number to examine
/// @exception none
/// @return Number of bits
//---------------------------------------------------------------------------
static inline uint32_t nbits64(uint64_t num)noexcept
{
    uint32_t Pos = (num & 0xffffffff00000000ULL) ? 32 : 0;
    if ((num >> Pos) & 0xffff0000ULL) Pos += 16;
    if ((num >> Pos) & 0xff00ULL) Pos += 8;
    return (tmsb[num >> Pos] + Pos);
}
//
//-----------------------------------------------------------------------------
//  function : construct_object
/// @brief create an object in the memory specified by ptr
///
/// @param ptr : pointer to the memory where to create the object
/// @param args : arguments to the constructor
//-----------------------------------------------------------------------------
template <class Value_t, class ... Args>
inline void construct_object (Value_t *ptr, Args &&... args)
{
    (::new (static_cast<void *>(ptr)) Value_t(std::forward< Args > (args)...));
};
//
//-----------------------------------------------------------------------------
//  function : destroy_object
/// @brief destroy an object in the memory specified by ptr
/// @param ptr : pointer to the object to destroy
//-----------------------------------------------------------------------------
template<class Value_t>
inline void destroy_object(Value_t *ptr)
{
    ptr->~Value_t();
};
//
//-----------------------------------------------------------------------------
//  function : initialize
/// @brief initialize a range of objects with the object val moving across them
///
/// @param first : itertor to the first element to initialize
/// @param last : iterator to the last element to initialize
/// @param val : object used for the initialization
//-----------------------------------------------------------------------------
template <class Iter_t, class Value_t = value_iter<Iter_t> >
inline void initialize (Iter_t first, Iter_t last, Value_t & val)
{
    //------------------------------------------------------------------------
    //                  Metaprogramming
    //------------------------------------------------------------------------
    typedef value_iter<Iter_t> value_t;
    static_assert (std::is_same< Value_t, value_t >::value,
                    "Incompatible iterators\n");

    //------------------------------------------------------------------------
    //                 Code
    //------------------------------------------------------------------------
    if (first == last) return;
    construct_object(&(*first), std::move(val));

    Iter_t it1 = first, it2 = first + 1;
    while (it2 != last)
    {
        construct_object(&(*(it2++)), std::move(*(it1++)));
    };
    val = std::move(*(last - 1));
};
//
//-----------------------------------------------------------------------------
//  function : move_forward
/// @brief Move initialized objets
/// @param it_dest : iterator to the final place of the objects
/// @param first : iterator to the first element to move
/// @param last : iterator to the last element to move
/// @return Output iterator to the element past the last element
///         moved (it_dest + (last - first))
//-----------------------------------------------------------------------------
template <class Iter1_t, class Iter2_t>
inline Iter2_t move_forward (Iter2_t it_dest, Iter1_t first, Iter1_t last)
{
    //------------------------------------------------------------------------
    //                  Metaprogramming
    //------------------------------------------------------------------------
    typedef value_iter<Iter1_t> value1_t;
    typedef value_iter<Iter2_t> value2_t;
    static_assert (std::is_same< value1_t, value2_t >::value,
                    "Incompatible iterators\n");

    //------------------------------------------------------------------------
    //                 Code
    //------------------------------------------------------------------------
    while (first != last)
    {   *it_dest++ = std::move(*first++);
    }
    return it_dest;

};
//
//-----------------------------------------------------------------------------
//  function : move_backard
/// @brief Move initialized objets in reverse order
/// @param it_dest : last iterator to the final place of the objects
/// @param first : iterator to the first element to move
/// @param last : iterator to the last element to move
//-----------------------------------------------------------------------------
template<class Iter1_t, class Iter2_t>
inline Iter2_t move_backward(Iter2_t it_dest, Iter1_t  first, Iter1_t last)
{
    //------------------------------------------------------------------------
    //                  Metaprogramming
    //------------------------------------------------------------------------
    typedef value_iter<Iter1_t> value1_t;
    typedef value_iter<Iter2_t> value2_t;
    static_assert (std::is_same< value1_t, value2_t >::value,
                    "Incompatible iterators\n");

    //------------------------------------------------------------------------
    //                 Code
    //------------------------------------------------------------------------
    while (first != last)
    {   *(--it_dest) = std::move (*(--last));
    }
    return it_dest;
};

//
//-----------------------------------------------------------------------------
//  function : move_construct
/// @brief Move objets to uninitialized memory
///
/// @param ptr : pointer to the memory where to create the objects
/// @param first : iterator to the first element to move
/// @param last : iterator to the last element to move
//-----------------------------------------------------------------------------
template<class Iter_t, class Value_t = value_iter<Iter_t> >
inline Value_t * move_construct(Value_t *ptr, Iter_t first, Iter_t last)
{
    //------------------------------------------------------------------------
    //                  Metaprogramming
    //------------------------------------------------------------------------
    typedef typename iterator_traits<Iter_t>::value_type value2_t;
    static_assert (std::is_same< Value_t, value2_t >::value,
                    "Incompatible iterators\n");

    //------------------------------------------------------------------------
    //                    Code
    //------------------------------------------------------------------------
    while (first != last)
    {
        ::new (static_cast<void *>(ptr++)) Value_t(std::move(*(first++)));
    };
    return ptr;
};
//
//-----------------------------------------------------------------------------
//  function : destroy
/// @brief destroy the elements between first and last
/// @param first : iterator to the first element to destroy
/// @param last : iterator to the last element to destroy
//-----------------------------------------------------------------------------
template<class Iter_t>
inline void destroy(Iter_t first, const Iter_t last)
{
    while (first != last)
        destroy_object(&(*(first++)));
};
//
//-----------------------------------------------------------------------------
//  function : reverse
/// @brief destroy the elements between first and last
/// @param first : iterator to the first element to destroy
/// @param last : iterator to the last element to destroy
//-----------------------------------------------------------------------------
template<class Iter_t>
inline void reverse(Iter_t first, Iter_t last)
{
    std::reverse ( first, last);
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
