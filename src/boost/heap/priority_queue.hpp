// boost heap: wrapper for stl heap
//
// Copyright (C) 2010 Tim Blechmann
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HEAP_PRIORITY_QUEUE_HPP
#define BOOST_HEAP_PRIORITY_QUEUE_HPP

#include <algorithm>
#include <queue>
#include <utility>
#include <vector>

#include <boost/assert.hpp>

#include <boost/heap/detail/heap_comparison.hpp>
#include <boost/heap/detail/stable_heap.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif


namespace boost  {
namespace heap   {
namespace detail {

typedef parameter::parameters<boost::parameter::optional<tag::allocator>,
                              boost::parameter::optional<tag::compare>,
                              boost::parameter::optional<tag::stable>,
                              boost::parameter::optional<tag::stability_counter_type>
                             > priority_queue_signature;
}

/**
 * \class priority_queue
 * \brief priority queue, based on stl heap functions
 *
 * The priority_queue class is a wrapper for the stl heap functions.<br>
 * The template parameter T is the type to be managed by the container.
 * The user can specify additional options and if no options are provided default options are used.
 *
 * The container supports the following options:
 * - \c boost::heap::compare<>, defaults to \c compare<std::less<T> >
 * - \c boost::heap::stable<>, defaults to \c stable<false>
 * - \c boost::heap::stability_counter_type<>, defaults to \c stability_counter_type<boost::uintmax_t>
 * - \c boost::heap::allocator<>, defaults to \c allocator<std::allocator<T> >
 *
 */
#ifdef BOOST_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template <typename T,
          class A0 = boost::parameter::void_,
          class A1 = boost::parameter::void_,
          class A2 = boost::parameter::void_,
          class A3 = boost::parameter::void_
         >
#endif
class priority_queue:
    private detail::make_heap_base<T, typename detail::priority_queue_signature::bind<A0, A1, A2, A3>::type, false>::type
{
    typedef detail::make_heap_base<T, typename detail::priority_queue_signature::bind<A0, A1, A2, A3>::type, false> heap_base_maker;

    typedef typename heap_base_maker::type super_t;
    typedef typename super_t::internal_type internal_type;
    typedef typename boost::allocator_rebind<typename heap_base_maker::allocator_argument, internal_type>::type internal_type_allocator;
    typedef std::vector<internal_type, internal_type_allocator> container_type;

    template <typename Heap1, typename Heap2>
    friend struct detail::heap_merge_emulate;

    container_type q_;

#ifndef BOOST_DOXYGEN_INVOKED
    struct implementation_defined:
        detail::extract_allocator_types<typename heap_base_maker::allocator_argument>
    {
        typedef typename heap_base_maker::compare_argument value_compare;
        typedef detail::stable_heap_iterator<T, typename container_type::const_iterator, super_t> iterator;
        typedef iterator const_iterator;
        typedef typename container_type::allocator_type allocator_type;
    };
#endif

public:
    typedef T value_type;
    typedef typename implementation_defined::size_type size_type;
    typedef typename implementation_defined::difference_type difference_type;
    typedef typename implementation_defined::value_compare value_compare;
    typedef typename implementation_defined::allocator_type allocator_type;
    typedef typename implementation_defined::reference reference;
    typedef typename implementation_defined::const_reference const_reference;
    typedef typename implementation_defined::pointer pointer;
    typedef typename implementation_defined::const_pointer const_pointer;
    /**
     * \b Note: The iterator does not traverse the priority queue in order of the priorities.
     * */
    typedef typename implementation_defined::iterator iterator;
    typedef typename implementation_defined::const_iterator const_iterator;

    static const bool constant_time_size = true;
    static const bool has_ordered_iterators = false;
    static const bool is_mergable = false;
    static const bool is_stable = heap_base_maker::is_stable;
    static const bool has_reserve = true;

    /**
     * \b Effects: constructs an empty priority queue.
     *
     * \b Complexity: Constant.
     *
     * */
    explicit priority_queue(value_compare const & cmp = value_compare()):
        super_t(cmp)
    {}

    /**
     * \b Effects: copy-constructs priority queue from rhs.
     *
     * \b Complexity: Linear.
     *
     * */
    priority_queue (priority_queue const & rhs):
        super_t(rhs), q_(rhs.q_)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    /**
     * \b Effects: C++11-style move constructor.
     *
     * \b Complexity: Constant.
     *
     * \b Note: Only available, if BOOST_NO_CXX11_RVALUE_REFERENCES is not defined
     * */
    priority_queue(priority_queue && rhs) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible<super_t>::value):
        super_t(std::move(rhs)), q_(std::move(rhs.q_))
    {}

    /**
     * \b Effects: C++11-style move assignment.
     *
     * \b Complexity: Constant.
     *
     * \b Note: Only available, if BOOST_NO_CXX11_RVALUE_REFERENCES is not defined
     * */
    priority_queue & operator=(priority_queue && rhs) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_assignable<super_t>::value)
    {
        super_t::operator=(std::move(rhs));
        q_ = std::move(rhs.q_);
        return *this;
    }
#endif

    /**
     * \b Effects: Assigns priority queue from rhs.
     *
     * \b Complexity: Linear.
     *
     * */
    priority_queue & operator=(priority_queue const & rhs)
    {
        static_cast<super_t&>(*this) = static_cast<super_t const &>(rhs);
        q_ = rhs.q_;
        return *this;
    }

    /**
     * \b Effects: Returns true, if the priority queue contains no elements.
     *
     * \b Complexity: Constant.
     *
     * */
    bool empty(void) const BOOST_NOEXCEPT
    {
        return q_.empty();
    }

    /**
     * \b Effects: Returns the number of elements contained in the priority queue.
     *
     * \b Complexity: Constant.
     *
     * */
    size_type size(void) const BOOST_NOEXCEPT
    {
        return q_.size();
    }

    /**
     * \b Effects: Returns the maximum number of elements the priority queue can contain.
     *
     * \b Complexity: Constant.
     *
     * */
    size_type max_size(void) const BOOST_NOEXCEPT
    {
        return q_.max_size();
    }

    /**
     * \b Effects: Removes all elements from the priority queue.
     *
     * \b Complexity: Linear.
     *
     * */
    void clear(void) BOOST_NOEXCEPT
    {
        q_.clear();
    }

    /**
     * \b Effects: Returns allocator.
     *
     * \b Complexity: Constant.
     *
     * */
    allocator_type get_allocator(void) const
    {
        return q_.get_allocator();
    }

    /**
     * \b Effects: Returns a const_reference to the maximum element.
     *
     * \b Complexity: Constant.
     *
     * */
    const_reference top(void) const
    {
        BOOST_ASSERT(!empty());
        return super_t::get_value(q_.front());
    }

    /**
     * \b Effects: Adds a new element to the priority queue.
     *
     * \b Complexity: Logarithmic (amortized). Linear (worst case).
     *
     * */
    void push(value_type const & v)
    {
        q_.push_back(super_t::make_node(v));
        std::push_heap(q_.begin(), q_.end(), static_cast<super_t const &>(*this));
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    /**
     * \b Effects: Adds a new element to the priority queue. The element is directly constructed in-place.
     *
     * \b Complexity: Logarithmic (amortized). Linear (worst case).
     *
     * */
    template <class... Args>
    void emplace(Args&&... args)
    {
        q_.emplace_back(super_t::make_node(std::forward<Args>(args)...));
        std::push_heap(q_.begin(), q_.end(), static_cast<super_t const &>(*this));
    }
#endif

    /**
     * \b Effects: Removes the top element from the priority queue.
     *
     * \b Complexity: Logarithmic (amortized). Linear (worst case).
     *
     * */
    void pop(void)
    {
        BOOST_ASSERT(!empty());
        std::pop_heap(q_.begin(), q_.end(), static_cast<super_t const &>(*this));
        q_.pop_back();
    }

    /**
     * \b Effects: Swaps two priority queues.
     *
     * \b Complexity: Constant.
     *
     * */
    void swap(priority_queue & rhs) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible<super_t>::value && boost::is_nothrow_move_assignable<super_t>::value)
    {
        super_t::swap(rhs);
        q_.swap(rhs.q_);
    }

    /**
     * \b Effects: Returns an iterator to the first element contained in the priority queue.
     *
     * \b Complexity: Constant.
     *
     * */
    iterator begin(void) const BOOST_NOEXCEPT
    {
        return iterator(q_.begin());
    }

    /**
     * \b Effects: Returns an iterator to the end of the priority queue.
     *
     * \b Complexity: Constant.
     *
     * */
    iterator end(void) const BOOST_NOEXCEPT
    {
        return iterator(q_.end());
    }

    /**
     * \b Effects: Reserves memory for element_count elements
     *
     * \b Complexity: Linear.
     *
     * \b Node: Invalidates iterators
     *
     * */
    void reserve(size_type element_count)
    {
        q_.reserve(element_count);
    }

    /**
     * \b Effect: Returns the value_compare object used by the priority queue
     *
     * */
    value_compare const & value_comp(void) const
    {
        return super_t::value_comp();
    }

    /**
     * \b Returns: Element-wise comparison of heap data structures
     *
     * \b Requirement: the \c value_compare object of both heaps must match.
     *
     * */
    template <typename HeapType>
    bool operator<(HeapType const & rhs) const
    {
        return detail::heap_compare(*this, rhs);
    }

    /**
     * \b Returns: Element-wise comparison of heap data structures
     *
     * \b Requirement: the \c value_compare object of both heaps must match.
     *
     * */
    template <typename HeapType>
    bool operator>(HeapType const & rhs) const
    {
        return detail::heap_compare(rhs, *this);
    }

    /**
     * \b Returns: Element-wise comparison of heap data structures
     *
     * \b Requirement: the \c value_compare object of both heaps must match.
     *
     * */
    template <typename HeapType>
    bool operator>=(HeapType const & rhs) const
    {
        return !operator<(rhs);
    }

    /**
     * \b Returns: Element-wise comparison of heap data structures
     *
     * \b Requirement: the \c value_compare object of both heaps must match.
     *
     * */
    template <typename HeapType>
    bool operator<=(HeapType const & rhs) const
    {
        return !operator>(rhs);
    }

    /** \brief Equivalent comparison
     * \b Returns: True, if both heap data structures are equivalent.
     *
     * \b Requirement: the \c value_compare object of both heaps must match.
     *
     * */
    template <typename HeapType>
    bool operator==(HeapType const & rhs) const
    {
        return detail::heap_equality(*this, rhs);
    }

    /** \brief Equivalent comparison
     * \b Returns: True, if both heap data structures are not equivalent.
     *
     * \b Requirement: the \c value_compare object of both heaps must match.
     *
     * */
    template <typename HeapType>
    bool operator!=(HeapType const & rhs) const
    {
        return !(*this == rhs);
    }
};

} /* namespace heap */
} /* namespace boost */

#endif /* BOOST_HEAP_PRIORITY_QUEUE_HPP */
