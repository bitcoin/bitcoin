// Copyright 2004, 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Nick Edmonds
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_MESH_GENERATOR_HPP
#define BOOST_GRAPH_MESH_GENERATOR_HPP

#include <iterator>
#include <utility>
#include <boost/assert.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost
{

template < typename Graph > class mesh_iterator
{
    typedef typename graph_traits< Graph >::directed_category directed_category;
    typedef
        typename graph_traits< Graph >::vertices_size_type vertices_size_type;

    BOOST_STATIC_CONSTANT(bool,
        is_undirected
        = (is_base_and_derived< undirected_tag, directed_category >::value
            || is_same< undirected_tag, directed_category >::value));

public:
    typedef std::input_iterator_tag iterator_category;
    typedef std::pair< vertices_size_type, vertices_size_type > value_type;
    typedef const value_type& reference;
    typedef const value_type* pointer;
    typedef void difference_type;

    mesh_iterator() : x(0), y(0), done(true) {}

    // Vertices are numbered in row-major order
    // Assumes directed
    mesh_iterator(
        vertices_size_type x, vertices_size_type y, bool toroidal = true)
    : x(x)
    , y(y)
    , n(x * y)
    , source(0)
    , target(1)
    , current(0, 1)
    , toroidal(toroidal)
    , done(false)
    {
        BOOST_ASSERT(x > 1 && y > 1);
    }

    reference operator*() const { return current; }
    pointer operator->() const { return &current; }

    mesh_iterator& operator++()
    {
        if (is_undirected)
        {
            if (!toroidal)
            {
                if (target == source + 1)
                    if (source < x * (y - 1))
                        target = source + x;
                    else
                    {
                        source++;
                        target = (source % x) < x - 1 ? source + 1 : source + x;
                        if (target > n)
                            done = true;
                    }
                else if (target == source + x)
                {
                    source++;
                    target = (source % x) < x - 1 ? source + 1 : source + x;
                }
            }
            else
            {
                if (target == source + 1 || target == source - (source % x))
                    target = (source + x) % n;
                else if (target == (source + x) % n)
                {
                    if (source == n - 1)
                        done = true;
                    else
                    {
                        source++;
                        target = (source % x) < (x - 1) ? source + 1
                                                        : source - (source % x);
                    }
                }
            }
        }
        else
        { // Directed
            if (!toroidal)
            {
                if (target == source - x)
                    target = source % x > 0 ? source - 1 : source + 1;
                else if (target == source - 1)
                    if ((source % x) < (x - 1))
                        target = source + 1;
                    else if (source < x * (y - 1))
                        target = source + x;
                    else
                    {
                        done = true;
                    }
                else if (target == source + 1)
                    if (source < x * (y - 1))
                        target = source + x;
                    else
                    {
                        source++;
                        target = source - x;
                    }
                else if (target == source + x)
                {
                    source++;
                    target = (source >= x) ? source - x : source - 1;
                }
            }
            else
            {
                if (source == n - 1 && target == (source + x) % n)
                    done = true;
                else if (target == source - 1 || target == source + x - 1)
                    target = (source + x) % n;
                else if (target == source + 1
                    || target == source - (source % x))
                    target = (source - x + n) % n;
                else if (target == (source - x + n) % n)
                    target = (source % x > 0) ? source - 1 : source + x - 1;
                else if (target == (source + x) % n)
                {
                    source++;
                    target = (source % x) < (x - 1) ? source + 1
                                                    : source - (source % x);
                }
            }
        }

        current.first = source;
        current.second = target;

        return *this;
    }

    mesh_iterator operator++(int)
    {
        mesh_iterator temp(*this);
        ++(*this);
        return temp;
    }

    bool operator==(const mesh_iterator& other) const
    {
        return done == other.done;
    }

    bool operator!=(const mesh_iterator& other) const
    {
        return !(*this == other);
    }

private:
    vertices_size_type x, y;
    vertices_size_type n;
    vertices_size_type source;
    vertices_size_type target;
    value_type current;
    bool toroidal;
    bool done;
};

} // end namespace boost

#endif // BOOST_GRAPH_MESH_GENERATOR_HPP
