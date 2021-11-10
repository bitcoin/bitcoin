//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef BOOST_SELF_AVOIDING_WALK_HPP
#define BOOST_SELF_AVOIDING_WALK_HPP

/*
  This file defines necessary components for SAW.

  mesh language: (defined by myself to clearify what is what)
  A triangle in mesh is called an triangle.
  An edge in mesh is called an line.
  A vertex in mesh is called a point.

  A triangular mesh corresponds to a graph in which a vertex is a
  triangle and an edge(u, v) stands for triangle u and triangle v
  share an line.

  After this point, a vertex always refers to vertex in graph,
  therefore it is a traingle in mesh.

 */

#include <utility>
#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>

#define SAW_SENTINAL -1

namespace boost
{

template < class T1, class T2, class T3 > struct triple
{
    T1 first;
    T2 second;
    T3 third;
    triple(const T1& a, const T2& b, const T3& c)
    : first(a), second(b), third(c)
    {
    }
    triple() : first(SAW_SENTINAL), second(SAW_SENTINAL), third(SAW_SENTINAL) {}
};

typedef triple< int, int, int > Triple;

/* Define a vertex property which has a triangle inside. Triangle is
  represented by a triple.  */
struct triangle_tag
{
    enum
    {
        num = 100
    };
};
typedef property< triangle_tag, Triple > triangle_property;

/* Define an edge property with a line. A line is represented by a
  pair.  This is not required for SAW though.
*/
struct line_tag
{
    enum
    {
        num = 101
    };
};
template < class T >
struct line_property : public property< line_tag, std::pair< T, T > >
{
};

/*Precondition: Points in a Triangle are in order */
template < class Triangle, class Line >
inline void get_sharing(const Triangle& a, const Triangle& b, Line& l)
{
    l.first = SAW_SENTINAL;
    l.second = SAW_SENTINAL;

    if (a.first == b.first)
    {
        l.first = a.first;
        if (a.second == b.second || a.second == b.third)
            l.second = a.second;
        else if (a.third == b.second || a.third == b.third)
            l.second = a.third;
    }
    else if (a.first == b.second)
    {
        l.first = a.first;
        if (a.second == b.third)
            l.second = a.second;
        else if (a.third == b.third)
            l.second = a.third;
    }
    else if (a.first == b.third)
    {
        l.first = a.first;
    }
    else if (a.second == b.first)
    {
        l.first = a.second;
        if (a.third == b.second || a.third == b.third)
            l.second = a.third;
    }
    else if (a.second == b.second)
    {
        l.first = a.second;
        if (a.third == b.third)
            l.second = a.third;
    }
    else if (a.second == b.third)
    {
        l.first = a.second;
    }
    else if (a.third == b.first || a.third == b.second || a.third == b.third)
        l.first = a.third;

    /*Make it in order*/
    if (l.first > l.second)
    {
        typename Line::first_type i = l.first;
        l.first = l.second;
        l.second = i;
    }
}

template < class TriangleDecorator, class Vertex, class Line >
struct get_vertex_sharing
{
    typedef std::pair< Vertex, Line > Pair;
    get_vertex_sharing(const TriangleDecorator& _td) : td(_td) {}
    inline Line operator()(const Vertex& u, const Vertex& v) const
    {
        Line l;
        get_sharing(td[u], td[v], l);
        return l;
    }
    inline Line operator()(const Pair& u, const Vertex& v) const
    {
        Line l;
        get_sharing(td[u.first], td[v], l);
        return l;
    }
    inline Line operator()(const Pair& u, const Pair& v) const
    {
        Line l;
        get_sharing(td[u.first], td[v.first], l);
        return l;
    }
    TriangleDecorator td;
};

/* HList has to be a handle of data holder so that pass-by-value is
 * in right logic.
 *
 * The element of HList is a pair of vertex and line. (remember a
 * line is a pair of two ints.). That indicates the walk w from
 * current vertex is across line. (If the first of line is -1, it is
 * a point though.
 */
template < class TriangleDecorator, class HList, class IteratorD >
class SAW_visitor : public bfs_visitor<>, public dfs_visitor<>
{
    typedef typename boost::property_traits< IteratorD >::value_type iter;
    /*use boost shared_ptr*/
    typedef typename HList::element_type::value_type::second_type Line;

public:
    typedef tree_edge_tag category;

    inline SAW_visitor(TriangleDecorator _td, HList _hlist, IteratorD ia)
    : td(_td), hlist(_hlist), iter_d(ia)
    {
    }

    template < class Vertex, class Graph >
    inline void start_vertex(Vertex v, Graph&)
    {
        Line l1;
        l1.first = SAW_SENTINAL;
        l1.second = SAW_SENTINAL;
        hlist->push_front(std::make_pair(v, l1));
        iter_d[v] = hlist->begin();
    }

    /*Several symbols:
      w(i): i-th triangle in walk w
      w(i) |- w(i+1): w enter w(i+1) from w(i) over a line
      w(i) ~> w(i+1): w enter w(i+1) from w(i) over a point
      w(i) -> w(i+1): w enter w(i+1) from w(i)
      w(i) ^ w(i+1): the line or point w go over from w(i) to w(i+1)
    */
    template < class Edge, class Graph > bool tree_edge(Edge e, Graph& G)
    {
        using std::make_pair;
        typedef typename boost::graph_traits< Graph >::vertex_descriptor Vertex;
        Vertex tau = target(e, G);
        Vertex i = source(e, G);

        get_vertex_sharing< TriangleDecorator, Vertex, Line > get_sharing_line(
            td);

        Line tau_i = get_sharing_line(tau, i);

        iter w_end = hlist->end();

        iter w_i = iter_d[i];

        iter w_i_m_1 = w_i;
        iter w_i_p_1 = w_i;

        /*----------------------------------------------------------
         *             true             false
         *==========================================================
         *a       w(i-1) |- w(i)    w(i-1) ~> w(i) or w(i-1) is null
         *----------------------------------------------------------
         *b       w(i) |- w(i+1)    w(i) ~> w(i+1) or no w(i+1) yet
         *----------------------------------------------------------
         */

        bool a = false, b = false;

        --w_i_m_1;
        ++w_i_p_1;
        b = (w_i->second.first != SAW_SENTINAL);

        if (w_i_m_1 != w_end)
        {
            a = (w_i_m_1->second.first != SAW_SENTINAL);
        }

        if (a)
        {

            if (b)
            {
                /*Case 1:

                  w(i-1) |- w(i) |- w(i+1)
                */
                Line l1 = get_sharing_line(*w_i_m_1, tau);

                iter w_i_m_2 = w_i_m_1;
                --w_i_m_2;

                bool c = true;

                if (w_i_m_2 != w_end)
                {
                    c = w_i_m_2->second != l1;
                }

                if (c)
                { /* w(i-1) ^ tau != w(i-2) ^ w(i-1)  */
                    /*extension: w(i-1) -> tau |- w(i) */
                    w_i_m_1->second = l1;
                    /*insert(pos, const T&) is to insert before pos*/
                    iter_d[tau] = hlist->insert(w_i, make_pair(tau, tau_i));
                }
                else
                { /* w(i-1) ^ tau == w(i-2) ^ w(i-1)  */
                    /*must be w(i-2) ~> w(i-1) */

                    bool d = true;
                    // need to handle the case when w_i_p_1 is null
                    Line l3 = get_sharing_line(*w_i_p_1, tau);
                    if (w_i_p_1 != w_end)
                        d = w_i_p_1->second != l3;
                    if (d)
                    { /* w(i+1) ^ tau != w(i+1) ^ w(i+2) */
                        /*extension: w(i) |- tau -> w(i+1) */
                        w_i->second = tau_i;
                        iter_d[tau]
                            = hlist->insert(w_i_p_1, make_pair(tau, l3));
                    }
                    else
                    { /* w(i+1) ^ tau == w(i+1) ^ w(i+2) */
                        /*must be w(1+1) ~> w(i+2) */
                        Line l5 = get_sharing_line(*w_i_m_1, *w_i_p_1);
                        if (l5 != w_i_p_1->second)
                        { /* w(i-1) ^ w(i+1) != w(i+1) ^ w(i+2) */
                            /*extension: w(i-2) -> tau |- w(i) |- w(i-1) ->
                             * w(i+1) */
                            w_i_m_2->second = get_sharing_line(*w_i_m_2, tau);
                            iter_d[tau]
                                = hlist->insert(w_i, make_pair(tau, tau_i));
                            w_i->second = w_i_m_1->second;
                            w_i_m_1->second = l5;
                            iter_d[w_i_m_1->first]
                                = hlist->insert(w_i_p_1, *w_i_m_1);
                            hlist->erase(w_i_m_1);
                        }
                        else
                        {
                            /*mesh is tetrahedral*/
                            // dont know what that means.
                            ;
                        }
                    }
                }
            }
            else
            {
                /*Case 2:

                  w(i-1) |- w(i) ~> w(1+1)
                */

                if (w_i->second.second == tau_i.first
                    || w_i->second.second == tau_i.second)
                { /*w(i) ^ w(i+1) < w(i) ^ tau*/
                    /*extension: w(i) |- tau -> w(i+1) */
                    w_i->second = tau_i;
                    Line l1 = get_sharing_line(*w_i_p_1, tau);
                    iter_d[tau] = hlist->insert(w_i_p_1, make_pair(tau, l1));
                }
                else
                { /*w(i) ^ w(i+1) !< w(i) ^ tau*/
                    Line l1 = get_sharing_line(*w_i_m_1, tau);
                    bool c = true;
                    iter w_i_m_2 = w_i_m_1;
                    --w_i_m_2;
                    if (w_i_m_2 != w_end)
                        c = l1 != w_i_m_2->second;
                    if (c)
                    { /*w(i-1) ^ tau != w(i-2) ^ w(i-1)*/
                        /*extension: w(i-1) -> tau |- w(i)*/
                        w_i_m_1->second = l1;
                        iter_d[tau] = hlist->insert(w_i, make_pair(tau, tau_i));
                    }
                    else
                    { /*w(i-1) ^ tau == w(i-2) ^ w(i-1)*/
                        /*must be w(i-2)~>w(i-1)*/
                        /*extension: w(i-2) -> tau |- w(i) |- w(i-1) -> w(i+1)*/
                        w_i_m_2->second = get_sharing_line(*w_i_m_2, tau);
                        iter_d[tau] = hlist->insert(w_i, make_pair(tau, tau_i));
                        w_i->second = w_i_m_1->second;
                        w_i_m_1->second = get_sharing_line(*w_i_m_1, *w_i_p_1);
                        iter_d[w_i_m_1->first]
                            = hlist->insert(w_i_p_1, *w_i_m_1);
                        hlist->erase(w_i_m_1);
                    }
                }
            }
        }
        else
        {

            if (b)
            {
                /*Case 3:

                  w(i-1) ~> w(i) |- w(i+1)
                */
                bool c = false;
                if (w_i_m_1 != w_end)
                    c = (w_i_m_1->second.second == tau_i.first)
                        || (w_i_m_1->second.second == tau_i.second);

                if (c)
                { /*w(i-1) ^ w(i) < w(i) ^ tau*/
                    /* extension: w(i-1) -> tau |- w(i) */
                    if (w_i_m_1 != w_end)
                        w_i_m_1->second = get_sharing_line(*w_i_m_1, tau);
                    iter_d[tau] = hlist->insert(w_i, make_pair(tau, tau_i));
                }
                else
                {
                    bool d = true;
                    Line l1;
                    l1.first = SAW_SENTINAL;
                    l1.second = SAW_SENTINAL;
                    if (w_i_p_1 != w_end)
                    {
                        l1 = get_sharing_line(*w_i_p_1, tau);
                        d = l1 != w_i_p_1->second;
                    }
                    if (d)
                    { /*w(i+1) ^ tau != w(i+1) ^ w(i+2)*/
                        /*extension: w(i) |- tau -> w(i+1) */
                        w_i->second = tau_i;
                        iter_d[tau]
                            = hlist->insert(w_i_p_1, make_pair(tau, l1));
                    }
                    else
                    {
                        /*must be w(i+1) ~> w(i+2)*/
                        /*extension: w(i-1) -> w(i+1) |- w(i) |- tau -> w(i+2)
                         */
                        iter w_i_p_2 = w_i_p_1;
                        ++w_i_p_2;

                        w_i_p_1->second = w_i->second;
                        iter_d[i] = hlist->insert(w_i_p_2, make_pair(i, tau_i));
                        hlist->erase(w_i);
                        Line l2 = get_sharing_line(*w_i_p_2, tau);
                        iter_d[tau]
                            = hlist->insert(w_i_p_2, make_pair(tau, l2));
                    }
                }
            }
            else
            {
                /*Case 4:

                  w(i-1) ~> w(i) ~> w(i+1)

                */
                bool c = false;
                if (w_i_m_1 != w_end)
                {
                    c = (w_i_m_1->second.second == tau_i.first)
                        || (w_i_m_1->second.second == tau_i.second);
                }
                if (c)
                { /*w(i-1) ^ w(i) < w(i) ^ tau */
                    /*extension: w(i-1) -> tau |- w(i) */
                    if (w_i_m_1 != w_end)
                        w_i_m_1->second = get_sharing_line(*w_i_m_1, tau);
                    iter_d[tau] = hlist->insert(w_i, make_pair(tau, tau_i));
                }
                else
                {
                    /*extension: w(i) |- tau -> w(i+1) */
                    w_i->second = tau_i;
                    Line l1;
                    l1.first = SAW_SENTINAL;
                    l1.second = SAW_SENTINAL;
                    if (w_i_p_1 != w_end)
                        l1 = get_sharing_line(*w_i_p_1, tau);
                    iter_d[tau] = hlist->insert(w_i_p_1, make_pair(tau, l1));
                }
            }
        }

        return true;
    }

protected:
    TriangleDecorator td; /*a decorator for vertex*/
    HList hlist;
    /*This must be a handle of list to record the SAW
      The element type of the list is pair<Vertex, Line>
     */

    IteratorD iter_d;
    /*Problem statement: Need a fast access to w for triangle i.
     *Possible solution: mantain an array to record.
     iter_d[i] will return an iterator
     which points to w(i), where i is a vertex
     representing triangle i.
    */
};

template < class Triangle, class HList, class Iterator >
inline SAW_visitor< Triangle, HList, Iterator > visit_SAW(
    Triangle t, HList hl, Iterator i)
{
    return SAW_visitor< Triangle, HList, Iterator >(t, hl, i);
}

template < class Tri, class HList, class Iter >
inline SAW_visitor< random_access_iterator_property_map< Tri*, Tri, Tri& >,
    HList, random_access_iterator_property_map< Iter*, Iter, Iter& > >
visit_SAW_ptr(Tri* t, HList hl, Iter* i)
{
    typedef random_access_iterator_property_map< Tri*, Tri, Tri& > TriD;
    typedef random_access_iterator_property_map< Iter*, Iter, Iter& > IterD;
    return SAW_visitor< TriD, HList, IterD >(t, hl, i);
}

// should also have combo's of pointers, and also const :(

}

#endif /*BOOST_SAW_H*/
