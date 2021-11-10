//----------------------------------------------------------------------------
/// @file   scheduler.hpp
/// @brief  This file contains the implementation of the scheduler for
///         dispatch the works stored
///
/// @author Copyright (c) 2010 2015 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanyingfile LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_COMMON_SCHEDULER_HPP
#define __BOOST_SORT_COMMON_SCHEDULER_HPP

#include <boost/sort/common/spinlock.hpp>
#include <boost/sort/common/search.hpp>
#include <boost/sort/common/compare_traits.hpp>
#include <scoped_allocator>
#include <utility>
#include <vector>
#include <deque>
#include <iostream>
#include <unordered_map>

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
//    #           C L A S S      S C H E D U L E R                   #     ##
//    #                                                              #     ##
//    ################################################################     ##
//                                                                         ##
//###########################################################################

//
//---------------------------------------------------------------------------
/// @class  scheduler
/// @brief This class is a concurrent stack controled by a spin_lock
/// @remarks
//---------------------------------------------------------------------------
template<typename Func_t, typename Allocator = std::allocator<Func_t> >
struct scheduler
{
    //-----------------------------------------------------------------------
    //                     D E F I N I T I O N S
    //-----------------------------------------------------------------------
    typedef std::scoped_allocator_adaptor <Allocator>   scoped_alloc;
    typedef std::deque <Func_t, scoped_alloc>           deque_t;
    typedef typename deque_t::iterator                  it_deque;
    typedef std::thread::id                             key_t;
    typedef std::hash <key_t>                           hash_t;
    typedef std::equal_to <key_t>                       equal_t;
    typedef std::unique_lock <spinlock_t>               lock_t;
    typedef std::unordered_map <key_t, deque_t, hash_t, 
                        equal_t, scoped_alloc>          map_t;
    typedef typename map_t::iterator                    it_map;

    //-----------------------------------------------------------------------
    //                     V A R I A B L E S
    //-----------------------------------------------------------------------
    map_t mp;
    size_t nelem;
    mutable spinlock_t spl;

    //------------------------------------------------------------------------
    //  function : scheduler
    /// @brief  constructor
    //------------------------------------------------------------------------
    scheduler(void) : mp(), nelem(0)  { };
    //
    //-----------------------------------------------------------------------
    //  function : scheduler
    /// @brief  Copy & move constructor
    /// @param [in] VT : stack_cnc from where copy the data
    //-----------------------------------------------------------------------
    scheduler(scheduler && VT) = delete;
    scheduler(const scheduler & VT) = delete;
    //
    //------------------------------------------------------------------------
    //  function : ~scheduler
    /// @brief  Destructor
    //------------------------------------------------------------------------
    virtual ~scheduler(void) {mp.clear();};
    //
    //------------------------------------------------------------------------
    //  function : operator =
    /// @brief Asignation operator
    /// @param [in] VT : stack_cnc from where copy the data
    /// @return Reference to the stack_cnc after the copy
    //------------------------------------------------------------------------
    scheduler & operator=(const scheduler &VT) = delete;
    //
    //------------------------------------------------------------------------
    //  function : size
    /// @brief Asignation operator
    /// @param [in] VT : stack_cnc from where copy the data
    /// @return Reference to the stack_cnc after the copy
    //------------------------------------------------------------------------
    size_t size(void) const
    {
        lock_t s(spl);
        return nelem;
    };
    //
    //------------------------------------------------------------------------
    //  function : clear
    /// @brief Delete all the elements of the stack_cnc.
    //------------------------------------------------------------------------
    void clear_all(void)
    {
        lock_t s(spl);
        mp.clear();
        nelem = 0;
    };

    //
    //------------------------------------------------------------------------
    //  function : insert
    /// @brief Insert one element in the back of the container
    /// @param [in] D : value to insert. Can ve a value, a reference or an
    ///                 rvalue
    /// @return iterator to the element inserted
    /// @remarks This operation is O ( const )
    //------------------------------------------------------------------------
    void insert(Func_t & f)
    {
        lock_t s(spl);
        key_t th_id = std::this_thread::get_id();
        it_map itmp = mp.find(th_id);
        if (itmp == mp.end())
        {
            auto aux = mp.emplace(th_id, deque_t());
            if (aux.second == false) throw std::bad_alloc();
            itmp = aux.first;
        };
        itmp->second.emplace_back(std::move(f));
        nelem++;
    };

    //
    //------------------------------------------------------------------------
    //  function :emplace
    /// @brief Insert one element in the back of the container
    /// @param [in] args :group of arguments for to build the object to insert
    /// @return iterator to the element inserted
    /// @remarks This operation is O ( const )
    //------------------------------------------------------------------------
    template<class ... Args>
    void emplace(Args && ... args)
    {
        lock_t s(spl);
        key_t th_id = std::this_thread::get_id();
        it_map itmp = mp.find(th_id);
        if (itmp == mp.end())
        {
            auto aux = mp.emplace(th_id, deque_t());
            if (aux.second == false) throw std::bad_alloc();
            itmp = aux.first;
        };
        itmp->second.emplace_back(std::forward <Args>(args) ...);
        nelem++;
    };
    //
    //------------------------------------------------------------------------
    //  function : insert
    /// @brief Insert one element in the back of the container
    /// @param [in] D : value to insert. Can ve a value, a reference or an rvalue
    /// @return iterator to the element inserted
    /// @remarks This operation is O ( const )
    //------------------------------------------------------------------------
    template<class it_func>
    void insert_range(it_func first, it_func last)
    {
        //--------------------------------------------------------------------
        //                    Metaprogramming
        //--------------------------------------------------------------------
        typedef value_iter<it_func> value2_t;
        static_assert (std::is_same< Func_t, value2_t >::value,
                        "Incompatible iterators\n");

        //--------------------------------------------------------------------
        //                     Code
        //--------------------------------------------------------------------
        assert((last - first) > 0);

        lock_t s(spl);
        key_t th_id = std::this_thread::get_id();
        it_map itmp = mp.find(th_id);
        if (itmp == mp.end())
        {
            auto aux = mp.emplace(th_id, deque_t());
            if (aux.second == true) throw std::bad_alloc();
            itmp = aux.first;
        };
        while (first != last)
        {
            itmp->second.emplace_back(std::move(*(first++)));
            nelem++;
        };
    };
    //
    //------------------------------------------------------------------------
    //  function : extract
    /// @brief erase the last element of the tree and return a copy
    /// @param [out] V : reference to a variable where copy the element
    /// @return code of the operation
    ///         0- Element erased
    ///         1 - Empty tree
    /// @remarks This operation is O(1)
    //------------------------------------------------------------------------
    bool extract(Func_t & f)
    {
        lock_t s(spl);
        if (nelem == 0) return false;
        key_t th_id = std::this_thread::get_id();
        it_map itmp = mp.find(th_id);
        if (itmp != mp.end() and not itmp->second.empty())
        {
            f = std::move(itmp->second.back());
            itmp->second.pop_back();
            --nelem;
            return true;
        };
        for (itmp = mp.begin(); itmp != mp.end(); ++itmp)
        {
            if (itmp->second.empty()) continue;
            f = std::move(itmp->second.back());
            itmp->second.pop_back();
            --nelem;
            return true;
        }
        return false;
    };
};
// end class scheduler
//*************************************************************************
//               P R I N T      F U N C T I O N S
//************************************************************************
template<class ... Args>
std::ostream & operator <<(std::ostream &out, const std::deque<Args ...> & dq)
{
    for (uint32_t i = 0; i < dq.size(); ++i)
        out << dq[i] << " ";
    out << std::endl;
    return out;
}

template<typename Func_t, typename Allocator = std::allocator<Func_t> >
std::ostream & operator <<(std::ostream &out,
                           const scheduler<Func_t, Allocator> &sch)
{
    std::unique_lock < spinlock_t > s(sch.spl);
    out << "Nelem :" << sch.nelem << std::endl;
    for (auto it = sch.mp.begin(); it != sch.mp.end(); ++it)
    {
        out << it->first << "  :" << it->second << std::endl;
    }
    return out;
}

//***************************************************************************
};// end namespace common
};// end namespace sort
};// end namespace boost
//***************************************************************************
#endif
