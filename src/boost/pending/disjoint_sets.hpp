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
#ifndef BOOST_DISJOINT_SETS_HPP
#define BOOST_DISJOINT_SETS_HPP

#include <vector>
#include <boost/graph/properties.hpp>
#include <boost/pending/detail/disjoint_sets.hpp>

namespace boost
{

struct find_with_path_halving
{
    template < class ParentPA, class Vertex >
    Vertex operator()(ParentPA p, Vertex v)
    {
        return detail::find_representative_with_path_halving(p, v);
    }
};

struct find_with_full_path_compression
{
    template < class ParentPA, class Vertex >
    Vertex operator()(ParentPA p, Vertex v)
    {
        return detail::find_representative_with_full_compression(p, v);
    }
};

// This is a generalized functor to provide disjoint sets operations
// with "union by rank" and "path compression".  A disjoint-set data
// structure maintains a collection S={S1, S2, ..., Sk} of disjoint
// sets. Each set is identified by a representative, which is some
// member of of the set. Sets are represented by rooted trees. Two
// heuristics: "union by rank" and "path compression" are used to
// speed up the operations.

// Disjoint Set requires two vertex properties for internal use.  A
// RankPA and a ParentPA. The RankPA must map Vertex to some Integral type
// (preferably the size_type associated with Vertex). The ParentPA
// must map Vertex to Vertex.
template < class RankPA, class ParentPA,
    class FindCompress = find_with_full_path_compression >
class disjoint_sets
{
    typedef disjoint_sets self;

    inline disjoint_sets() {}

public:
    inline disjoint_sets(RankPA r, ParentPA p) : rank(r), parent(p) {}

    inline disjoint_sets(const self& c) : rank(c.rank), parent(c.parent) {}

    // Make Set -- Create a singleton set containing vertex x
    template < class Element > inline void make_set(Element x)
    {
        put(parent, x, x);
        typedef typename property_traits< RankPA >::value_type R;
        put(rank, x, R());
    }

    // Link - union the two sets represented by vertex x and y
    template < class Element > inline void link(Element x, Element y)
    {
        detail::link_sets(parent, rank, x, y, rep);
    }

    // Union-Set - union the two sets containing vertex x and y
    template < class Element > inline void union_set(Element x, Element y)
    {
        link(find_set(x), find_set(y));
    }

    // Find-Set - returns the Element representative of the set
    // containing Element x and applies path compression.
    template < class Element > inline Element find_set(Element x)
    {
        return rep(parent, x);
    }

    template < class ElementIterator >
    inline std::size_t count_sets(ElementIterator first, ElementIterator last)
    {
        std::size_t count = 0;
        for (; first != last; ++first)
            if (get(parent, *first) == *first)
                ++count;
        return count;
    }

    template < class ElementIterator >
    inline void normalize_sets(ElementIterator first, ElementIterator last)
    {
        for (; first != last; ++first)
            detail::normalize_node(parent, *first);
    }

    template < class ElementIterator >
    inline void compress_sets(ElementIterator first, ElementIterator last)
    {
        for (; first != last; ++first)
            detail::find_representative_with_full_compression(parent, *first);
    }

protected:
    RankPA rank;
    ParentPA parent;
    FindCompress rep;
};

template < class ID = identity_property_map,
    class InverseID = identity_property_map,
    class FindCompress = find_with_full_path_compression >
class disjoint_sets_with_storage
{
    typedef typename property_traits< ID >::value_type Index;
    typedef std::vector< Index > ParentContainer;
    typedef std::vector< unsigned char > RankContainer;

public:
    typedef typename ParentContainer::size_type size_type;

    disjoint_sets_with_storage(
        size_type n = 0, ID id_ = ID(), InverseID inv = InverseID())
    : id(id_), id_to_vertex(inv), rank(n, 0), parent(n)
    {
        for (Index i = 0; i < n; ++i)
            parent[i] = i;
    }
    // note this is not normally needed
    template < class Element > inline void make_set(Element x)
    {
        parent[x] = x;
        rank[x] = 0;
    }
    template < class Element > inline void link(Element x, Element y)
    {
        extend_sets(x, y);
        detail::link_sets(&parent[0], &rank[0], get(id, x), get(id, y), rep);
    }
    template < class Element > inline void union_set(Element x, Element y)
    {
        Element rx = find_set(x);
        Element ry = find_set(y);
        link(rx, ry);
    }
    template < class Element > inline Element find_set(Element x)
    {
        return id_to_vertex[rep(&parent[0], get(id, x))];
    }

    template < class ElementIterator >
    inline std::size_t count_sets(ElementIterator first, ElementIterator last)
    {
        std::size_t count = 0;
        for (; first != last; ++first)
            if (parent[*first] == *first)
                ++count;
        return count;
    }

    template < class ElementIterator >
    inline void normalize_sets(ElementIterator first, ElementIterator last)
    {
        for (; first != last; ++first)
            detail::normalize_node(&parent[0], *first);
    }

    template < class ElementIterator >
    inline void compress_sets(ElementIterator first, ElementIterator last)
    {
        for (; first != last; ++first)
            detail::find_representative_with_full_compression(
                &parent[0], *first);
    }

    const ParentContainer& parents() { return parent; }

protected:
    template < class Element > inline void extend_sets(Element x, Element y)
    {
        Index needed
            = get(id, x) > get(id, y) ? get(id, x) + 1 : get(id, y) + 1;
        if (needed > parent.size())
        {
            rank.insert(rank.end(), needed - rank.size(), 0);
            for (Index k = parent.size(); k < needed; ++k)
                parent.push_back(k);
        }
    }

    ID id;
    InverseID id_to_vertex;
    RankContainer rank;
    ParentContainer parent;
    FindCompress rep;
};

} // namespace boost

#endif // BOOST_DISJOINT_SETS_HPP
