//----------------------------------------------------------------------------
/// @file   circular_buffer.hpp
/// @brief  This file contains the implementation of the circular buffer
///
/// @author Copyright (c) 2010 2015 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanyingfile LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_UTIL_CIRCULAR_BUFFER_HPP
#define __BOOST_SORT_COMMON_UTIL_CIRCULAR_BUFFER_HPP

#include <memory>
#include <cassert>
#include <exception>
#include <boost/sort/common/util/algorithm.hpp>
#include <boost/sort/common/util/traits.hpp>

namespace boost
{
namespace sort
{
namespace common
{
namespace util
{

//---------------------------------------------------------------------------
/// @class  circular_buffer
/// @brief  This class implement a circular buffer
/// @remarks
//---------------------------------------------------------------------------
template <class Value_t, uint32_t Power2 = 11>
struct circular_buffer
{
    //------------------------------------------------------------------------
    //                          STATIC CHECK
    //------------------------------------------------------------------------
    static_assert ( Power2 != 0, "Wrong Power2");

    //------------------------------------------------------------------------
    //                          DEFINITIONS
    //------------------------------------------------------------------------
    typedef Value_t value_t;

    //------------------------------------------------------------------------
    //                          VARIABLES
    //------------------------------------------------------------------------
    const size_t NMAX = (size_t) 1 << Power2;
    const size_t MASK = (NMAX - 1);
    const size_t BLOCK_SIZE = NMAX >> 1;
    const size_t LOG_BLOCK = Power2 - 1;
    Value_t * ptr = nullptr;

    //------------------------------------------------------------------------
    // first and last are  the position of the first and last elements
    // always are in the range [0, NMAX - 1]
    //------------------------------------------------------------------------
    size_t nelem, first_pos;
    bool initialized;

    //
    //------------------------------------------------------------------------
    //  function : circular_buffer
    /// @brief  constructor of the class
    //-----------------------------------------------------------------------
    circular_buffer(void)
    : ptr(nullptr), nelem(0), first_pos(0), initialized(false)
    {
        ptr = std::get_temporary_buffer < Value_t > (NMAX).first;
        if (ptr == nullptr) throw std::bad_alloc();
    };
    //
    //------------------------------------------------------------------------
    //  function : ~circular_buffer
    /// @brief destructor of the class
    //-----------------------------------------------------------------------
    ~circular_buffer()
    {
        if (initialized)
        {   for (size_t i = 0; i < NMAX; ++i) (ptr + i)->~Value_t();
            initialized = false;
        };
        std::return_temporary_buffer(ptr);
    }
    ;
    //
    //------------------------------------------------------------------------
    //  function : initialize
    /// @brief : initialize the memory of the buffer from the uninitialize
    //           memory obtained from the temporary buffer
    /// @param val : value used to initialize the memory
    //-----------------------------------------------------------------------
    void initialize(Value_t & val)
    {
        assert (initialized == false);
        ::new (static_cast<void*>(ptr)) Value_t(std::move(val));
        for (size_t i = 1; i < NMAX; ++i)
            ::new (static_cast<void*>(ptr + i)) Value_t(std::move(ptr[i - 1]));
        val = std::move(ptr[NMAX - 1]);
        initialized = true;
    };
    //
    //------------------------------------------------------------------------
    //  function : destroy_all
    /// @brief : destroy all the objects in the internal memory
    //-----------------------------------------------------------------------
    void destroy_all(void) { destroy(ptr, ptr + NMAX); };
    //
    //------------------------------------------------------------------------
    //  function : get_buffer
    /// @brief return the internal memory of the circular buffer
    /// @return pointer to the internal memory of the buffer
    //-----------------------------------------------------------------------
    Value_t * get_buffer(void) { return ptr; };
    //
    //------------------------------------------------------------------------
    //  function : empty
    /// @brief return if the buffer is empty
    /// @return true : empty
    //-----------------------------------------------------------------------
    bool empty(void) const {return (nelem == 0); };
    //
    //------------------------------------------------------------------------
    //  function : full
    /// @brief return if the buffer is full
    /// @return true : full
    //-----------------------------------------------------------------------
    bool full(void) const { return (nelem == NMAX); };
    //
    //------------------------------------------------------------------------
    //  function : size
    /// @brief return the number of elements stored in the buffer
    /// @return number of elements stored
    //-----------------------------------------------------------------------
    size_t size(void) const { return nelem;};
    //
    //------------------------------------------------------------------------
    //  function : capacity
    /// @brief : return the maximun capacity of the buffer
    /// @return number of elements
    //-----------------------------------------------------------------------
    size_t capacity(void) const { return NMAX;};
    //
    //------------------------------------------------------------------------
    //  function : free_size
    /// @brief return the free positions in the buffer
    /// @return number of elements
    //-----------------------------------------------------------------------
    size_t free_size(void) const  { return (NMAX - nelem); };
    //
    //------------------------------------------------------------------------
    //  function : clear
    /// @brief clear the buffer
    //-----------------------------------------------------------------------
    void clear(void)  { nelem = first_pos = 0; };
    //
    //------------------------------------------------------------------------
    //  function : front
    /// @brief return the first element of the buffer
    /// @return reference to the first value
    //-----------------------------------------------------------------------
    Value_t & front(void)
    {
#ifdef __BS_DEBUG
        assert (nelem > 0);
#endif
        return (ptr[first_pos]);
    };
    //
    //------------------------------------------------------------------------
    //  function :front
    /// @brief return the first element of the buffer
    /// @return const reference to the first value
    //-----------------------------------------------------------------------
    const Value_t & front(void) const
    {
#ifdef __BS_DEBUG
        assert ( nelem > 0 );
#endif
        return (ptr[first_pos]);
    };
    //
    //------------------------------------------------------------------------
    //  function : back
    /// @brief reference to the last value of the buffer
    /// @return reference to the last value
    //-----------------------------------------------------------------------
    Value_t & back(void)
    {
#ifdef __BS_DEBUG
        assert ( nelem > 0 );
#endif
        return (ptr[(first_pos + nelem - 1) & MASK]);
    };
    //
    //------------------------------------------------------------------------
    //  function : back
    /// @brief reference to the last value of the buffer
    /// @return const reference to the last value
    //-----------------------------------------------------------------------
    const Value_t & back(void) const
    {
#ifdef __BS_DEBUG
        assert ( nelem > 0 );
#endif
        return (ptr[(first_pos + nelem - 1) & MASK]);
    };
    //
    //------------------------------------------------------------------------
    //  function : operator []
    /// @brief positional access to the elements
    /// @param pos rquested
    /// @return reference to the element
    //-----------------------------------------------------------------------
    Value_t & operator[](uint32_t pos)
    {
#ifdef __BS_DEBUG
        assert ( nelem > 0 );
#endif
        return ptr[(first_pos + pos) & MASK];
    };
    //
    //------------------------------------------------------------------------
    //  function : operator []
    /// @brief positional access to the elements
    /// @param pos rquested
    /// @return const reference to the element
    //-----------------------------------------------------------------------
    const Value_t & operator[](uint32_t pos) const
    {

#ifdef __BS_DEBUG
        assert ( nelem > 0 );
#endif
        return ptr[(first_pos + pos) & MASK];
    };
    //
    //------------------------------------------------------------------------
    //  function : push_front
    /// @brief insert an element in the first position of the buffer
    /// @param val : const value to insert
    //-----------------------------------------------------------------------
    void push_front(const Value_t & val)
    {
#ifdef __BS_DEBUG
        assert ( nelem != NMAX);
#endif
        ++nelem;
        first_pos = ((first_pos + MASK) & MASK);
        ptr[first_pos] = val;

    };
    //
    //------------------------------------------------------------------------
    //  function : push_front
    /// @brief insert an element in the first position of the buffer
    /// @param val : rvalue to insert
    //-----------------------------------------------------------------------
    void push_front(Value_t && val)
    {
#ifdef __BS_DEBUG
        assert ( nelem != NMAX);
#endif
        ++nelem;
        first_pos = ((first_pos + MASK) & MASK);
        ptr[first_pos] = val;
    };
    //
    //------------------------------------------------------------------------
    //  function : push_back
    /// @brief insert an element in the last position of the buffer
    /// @param val : value to insert
    //-----------------------------------------------------------------------
    void push_back(const Value_t & val)
    {
#ifdef __BS_DEBUG
        assert ( nelem != NMAX);
#endif
        ptr[(first_pos + (nelem++)) & MASK] = val;
    };
    //
    //------------------------------------------------------------------------
    //  function : push_back
    /// @brief insert an element in the last position of the buffer
    /// @param val : value to insert
    //-----------------------------------------------------------------------
    void push_back(Value_t && val)
    {
#ifdef __BS_DEBUG
        assert ( nelem != NMAX);
#endif
        ptr[(first_pos + (nelem++)) & MASK] = std::move(val);
    };
    //
    //------------------------------------------------------------------------
    //  function : pop_front
    /// @brief remove the first element of the buffer
    //-----------------------------------------------------------------------
    void pop_front(void)
    {
#ifdef __BS_DEBUG
        assert ( nelem > 0 );
#endif
        --nelem;
        (++first_pos) &= MASK;
    };
    //
    //------------------------------------------------------------------------
    //  function : pop_back
    /// @brief remove the last element of the buffer
    //-----------------------------------------------------------------------
    void pop_back(void)
    {
#ifdef __BS_DEBUG
        assert ( nelem > 0 );
#endif
        --nelem;
    };

    template<class iter_t>
    void pop_copy_front(iter_t it_dest, size_t num);

    template<class iter_t>
    void pop_move_front(iter_t it_dest, size_t num);

    template<class iter_t>
    void pop_copy_back(iter_t it_dest, size_t num);

    template<class iter_t>
    void pop_move_back(iter_t it_dest, size_t num);

    template<class iter_t>
    void push_copy_front(iter_t it_src, size_t num);

    template<class iter_t>
    void push_move_front(iter_t it_src, size_t num);

    template<class iter_t>
    void push_copy_back(iter_t it_src, size_t num);

    template<class iter_t>
    void push_move_back(iter_t it_src, size_t num);

//---------------------------------------------------------------------------
};//               End of class circular_buffer
//---------------------------------------------------------------------------
//
//
//############################################################################
//                                                                          ##
//             N O N    I N L I N E    F U N C T I O N S                    ##
//                                                                          ##
//############################################################################
//
//------------------------------------------------------------------------
//  function : pop_copy_front
/// @brief copy and delete num elements from the front of the buffer
/// @param it_dest : iterator to the first position where copy the elements
/// @param num : number of elements to copy
//-----------------------------------------------------------------------
template <class Value_t, uint32_t Power2>
template<class iter_t>
void circular_buffer<Value_t, Power2>
::pop_copy_front(iter_t it_dest, size_t num)
{
    static_assert ( std::is_same <value_iter<iter_t>, Value_t>::value,
                    "Incompatible iterator");
    if (num == 0) return;
#ifdef __BS_DEBUG
    assert ( num <= nelem);
#endif
    nelem -= num;
    size_t pos = first_pos;
    first_pos = (first_pos + num) & MASK;
    for (size_t i = 0; i < num; ++i)
    {
        *(it_dest++) = ptr[pos++ & MASK];
    };
    first_pos &= MASK;
};
//
//------------------------------------------------------------------------
//  function : pop_move_front
/// @brief move num elements from the front of the buffer to the place
//         pointed by it_dest
/// @param it_dest : iterator to the first position where move the elements
/// @param num : number of elements to move
//-----------------------------------------------------------------------
template <class Value_t, uint32_t Power2>
template<class iter_t>
void circular_buffer<Value_t, Power2>
:: pop_move_front(iter_t it_dest, size_t num)
{
    static_assert ( std::is_same <value_iter<iter_t>, Value_t>::value,
                    "Incompatible iterator");
    if (num == 0) return;
#ifdef __BS_DEBUG
    assert ( num <= nelem);
#endif
    nelem -= num;
    size_t pos = first_pos;
    first_pos = (first_pos + num) & MASK;
    for (size_t i = 0; i < num; ++i)
    {
        *(it_dest++) = std::move(ptr[pos++ & MASK]);
    };
    first_pos &= MASK;
};
//
//------------------------------------------------------------------------
//  function : pop_copy_back
/// @brief copy and delete num elements from the back of the buffer
/// @param p1 : iterator where begin to copy the elements
/// @param num : number of elements to copy
//-----------------------------------------------------------------------
template <class Value_t, uint32_t Power2>
template<class iter_t>
void circular_buffer<Value_t, Power2>
::pop_copy_back(iter_t it_dest, size_t num)
{
    static_assert ( std::is_same <value_iter<iter_t>, Value_t>::value,
                    "Incompatible iterator");
    if (num == 0) return;
#ifdef __BS_DEBUG
    assert ( num <= nelem);
#endif
    nelem -= num;
    size_t pos = (first_pos + nelem) & MASK;
    for (size_t i = 0; i < num; ++i)
    {
        *(it_dest++) = ptr[pos++ & MASK];
    };
};
//
//------------------------------------------------------------------------
//  function : pop_move_back
/// @brief move and delete num elements from the back of the buffer
/// @param p1 : iterator where begin to move the elements
/// @param num : number of elements to move
//-----------------------------------------------------------------------
template <class Value_t, uint32_t Power2>
template<class iter_t>
void circular_buffer<Value_t, Power2>
::pop_move_back(iter_t it_dest, size_t num)
{
    static_assert ( std::is_same <value_iter<iter_t>, Value_t>::value,
                    "Incompatible iterator");
    if (num == 0) return;
#ifdef __BS_DEBUG
    assert ( num <= nelem);
#endif
    nelem -= num;
    size_t pos = (first_pos + nelem) & MASK;
    for (size_t i = 0; i < num; ++i)
    {
        *(it_dest++) = std::move(ptr[pos++ & MASK]);
    };
};
//
//------------------------------------------------------------------------
//  function : push_copy_front
/// @brief copy num elements in the front of the buffer
/// @param it_src : iterator from where begin to copy the elements
/// @param mun : number of element to copy
//-----------------------------------------------------------------------
template <class Value_t, uint32_t Power2>
template<class iter_t>
void circular_buffer<Value_t, Power2>
::push_copy_front(iter_t it_src, size_t num)
{
    static_assert ( std::is_same <value_iter<iter_t>, Value_t>::value,
                    "Incompatible iterator");
    if (num == 0) return;
#ifdef __BS_DEBUG
    assert ( free_size() >= num);
#endif
    nelem += num;

    first_pos = (first_pos + NMAX - num) & MASK;
    size_t pos = first_pos;
    for (size_t i = 0; i < num; ++i)
    {
        ptr[(pos++) & MASK] = *(it_src++);
    };
};
//
//------------------------------------------------------------------------
//  function : push_move_front
/// @brief move num elements in the front of the buffer
/// @param p1 : iterator from where begin to move the elements
/// @param mun : number of element to move
//-----------------------------------------------------------------------
template <class Value_t, uint32_t Power2>
template<class iter_t>
void circular_buffer<Value_t, Power2>
::push_move_front(iter_t it_src, size_t num)
{
    static_assert ( std::is_same <value_iter<iter_t>, Value_t>::value,
                    "Incompatible iterator");
    if (num == 0) return;
#ifdef __BS_DEBUG
    assert ( free_size() >= num);
#endif
    nelem += num;
    size_t pos = first_pos;
    for (size_t i = 0; i < num; ++i)
    {
        ptr[(pos++) & MASK] = std::move(*(it_src++));
    };
};
//
//------------------------------------------------------------------------
//  function : push_copy_back
/// @brief copy num elements in the back of the buffer
/// @param p1 : iterator from where begin to copy the elements
/// @param mun : number of element to copy
//-----------------------------------------------------------------------
template <class Value_t, uint32_t Power2>
template<class iter_t>
void circular_buffer<Value_t, Power2>
::push_copy_back(iter_t it_src, size_t num)
{
    static_assert ( std::is_same <value_iter<iter_t>, Value_t>::value,
                    "Incompatible iterator");
    if (num == 0) return;
#ifdef __BS_DEBUG
    assert ( free_size() >= num);
#endif
    size_t pos = first_pos + nelem;
    nelem += num;
    for (size_t i = 0; i < num; ++i)
    {
        ptr[(pos++) & MASK] = *(it_src++);
    };
};
//
//------------------------------------------------------------------------
//  function : push_move_back
/// @brief move num elements in the back of the buffer
/// @param p1 : iterator from where begin to move the elements
/// @param mun : number of element to move
//-----------------------------------------------------------------------
template <class Value_t, uint32_t Power2>
template<class iter_t>
void circular_buffer<Value_t, Power2>
::push_move_back(iter_t it_src, size_t num)
{
    static_assert ( std::is_same <value_iter<iter_t>, Value_t>::value,
                    "Incompatible iterator");
    if (num == 0) return;
#ifdef __BS_DEBUG
    assert ( free_size() >= num);
#endif
    size_t pos = first_pos + nelem;
    nelem += num;
    for (size_t i = 0; i < num; ++i)
    {
        ptr[(pos++) & MASK] = std::move(*(it_src++));
    };
};

//****************************************************************************
};// End namespace util
};// End namespace common
};// End namespace sort
};// End namespace boost
//****************************************************************************
#endif
