//----------------------------------------------------------------------------
/// @file   deque_cnc.hpp
/// @brief  This file contains the implementation of the several types of
///         recursive fastmutex for read and write
///
/// @author Copyright (c) 2010 2015 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanyingfile LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __TOOLS_DEQUE_CNC_HPP
#define __TOOLS_DEQUE_CNC_HPP

#include <sort/tools/spinlock.hpp>
#include <vector>
#include <deque>

namespace sort
{
namespace tools
{

//###########################################################################
//                                                                         ##
//    ################################################################     ##
//    #                                                              #     ##
//    #                      C L A S S                               #     ##
//    #                   S T A C K _ C N C                          #     ##
//    #                                                              #     ##
//    ################################################################     ##
//                                                                         ##
//###########################################################################
//
//---------------------------------------------------------------------------
/// @class  deque_cnc
/// @brief This class is a concurrent stack controled by a spin_lock
/// @remarks
//---------------------------------------------------------------------------
template<typename T, typename Allocator = std::allocator<T> >
class deque_cnc
{
public:
    //-----------------------------------------------------------------------
    //                     D E F I N I T I O N S
    //-----------------------------------------------------------------------
    typedef std::deque<T, Allocator>                deque_t;
    typedef typename deque_t::size_type             size_type;
    typedef typename deque_t::difference_type       difference_type;
    typedef typename deque_t::value_type            value_type;
    typedef typename deque_t::pointer               pointer;
    typedef typename deque_t::const_pointer         const_pointer;
    typedef typename deque_t::reference             reference;
    typedef typename deque_t::const_reference       const_reference;
    typedef typename deque_t::allocator_type        allocator_type;

protected:
    //------------------------------------------------------------------------
    //                     VARIABLES
    //------------------------------------------------------------------------
    deque_t dq;
    mutable spinlock spl;

public:
    //
    //-----------------------------------------------------------------------
    //  C O N S T R U C T O R S     A N D    D E S T R U C T O R
    //-----------------------------------------------------------------------
    //
    //-----------------------------------------------------------------------
    //  function : deque_cnc
    /// @brief  constructor
    //----------------------------------------------------------------------
    explicit inline deque_cnc(void): dq() { };
//
    //----------------------------------------------------------------------
    //  function : deque_cnc
    /// @brief  constructor
    /// @param [in] ALLC : Allocator
    //----------------------------------------------------------------------
    explicit inline deque_cnc(const Allocator &ALLC): dq(ALLC){ };
    //
    //----------------------------------------------------------------------
    //  function : ~deque_cnc
    /// @brief  Destructor
    //----------------------------------------------------------------------
    virtual ~deque_cnc(void){ dq.clear(); };
    //
    //----------------------------------------------------------------------
    //  function : clear
    /// @brief Delete all the elements of the deque_cnc.
    //----------------------------------------------------------------------
    void clear(void)
    {
        std::lock_guard < spinlock > S(spl);
        dq.clear();
    };
    //
    //------------------------------------------------------------------------
    //  function : swap
    /// @brief swap the data between the two deque_cnc
    /// @param [in] A : deque_cnc to swap
    /// @return none
    //-----------------------------------------------------------------------
    void swap(deque_cnc & A) noexcept
    {
        if (this == &A) return;
        std::lock_guard < spinlock > S(spl);
        dq.swap(A.dq);
    };
    //
    //-----------------------------------------------------------------------
    //  S I Z E , M A X _ S I Z E , R E S I Z E
    //  C A P A C I T Y , E M P T Y , R E S E R V E
    //-----------------------------------------------------------------------
    //
    //------------------------------------------------------------------------
    //  function : size
    /// @brief return the number of elements in the deque_cnc
    /// @return number of elements in the deque_cnc
    //------------------------------------------------------------------------
    size_type size(void) const noexcept
    {
        std::lock_guard < spinlock > S(spl);
        return dq.size();
    };
    //
    //------------------------------------------------------------------------
    //  function :max_size
    /// @brief return the maximun size of the container
    /// @return maximun size of the container
    //------------------------------------------------------------------------
    size_type max_size(void) const noexcept
    {
        std::lock_guard < spinlock > S(spl);
        return (dq.max_size());
    };
    //
    //-------------------------------------------------------------------------
    //  function : shrink_to_fit
    /// @brief resize the current vector size and change to size.\n
    ///        If sz is smaller than the current size, delete elements to end\n
    ///        If sz is greater than the current size, insert elements to the
    ///        end with the value c
    /// @param [in] sz : new size of the deque_cnc after the resize
    /// @param [in] c : Value to insert if sz is greather than the current size
    /// @return none
    //------------------------------------------------------------------------
    void shrink_to_fit()
    {
        std::lock_guard < spinlock > S(spl);
        dq.shrink_to_fit();
    };
    //
    //------------------------------------------------------------------------
    //  function : empty
    /// @brief indicate if the map is empty
    /// @return true if the map is empty, false in any other case
    //------------------------------------------------------------------------
    bool empty(void) const noexcept
    {
        std::lock_guard < spinlock > S(spl);
        return (dq.empty());
    };
    //---------------------------------------------------------------------------
    //  function : push_back
    /// @brief Insert one element in the back of the container
    /// @param [in] D : value to insert. Can ve a value, a reference or an
    ///                 rvalue
    //---------------------------------------------------------------------------
    void push_back(const value_type & D)
    {
        std::lock_guard < spinlock > S(spl);
        dq.push_back(D);
    };

    //------------------------------------------------------------------------
    //  function : emplace_back
    /// @brief Insert one element in the back of the container
    /// @param [in] args :group of arguments for to build the object to insert
    //-------------------------------------------------------------------------
    template<class ... Args>
    void emplace_back(Args && ... args)
    {
        std::lock_guard < spinlock > S(spl);
        dq.emplace_back(std::forward <Args>(args) ...);
    };
    //------------------------------------------------------------------------
    //  function : push_back
    /// @brief Insert one element in the back of the container
    /// @param [in] D : deque to insert in the actual deque, inserting a copy
    ///                  of the elements
    /// @return reference to the deque after the insertion
    //------------------------------------------------------------------------
    template<class Allocator2>
    deque_cnc & push_back(const std::deque<value_type, Allocator2> & D)
    {
        std::lock_guard < spinlock > S(spl);
        for (size_type i = 0; i < D.size(); ++i)
            dq.push_back(D[i]);
        return *this;
    };
    //------------------------------------------------------------------------
    //  function : push_back
    /// @brief Insert one element in the back of the container
    /// @param [in] D : deque to insert in the actual deque, inserting a move
    ///                 of the elements
    /// @return reference to the deque after the insertion
    //------------------------------------------------------------------------
    deque_cnc & push_back(std::deque<value_type, Allocator> && D)
    {
        std::lock_guard < spinlock > S(spl);
        for (size_type i = 0; i < D.size(); ++i)
            dq.emplace_back(std::move(D[i]));
        return *this;
    };
    //
    //------------------------------------------------------------------------
    //  function :pop_back
    /// @brief erase the last element of the container
    //-----------------------------------------------------------------------
    void pop_back(void)
    {
        std::lock_guard < spinlock > S(spl);
        dq.pop_back();
    };
    //
    //------------------------------------------------------------------------
    //  function :pop_copy_back
    /// @brief erase the last element and return a copy over P
    /// @param [out] P : reference to a variable where copy the element
    /// @return code of the operation
    ///         true - Element erased
    ///         false - Empty tree
    //------------------------------------------------------------------------
    bool pop_copy_back(value_type & P)
    {   //-------------------------- begin -----------------------------
        std::lock_guard < spinlock > S(spl);
        if (dq.size() == 0) return false;
        P = dq.back();
        dq.pop_back();
        return true;
    };
    //
    //------------------------------------------------------------------------
    //  function :pop_move_back
    /// @brief erase the last element and move over P
    /// @param [out] P : reference to a variable where move the element
    /// @return code of the operation
    ///         true - Element erased
    ///         false - Empty tree
    //------------------------------------------------------------------------
    bool pop_move_back(value_type & P)
    {   //-------------------------- begin -----------------------------
        std::lock_guard < spinlock > S(spl);
        if (dq.size() == 0) return false;
        P = std::move(dq.back());
        dq.pop_back();
        return true;
    };

    //------------------------------------------------------------------------
    //  function : push_front
    /// @brief Insert one copy of the element in the front of the container
    /// @param [in] D : value to insert
    //------------------------------------------------------------------------
    void push_front(const value_type & D)
    {
        std::lock_guard < spinlock > S(spl);
        dq.push_front(D);
    };

    //------------------------------------------------------------------------
    //  function : emplace_front
    /// @brief Insert one element in the front of the container
    /// @param [in] args :group of arguments for to build the object to insert
    //-------------------------------------------------------------------------
    template<class ... Args>
    void emplace_front(Args && ... args)
    {
        std::lock_guard < spinlock > S(spl);
        dq.emplace_front(std::forward <Args>(args) ...);
    };
    //------------------------------------------------------------------------
    //  function : push_front
    /// @brief Insert a copy of the elements of the deque V1 in the front
    ///        of the container
    /// @param [in] V1 : deque with the elements to insert
    /// @return reference to the deque after the insertion
    //------------------------------------------------------------------------
    template<class Allocator2>
    deque_cnc & push_front(const std::deque<value_type, Allocator2> & V1)
    {
        std::lock_guard < spinlock > S(spl);
        for (size_type i = 0; i < V1.size(); ++i)
            dq.push_front(V1[i]);
        return *this;
    };
    //-----------------------------------------------------------------------
    //  function : push_front
    /// @brief Insert a move of the elements of the deque V1 in the front
    ///        of the container
    /// @param [in] V1 : deque with the elements to insert
    /// @return reference to the deque after the insertion
    //-----------------------------------------------------------------------
    deque_cnc & push_front(std::deque<value_type, Allocator> && V1)
    {
        std::lock_guard < spinlock > S(spl);
        for (size_type i = 0; i < V1.size(); ++i)
            dq.emplace_front(std::move(V1[i]));
        return *this;
    };
    //
    //-----------------------------------------------------------------------
    //  function :pop_front
    /// @brief erase the first element of the container
    //-----------------------------------------------------------------------
    void pop_front(void)
    {
        std::lock_guard < spinlock > S(spl);
        dq.pop_front();
    };
    //
    //-----------------------------------------------------------------------
    //  function :pop_copy_front
    /// @brief erase the first element of the tree and return a copy over P
    /// @param [out] P : reference to a variable where copy the element
    /// @return code of the operation
    ///         true- Element erased
    ///         false - Empty tree
    //-----------------------------------------------------------------------
    bool pop_copy_front(value_type & P)
    {   //-------------------------- begin -----------------------------
        std::lock_guard < spinlock > S(spl);
        if (dq.size() == 0) return false;
        P = dq.front();
        dq.pop_front();
        return true;
    };
    //
    //------------------------------------------------------------------------
    //  function :pop_move_front
    /// @brief erase the first element of the tree and return a move over P
    /// @param [out] P : reference to a variable where move the element
    /// @return code of the operation
    ///         true- Element erased
    ///         false - Empty tree
    //------------------------------------------------------------------------
    bool pop_move_front(value_type & P)
    {   //-------------------------- begin -----------------------------
        std::lock_guard < spinlock > S(spl);
        if (dq.size() == 0) return false;
        P = std::move(dq.front());
        dq.pop_front();
        return true;
    };
};
// end class deque_cnc

//***************************************************************************
};// end namespace tools
};// end namespace sort
//***************************************************************************
#endif
