/*
 *          Copyright Andrey Semashev 2007 - 2021.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   threadsafe_queue.hpp
 * \author Andrey Semashev
 * \date   05.11.2010
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_THREADSAFE_QUEUE_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_THREADSAFE_QUEUE_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifndef BOOST_LOG_NO_THREADS

#include <new>
#include <memory>
#include <cstddef>
#include <boost/atomic/atomic.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/log/utility/use_std_allocator.hpp>
#include <boost/log/detail/allocator_traits.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! Base class for the thread-safe queue implementation
class threadsafe_queue_impl
{
public:
    struct node_base
    {
        boost::atomic< node_base* > next;
    };

protected:
    threadsafe_queue_impl();
    ~threadsafe_queue_impl();

public:
    static BOOST_LOG_API threadsafe_queue_impl* create(node_base* first_node);
    static BOOST_LOG_API void destroy(threadsafe_queue_impl* impl) BOOST_NOEXCEPT;

    static BOOST_LOG_API node_base* reset_last_node(threadsafe_queue_impl* impl) BOOST_NOEXCEPT;
    static BOOST_LOG_API bool unsafe_empty(const threadsafe_queue_impl* impl) BOOST_NOEXCEPT;
    static BOOST_LOG_API void push(threadsafe_queue_impl* impl, node_base* p);
    static BOOST_LOG_API bool try_pop(threadsafe_queue_impl* impl, node_base*& node_to_free, node_base*& node_with_value);

    // Copying and assignment is prohibited
    BOOST_DELETED_FUNCTION(threadsafe_queue_impl(threadsafe_queue_impl const&))
    BOOST_DELETED_FUNCTION(threadsafe_queue_impl& operator= (threadsafe_queue_impl const&))
};

//! Thread-safe queue node type
template< typename T >
struct threadsafe_queue_node :
    public threadsafe_queue_impl::node_base
{
    typedef typename aligned_storage< sizeof(T), alignment_of< T >::value >::type storage_type;
    storage_type storage;

    BOOST_DEFAULTED_FUNCTION(threadsafe_queue_node(), {})
    explicit threadsafe_queue_node(T const& val) { new (storage.address()) T(val); }
    T& value() BOOST_NOEXCEPT { return *static_cast< T* >(storage.address()); }
    void destroy() BOOST_NOEXCEPT { static_cast< T* >(storage.address())->~T(); }

    // Copying and assignment is prohibited
    BOOST_DELETED_FUNCTION(threadsafe_queue_node(threadsafe_queue_node const&))
    BOOST_DELETED_FUNCTION(threadsafe_queue_node& operator= (threadsafe_queue_node const&))
};

/*!
 * \brief An unbounded thread-safe queue
 *
 * The implementation is based on algorithms published in the "Simple, Fast,
 * and Practical Non-Blocking and Blocking Concurrent Queue Algorithms" article
 * in PODC96 by Maged M. Michael and Michael L. Scott. Pseudocode is available here:
 * http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html
 *
 * The implementation provides thread-safe \c push and \c try_pop operations, as well as
 * a thread-unsafe \c empty operation. The queue imposes the following requirements
 * on the element type:
 *
 * \li Default constructible, the default constructor must not throw.
 * \li Copy constructible.
 * \li Movable (i.e. there should be an efficient move assignment for this type).
 *
 * The last requirement is not mandatory but is crucial for decent performance.
 */
template< typename T, typename AllocatorT = use_std_allocator >
class threadsafe_queue :
    private boost::log::aux::rebind_alloc< AllocatorT, threadsafe_queue_node< T > >::type
{
private:
    typedef threadsafe_queue_node< T > node;

public:
    typedef typename boost::log::aux::rebind_alloc< AllocatorT, node >::type allocator_type;
    typedef T value_type;
    typedef T& reference;
    typedef T const& const_reference;
    typedef T* pointer;
    typedef T const* const_pointer;
    typedef std::ptrdiff_t difference_type;
    typedef std::size_t size_type;

private:
    typedef boost::log::aux::allocator_traits< allocator_type > alloc_traits;

    //! A simple scope guard to automate memory reclaiming
    struct auto_deallocate;
    friend struct auto_deallocate;
    struct auto_deallocate
    {
        auto_deallocate(allocator_type* alloc, node* dealloc, node* destr) BOOST_NOEXCEPT :
            m_pAllocator(alloc),
            m_pDeallocate(dealloc),
            m_pDestroy(destr)
        {
        }
        ~auto_deallocate() BOOST_NOEXCEPT
        {
            alloc_traits::destroy(*m_pAllocator, m_pDeallocate);
            alloc_traits::deallocate(*m_pAllocator, m_pDeallocate, 1);
            m_pDestroy->destroy();
        }

    private:
        allocator_type* m_pAllocator;
        node* m_pDeallocate;
        node* m_pDestroy;
    };

public:
    /*!
     * Default constructor, creates an empty queue. Unlike most containers,
     * the constructor requires memory allocation.
     *
     * \throw std::bad_alloc if there is not sufficient memory
     */
    threadsafe_queue(allocator_type const& alloc = allocator_type()) :
        allocator_type(alloc)
    {
        node* p = alloc_traits::allocate(get_allocator(), 1);
        if (BOOST_LIKELY(!!p))
        {
            try
            {
                alloc_traits::construct(get_allocator(), p);
                try
                {
                    m_pImpl = threadsafe_queue_impl::create(p);
                }
                catch (...)
                {
                    alloc_traits::destroy(get_allocator(), p);
                    throw;
                }
            }
            catch (...)
            {
                alloc_traits::deallocate(get_allocator(), p, 1);
                throw;
            }
        }
        else
            throw std::bad_alloc();
    }
    /*!
     * Destructor
     */
    ~threadsafe_queue() BOOST_NOEXCEPT
    {
        // Clear the queue
        if (!unsafe_empty())
        {
            value_type value;
            while (try_pop(value)) {}
        }

        // Remove the last dummy node
        node* p = static_cast< node* >(threadsafe_queue_impl::reset_last_node(m_pImpl));
        alloc_traits::destroy(get_allocator(), p);
        alloc_traits::deallocate(get_allocator(), p, 1);

        threadsafe_queue_impl::destroy(m_pImpl);
    }

    /*!
     * Checks if the queue is empty. Not thread-safe, the returned result may not be actual.
     */
    bool unsafe_empty() const BOOST_NOEXCEPT { return threadsafe_queue_impl::unsafe_empty(m_pImpl); }

    /*!
     * Puts a new element to the end of the queue. Thread-safe, can be called
     * concurrently by several threads, and concurrently with the \c pop operation.
     */
    void push(const_reference value)
    {
        node* p = alloc_traits::allocate(get_allocator(), 1);
        if (BOOST_LIKELY(!!p))
        {
            try
            {
                alloc_traits::construct(get_allocator(), p, value);
            }
            catch (...)
            {
                alloc_traits::deallocate(get_allocator(), p, 1);
                throw;
            }
            threadsafe_queue_impl::push(m_pImpl, p);
        }
        else
            throw std::bad_alloc();
    }

    /*!
     * Attempts to pop an element from the beginning of the queue. Thread-safe, can
     * be called concurrently with the \c push operation. Should not be called by
     * several threads concurrently.
     */
    bool try_pop(reference value)
    {
        threadsafe_queue_impl::node_base *dealloc, *destr;
        if (threadsafe_queue_impl::try_pop(m_pImpl, dealloc, destr))
        {
            node* p = static_cast< node* >(destr);
            auto_deallocate guard(static_cast< allocator_type* >(this), static_cast< node* >(dealloc), p);
            value = boost::move(p->value());
            return true;
        }
        else
            return false;
    }

    // Copying and assignment is prohibited
    BOOST_DELETED_FUNCTION(threadsafe_queue(threadsafe_queue const&))
    BOOST_DELETED_FUNCTION(threadsafe_queue& operator= (threadsafe_queue const&))

private:
    //! Returns the allocator instance
    allocator_type& get_allocator() BOOST_NOEXCEPT { return *static_cast< allocator_type* >(this); }

private:
    //! Pointer to the implementation
    threadsafe_queue_impl* m_pImpl;
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_NO_THREADS

#endif // BOOST_LOG_DETAIL_THREADSAFE_QUEUE_HPP_INCLUDED_
