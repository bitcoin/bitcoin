//----------------------------------------------------------------------------
/// @file spinlock_t.hpp
/// @brief
///
/// @author Copyright (c) 2010 2015 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanyingfile LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_DETAIL_UTIL_SPINLOCK_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_UTIL_SPINLOCK_HPP

#include <atomic>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace boost
{
namespace sort
{
namespace common
{
//
//---------------------------------------------------------------------------
/// @class spinlock_t
/// @brief This class implement, from atomic variables, a spinlock
/// @remarks This class meet the BasicLockable requirements ( lock, unlock )
//---------------------------------------------------------------------------
class spinlock_t
{
  private:
    //------------------------------------------------------------------------
    //             P R I V A T E      V A R I A B L E S
    //------------------------------------------------------------------------
    std::atomic_flag af;

  public:
    //
    //-------------------------------------------------------------------------
    //  function : spinlock_t
    /// @brief  class constructor
    /// @param [in]
    //-------------------------------------------------------------------------
    explicit spinlock_t ( ) noexcept { af.clear ( ); };
    //
    //-------------------------------------------------------------------------
    //  function : lock
    /// @brief  Lock the spinlock_t
    //-------------------------------------------------------------------------
    void lock ( ) noexcept
    {
    	while (af.test_and_set (std::memory_order_acquire))
        {
            std::this_thread::yield ( );
        };
    };
    //
    //-------------------------------------------------------------------------
    //  function : try_lock
    /// @brief Try to lock the spinlock_t, if not, return false
    /// @return true : locked
    ///         false: not previous locked
    //-------------------------------------------------------------------------
    bool try_lock ( ) noexcept
    {
        return not af.test_and_set (std::memory_order_acquire);
    };
    //
    //-------------------------------------------------------------------------
    //  function : unlock
    /// @brief  unlock the spinlock_t
    //-------------------------------------------------------------------------
    void unlock ( ) noexcept { af.clear (std::memory_order_release); };

}; // E N D    C L A S S     S P I N L O C K
//
//***************************************************************************
}; // end namespace common
}; // end namespace sort
}; // end namespace boost
//***************************************************************************
#endif
