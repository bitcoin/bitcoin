//  (C) Copyright Jeremy Siek 2004
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DETAIL_DISJOINT_SETS_HPP
#define BOOST_DETAIL_DISJOINT_SETS_HPP

namespace boost
{

namespace detail
{

    template < class ParentPA, class Vertex >
    Vertex find_representative_with_path_halving(ParentPA p, Vertex v)
    {
        Vertex parent = get(p, v);
        Vertex grandparent = get(p, parent);
        while (parent != grandparent)
        {
            put(p, v, grandparent);
            v = grandparent;
            parent = get(p, v);
            grandparent = get(p, parent);
        }
        return parent;
    }

    template < class ParentPA, class Vertex >
    Vertex find_representative_with_full_compression(ParentPA parent, Vertex v)
    {
        Vertex old = v;
        Vertex ancestor = get(parent, v);
        while (ancestor != v)
        {
            v = ancestor;
            ancestor = get(parent, v);
        }
        v = get(parent, old);
        while (ancestor != v)
        {
            put(parent, old, ancestor);
            old = v;
            v = get(parent, old);
        }
        return ancestor;
    }

    /* the postcondition of link sets is:
     component_representative(i) == component_representative(j)
     */
    template < class ParentPA, class RankPA, class Vertex,
        class ComponentRepresentative >
    inline void link_sets(ParentPA p, RankPA rank, Vertex i, Vertex j,
        ComponentRepresentative comp_rep)
    {
        i = comp_rep(p, i);
        j = comp_rep(p, j);
        if (i == j)
            return;
        if (get(rank, i) > get(rank, j))
            put(p, j, i);
        else
        {
            put(p, i, j);
            if (get(rank, i) == get(rank, j))
                put(rank, j, get(rank, j) + 1);
        }
    }

    // normalize components has the following postcondidition:
    // i >= p[i]
    // that is, the representative is the node with the smallest index in its
    // class as its precondition it it assumes that the node container is
    // compressed

    template < class ParentPA, class Vertex >
    inline void normalize_node(ParentPA p, Vertex i)
    {
        if (i > get(p, i) || get(p, get(p, i)) != get(p, i))
            put(p, i, get(p, get(p, i)));
        else
        {
            put(p, get(p, i), i);
            put(p, i, i);
        }
    }

} // namespace detail
} // namespace boost

#endif // BOOST_DETAIL_DISJOINT_SETS_HPP
