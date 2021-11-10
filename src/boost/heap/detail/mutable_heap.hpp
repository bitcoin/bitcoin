// boost heap
//
// Copyright (C) 2010 Tim Blechmann
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HEAP_DETAIL_MUTABLE_HEAP_HPP
#define BOOST_HEAP_DETAIL_MUTABLE_HEAP_HPP

/*! \file
 * INTERNAL ONLY
 */

#include <list>
#include <utility>

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/heap/detail/ordered_adaptor_iterator.hpp>

namespace boost  {
namespace heap   {
namespace detail {

/* wrapper for a mutable heap container adaptors
 *
 * this wrapper introduces an additional indirection. the heap is not constructed from objects,
 * but instead from std::list iterators. this way, the mutability is achieved
 *
 */
template <typename PriorityQueueType>
class priority_queue_mutable_wrapper
{
public:
    typedef typename PriorityQueueType::value_type value_type;
    typedef typename PriorityQueueType::size_type size_type;
    typedef typename PriorityQueueType::value_compare value_compare;
    typedef typename PriorityQueueType::allocator_type allocator_type;

    typedef typename PriorityQueueType::reference reference;
    typedef typename PriorityQueueType::const_reference const_reference;
    typedef typename PriorityQueueType::pointer pointer;
    typedef typename PriorityQueueType::const_pointer const_pointer;
    static const bool is_stable = PriorityQueueType::is_stable;

private:
    typedef std::pair<value_type, size_type> node_type;

    typedef std::list<node_type, typename boost::allocator_rebind<allocator_type, node_type>::type> object_list;

    typedef typename object_list::iterator list_iterator;
    typedef typename object_list::const_iterator const_list_iterator;

    template <typename Heap1, typename Heap2>
    friend struct heap_merge_emulate;

    typedef typename PriorityQueueType::super_t::stability_counter_type stability_counter_type;

    stability_counter_type get_stability_count(void) const
    {
        return q_.get_stability_count();
    }

    void set_stability_count(stability_counter_type new_count)
    {
        q_.set_stability_count(new_count);
    }

    struct index_updater
    {
        template <typename It>
        static void run(It & it, size_type new_index)
        {
            q_type::get_value(it)->second = new_index;
        }
    };

public:
    struct handle_type
    {
        value_type & operator*() const
        {
            return iterator->first;
        }

        handle_type (void)
        {}

        handle_type(handle_type const & rhs):
            iterator(rhs.iterator)
        {}

        bool operator==(handle_type const & rhs) const
        {
            return iterator == rhs.iterator;
        }

        bool operator!=(handle_type const & rhs) const
        {
            return iterator != rhs.iterator;
        }

    private:
        explicit handle_type(list_iterator const & it):
            iterator(it)
        {}

        list_iterator iterator;

        friend class priority_queue_mutable_wrapper;
    };

private:
    struct indirect_cmp:
        public value_compare
    {
        indirect_cmp(value_compare const & cmp = value_compare()):
            value_compare(cmp)
        {}

        bool operator()(const_list_iterator const & lhs, const_list_iterator const & rhs) const
        {
            return value_compare::operator()(lhs->first, rhs->first);
        }
    };

    typedef typename PriorityQueueType::template rebind<list_iterator,
                                                        indirect_cmp,
                                                        allocator_type, index_updater >::other q_type;

protected:
    q_type q_;
    object_list objects;

protected:
    priority_queue_mutable_wrapper(value_compare const & cmp = value_compare()):
        q_(cmp)
    {}

    priority_queue_mutable_wrapper(priority_queue_mutable_wrapper const & rhs):
        q_(rhs.q_), objects(rhs.objects)
    {
        for (typename object_list::iterator it = objects.begin(); it != objects.end(); ++it)
            q_.push(it);
    }

    priority_queue_mutable_wrapper & operator=(priority_queue_mutable_wrapper const & rhs)
    {
        q_ = rhs.q_;
        objects = rhs.objects;
        q_.clear();
        for (typename object_list::iterator it = objects.begin(); it != objects.end(); ++it)
            q_.push(it);
        return *this;
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    priority_queue_mutable_wrapper (priority_queue_mutable_wrapper && rhs):
        q_(std::move(rhs.q_))
    {
        /// FIXME: msvc seems to invalidate iterators when moving std::list
        std::swap(objects, rhs.objects);
    }

    priority_queue_mutable_wrapper & operator=(priority_queue_mutable_wrapper && rhs)
    {
        q_ = std::move(rhs.q_);
        objects.clear();
        std::swap(objects, rhs.objects);
        return *this;
    }
#endif


public:
    template <typename iterator_type>
    class iterator_base:
        public boost::iterator_adaptor<iterator_base<iterator_type>,
                                       iterator_type,
                                       value_type const,
                                       boost::bidirectional_traversal_tag>
    {
        typedef boost::iterator_adaptor<iterator_base<iterator_type>,
                                       iterator_type,
                                       value_type const,
                                       boost::bidirectional_traversal_tag> super_t;

        friend class boost::iterator_core_access;
        friend class priority_queue_mutable_wrapper;

        iterator_base(void):
            super_t(0)
        {}

        template <typename T>
        explicit iterator_base(T const & it):
            super_t(it)
        {}

        value_type const & dereference() const
        {
            return super_t::base()->first;
        }

        iterator_type get_list_iterator() const
        {
            return super_t::base_reference();
        }
    };

    typedef iterator_base<list_iterator> iterator;
    typedef iterator_base<const_list_iterator> const_iterator;

    typedef typename object_list::difference_type difference_type;

    class ordered_iterator:
        public boost::iterator_adaptor<ordered_iterator,
                                       const_list_iterator,
                                       value_type const,
                                       boost::forward_traversal_tag
                                      >,
        q_type::ordered_iterator_dispatcher
    {
        typedef boost::iterator_adaptor<ordered_iterator,
                                        const_list_iterator,
                                        value_type const,
                                        boost::forward_traversal_tag
                                      > adaptor_type;

        typedef const_list_iterator iterator;
        typedef typename q_type::ordered_iterator_dispatcher ordered_iterator_dispatcher;

        friend class boost::iterator_core_access;

    public:
        ordered_iterator(void):
            adaptor_type(0), unvisited_nodes(indirect_cmp()), q_(NULL)
        {}

        ordered_iterator(const priority_queue_mutable_wrapper * q, indirect_cmp const & cmp):
            adaptor_type(0), unvisited_nodes(cmp), q_(q)
        {}

        ordered_iterator(const_list_iterator it, const priority_queue_mutable_wrapper * q, indirect_cmp const & cmp):
            adaptor_type(it), unvisited_nodes(cmp), q_(q)
        {
            if (it != q->objects.end())
                discover_nodes(it);
        }

        bool operator!=(ordered_iterator const & rhs) const
        {
            return adaptor_type::base() != rhs.base();
        }

        bool operator==(ordered_iterator const & rhs) const
        {
            return !operator!=(rhs);
        }

    private:
        void increment(void)
        {
            if (unvisited_nodes.empty())
                adaptor_type::base_reference() = q_->objects.end();
            else {
                iterator next = unvisited_nodes.top();
                unvisited_nodes.pop();
                discover_nodes(next);
                adaptor_type::base_reference() = next;
            }
        }

        value_type const & dereference() const
        {
            return adaptor_type::base()->first;
        }

        void discover_nodes(iterator current)
        {
            size_type current_index = current->second;
            const q_type * q = &(q_->q_);

            if (ordered_iterator_dispatcher::is_leaf(q, current_index))
                return;

            std::pair<size_type, size_type> child_range = ordered_iterator_dispatcher::get_child_nodes(q, current_index);

            for (size_type i = child_range.first; i <= child_range.second; ++i) {
                typename q_type::internal_type const & internal_value_at_index = ordered_iterator_dispatcher::get_internal_value(q, i);
                typename q_type::value_type const & value_at_index = q_->q_.get_value(internal_value_at_index);

                unvisited_nodes.push(value_at_index);
            }
        }

        std::priority_queue<iterator,
                            std::vector<iterator, typename boost::allocator_rebind<allocator_type, iterator>::type>,
                            indirect_cmp
                           > unvisited_nodes;
        const priority_queue_mutable_wrapper * q_;
    };

    bool empty(void) const
    {
        return q_.empty();
    }

    size_type size(void) const
    {
        return q_.size();
    }

    size_type max_size(void) const
    {
        return objects.max_size();
    }

    void clear(void)
    {
        q_.clear();
        objects.clear();
    }

    allocator_type get_allocator(void) const
    {
        return q_.get_allocator();
    }

    void swap(priority_queue_mutable_wrapper & rhs)
    {
        objects.swap(rhs.objects);
        q_.swap(rhs.q_);
    }

    const_reference top(void) const
    {
        BOOST_ASSERT(!empty());
        return q_.top()->first;
    }

    handle_type push(value_type const & v)
    {
        objects.push_front(std::make_pair(v, 0));
        list_iterator ret = objects.begin();
        q_.push(ret);
        return handle_type(ret);
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <class... Args>
    handle_type emplace(Args&&... args)
    {
        objects.push_front(std::make_pair(std::forward<Args>(args)..., 0));
        list_iterator ret = objects.begin();
        q_.push(ret);
        return handle_type(ret);
    }
#endif

    void pop(void)
    {
        BOOST_ASSERT(!empty());
        list_iterator q_top = q_.top();
        q_.pop();
        objects.erase(q_top);
    }

    /**
     * \b Effects: Assigns \c v to the element handled by \c handle & updates the priority queue.
     *
     * \b Complexity: Logarithmic.
     *
     * */
    void update(handle_type handle, const_reference v)
    {
        list_iterator it = handle.iterator;
        value_type const & current_value = it->first;
        value_compare const & cmp = q_.value_comp();
        if (cmp(v, current_value))
            decrease(handle, v);
        else
            increase(handle, v);
    }

    /**
     * \b Effects: Updates the heap after the element handled by \c handle has been changed.
     *
     * \b Complexity: Logarithmic.
     *
     * \b Note: If this is not called, after a handle has been updated, the behavior of the data structure is undefined!
     * */
    void update(handle_type handle)
    {
        list_iterator it = handle.iterator;
        size_type index = it->second;
        q_.update(index);
    }

     /**
     * \b Effects: Assigns \c v to the element handled by \c handle & updates the priority queue.
     *
     * \b Complexity: Logarithmic.
     *
     * \b Note: The new value is expected to be greater than the current one
     * */
    void increase(handle_type handle, const_reference v)
    {
        BOOST_ASSERT(!value_compare()(v, handle.iterator->first));
        handle.iterator->first = v;
        increase(handle);
    }

    /**
     * \b Effects: Updates the heap after the element handled by \c handle has been changed.
     *
     * \b Complexity: Logarithmic.
     *
     * \b Note: The new value is expected to be greater than the current one. If this is not called, after a handle has been updated, the behavior of the data structure is undefined!
     * */
    void increase(handle_type handle)
    {
        list_iterator it = handle.iterator;
        size_type index = it->second;
        q_.increase(index);
    }

     /**
     * \b Effects: Assigns \c v to the element handled by \c handle & updates the priority queue.
     *
     * \b Complexity: Logarithmic.
     *
     * \b Note: The new value is expected to be less than the current one
     * */
    void decrease(handle_type handle, const_reference v)
    {
        BOOST_ASSERT(!value_compare()(handle.iterator->first, v));
        handle.iterator->first = v;
        decrease(handle);
    }

    /**
     * \b Effects: Updates the heap after the element handled by \c handle has been changed.
     *
     * \b Complexity: Logarithmic.
     *
     * \b Note: The new value is expected to be less than the current one. If this is not called, after a handle has been updated, the behavior of the data structure is undefined!
     * */
    void decrease(handle_type handle)
    {
        list_iterator it = handle.iterator;
        size_type index = it->second;
        q_.decrease(index);
    }

    /**
     * \b Effects: Removes the element handled by \c handle from the priority_queue.
     *
     * \b Complexity: Logarithmic.
     * */
    void erase(handle_type handle)
    {
        list_iterator it = handle.iterator;
        size_type index = it->second;
        q_.erase(index);
        objects.erase(it);
    }

    const_iterator begin(void) const
    {
        return const_iterator(objects.begin());
    }

    const_iterator end(void) const
    {
        return const_iterator(objects.end());
    }

    iterator begin(void)
    {
        return iterator(objects.begin());
    }

    iterator end(void)
    {
        return iterator(objects.end());
    }

    ordered_iterator ordered_begin(void) const
    {
        if (!empty())
            return ordered_iterator(q_.top(), this, indirect_cmp(q_.value_comp()));
        else
            return ordered_end();
    }

    ordered_iterator ordered_end(void) const
    {
        return ordered_iterator(objects.end(), this, indirect_cmp(q_.value_comp()));
    }

    static handle_type s_handle_from_iterator(iterator const & it)
    {
        return handle_type(it.get_list_iterator());
    }

    value_compare const & value_comp(void) const
    {
        return q_.value_comp();
    }

    void reserve (size_type element_count)
    {
        q_.reserve(element_count);
    }
};


} /* namespace detail */
} /* namespace heap */
} /* namespace boost */

#endif /* BOOST_HEAP_DETAIL_MUTABLE_HEAP_HPP */
