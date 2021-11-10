//----------------------------------------------------------------------------
/// @file atomic.hpp
/// @brief Basic layer for to simplify the use of atomic functions
/// @author Copyright(c) 2016 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_DETAIL_UTIL_ATOMIC_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_UTIL_ATOMIC_HPP

#include <atomic>
#include <cassert>
#include <type_traits>

namespace boost
{
namespace sort
{
namespace common
{
namespace util
{
//-----------------------------------------------------------------------------
//  function : atomic_read
/// @brief make the atomic read of an atomic variable, using a memory model
/// @param at_var : atomic variable to read
/// @return value obtained
//-----------------------------------------------------------------------------
template<typename T>
inline T atomic_read(std::atomic<T> &at_var)
{
    return std::atomic_load_explicit < T > (&at_var, std::memory_order_acquire);
};
//
//-----------------------------------------------------------------------------
//  function : atomic_add
/// @brief Add a number to an atomic variable, using a memory model
/// @param at_var : variable to add
/// @param num : value to add to at_var
/// @return result of the operation
//-----------------------------------------------------------------------------
template<typename T, typename T2>
inline T atomic_add(std::atomic<T> &at_var, T2 num)
{
    static_assert (std::is_integral< T2 >::value, "Bad parameter");
    return std::atomic_fetch_add_explicit <T> 
                               (&at_var, (T) num, std::memory_order_acq_rel);
};
//
//-----------------------------------------------------------------------------
//  function : atomic_sub
/// @brief Atomic subtract of an atomic variable using memory model
/// @param at_var : Varibale to subtract
/// @param num : value to sub to at_var
/// @return result of the operation
//-----------------------------------------------------------------------------
template<typename T, typename T2>
inline T atomic_sub(std::atomic<T> &at_var, T2 num)
{
    static_assert (std::is_integral< T2 >::value, "Bad parameter");
    return std::atomic_fetch_sub_explicit <T> 
                                (&at_var, (T) num, std::memory_order_acq_rel);
};
//
//-----------------------------------------------------------------------------
//  function : atomic_write
/// @brief Write a value in an atomic variable using memory model
/// @param at_var : varible to write
/// @param num : value to write in at_var
//-----------------------------------------------------------------------------
template<typename T, typename T2>
inline void atomic_write(std::atomic<T> &at_var, T2 num)
{
    static_assert (std::is_integral< T2 >::value, "Bad parameter");
    std::atomic_store_explicit <T> 
                                (&at_var, (T) num, std::memory_order_release);
};
template<typename T>
struct counter_guard
{
    typedef std::atomic<T> atomic_t;
    atomic_t &count;

    counter_guard(atomic_t & counter): count(counter) { };
    ~counter_guard() {atomic_sub(count, 1); };
};
//
//****************************************************************************
};// End namespace util
};// End namespace common
};// End namespace sort
};// End namespace boost
//****************************************************************************
#endif
