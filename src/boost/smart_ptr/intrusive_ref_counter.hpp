/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   intrusive_ref_counter.hpp
 * \author Andrey Semashev
 * \date   12.03.2009
 *
 * This header contains a reference counter class for \c intrusive_ptr.
 */

#ifndef BOOST_SMART_PTR_INTRUSIVE_REF_COUNTER_HPP_INCLUDED_
#define BOOST_SMART_PTR_INTRUSIVE_REF_COUNTER_HPP_INCLUDED_

#include <boost/config.hpp>
#include <boost/smart_ptr/detail/atomic_count.hpp>
#include <boost/smart_ptr/detail/sp_noexcept.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(_MSC_VER)
#pragma warning(push)
// This is a bogus MSVC warning, which is flagged by friend declarations of intrusive_ptr_add_ref and intrusive_ptr_release in intrusive_ref_counter:
// 'name' : the inline specifier cannot be used when a friend declaration refers to a specialization of a function template
// Note that there is no inline specifier in the declarations.
#pragma warning(disable: 4396)
#endif

namespace boost {

namespace sp_adl_block {

/*!
 * \brief Thread unsafe reference counter policy for \c intrusive_ref_counter
 *
 * The policy instructs the \c intrusive_ref_counter base class to implement
 * a reference counter suitable for single threaded use only. Pointers to the same
 * object with this kind of reference counter must not be used by different threads.
 */
struct thread_unsafe_counter
{
    typedef unsigned int type;

    static unsigned int load(unsigned int const& counter) BOOST_SP_NOEXCEPT
    {
        return counter;
    }

    static void increment(unsigned int& counter) BOOST_SP_NOEXCEPT
    {
        ++counter;
    }

    static unsigned int decrement(unsigned int& counter) BOOST_SP_NOEXCEPT
    {
        return --counter;
    }
};

/*!
 * \brief Thread safe reference counter policy for \c intrusive_ref_counter
 *
 * The policy instructs the \c intrusive_ref_counter base class to implement
 * a thread-safe reference counter, if the target platform supports multithreading.
 */
struct thread_safe_counter
{
    typedef boost::detail::atomic_count type;

    static unsigned int load(boost::detail::atomic_count const& counter) BOOST_SP_NOEXCEPT
    {
        return static_cast< unsigned int >(static_cast< long >(counter));
    }

    static void increment(boost::detail::atomic_count& counter) BOOST_SP_NOEXCEPT
    {
        ++counter;
    }

    static unsigned int decrement(boost::detail::atomic_count& counter) BOOST_SP_NOEXCEPT
    {
        return static_cast< unsigned int >(--counter);
    }
};

template< typename DerivedT, typename CounterPolicyT = thread_safe_counter >
class intrusive_ref_counter;

template< typename DerivedT, typename CounterPolicyT >
void intrusive_ptr_add_ref(const intrusive_ref_counter< DerivedT, CounterPolicyT >* p) BOOST_SP_NOEXCEPT;
template< typename DerivedT, typename CounterPolicyT >
void intrusive_ptr_release(const intrusive_ref_counter< DerivedT, CounterPolicyT >* p) BOOST_SP_NOEXCEPT;

/*!
 * \brief A reference counter base class
 *
 * This base class can be used with user-defined classes to add support
 * for \c intrusive_ptr. The class contains a reference counter defined by the \c CounterPolicyT.
 * Upon releasing the last \c intrusive_ptr referencing the object
 * derived from the \c intrusive_ref_counter class, operator \c delete
 * is automatically called on the pointer to the object.
 *
 * The other template parameter, \c DerivedT, is the user's class that derives from \c intrusive_ref_counter.
 */
template< typename DerivedT, typename CounterPolicyT >
class intrusive_ref_counter
{
private:
    //! Reference counter type
    typedef typename CounterPolicyT::type counter_type;
    //! Reference counter
    mutable counter_type m_ref_counter;

public:
    /*!
     * Default constructor
     *
     * \post <tt>use_count() == 0</tt>
     */
    intrusive_ref_counter() BOOST_SP_NOEXCEPT : m_ref_counter(0)
    {
    }

    /*!
     * Copy constructor
     *
     * \post <tt>use_count() == 0</tt>
     */
    intrusive_ref_counter(intrusive_ref_counter const&) BOOST_SP_NOEXCEPT : m_ref_counter(0)
    {
    }

    /*!
     * Assignment
     *
     * \post The reference counter is not modified after assignment
     */
    intrusive_ref_counter& operator= (intrusive_ref_counter const&) BOOST_SP_NOEXCEPT { return *this; }

    /*!
     * \return The reference counter
     */
    unsigned int use_count() const BOOST_SP_NOEXCEPT
    {
        return CounterPolicyT::load(m_ref_counter);
    }

protected:
    /*!
     * Destructor
     */
    BOOST_DEFAULTED_FUNCTION(~intrusive_ref_counter(), {})

    friend void intrusive_ptr_add_ref< DerivedT, CounterPolicyT >(const intrusive_ref_counter< DerivedT, CounterPolicyT >* p) BOOST_SP_NOEXCEPT;
    friend void intrusive_ptr_release< DerivedT, CounterPolicyT >(const intrusive_ref_counter< DerivedT, CounterPolicyT >* p) BOOST_SP_NOEXCEPT;
};

template< typename DerivedT, typename CounterPolicyT >
inline void intrusive_ptr_add_ref(const intrusive_ref_counter< DerivedT, CounterPolicyT >* p) BOOST_SP_NOEXCEPT
{
    CounterPolicyT::increment(p->m_ref_counter);
}

template< typename DerivedT, typename CounterPolicyT >
inline void intrusive_ptr_release(const intrusive_ref_counter< DerivedT, CounterPolicyT >* p) BOOST_SP_NOEXCEPT
{
    if (CounterPolicyT::decrement(p->m_ref_counter) == 0)
        delete static_cast< const DerivedT* >(p);
}

} // namespace sp_adl_block

using sp_adl_block::intrusive_ref_counter;
using sp_adl_block::thread_unsafe_counter;
using sp_adl_block::thread_safe_counter;

} // namespace boost

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif // BOOST_SMART_PTR_INTRUSIVE_REF_COUNTER_HPP_INCLUDED_
