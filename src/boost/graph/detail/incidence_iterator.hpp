//
//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
#ifndef BOOST_GRAPH_DETAIL_INCIDENCE_ITERATOR_HPP
#define BOOST_GRAPH_DETAIL_INCIDENCE_ITERATOR_HPP

#include <utility>
#include <iterator>

// OBSOLETE

namespace boost
{

namespace detail
{
    // EdgeDir tags
    struct in_edge_tag
    {
    };
    struct out_edge_tag
    {
    };

    template < class Vertex, class Edge, class Iterator1D, class EdgeDir >
    struct bidir_incidence_iterator
    {
        typedef bidir_incidence_iterator self;
        typedef Edge edge_type;
        typedef typename Edge::property_type EdgeProperty;

    public:
        typedef int difference_type;
        typedef std::forward_iterator_tag iterator_category;
        typedef edge_type reference;
        typedef edge_type value_type;
        typedef value_type* pointer;
        inline bidir_incidence_iterator() {}
        inline bidir_incidence_iterator(Iterator1D ii, Vertex src)
        : i(ii), _src(src)
        {
        }

        inline self& operator++()
        {
            ++i;
            return *this;
        }
        inline self operator++(int)
        {
            self tmp = *this;
            ++(*this);
            return tmp;
        }

        inline reference operator*() const { return deref_helper(EdgeDir()); }
        inline self* operator->() { return this; }

        Iterator1D& iter() { return i; }
        const Iterator1D& iter() const { return i; }

        Iterator1D i;
        Vertex _src;

    protected:
        inline reference deref_helper(out_edge_tag) const
        {
            return edge_type(_src, (*i).get_target(), &(*i).get_property());
        }
        inline reference deref_helper(in_edge_tag) const
        {
            return edge_type((*i).get_target(), _src, &(*i).get_property());
        }
    };

    template < class V, class E, class Iter, class Dir >
    inline bool operator==(const bidir_incidence_iterator< V, E, Iter, Dir >& x,
        const bidir_incidence_iterator< V, E, Iter, Dir >& y)
    {
        return x.i == y.i;
    }
    template < class V, class E, class Iter, class Dir >
    inline bool operator!=(const bidir_incidence_iterator< V, E, Iter, Dir >& x,
        const bidir_incidence_iterator< V, E, Iter, Dir >& y)
    {
        return x.i != y.i;
    }

}
}
#endif
