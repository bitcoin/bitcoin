//----------------------------------------------------------------------------
/// @file   stack_cnc.hpp
/// @brief  This file contains the implementation concurrent stack
///
/// @author Copyright (c) 2010 2015 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanyingfile LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_DETAIL_UTIL_STACK_CNC_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_UTIL_STACK_CNC_HPP

#include <boost/sort/common/spinlock.hpp>
#include <vector>

namespace boost
{
namespace sort
{
namespace common
{

//
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
/// @class  stack_cnc
/// @brief This class is a concurrent stack controled by a spin_lock
/// @remarks
//---------------------------------------------------------------------------
template<typename T, typename Allocator = std::allocator<T> >
class stack_cnc
{
public:
    //------------------------------------------------------------------------
    //                     D E F I N I T I O N S
    //------------------------------------------------------------------------
    typedef std::vector<T, Allocator> vector_t;
    typedef typename vector_t::size_type size_type;
    typedef typename vector_t::difference_type difference_type;
    typedef typename vector_t::value_type value_type;
    typedef typename vector_t::pointer pointer;
    typedef typename vector_t::const_pointer const_pointer;
    typedef typename vector_t::reference reference;
    typedef typename vector_t::const_reference const_reference;
    typedef typename vector_t::allocator_type allocator_type;
    typedef Allocator alloc_t;

protected:
    //-------------------------------------------------------------------------
    //                   INTERNAL VARIABLES
    //-------------------------------------------------------------------------
    vector_t v_t;
    mutable spinlock_t spl;

public:
    //
    //-------------------------------------------------------------------------
    //  function : stack_cnc
    /// @brief  constructor
    //-------------------------------------------------------------------------
    explicit stack_cnc(void): v_t() { };

    //
    //-------------------------------------------------------------------------
    //  function : stack_cnc
    /// @brief  Move constructor
    //-------------------------------------------------------------------------
    stack_cnc(stack_cnc &&) = delete;
    //
    //-------------------------------------------------------------------------
    //  function : ~stack_cnc
    /// @brief  Destructor
    //-------------------------------------------------------------------------
    virtual ~stack_cnc(void) { v_t.clear(); };

    //-------------------------------------------------------------------------
    //  function : emplace_back
    /// @brief Insert one element in the back of the container
    /// @param args : group of arguments for to build the object to insert. Can
    ///               be values, references or rvalues
    //-------------------------------------------------------------------------
    template<class ... Args>
    void emplace_back(Args &&... args)
    {
        std::lock_guard < spinlock_t > guard(spl);
        v_t.emplace_back(std::forward< Args > (args)...);
    };

    //
    //-------------------------------------------------------------------------
    //  function :pop_move_back
    /// @brief if exist, move the last element to P, and delete it
    /// @param P : reference to a variable where move the element
    /// @return  true  - Element moved and deleted
    ///          false - Empty stack_cnc
    //-------------------------------------------------------------------------
    bool pop_move_back(value_type &P)
    {
        std::lock_guard < spinlock_t > S(spl);
        if (v_t.size() == 0) return false;
        P = std::move(v_t.back());
        v_t.pop_back();
        return true;
    };
    //-------------------------------------------------------------------------
    //  function : push_back
    /// @brief Insert one vector at the end of the container
    /// @param v_other : vector to insert
    /// @return reference to the stack_cnc after the insertion
    //-------------------------------------------------------------------------
    template<class Allocator2>
    stack_cnc &push_back(const std::vector<value_type, Allocator2> &v_other)
    {
        std::lock_guard < spinlock_t > guard(spl);
        for (size_type i = 0; i < v_other.size(); ++i)
        {
            v_t.push_back(v_other[i]);
        }
        return *this;
    };
};
// end class stack_cnc

//***************************************************************************
};// end namespace common
};// end namespace sort
};// end namespace boost
//***************************************************************************
#endif
