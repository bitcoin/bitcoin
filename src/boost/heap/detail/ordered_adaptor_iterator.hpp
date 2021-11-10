// boost heap: ordered iterator helper classes for container adaptors
//
// Copyright (C) 2011 Tim Blechmann
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HEAP_DETAIL_ORDERED_ADAPTOR_ITERATOR_HPP
#define BOOST_HEAP_DETAIL_ORDERED_ADAPTOR_ITERATOR_HPP

#include <cassert>
#include <limits>

#include <boost/assert.hpp>
#include <boost/heap/detail/tree_iterator.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/concept_check.hpp>

namespace boost  {
namespace heap   {
namespace detail {

/* ordered iterator helper classes for container adaptors
 *
 * Requirements for Dispatcher:
 *
 * * static size_type max_index(const ContainerType * heap); // return maximum index
 * * static bool is_leaf(const ContainerType * heap, size_type index); // return if index denotes a leaf
 * * static std::pair<size_type, size_type> get_child_nodes(const ContainerType * heap, size_type index); // get index range of child nodes
 * * static internal_type const & get_internal_value(const ContainerType * heap, size_type index); // get internal value at index
 * * static value_type const & get_value(internal_type const & arg) const; // get value_type from internal_type
 *
 * */
template <typename ValueType,
          typename InternalType,
          typename ContainerType,
          typename Alloc,
          typename ValueCompare,
          typename Dispatcher
         >
class ordered_adaptor_iterator:
    public boost::iterator_facade<ordered_adaptor_iterator<ValueType,
                                                           InternalType,
                                                           ContainerType,
                                                           Alloc,
                                                           ValueCompare,
                                                           Dispatcher>,
                                  ValueType,
                                  boost::forward_traversal_tag
                                 >,
    Dispatcher
{
    friend class boost::iterator_core_access;

    struct compare_by_heap_value:
        ValueCompare
    {
        const ContainerType * container;

        compare_by_heap_value (const ContainerType * container, ValueCompare const & cmp):
            ValueCompare(cmp), container(container)
        {}

        bool operator()(size_t lhs, size_t rhs)
        {
            BOOST_ASSERT(lhs <= Dispatcher::max_index(container));
            BOOST_ASSERT(rhs <= Dispatcher::max_index(container));
            return ValueCompare::operator()(Dispatcher::get_internal_value(container, lhs),
                                            Dispatcher::get_internal_value(container, rhs));
        }
    };

    const ContainerType * container;
    size_t current_index; // current index: special value -1 denotes `end' iterator

public:
    ordered_adaptor_iterator(void):
        container(NULL), current_index((std::numeric_limits<size_t>::max)()),
        unvisited_nodes(compare_by_heap_value(NULL, ValueCompare()))
    {}

    ordered_adaptor_iterator(const ContainerType * container, ValueCompare const & cmp):
        container(container), current_index(container->size()),
        unvisited_nodes(compare_by_heap_value(container, ValueCompare()))
    {}

    ordered_adaptor_iterator(size_t initial_index, const ContainerType * container, ValueCompare const & cmp):
        container(container), current_index(initial_index),
        unvisited_nodes(compare_by_heap_value(container, cmp))
    {
        discover_nodes(initial_index);
    }

private:
    bool equal (ordered_adaptor_iterator const & rhs) const
    {
        if (current_index != rhs.current_index)
            return false;

        if (container != rhs.container) // less likely than first check
            return false;

        return true;
    }

    void increment(void)
    {
        if (unvisited_nodes.empty())
            current_index = Dispatcher::max_index(container) + 1;
        else {
            current_index = unvisited_nodes.top();
            unvisited_nodes.pop();
            discover_nodes(current_index);
        }
    }

    ValueType const & dereference() const
    {
        BOOST_ASSERT(current_index <= Dispatcher::max_index(container));
        return Dispatcher::get_value(Dispatcher::get_internal_value(container, current_index));
    }

    void discover_nodes(size_t index)
    {
        if (Dispatcher::is_leaf(container, index))
            return;

        std::pair<size_t, size_t> child_range = Dispatcher::get_child_nodes(container, index);

        for (size_t i = child_range.first; i <= child_range.second; ++i)
            unvisited_nodes.push(i);
    }

    std::priority_queue<size_t,
                        std::vector<size_t, typename boost::allocator_rebind<Alloc, size_t>::type>,
                        compare_by_heap_value
                       > unvisited_nodes;
};


} /* namespace detail */
} /* namespace heap */
} /* namespace boost */

#endif /* BOOST_HEAP_DETAIL_ORDERED_ADAPTOR_ITERATOR_HPP */
