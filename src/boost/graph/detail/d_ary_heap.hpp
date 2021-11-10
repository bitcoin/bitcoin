//
//=======================================================================
// Copyright 2009 Trustees of Indiana University
// Authors: Jeremiah J. Willcock, Andrew Lumsdaine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
#ifndef BOOST_D_ARY_HEAP_HPP
#define BOOST_D_ARY_HEAP_HPP

#include <vector>
#include <cstddef>
#include <algorithm>
#include <utility>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/shared_array.hpp>
#include <boost/property_map/property_map.hpp>

// WARNING: it is not safe to copy a d_ary_heap_indirect and then modify one of
// the copies.  The class is required to be copyable so it can be passed around
// (without move support from C++11), but it deep-copies the heap contents yet
// shallow-copies the index_in_heap_map.

namespace boost
{

// Swap two elements in a property map without assuming they model
// LvaluePropertyMap -- currently not used
template < typename PropMap >
inline void property_map_swap(PropMap prop_map,
    const typename boost::property_traits< PropMap >::key_type& ka,
    const typename boost::property_traits< PropMap >::key_type& kb)
{
    typename boost::property_traits< PropMap >::value_type va
        = get(prop_map, ka);
    put(prop_map, ka, get(prop_map, kb));
    put(prop_map, kb, va);
}

namespace detail
{
    template < typename Value > class fixed_max_size_vector
    {
        boost::shared_array< Value > m_data;
        std::size_t m_size;

    public:
        typedef std::size_t size_type;
        fixed_max_size_vector(std::size_t max_size)
        : m_data(new Value[max_size]), m_size(0)
        {
        }
        std::size_t size() const { return m_size; }
        bool empty() const { return m_size == 0; }
        Value& operator[](std::size_t i) { return m_data[i]; }
        const Value& operator[](std::size_t i) const { return m_data[i]; }
        void push_back(Value v) { m_data[m_size++] = v; }
        void pop_back() { --m_size; }
        Value& back() { return m_data[m_size - 1]; }
        const Value& back() const { return m_data[m_size - 1]; }
    };
}

// D-ary heap using an indirect compare operator (use identity_property_map
// as DistanceMap to get a direct compare operator).  This heap appears to be
// commonly used for Dijkstra's algorithm for its good practical performance
// on some platforms; asymptotically, it has an O(lg N) decrease-key
// operation while that can be done in constant time on a relaxed heap.  The
// implementation is mostly based on the binary heap page on Wikipedia and
// online sources that state that the operations are the same for d-ary
// heaps.  This code is not based on the old Boost d-ary heap code.
//
// - d_ary_heap_indirect is a model of UpdatableQueue as is needed for
//   dijkstra_shortest_paths.
//
// - Value must model Assignable.
// - Arity must be at least 2 (optimal value appears to be 4, both in my and
//   third-party experiments).
// - IndexInHeapMap must be a ReadWritePropertyMap from Value to
//   Container::size_type (to store the index of each stored value within the
//   heap for decrease-key aka update).
// - DistanceMap must be a ReadablePropertyMap from Value to something
//   (typedef'ed as distance_type).
// - Compare must be a BinaryPredicate used as a less-than operator on
//   distance_type.
// - Container must be a random-access, contiguous container (in practice,
//   the operations used probably require that it is std::vector<Value>).
//
template < typename Value, std::size_t Arity, typename IndexInHeapPropertyMap,
    typename DistanceMap, typename Compare = std::less< Value >,
    typename Container = std::vector< Value > >
class d_ary_heap_indirect
{
    BOOST_STATIC_ASSERT(Arity >= 2);

public:
    typedef typename Container::size_type size_type;
    typedef Value value_type;
    typedef typename boost::property_traits< DistanceMap >::value_type key_type;
    typedef DistanceMap key_map;

    d_ary_heap_indirect(DistanceMap distance,
        IndexInHeapPropertyMap index_in_heap,
        const Compare& compare = Compare(), const Container& data = Container())
    : compare(compare)
    , data(data)
    , distance(distance)
    , index_in_heap(index_in_heap)
    {
    }
    /* Implicit copy constructor */
    /* Implicit assignment operator */

    size_type size() const { return data.size(); }

    bool empty() const { return data.empty(); }

    void push(const Value& v)
    {
        size_type index = data.size();
        data.push_back(v);
        put(index_in_heap, v, index);
        preserve_heap_property_up(index);
        verify_heap();
    }

    Value& top()
    {
        BOOST_ASSERT(!this->empty());
        return data[0];
    }

    const Value& top() const
    {
        BOOST_ASSERT(!this->empty());
        return data[0];
    }

    void pop()
    {
        BOOST_ASSERT(!this->empty());
        put(index_in_heap, data[0], (size_type)(-1));
        if (data.size() != 1)
        {
            data[0] = data.back();
            put(index_in_heap, data[0], (size_type)(0));
            data.pop_back();
            preserve_heap_property_down();
            verify_heap();
        }
        else
        {
            data.pop_back();
        }
    }

    // This function assumes the key has been updated (using an external write
    // to the distance map or such)
    // See
    // http://coding.derkeiler.com/Archive/General/comp.theory/2007-05/msg00043.html
    void update(const Value& v)
    { /* decrease-key */
        size_type index = get(index_in_heap, v);
        preserve_heap_property_up(index);
        verify_heap();
    }

    bool contains(const Value& v) const
    {
        size_type index = get(index_in_heap, v);
        return (index != (size_type)(-1));
    }

    void push_or_update(const Value& v)
    { /* insert if not present, else update */
        size_type index = get(index_in_heap, v);
        if (index == (size_type)(-1))
        {
            index = data.size();
            data.push_back(v);
            put(index_in_heap, v, index);
        }
        preserve_heap_property_up(index);
        verify_heap();
    }

    DistanceMap keys() const { return distance; }

private:
    Compare compare;
    Container data;
    DistanceMap distance;
    IndexInHeapPropertyMap index_in_heap;

    // The distances being compared using compare and that are stored in the
    // distance map
    typedef typename boost::property_traits< DistanceMap >::value_type
        distance_type;

    // Get the parent of a given node in the heap
    static size_type parent(size_type index) { return (index - 1) / Arity; }

    // Get the child_idx'th child of a given node; 0 <= child_idx < Arity
    static size_type child(size_type index, std::size_t child_idx)
    {
        return index * Arity + child_idx + 1;
    }

    // Swap two elements in the heap by index, updating index_in_heap
    void swap_heap_elements(size_type index_a, size_type index_b)
    {
        using std::swap;
        Value value_a = data[index_a];
        Value value_b = data[index_b];
        data[index_a] = value_b;
        data[index_b] = value_a;
        put(index_in_heap, value_a, index_b);
        put(index_in_heap, value_b, index_a);
    }

    // Emulate the indirect_cmp that is now folded into this heap class
    bool compare_indirect(const Value& a, const Value& b) const
    {
        return compare(get(distance, a), get(distance, b));
    }

    // Verify that the array forms a heap; commented out by default
    void verify_heap() const
    {
        // This is a very expensive test so it should be disabled even when
        // NDEBUG is not defined
#if 0
      for (size_t i = 1; i < data.size(); ++i) {
        if (compare_indirect(data[i], data[parent(i)])) {
          BOOST_ASSERT (!"Element is smaller than its parent");
        }
      }
#endif
    }

    // Starting at a node, move up the tree swapping elements to preserve the
    // heap property
    void preserve_heap_property_up(size_type index)
    {
        size_type orig_index = index;
        size_type num_levels_moved = 0;
        // The first loop just saves swaps that need to be done in order to
        // avoid aliasing issues in its search; there is a second loop that does
        // the necessary swap operations
        if (index == 0)
            return; // Do nothing on root
        Value currently_being_moved = data[index];
        distance_type currently_being_moved_dist
            = get(distance, currently_being_moved);
        for (;;)
        {
            if (index == 0)
                break; // Stop at root
            size_type parent_index = parent(index);
            Value parent_value = data[parent_index];
            if (compare(
                    currently_being_moved_dist, get(distance, parent_value)))
            {
                ++num_levels_moved;
                index = parent_index;
                continue;
            }
            else
            {
                break; // Heap property satisfied
            }
        }
        // Actually do the moves -- move num_levels_moved elements down in the
        // tree, then put currently_being_moved at the top
        index = orig_index;
        for (size_type i = 0; i < num_levels_moved; ++i)
        {
            size_type parent_index = parent(index);
            Value parent_value = data[parent_index];
            put(index_in_heap, parent_value, index);
            data[index] = parent_value;
            index = parent_index;
        }
        data[index] = currently_being_moved;
        put(index_in_heap, currently_being_moved, index);
        verify_heap();
    }

    // From the root, swap elements (each one with its smallest child) if there
    // are any parent-child pairs that violate the heap property
    void preserve_heap_property_down()
    {
        if (data.empty())
            return;
        size_type index = 0;
        Value currently_being_moved = data[0];
        distance_type currently_being_moved_dist
            = get(distance, currently_being_moved);
        size_type heap_size = data.size();
        Value* data_ptr = &data[0];
        for (;;)
        {
            size_type first_child_index = child(index, 0);
            if (first_child_index >= heap_size)
                break; /* No children */
            Value* child_base_ptr = data_ptr + first_child_index;
            size_type smallest_child_index = 0;
            distance_type smallest_child_dist
                = get(distance, child_base_ptr[smallest_child_index]);
            if (first_child_index + Arity <= heap_size)
            {
                // Special case for a statically known loop count (common case)
                for (size_t i = 1; i < Arity; ++i)
                {
                    Value i_value = child_base_ptr[i];
                    distance_type i_dist = get(distance, i_value);
                    if (compare(i_dist, smallest_child_dist))
                    {
                        smallest_child_index = i;
                        smallest_child_dist = i_dist;
                    }
                }
            }
            else
            {
                for (size_t i = 1; i < heap_size - first_child_index; ++i)
                {
                    distance_type i_dist = get(distance, child_base_ptr[i]);
                    if (compare(i_dist, smallest_child_dist))
                    {
                        smallest_child_index = i;
                        smallest_child_dist = i_dist;
                    }
                }
            }
            if (compare(smallest_child_dist, currently_being_moved_dist))
            {
                swap_heap_elements(
                    smallest_child_index + first_child_index, index);
                index = smallest_child_index + first_child_index;
                continue;
            }
            else
            {
                break; // Heap property satisfied
            }
        }
        verify_heap();
    }
};

} // namespace boost

#endif // BOOST_D_ARY_HEAP_HPP
