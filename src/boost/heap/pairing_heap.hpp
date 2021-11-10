// boost heap: pairing heap
//
// Copyright (C) 2010 Tim Blechmann
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HEAP_PAIRING_HEAP_HPP
#define BOOST_HEAP_PAIRING_HEAP_HPP

#include <algorithm>
#include <utility>
#include <vector>

#include <boost/assert.hpp>

#include <boost/heap/detail/heap_comparison.hpp>
#include <boost/heap/detail/heap_node.hpp>
#include <boost/heap/policies.hpp>
#include <boost/heap/detail/stable_heap.hpp>
#include <boost/heap/detail/tree_iterator.hpp>
#include <boost/type_traits/integral_constant.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif


#ifndef BOOST_DOXYGEN_INVOKED
#ifdef BOOST_HEAP_SANITYCHECKS
#define BOOST_HEAP_ASSERT BOOST_ASSERT
#else
#define BOOST_HEAP_ASSERT(expression)
#endif
#endif

namespace boost  {
namespace heap   {
namespace detail {

typedef parameter::parameters<boost::parameter::optional<tag::allocator>,
                              boost::parameter::optional<tag::compare>,
                              boost::parameter::optional<tag::stable>,
                              boost::parameter::optional<tag::constant_time_size>,
                              boost::parameter::optional<tag::stability_counter_type>
                             > pairing_heap_signature;

template <typename T, typename Parspec>
struct make_pairing_heap_base
{
    static const bool constant_time_size = parameter::binding<Parspec,
                                                              tag::constant_time_size,
                                                              boost::true_type
                                                             >::type::value;
    typedef typename detail::make_heap_base<T, Parspec, constant_time_size>::type base_type;
    typedef typename detail::make_heap_base<T, Parspec, constant_time_size>::allocator_argument allocator_argument;
    typedef typename detail::make_heap_base<T, Parspec, constant_time_size>::compare_argument compare_argument;

    typedef heap_node<typename base_type::internal_type, false> node_type;

    typedef typename boost::allocator_rebind<allocator_argument, node_type>::type allocator_type;

    struct type:
        base_type,
        allocator_type
    {
        type(compare_argument const & arg):
            base_type(arg)
        {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        type(type const & rhs):
            base_type(rhs), allocator_type(rhs)
        {}

        type(type && rhs):
            base_type(std::move(static_cast<base_type&>(rhs))),
            allocator_type(std::move(static_cast<allocator_type&>(rhs)))
        {}

        type & operator=(type && rhs)
        {
            base_type::operator=(std::move(static_cast<base_type&>(rhs)));
            allocator_type::operator=(std::move(static_cast<allocator_type&>(rhs)));
            return *this;
        }

        type & operator=(type const & rhs)
        {
            base_type::operator=(static_cast<base_type const &>(rhs));
            allocator_type::operator=(static_cast<const allocator_type&>(rhs));
            return *this;
        }
#endif
    };
};

}

/**
 * \class pairing_heap
 * \brief pairing heap
 *
 * Pairing heaps are self-adjusting binary heaps. Although design and implementation are rather simple,
 * the complexity analysis is yet unsolved. For details, consult:
 *
 * Pettie, Seth (2005), "Towards a final analysis of pairing heaps",
 * Proc. 46th Annual IEEE Symposium on Foundations of Computer Science, pp. 174-183
 *
 * The template parameter T is the type to be managed by the container.
 * The user can specify additional options and if no options are provided default options are used.
 *
 * The container supports the following options:
 * - \c boost::heap::compare<>, defaults to \c compare<std::less<T> >
 * - \c boost::heap::stable<>, defaults to \c stable<false>
 * - \c boost::heap::stability_counter_type<>, defaults to \c stability_counter_type<boost::uintmax_t>
 * - \c boost::heap::allocator<>, defaults to \c allocator<std::allocator<T> >
 * - \c boost::heap::constant_time_size<>, defaults to \c constant_time_size<true>
 *
 *
 */
#ifdef BOOST_DOXYGEN_INVOKED
template<class T, class ...Options>
#else
template <typename T,
          class A0 = boost::parameter::void_,
          class A1 = boost::parameter::void_,
          class A2 = boost::parameter::void_,
          class A3 = boost::parameter::void_,
          class A4 = boost::parameter::void_
         >
#endif
class pairing_heap:
    private detail::make_pairing_heap_base<T,
                                           typename detail::pairing_heap_signature::bind<A0, A1, A2, A3, A4>::type
                                          >::type
{
    typedef typename detail::pairing_heap_signature::bind<A0, A1, A2, A3, A4>::type bound_args;
    typedef detail::make_pairing_heap_base<T, bound_args> base_maker;
    typedef typename base_maker::type super_t;

    typedef typename super_t::internal_type internal_type;
    typedef typename super_t::size_holder_type size_holder;
    typedef typename base_maker::allocator_argument allocator_argument;

private:
    template <typename Heap1, typename Heap2>
    friend struct heap_merge_emulate;

#ifndef BOOST_DOXYGEN_INVOKED
    struct implementation_defined:
        detail::extract_allocator_types<typename base_maker::allocator_argument>
    {
        typedef T value_type;
        typedef typename detail::extract_allocator_types<typename base_maker::allocator_argument>::size_type size_type;
        typedef typename detail::extract_allocator_types<typename base_maker::allocator_argument>::reference reference;

        typedef typename base_maker::compare_argument value_compare;
        typedef typename base_maker::allocator_type allocator_type;

        typedef typename boost::allocator_pointer<allocator_type>::type node_pointer;
        typedef typename boost::allocator_const_pointer<allocator_type>::type const_node_pointer;

        typedef detail::heap_node_list node_list_type;
        typedef typename node_list_type::iterator node_list_iterator;
        typedef typename node_list_type::const_iterator node_list_const_iterator;

        typedef typename base_maker::node_type node;

        typedef detail::value_extractor<value_type, internal_type, super_t> value_extractor;
        typedef typename super_t::internal_compare internal_compare;
        typedef detail::node_handle<node_pointer, super_t, reference> handle_type;

        typedef detail::tree_iterator<node,
                                      const value_type,
                                      allocator_type,
                                      value_extractor,
                                      detail::pointer_to_reference<node>,
                                      false,
                                      false,
                                      value_compare
                                     > iterator;

        typedef iterator const_iterator;

        typedef detail::tree_iterator<node,
                                      const value_type,
                                      allocator_type,
                                      value_extractor,
                                      detail::pointer_to_reference<node>,
                                      false,
                                      true,
                                      value_compare
                                     > ordered_iterator;
    };

    typedef typename implementation_defined::node node;
    typedef typename implementation_defined::node_pointer node_pointer;
    typedef typename implementation_defined::node_list_type node_list_type;
    typedef typename implementation_defined::node_list_iterator node_list_iterator;
    typedef typename implementation_defined::node_list_const_iterator node_list_const_iterator;
    typedef typename implementation_defined::internal_compare internal_compare;

    typedef boost::intrusive::list<detail::heap_node_base<true>,
                                   boost::intrusive::constant_time_size<false>
                                  > node_child_list;
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
    /// \copydoc boost::heap::priority_queue::iterator
    typedef typename implementation_defined::iterator iterator;
    typedef typename implementation_defined::const_iterator const_iterator;
    typedef typename implementation_defined::ordered_iterator ordered_iterator;

    typedef typename implementation_defined::handle_type handle_type;

    static const bool constant_time_size = super_t::constant_time_size;
    static const bool has_ordered_iterators = true;
    static const bool is_mergable = true;
    static const bool is_stable = detail::extract_stable<bound_args>::value;
    static const bool has_reserve = false;

    /// \copydoc boost::heap::priority_queue::priority_queue(value_compare const &)
    explicit pairing_heap(value_compare const & cmp = value_compare()):
        super_t(cmp), root(NULL)
    {}

    /// \copydoc boost::heap::priority_queue::priority_queue(priority_queue const &)
    pairing_heap(pairing_heap const & rhs):
        super_t(rhs), root(NULL)
    {
        if (rhs.empty())
            return;

        clone_tree(rhs);
        size_holder::set_size(rhs.get_size());
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    /// \copydoc boost::heap::priority_queue::priority_queue(priority_queue &&)
    pairing_heap(pairing_heap && rhs):
        super_t(std::move(rhs)), root(rhs.root)
    {
        rhs.root = NULL;
    }

    /// \copydoc boost::heap::priority_queue::operator=(priority_queue &&)
    pairing_heap & operator=(pairing_heap && rhs)
    {
        super_t::operator=(std::move(rhs));
        root = rhs.root;
        rhs.root = NULL;
        return *this;
    }
#endif

    /// \copydoc boost::heap::priority_queue::operator=(priority_queue const & rhs)
    pairing_heap & operator=(pairing_heap const & rhs)
    {
        clear();
        size_holder::set_size(rhs.get_size());
        static_cast<super_t&>(*this) = rhs;

        clone_tree(rhs);
        return *this;
    }

    ~pairing_heap(void)
    {
        while (!empty())
            pop();
    }

    /// \copydoc boost::heap::priority_queue::empty
    bool empty(void) const
    {
        return root == NULL;
    }

    /// \copydoc boost::heap::binomial_heap::size
    size_type size(void) const
    {
        if (constant_time_size)
            return size_holder::get_size();

        if (root == NULL)
            return 0;
        else
            return detail::count_nodes(root);
    }

    /// \copydoc boost::heap::priority_queue::max_size
    size_type max_size(void) const
    {
        const allocator_type& alloc = *this;
        return boost::allocator_max_size(alloc);
    }

    /// \copydoc boost::heap::priority_queue::clear
    void clear(void)
    {
        if (empty())
            return;

        root->template clear_subtree<allocator_type>(*this);
        root->~node();
        allocator_type& alloc = *this;
        alloc.deallocate(root, 1);
        root = NULL;
        size_holder::set_size(0);
    }

    /// \copydoc boost::heap::priority_queue::get_allocator
    allocator_type get_allocator(void) const
    {
        return *this;
    }

    /// \copydoc boost::heap::priority_queue::swap
    void swap(pairing_heap & rhs)
    {
        super_t::swap(rhs);
        std::swap(root, rhs.root);
    }


    /// \copydoc boost::heap::priority_queue::top
    const_reference top(void) const
    {
        BOOST_ASSERT(!empty());

        return super_t::get_value(root->value);
    }

    /**
     * \b Effects: Adds a new element to the priority queue. Returns handle to element
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     *
     * */
    handle_type push(value_type const & v)
    {
        size_holder::increment();

        allocator_type& alloc = *this;
        node_pointer n = alloc.allocate(1);
        new(n) node(super_t::make_node(v));
        merge_node(n);
        return handle_type(n);
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    /**
     * \b Effects: Adds a new element to the priority queue. The element is directly constructed in-place. Returns handle to element.
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     *
     * */
    template <class... Args>
    handle_type emplace(Args&&... args)
    {
        size_holder::increment();

        allocator_type& alloc = *this;
        node_pointer n = alloc.allocate(1);
        new(n) node(super_t::make_node(std::forward<Args>(args)...));
        merge_node(n);
        return handle_type(n);
    }
#endif

    /**
     * \b Effects: Removes the top element from the priority queue.
     *
     * \b Complexity: Logarithmic (amortized).
     *
     * */
    void pop(void)
    {
        BOOST_ASSERT(!empty());

        erase(handle_type(root));
    }

    /**
     * \b Effects: Assigns \c v to the element handled by \c handle & updates the priority queue.
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     *
     * */
    void update (handle_type handle, const_reference v)
    {
        handle.node_->value = super_t::make_node(v);
        update(handle);
    }

    /**
     * \b Effects: Updates the heap after the element handled by \c handle has been changed.
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     *
     * \b Note: If this is not called, after a handle has been updated, the behavior of the data structure is undefined!
     * */
    void update (handle_type handle)
    {
        node_pointer n = handle.node_;

        n->unlink();
        if (!n->children.empty())
            n = merge_nodes(n, merge_node_list(n->children));

        if (n != root)
            merge_node(n);
    }

     /**
     * \b Effects: Assigns \c v to the element handled by \c handle & updates the priority queue.
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     *
     * \b Note: The new value is expected to be greater than the current one
     * */
    void increase (handle_type handle, const_reference v)
    {
        update(handle, v);
    }

    /**
     * \b Effects: Updates the heap after the element handled by \c handle has been changed.
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     *
     * \b Note: If this is not called, after a handle has been updated, the behavior of the data structure is undefined!
     * */
    void increase (handle_type handle)
    {
        update(handle);
    }

    /**
     * \b Effects: Assigns \c v to the element handled by \c handle & updates the priority queue.
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     *
     * \b Note: The new value is expected to be less than the current one
     * */
    void decrease (handle_type handle, const_reference v)
    {
        update(handle, v);
    }

    /**
     * \b Effects: Updates the heap after the element handled by \c handle has been changed.
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     *
     * \b Note: The new value is expected to be less than the current one. If this is not called, after a handle has been updated, the behavior of the data structure is undefined!
     * */
    void decrease (handle_type handle)
    {
        update(handle);
    }

    /**
     * \b Effects: Removes the element handled by \c handle from the priority_queue.
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     * */
    void erase(handle_type handle)
    {
        node_pointer n = handle.node_;
        if (n != root) {
            n->unlink();
            if (!n->children.empty())
                merge_node(merge_node_list(n->children));
        } else {
            if (!n->children.empty())
                root = merge_node_list(n->children);
            else
                root = NULL;
        }

        size_holder::decrement();
        n->~node();
        allocator_type& alloc = *this;
        alloc.deallocate(n, 1);
    }

    /// \copydoc boost::heap::priority_queue::begin
    iterator begin(void) const
    {
        return iterator(root, super_t::value_comp());
    }

    /// \copydoc boost::heap::priority_queue::end
    iterator end(void) const
    {
        return iterator(super_t::value_comp());
    }

    /// \copydoc boost::heap::fibonacci_heap::ordered_begin
    ordered_iterator ordered_begin(void) const
    {
        return ordered_iterator(root, super_t::value_comp());
    }

    /// \copydoc boost::heap::fibonacci_heap::ordered_begin
    ordered_iterator ordered_end(void) const
    {
        return ordered_iterator(NULL, super_t::value_comp());
    }


    /// \copydoc boost::heap::d_ary_heap_mutable::s_handle_from_iterator
    static handle_type s_handle_from_iterator(iterator const & it)
    {
        node * ptr = const_cast<node *>(it.get_node());
        return handle_type(ptr);
    }

    /**
     * \b Effects: Merge all elements from rhs into this
     *
     * \cond
     * \b Complexity: \f$2^2log(log(N))\f$ (amortized).
     * \endcond
     *
     * \b Complexity: 2**2*log(log(N)) (amortized).
     *
     * */
    void merge(pairing_heap & rhs)
    {
        if (rhs.empty())
            return;

        merge_node(rhs.root);

        size_holder::add(rhs.get_size());
        rhs.set_size(0);
        rhs.root = NULL;

        super_t::set_stability_count((std::max)(super_t::get_stability_count(),
                                     rhs.get_stability_count()));
        rhs.set_stability_count(0);
    }

    /// \copydoc boost::heap::priority_queue::value_comp
    value_compare const & value_comp(void) const
    {
        return super_t::value_comp();
    }

    /// \copydoc boost::heap::priority_queue::operator<(HeapType const & rhs) const
    template <typename HeapType>
    bool operator<(HeapType const & rhs) const
    {
        return detail::heap_compare(*this, rhs);
    }

    /// \copydoc boost::heap::priority_queue::operator>(HeapType const & rhs) const
    template <typename HeapType>
    bool operator>(HeapType const & rhs) const
    {
        return detail::heap_compare(rhs, *this);
    }

    /// \copydoc boost::heap::priority_queue::operator>=(HeapType const & rhs) const
    template <typename HeapType>
    bool operator>=(HeapType const & rhs) const
    {
        return !operator<(rhs);
    }

    /// \copydoc boost::heap::priority_queue::operator<=(HeapType const & rhs) const
    template <typename HeapType>
    bool operator<=(HeapType const & rhs) const
    {
        return !operator>(rhs);
    }

    /// \copydoc boost::heap::priority_queue::operator==(HeapType const & rhs) const
    template <typename HeapType>
    bool operator==(HeapType const & rhs) const
    {
        return detail::heap_equality(*this, rhs);
    }

    /// \copydoc boost::heap::priority_queue::operator!=(HeapType const & rhs) const
    template <typename HeapType>
    bool operator!=(HeapType const & rhs) const
    {
        return !(*this == rhs);
    }

private:
#if !defined(BOOST_DOXYGEN_INVOKED)
    void clone_tree(pairing_heap const & rhs)
    {
        BOOST_HEAP_ASSERT(root == NULL);
        if (rhs.empty())
            return;

        root = allocator_type::allocate(1);

        new(root) node(static_cast<node const &>(*rhs.root), static_cast<allocator_type&>(*this));
    }

    void merge_node(node_pointer other)
    {
        BOOST_HEAP_ASSERT(other);
        if (root != NULL)
            root = merge_nodes(root, other);
        else
            root = other;
    }

    node_pointer merge_node_list(node_child_list & children)
    {
        BOOST_HEAP_ASSERT(!children.empty());
        node_pointer merged = merge_first_pair(children);
        if (children.empty())
            return merged;

        node_child_list node_list;
        node_list.push_back(*merged);

        do {
            node_pointer next_merged = merge_first_pair(children);
            node_list.push_back(*next_merged);
        } while (!children.empty());

        return merge_node_list(node_list);
    }

    node_pointer merge_first_pair(node_child_list & children)
    {
        BOOST_HEAP_ASSERT(!children.empty());
        node_pointer first_child = static_cast<node_pointer>(&children.front());
        children.pop_front();
        if (children.empty())
            return first_child;

        node_pointer second_child = static_cast<node_pointer>(&children.front());
        children.pop_front();

        return merge_nodes(first_child, second_child);
    }

    node_pointer merge_nodes(node_pointer node1, node_pointer node2)
    {
        if (super_t::operator()(node1->value, node2->value))
            std::swap(node1, node2);

        node2->unlink();
        node1->children.push_front(*node2);
        return node1;
    }

    node_pointer root;
#endif
};


} /* namespace heap */
} /* namespace boost */

#undef BOOST_HEAP_ASSERT
#endif /* BOOST_HEAP_PAIRING_HEAP_HPP */
