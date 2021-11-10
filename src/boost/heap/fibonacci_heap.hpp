// boost heap: fibonacci heap
//
// Copyright (C) 2010 Tim Blechmann
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HEAP_FIBONACCI_HEAP_HPP
#define BOOST_HEAP_FIBONACCI_HEAP_HPP

#include <algorithm>
#include <utility>
#include <vector>

#include <boost/array.hpp>
#include <boost/assert.hpp>

#include <boost/heap/detail/heap_comparison.hpp>
#include <boost/heap/detail/heap_node.hpp>
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
                             > fibonacci_heap_signature;

template <typename T, typename Parspec>
struct make_fibonacci_heap_base
{
    static const bool constant_time_size = parameter::binding<Parspec,
                                                              tag::constant_time_size,
                                                              boost::true_type
                                                             >::type::value;

    typedef typename detail::make_heap_base<T, Parspec, constant_time_size>::type base_type;
    typedef typename detail::make_heap_base<T, Parspec, constant_time_size>::allocator_argument allocator_argument;
    typedef typename detail::make_heap_base<T, Parspec, constant_time_size>::compare_argument compare_argument;
    typedef marked_heap_node<typename base_type::internal_type> node_type;

    typedef typename boost::allocator_rebind<allocator_argument, node_type>::type allocator_type;

    struct type:
        base_type,
        allocator_type
    {
        type(compare_argument const & arg):
            base_type(arg)
        {}

        type(type const & rhs):
            base_type(static_cast<base_type const &>(rhs)),
            allocator_type(static_cast<allocator_type const &>(rhs))
        {}

        type & operator=(type const & rhs)
        {
            base_type::operator=(static_cast<base_type const &>(rhs));
            allocator_type::operator=(static_cast<allocator_type const &>(rhs));
            return *this;
        }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
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
#endif
    };
};

}



/**
 * \class fibonacci_heap
 * \brief fibonacci heap
 *
 * The template parameter T is the type to be managed by the container.
 * The user can specify additional options and if no options are provided default options are used.
 *
 * The container supports the following options:
 * - \c boost::heap::stable<>, defaults to \c stable<false>
 * - \c boost::heap::compare<>, defaults to \c compare<std::less<T> >
 * - \c boost::heap::allocator<>, defaults to \c allocator<std::allocator<T> >
 * - \c boost::heap::constant_time_size<>, defaults to \c constant_time_size<true>
 * - \c boost::heap::stability_counter_type<>, defaults to \c stability_counter_type<boost::uintmax_t>
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
class fibonacci_heap:
    private detail::make_fibonacci_heap_base<T,
                                             typename detail::fibonacci_heap_signature::bind<A0, A1, A2, A3, A4>::type
                                            >::type
{
    typedef typename detail::fibonacci_heap_signature::bind<A0, A1, A2, A3, A4>::type bound_args;
    typedef detail::make_fibonacci_heap_base<T, bound_args> base_maker;
    typedef typename base_maker::type super_t;

    typedef typename super_t::size_holder_type size_holder;
    typedef typename super_t::internal_type internal_type;
    typedef typename base_maker::allocator_argument allocator_argument;

    template <typename Heap1, typename Heap2>
    friend struct heap_merge_emulate;

private:
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

        typedef detail::recursive_tree_iterator<node,
                                                node_list_const_iterator,
                                                const value_type,
                                                value_extractor,
                                                detail::list_iterator_converter<node, node_list_type>
                                               > iterator;
        typedef iterator const_iterator;

        typedef detail::tree_iterator<node,
                                      const value_type,
                                      allocator_type,
                                      value_extractor,
                                      detail::list_iterator_converter<node, node_list_type>,
                                      true,
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

    static const bool constant_time_size = base_maker::constant_time_size;
    static const bool has_ordered_iterators = true;
    static const bool is_mergable = true;
    static const bool is_stable = detail::extract_stable<bound_args>::value;
    static const bool has_reserve = false;

    /// \copydoc boost::heap::priority_queue::priority_queue(value_compare const &)
    explicit fibonacci_heap(value_compare const & cmp = value_compare()):
        super_t(cmp), top_element(0)
    {}

    /// \copydoc boost::heap::priority_queue::priority_queue(priority_queue const &)
    fibonacci_heap(fibonacci_heap const & rhs):
        super_t(rhs), top_element(0)
    {
        if (rhs.empty())
            return;

        clone_forest(rhs);
        size_holder::set_size(rhs.size());
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    /// \copydoc boost::heap::priority_queue::priority_queue(priority_queue &&)
    fibonacci_heap(fibonacci_heap && rhs):
        super_t(std::move(rhs)), top_element(rhs.top_element)
    {
        roots.splice(roots.begin(), rhs.roots);
        rhs.top_element = NULL;
    }

    /// \copydoc boost::heap::priority_queue::operator=(priority_queue &&)
    fibonacci_heap & operator=(fibonacci_heap && rhs)
    {
        clear();

        super_t::operator=(std::move(rhs));
        roots.splice(roots.begin(), rhs.roots);
        top_element = rhs.top_element;
        rhs.top_element = NULL;
        return *this;
    }
#endif

    /// \copydoc boost::heap::priority_queue::operator=(priority_queue const &)
    fibonacci_heap & operator=(fibonacci_heap const & rhs)
    {
        clear();
        size_holder::set_size(rhs.size());
        static_cast<super_t&>(*this) = rhs;

        if (rhs.empty())
            top_element = NULL;
        else
            clone_forest(rhs);
        return *this;
    }

    ~fibonacci_heap(void)
    {
        clear();
    }

    /// \copydoc boost::heap::priority_queue::empty
    bool empty(void) const
    {
        if (constant_time_size)
            return size() == 0;
        else
            return roots.empty();
    }

    /// \copydoc boost::heap::priority_queue::size
    size_type size(void) const
    {
        if (constant_time_size)
            return size_holder::get_size();

        if (empty())
            return 0;
        else
            return detail::count_list_nodes<node, node_list_type>(roots);
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
        typedef detail::node_disposer<node, typename node_list_type::value_type, allocator_type> disposer;
        roots.clear_and_dispose(disposer(*this));

        size_holder::set_size(0);
        top_element = NULL;
    }

    /// \copydoc boost::heap::priority_queue::get_allocator
    allocator_type get_allocator(void) const
    {
        return *this;
    }

    /// \copydoc boost::heap::priority_queue::swap
    void swap(fibonacci_heap & rhs)
    {
        super_t::swap(rhs);
        std::swap(top_element, rhs.top_element);
        roots.swap(rhs.roots);
    }


    /// \copydoc boost::heap::priority_queue::top
    value_type const & top(void) const
    {
        BOOST_ASSERT(!empty());

        return super_t::get_value(top_element->value);
    }

    /**
     * \b Effects: Adds a new element to the priority queue. Returns handle to element
     *
     * \b Complexity: Constant.
     *
     * \b Note: Does not invalidate iterators.
     *
     * */
    handle_type push(value_type const & v)
    {
        size_holder::increment();

        allocator_type& alloc = *this;
        node_pointer n = alloc.allocate(1);
        new(n) node(super_t::make_node(v));
        roots.push_front(*n);

        if (!top_element || super_t::operator()(top_element->value, n->value))
            top_element = n;
        return handle_type(n);
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    /**
     * \b Effects: Adds a new element to the priority queue. The element is directly constructed in-place. Returns handle to element.
     *
     * \b Complexity: Constant.
     *
     * \b Note: Does not invalidate iterators.
     *
     * */
    template <class... Args>
    handle_type emplace(Args&&... args)
    {
        size_holder::increment();

        allocator_type& alloc = *this;
        node_pointer n = alloc.allocate(1);
        new(n) node(super_t::make_node(std::forward<Args>(args)...));
        roots.push_front(*n);

        if (!top_element || super_t::operator()(top_element->value, n->value))
            top_element = n;
        return handle_type(n);
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

        node_pointer element = top_element;
        roots.erase(node_list_type::s_iterator_to(*element));

        finish_erase_or_pop(element);
    }

    /**
     * \b Effects: Assigns \c v to the element handled by \c handle & updates the priority queue.
     *
     * \b Complexity: Logarithmic if current value < v, Constant otherwise.
     *
     * */
    void update (handle_type handle, const_reference v)
    {
        if (super_t::operator()(super_t::get_value(handle.node_->value), v))
            increase(handle, v);
        else
            decrease(handle, v);
    }

    /** \copydoc boost::heap::fibonacci_heap::update(handle_type, const_reference)
     *
     * \b Rationale: The lazy update function is a modification of the traditional update, that just invalidates
     *               the iterator to the object referred to by the handle.
     * */
    void update_lazy(handle_type handle, const_reference v)
    {
        handle.node_->value = super_t::make_node(v);
        update_lazy(handle);
    }

    /**
     * \b Effects: Updates the heap after the element handled by \c handle has been changed.
     *
     * \b Complexity: Logarithmic.
     *
     * \b Note: If this is not called, after a handle has been updated, the behavior of the data structure is undefined!
     * */
    void update (handle_type handle)
    {
        update_lazy(handle);
        consolidate();
    }

    /** \copydoc boost::heap::fibonacci_heap::update (handle_type handle)
     *
     * \b Rationale: The lazy update function is a modification of the traditional update, that just invalidates
     *               the iterator to the object referred to by the handle.
     * */
    void update_lazy (handle_type handle)
    {
        node_pointer n = handle.node_;
        node_pointer parent = n->get_parent();

        if (parent) {
            n->parent = NULL;
            roots.splice(roots.begin(), parent->children, node_list_type::s_iterator_to(*n));
        }
        add_children_to_root(n);

        if (super_t::operator()(top_element->value, n->value))
            top_element = n;
    }


     /**
     * \b Effects: Assigns \c v to the element handled by \c handle & updates the priority queue.
     *
     * \b Complexity: Constant.
     *
     * \b Note: The new value is expected to be greater than the current one
     * */
    void increase (handle_type handle, const_reference v)
    {
        handle.node_->value = super_t::make_node(v);
        increase(handle);
    }

    /**
     * \b Effects: Updates the heap after the element handled by \c handle has been changed.
     *
     * \b Complexity: Constant.
     *
     * \b Note: If this is not called, after a handle has been updated, the behavior of the data structure is undefined!
     * */
    void increase (handle_type handle)
    {
        node_pointer n = handle.node_;

        if (n->parent) {
            if (super_t::operator()(n->get_parent()->value, n->value)) {
                node_pointer parent = n->get_parent();
                cut(n);
                cascading_cut(parent);
            }
        }

        if (super_t::operator()(top_element->value, n->value)) {
            top_element = n;
            return;
        }
    }

    /**
     * \b Effects: Assigns \c v to the element handled by \c handle & updates the priority queue.
     *
     * \b Complexity: Logarithmic.
     *
     * \b Note: The new value is expected to be less than the current one
     * */
    void decrease (handle_type handle, const_reference v)
    {
        handle.node_->value = super_t::make_node(v);
        decrease(handle);
    }

    /**
     * \b Effects: Updates the heap after the element handled by \c handle has been changed.
     *
     * \b Complexity: Logarithmic.
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
     * \b Complexity: Logarithmic.
     * */
    void erase(handle_type const & handle)
    {
        node_pointer element = handle.node_;
        node_pointer parent = element->get_parent();

        if (parent)
            parent->children.erase(node_list_type::s_iterator_to(*element));
        else
            roots.erase(node_list_type::s_iterator_to(*element));

        finish_erase_or_pop(element);
    }

    /// \copydoc boost::heap::priority_queue::begin
    iterator begin(void) const
    {
        return iterator(roots.begin());
    }

    /// \copydoc boost::heap::priority_queue::end
    iterator end(void) const
    {
        return iterator(roots.end());
    }


    /**
     * \b Effects: Returns an ordered iterator to the first element contained in the priority queue.
     *
     * \b Note: Ordered iterators traverse the priority queue in heap order.
     * */
    ordered_iterator ordered_begin(void) const
    {
        return ordered_iterator(roots.begin(), roots.end(), top_element, super_t::value_comp());
    }

    /**
     * \b Effects: Returns an ordered iterator to the end of the priority queue.
     *
     * \b Note: Ordered iterators traverse the priority queue in heap order.
     * */
    ordered_iterator ordered_end(void) const
    {
        return ordered_iterator(NULL, super_t::value_comp());
    }

    /**
     * \b Effects: Merge with priority queue rhs.
     *
     * \b Complexity: Constant.
     *
     * */
    void merge(fibonacci_heap & rhs)
    {
        size_holder::add(rhs.get_size());

        if (!top_element ||
            (rhs.top_element && super_t::operator()(top_element->value, rhs.top_element->value)))
            top_element = rhs.top_element;

        roots.splice(roots.end(), rhs.roots);

        rhs.top_element = NULL;
        rhs.set_size(0);

        super_t::set_stability_count((std::max)(super_t::get_stability_count(),
                                     rhs.get_stability_count()));
        rhs.set_stability_count(0);
    }

    /// \copydoc boost::heap::d_ary_heap_mutable::s_handle_from_iterator
    static handle_type s_handle_from_iterator(iterator const & it)
    {
        node * ptr = const_cast<node *>(it.get_node());
        return handle_type(ptr);
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
    void clone_forest(fibonacci_heap const & rhs)
    {
        BOOST_HEAP_ASSERT(roots.empty());
        typedef typename node::template node_cloner<allocator_type> node_cloner;
        roots.clone_from(rhs.roots, node_cloner(*this, NULL), detail::nop_disposer());

        top_element = detail::find_max_child<node_list_type, node, internal_compare>(roots, super_t::get_internal_cmp());
    }

    void cut(node_pointer n)
    {
        node_pointer parent = n->get_parent();
        roots.splice(roots.begin(), parent->children, node_list_type::s_iterator_to(*n));
        n->parent = 0;
        n->mark = false;
    }

    void cascading_cut(node_pointer n)
    {
        node_pointer parent = n->get_parent();

        if (parent) {
            if (!parent->mark)
                parent->mark = true;
            else {
                cut(n);
                cascading_cut(parent);
            }
        }
    }

    void add_children_to_root(node_pointer n)
    {
        for (node_list_iterator it = n->children.begin(); it != n->children.end(); ++it) {
            node_pointer child = static_cast<node_pointer>(&*it);
            child->parent = 0;
        }

        roots.splice(roots.end(), n->children);
    }

    void consolidate(void)
    {
        if (roots.empty())
            return;

        static const size_type max_log2 = sizeof(size_type) * 8;
        boost::array<node_pointer, max_log2> aux;
        aux.assign(NULL);

        node_list_iterator it = roots.begin();
        top_element = static_cast<node_pointer>(&*it);

        do {
            node_pointer n = static_cast<node_pointer>(&*it);
            ++it;
            size_type node_rank = n->child_count();

            if (aux[node_rank] == NULL)
                aux[node_rank] = n;
            else {
                do {
                    node_pointer other = aux[node_rank];
                    if (super_t::operator()(n->value, other->value))
                        std::swap(n, other);

                    if (other->parent)
                        n->children.splice(n->children.end(), other->parent->children, node_list_type::s_iterator_to(*other));
                    else
                        n->children.splice(n->children.end(), roots, node_list_type::s_iterator_to(*other));

                    other->parent = n;

                    aux[node_rank] = NULL;
                    node_rank = n->child_count();
                } while (aux[node_rank] != NULL);
                aux[node_rank] = n;
            }

            if (!super_t::operator()(n->value, top_element->value))
                top_element = n;
        }
        while (it != roots.end());
    }

    void finish_erase_or_pop(node_pointer erased_node)
    {
        add_children_to_root(erased_node);

        erased_node->~node();
        allocator_type& alloc = *this;
        alloc.deallocate(erased_node, 1);

        size_holder::decrement();
        if (!empty())
            consolidate();
        else
            top_element = NULL;
    }

    mutable node_pointer top_element;
    node_list_type roots;
#endif
};

} /* namespace heap */
} /* namespace boost */

#undef BOOST_HEAP_ASSERT

#endif /* BOOST_HEAP_FIBONACCI_HEAP_HPP */
