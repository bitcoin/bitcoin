//  Copyright (c) 2006, Stephan Diederich
//
//  This code may be used under either of the following two licences:
//
//    Permission is hereby granted, free of charge, to any person
//    obtaining a copy of this software and associated documentation
//    files (the "Software"), to deal in the Software without
//    restriction, including without limitation the rights to use,
//    copy, modify, merge, publish, distribute, sublicense, and/or
//    sell copies of the Software, and to permit persons to whom the
//    Software is furnished to do so, subject to the following
//    conditions:
//
//    The above copyright notice and this permission notice shall be
//    included in all copies or substantial portions of the Software.
//
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//    OTHER DEALINGS IN THE SOFTWARE. OF SUCH DAMAGE.
//
//  Or:
//
//    Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//    http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_BOYKOV_KOLMOGOROV_MAX_FLOW_HPP
#define BOOST_BOYKOV_KOLMOGOROV_MAX_FLOW_HPP

#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <vector>
#include <list>
#include <utility>
#include <iosfwd>
#include <algorithm> // for std::min and std::max

#include <boost/pending/queue.hpp>
#include <boost/limits.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/none_t.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/lookup_edge.hpp>
#include <boost/concept/assert.hpp>

// The algorithm impelemented here is described in:
//
// Boykov, Y., Kolmogorov, V. "An Experimental Comparison of Min-Cut/Max-Flow
// Algorithms for Energy Minimization in Vision", In IEEE Transactions on
// Pattern Analysis and Machine Intelligence, vol. 26, no. 9, pp. 1124-1137,
// Sep 2004.
//
// For further reading, also see:
//
// Kolmogorov, V. "Graph Based Algorithms for Scene Reconstruction from Two or
// More Views". PhD thesis, Cornell University, Sep 2003.

namespace boost
{

namespace detail
{

    template < class Graph, class EdgeCapacityMap,
        class ResidualCapacityEdgeMap, class ReverseEdgeMap,
        class PredecessorMap, class ColorMap, class DistanceMap,
        class IndexMap >
    class bk_max_flow
    {
        typedef
            typename property_traits< EdgeCapacityMap >::value_type tEdgeVal;
        typedef graph_traits< Graph > tGraphTraits;
        typedef typename tGraphTraits::vertex_iterator vertex_iterator;
        typedef typename tGraphTraits::vertex_descriptor vertex_descriptor;
        typedef typename tGraphTraits::edge_descriptor edge_descriptor;
        typedef typename tGraphTraits::edge_iterator edge_iterator;
        typedef typename tGraphTraits::out_edge_iterator out_edge_iterator;
        typedef boost::queue< vertex_descriptor >
            tQueue; // queue of vertices, used in adoption-stage
        typedef typename property_traits< ColorMap >::value_type tColorValue;
        typedef color_traits< tColorValue > tColorTraits;
        typedef
            typename property_traits< DistanceMap >::value_type tDistanceVal;

    public:
        bk_max_flow(Graph& g, EdgeCapacityMap cap, ResidualCapacityEdgeMap res,
            ReverseEdgeMap rev, PredecessorMap pre, ColorMap color,
            DistanceMap dist, IndexMap idx, vertex_descriptor src,
            vertex_descriptor sink)
        : m_g(g)
        , m_index_map(idx)
        , m_cap_map(cap)
        , m_res_cap_map(res)
        , m_rev_edge_map(rev)
        , m_pre_map(pre)
        , m_tree_map(color)
        , m_dist_map(dist)
        , m_source(src)
        , m_sink(sink)
        , m_active_nodes()
        , m_in_active_list_vec(num_vertices(g), false)
        , m_in_active_list_map(make_iterator_property_map(
              m_in_active_list_vec.begin(), m_index_map))
        , m_has_parent_vec(num_vertices(g), false)
        , m_has_parent_map(
              make_iterator_property_map(m_has_parent_vec.begin(), m_index_map))
        , m_time_vec(num_vertices(g), 0)
        , m_time_map(
              make_iterator_property_map(m_time_vec.begin(), m_index_map))
        , m_flow(0)
        , m_time(1)
        , m_last_grow_vertex(graph_traits< Graph >::null_vertex())
        {
            // initialize the color-map with gray-values
            vertex_iterator vi, v_end;
            for (boost::tie(vi, v_end) = vertices(m_g); vi != v_end; ++vi)
            {
                set_tree(*vi, tColorTraits::gray());
            }
            // Initialize flow to zero which means initializing
            // the residual capacity equal to the capacity
            edge_iterator ei, e_end;
            for (boost::tie(ei, e_end) = edges(m_g); ei != e_end; ++ei)
            {
                put(m_res_cap_map, *ei, get(m_cap_map, *ei));
                BOOST_ASSERT(get(m_rev_edge_map, get(m_rev_edge_map, *ei))
                    == *ei); // check if the reverse edge map is build up
                             // properly
            }
            // init the search trees with the two terminals
            set_tree(m_source, tColorTraits::black());
            set_tree(m_sink, tColorTraits::white());
            put(m_time_map, m_source, 1);
            put(m_time_map, m_sink, 1);
        }

        tEdgeVal max_flow()
        {
            // augment direct paths from SOURCE->SINK and SOURCE->VERTEX->SINK
            augment_direct_paths();
            // start the main-loop
            while (true)
            {
                bool path_found;
                edge_descriptor connecting_edge;
                boost::tie(connecting_edge, path_found)
                    = grow(); // find a path from source to sink
                if (!path_found)
                {
                    // we're finished, no more paths were found
                    break;
                }
                ++m_time;
                augment(connecting_edge); // augment that path
                adopt(); // rebuild search tree structure
            }
            return m_flow;
        }

        // the complete class is protected, as we want access to members in
        // derived test-class (see test/boykov_kolmogorov_max_flow_test.cpp)
    protected:
        void augment_direct_paths()
        {
            // in a first step, we augment all direct paths from
            // source->NODE->sink and additionally paths from source->sink. This
            // improves especially graphcuts for segmentation, as most of the
            // nodes have source/sink connects but shouldn't have an impact on
            // other maxflow problems (this is done in grow() anyway)
            out_edge_iterator ei, e_end;
            for (boost::tie(ei, e_end) = out_edges(m_source, m_g); ei != e_end;
                 ++ei)
            {
                edge_descriptor from_source = *ei;
                vertex_descriptor current_node = target(from_source, m_g);
                if (current_node == m_sink)
                {
                    tEdgeVal cap = get(m_res_cap_map, from_source);
                    put(m_res_cap_map, from_source, 0);
                    m_flow += cap;
                    continue;
                }
                edge_descriptor to_sink;
                bool is_there;
                boost::tie(to_sink, is_there)
                    = lookup_edge(current_node, m_sink, m_g);
                if (is_there)
                {
                    tEdgeVal cap_from_source = get(m_res_cap_map, from_source);
                    tEdgeVal cap_to_sink = get(m_res_cap_map, to_sink);
                    if (cap_from_source > cap_to_sink)
                    {
                        set_tree(current_node, tColorTraits::black());
                        add_active_node(current_node);
                        set_edge_to_parent(current_node, from_source);
                        put(m_dist_map, current_node, 1);
                        put(m_time_map, current_node, 1);
                        // add stuff to flow and update residuals. we dont need
                        // to update reverse_edges, as incoming/outgoing edges
                        // to/from source/sink don't count for max-flow
                        put(m_res_cap_map, from_source,
                            get(m_res_cap_map, from_source) - cap_to_sink);
                        put(m_res_cap_map, to_sink, 0);
                        m_flow += cap_to_sink;
                    }
                    else if (cap_to_sink > 0)
                    {
                        set_tree(current_node, tColorTraits::white());
                        add_active_node(current_node);
                        set_edge_to_parent(current_node, to_sink);
                        put(m_dist_map, current_node, 1);
                        put(m_time_map, current_node, 1);
                        // add stuff to flow and update residuals. we dont need
                        // to update reverse_edges, as incoming/outgoing edges
                        // to/from source/sink don't count for max-flow
                        put(m_res_cap_map, to_sink,
                            get(m_res_cap_map, to_sink) - cap_from_source);
                        put(m_res_cap_map, from_source, 0);
                        m_flow += cap_from_source;
                    }
                }
                else if (get(m_res_cap_map, from_source))
                {
                    // there is no sink connect, so we can't augment this path,
                    // but to avoid adding m_source to the active nodes, we just
                    // activate this node and set the approciate things
                    set_tree(current_node, tColorTraits::black());
                    set_edge_to_parent(current_node, from_source);
                    put(m_dist_map, current_node, 1);
                    put(m_time_map, current_node, 1);
                    add_active_node(current_node);
                }
            }
            for (boost::tie(ei, e_end) = out_edges(m_sink, m_g); ei != e_end;
                 ++ei)
            {
                edge_descriptor to_sink = get(m_rev_edge_map, *ei);
                vertex_descriptor current_node = source(to_sink, m_g);
                if (get(m_res_cap_map, to_sink))
                {
                    set_tree(current_node, tColorTraits::white());
                    set_edge_to_parent(current_node, to_sink);
                    put(m_dist_map, current_node, 1);
                    put(m_time_map, current_node, 1);
                    add_active_node(current_node);
                }
            }
        }

        /**
         * Returns a pair of an edge and a boolean. if the bool is true, the
         * edge is a connection of a found path from s->t , read "the link" and
         * source(returnVal, m_g) is the end of the path found in the
         * source-tree target(returnVal, m_g) is the beginning of the path found
         * in the sink-tree
         */
        std::pair< edge_descriptor, bool > grow()
        {
            BOOST_ASSERT(m_orphans.empty());
            vertex_descriptor current_node;
            while ((current_node = get_next_active_node())
                != graph_traits< Graph >::null_vertex())
            { // if there is one
                BOOST_ASSERT(get_tree(current_node) != tColorTraits::gray()
                    && (has_parent(current_node) || current_node == m_source
                        || current_node == m_sink));

                if (get_tree(current_node) == tColorTraits::black())
                {
                    // source tree growing
                    out_edge_iterator ei, e_end;
                    if (current_node != m_last_grow_vertex)
                    {
                        m_last_grow_vertex = current_node;
                        boost::tie(m_last_grow_edge_it, m_last_grow_edge_end)
                            = out_edges(current_node, m_g);
                    }
                    for (; m_last_grow_edge_it != m_last_grow_edge_end;
                         ++m_last_grow_edge_it)
                    {
                        edge_descriptor out_edge = *m_last_grow_edge_it;
                        if (get(m_res_cap_map, out_edge) > 0)
                        { // check if we have capacity left on this edge
                            vertex_descriptor other_node
                                = target(out_edge, m_g);
                            if (get_tree(other_node) == tColorTraits::gray())
                            { // it's a free node
                                set_tree(other_node,
                                    tColorTraits::black()); // aquire other node
                                                            // to our search
                                                            // tree
                                set_edge_to_parent(
                                    other_node, out_edge); // set us as parent
                                put(m_dist_map, other_node,
                                    get(m_dist_map, current_node)
                                        + 1); // and update the
                                              // distance-heuristic
                                put(m_time_map, other_node,
                                    get(m_time_map, current_node));
                                add_active_node(other_node);
                            }
                            else if (get_tree(other_node)
                                == tColorTraits::black())
                            {
                                // we do this to get shorter paths. check if we
                                // are nearer to the source as its parent is
                                if (is_closer_to_terminal(
                                        current_node, other_node))
                                {
                                    set_edge_to_parent(other_node, out_edge);
                                    put(m_dist_map, other_node,
                                        get(m_dist_map, current_node) + 1);
                                    put(m_time_map, other_node,
                                        get(m_time_map, current_node));
                                }
                            }
                            else
                            {
                                BOOST_ASSERT(get_tree(other_node)
                                    == tColorTraits::white());
                                // kewl, found a path from one to the other
                                // search tree, return
                                // the connecting edge in src->sink dir
                                return std::make_pair(out_edge, true);
                            }
                        }
                    } // for all out-edges
                } // source-tree-growing
                else
                {
                    BOOST_ASSERT(
                        get_tree(current_node) == tColorTraits::white());
                    out_edge_iterator ei, e_end;
                    if (current_node != m_last_grow_vertex)
                    {
                        m_last_grow_vertex = current_node;
                        boost::tie(m_last_grow_edge_it, m_last_grow_edge_end)
                            = out_edges(current_node, m_g);
                    }
                    for (; m_last_grow_edge_it != m_last_grow_edge_end;
                         ++m_last_grow_edge_it)
                    {
                        edge_descriptor in_edge
                            = get(m_rev_edge_map, *m_last_grow_edge_it);
                        if (get(m_res_cap_map, in_edge) > 0)
                        { // check if there is capacity left
                            vertex_descriptor other_node = source(in_edge, m_g);
                            if (get_tree(other_node) == tColorTraits::gray())
                            { // it's a free node
                                set_tree(other_node,
                                    tColorTraits::white()); // aquire that node
                                                            // to our search
                                                            // tree
                                set_edge_to_parent(
                                    other_node, in_edge); // set us as parent
                                add_active_node(
                                    other_node); // activate that node
                                put(m_dist_map, other_node,
                                    get(m_dist_map, current_node)
                                        + 1); // set its distance
                                put(m_time_map, other_node,
                                    get(m_time_map, current_node)); // and time
                            }
                            else if (get_tree(other_node)
                                == tColorTraits::white())
                            {
                                if (is_closer_to_terminal(
                                        current_node, other_node))
                                {
                                    // we are closer to the sink than its parent
                                    // is, so we "adopt" him
                                    set_edge_to_parent(other_node, in_edge);
                                    put(m_dist_map, other_node,
                                        get(m_dist_map, current_node) + 1);
                                    put(m_time_map, other_node,
                                        get(m_time_map, current_node));
                                }
                            }
                            else
                            {
                                BOOST_ASSERT(get_tree(other_node)
                                    == tColorTraits::black());
                                // kewl, found a path from one to the other
                                // search tree,
                                // return the connecting edge in src->sink dir
                                return std::make_pair(in_edge, true);
                            }
                        }
                    } // for all out-edges
                } // sink-tree growing

                // all edges of that node are processed, and no more paths were
                // found.
                // remove if from the front of the active queue
                finish_node(current_node);
            } // while active_nodes not empty

            // no active nodes anymore and no path found, we're done
            return std::make_pair(edge_descriptor(), false);
        }

        /**
         * augments path from s->t and updates residual graph
         * source(e, m_g) is the end of the path found in the source-tree
         * target(e, m_g) is the beginning of the path found in the sink-tree
         * this phase generates orphans on satured edges, if the attached verts
         * are from different search-trees orphans are ordered in distance to
         * sink/source. first the farest from the source are front_inserted into
         * the orphans list, and after that the sink-tree-orphans are
         * front_inserted. when going to adoption stage the orphans are
         * popped_front, and so we process the nearest verts to the terminals
         * first
         */
        void augment(edge_descriptor e)
        {
            BOOST_ASSERT(get_tree(target(e, m_g)) == tColorTraits::white());
            BOOST_ASSERT(get_tree(source(e, m_g)) == tColorTraits::black());
            BOOST_ASSERT(m_orphans.empty());

            const tEdgeVal bottleneck = find_bottleneck(e);
            // now we push the found flow through the path
            // for each edge we saturate we have to look for the verts that
            // belong to that edge, one of them becomes an orphans now process
            // the connecting edge
            put(m_res_cap_map, e, get(m_res_cap_map, e) - bottleneck);
            BOOST_ASSERT(get(m_res_cap_map, e) >= 0);
            put(m_res_cap_map, get(m_rev_edge_map, e),
                get(m_res_cap_map, get(m_rev_edge_map, e)) + bottleneck);

            // now we follow the path back to the source
            vertex_descriptor current_node = source(e, m_g);
            while (current_node != m_source)
            {
                edge_descriptor pred = get_edge_to_parent(current_node);
                put(m_res_cap_map, pred, get(m_res_cap_map, pred) - bottleneck);
                BOOST_ASSERT(get(m_res_cap_map, pred) >= 0);
                put(m_res_cap_map, get(m_rev_edge_map, pred),
                    get(m_res_cap_map, get(m_rev_edge_map, pred)) + bottleneck);
                if (get(m_res_cap_map, pred) == 0)
                {
                    set_no_parent(current_node);
                    m_orphans.push_front(current_node);
                }
                current_node = source(pred, m_g);
            }
            // then go forward in the sink-tree
            current_node = target(e, m_g);
            while (current_node != m_sink)
            {
                edge_descriptor pred = get_edge_to_parent(current_node);
                put(m_res_cap_map, pred, get(m_res_cap_map, pred) - bottleneck);
                BOOST_ASSERT(get(m_res_cap_map, pred) >= 0);
                put(m_res_cap_map, get(m_rev_edge_map, pred),
                    get(m_res_cap_map, get(m_rev_edge_map, pred)) + bottleneck);
                if (get(m_res_cap_map, pred) == 0)
                {
                    set_no_parent(current_node);
                    m_orphans.push_front(current_node);
                }
                current_node = target(pred, m_g);
            }
            // and add it to the max-flow
            m_flow += bottleneck;
        }

        /**
         * returns the bottleneck of a s->t path (end_of_path is last vertex in
         * source-tree, begin_of_path is first vertex in sink-tree)
         */
        inline tEdgeVal find_bottleneck(edge_descriptor e)
        {
            BOOST_USING_STD_MIN();
            tEdgeVal minimum_cap = get(m_res_cap_map, e);
            vertex_descriptor current_node = source(e, m_g);
            // first go back in the source tree
            while (current_node != m_source)
            {
                edge_descriptor pred = get_edge_to_parent(current_node);
                minimum_cap = min BOOST_PREVENT_MACRO_SUBSTITUTION(
                    minimum_cap, get(m_res_cap_map, pred));
                current_node = source(pred, m_g);
            }
            // then go forward in the sink-tree
            current_node = target(e, m_g);
            while (current_node != m_sink)
            {
                edge_descriptor pred = get_edge_to_parent(current_node);
                minimum_cap = min BOOST_PREVENT_MACRO_SUBSTITUTION(
                    minimum_cap, get(m_res_cap_map, pred));
                current_node = target(pred, m_g);
            }
            return minimum_cap;
        }

        /**
         * rebuild search trees
         * empty the queue of orphans, and find new parents for them or just
         * drop them from the search trees
         */
        void adopt()
        {
            while (!m_orphans.empty() || !m_child_orphans.empty())
            {
                vertex_descriptor current_node;
                if (m_child_orphans.empty())
                {
                    // get the next orphan from the main-queue  and remove it
                    current_node = m_orphans.front();
                    m_orphans.pop_front();
                }
                else
                {
                    current_node = m_child_orphans.front();
                    m_child_orphans.pop();
                }
                if (get_tree(current_node) == tColorTraits::black())
                {
                    // we're in the source-tree
                    tDistanceVal min_distance
                        = (std::numeric_limits< tDistanceVal >::max)();
                    edge_descriptor new_parent_edge;
                    out_edge_iterator ei, e_end;
                    for (boost::tie(ei, e_end) = out_edges(current_node, m_g);
                         ei != e_end; ++ei)
                    {
                        const edge_descriptor in_edge
                            = get(m_rev_edge_map, *ei);
                        BOOST_ASSERT(target(in_edge, m_g)
                            == current_node); // we should be the target of this
                                              // edge
                        if (get(m_res_cap_map, in_edge) > 0)
                        {
                            vertex_descriptor other_node = source(in_edge, m_g);
                            if (get_tree(other_node) == tColorTraits::black()
                                && has_source_connect(other_node))
                            {
                                if (get(m_dist_map, other_node) < min_distance)
                                {
                                    min_distance = get(m_dist_map, other_node);
                                    new_parent_edge = in_edge;
                                }
                            }
                        }
                    }
                    if (min_distance
                        != (std::numeric_limits< tDistanceVal >::max)())
                    {
                        set_edge_to_parent(current_node, new_parent_edge);
                        put(m_dist_map, current_node, min_distance + 1);
                        put(m_time_map, current_node, m_time);
                    }
                    else
                    {
                        put(m_time_map, current_node, 0);
                        for (boost::tie(ei, e_end)
                             = out_edges(current_node, m_g);
                             ei != e_end; ++ei)
                        {
                            edge_descriptor in_edge = get(m_rev_edge_map, *ei);
                            vertex_descriptor other_node = source(in_edge, m_g);
                            if (get_tree(other_node) == tColorTraits::black()
                                && other_node != m_source)
                            {
                                if (get(m_res_cap_map, in_edge) > 0)
                                {
                                    add_active_node(other_node);
                                }
                                if (has_parent(other_node)
                                    && source(
                                           get_edge_to_parent(other_node), m_g)
                                        == current_node)
                                {
                                    // we are the parent of that node
                                    // it has to find a new parent, too
                                    set_no_parent(other_node);
                                    m_child_orphans.push(other_node);
                                }
                            }
                        }
                        set_tree(current_node, tColorTraits::gray());
                    } // no parent found
                } // source-tree-adoption
                else
                {
                    // now we should be in the sink-tree, check that...
                    BOOST_ASSERT(
                        get_tree(current_node) == tColorTraits::white());
                    out_edge_iterator ei, e_end;
                    edge_descriptor new_parent_edge;
                    tDistanceVal min_distance
                        = (std::numeric_limits< tDistanceVal >::max)();
                    for (boost::tie(ei, e_end) = out_edges(current_node, m_g);
                         ei != e_end; ++ei)
                    {
                        const edge_descriptor out_edge = *ei;
                        if (get(m_res_cap_map, out_edge) > 0)
                        {
                            const vertex_descriptor other_node
                                = target(out_edge, m_g);
                            if (get_tree(other_node) == tColorTraits::white()
                                && has_sink_connect(other_node))
                                if (get(m_dist_map, other_node) < min_distance)
                                {
                                    min_distance = get(m_dist_map, other_node);
                                    new_parent_edge = out_edge;
                                }
                        }
                    }
                    if (min_distance
                        != (std::numeric_limits< tDistanceVal >::max)())
                    {
                        set_edge_to_parent(current_node, new_parent_edge);
                        put(m_dist_map, current_node, min_distance + 1);
                        put(m_time_map, current_node, m_time);
                    }
                    else
                    {
                        put(m_time_map, current_node, 0);
                        for (boost::tie(ei, e_end)
                             = out_edges(current_node, m_g);
                             ei != e_end; ++ei)
                        {
                            const edge_descriptor out_edge = *ei;
                            const vertex_descriptor other_node
                                = target(out_edge, m_g);
                            if (get_tree(other_node) == tColorTraits::white()
                                && other_node != m_sink)
                            {
                                if (get(m_res_cap_map, out_edge) > 0)
                                {
                                    add_active_node(other_node);
                                }
                                if (has_parent(other_node)
                                    && target(
                                           get_edge_to_parent(other_node), m_g)
                                        == current_node)
                                {
                                    // we were it's parent, so it has to find a
                                    // new one, too
                                    set_no_parent(other_node);
                                    m_child_orphans.push(other_node);
                                }
                            }
                        }
                        set_tree(current_node, tColorTraits::gray());
                    } // no parent found
                } // sink-tree adoption
            } // while !orphans.empty()
        } // adopt

        /**
         * return next active vertex if there is one, otherwise a null_vertex
         */
        inline vertex_descriptor get_next_active_node()
        {
            while (true)
            {
                if (m_active_nodes.empty())
                    return graph_traits< Graph >::null_vertex();
                vertex_descriptor v = m_active_nodes.front();

                // if it has no parent, this node can't be active (if its not
                // source or sink)
                if (!has_parent(v) && v != m_source && v != m_sink)
                {
                    m_active_nodes.pop();
                    put(m_in_active_list_map, v, false);
                }
                else
                {
                    BOOST_ASSERT(get_tree(v) == tColorTraits::black()
                        || get_tree(v) == tColorTraits::white());
                    return v;
                }
            }
        }

        /**
         * adds v as an active vertex, but only if its not in the list already
         */
        inline void add_active_node(vertex_descriptor v)
        {
            BOOST_ASSERT(get_tree(v) != tColorTraits::gray());
            if (get(m_in_active_list_map, v))
            {
                if (m_last_grow_vertex == v)
                {
                    m_last_grow_vertex = graph_traits< Graph >::null_vertex();
                }
                return;
            }
            else
            {
                put(m_in_active_list_map, v, true);
                m_active_nodes.push(v);
            }
        }

        /**
         * finish_node removes a node from the front of the active queue (its
         * called in grow phase, if no more paths can be found using this node)
         */
        inline void finish_node(vertex_descriptor v)
        {
            BOOST_ASSERT(m_active_nodes.front() == v);
            m_active_nodes.pop();
            put(m_in_active_list_map, v, false);
            m_last_grow_vertex = graph_traits< Graph >::null_vertex();
        }

        /**
         * removes a vertex from the queue of active nodes (actually this does
         * nothing, but checks if this node has no parent edge, as this is the
         * criteria for being no more active)
         */
        inline void remove_active_node(vertex_descriptor v)
        {
            BOOST_ASSERT(!has_parent(v));
        }

        /**
         * returns the search tree of v; tColorValue::black() for source tree,
         * white() for sink tree, gray() for no tree
         */
        inline tColorValue get_tree(vertex_descriptor v) const
        {
            return get(m_tree_map, v);
        }

        /**
         * sets search tree of v; tColorValue::black() for source tree, white()
         * for sink tree, gray() for no tree
         */
        inline void set_tree(vertex_descriptor v, tColorValue t)
        {
            put(m_tree_map, v, t);
        }

        /**
         * returns edge to parent vertex of v;
         */
        inline edge_descriptor get_edge_to_parent(vertex_descriptor v) const
        {
            return get(m_pre_map, v);
        }

        /**
         * returns true if the edge stored in m_pre_map[v] is a valid entry
         */
        inline bool has_parent(vertex_descriptor v) const
        {
            return get(m_has_parent_map, v);
        }

        /**
         * sets edge to parent vertex of v;
         */
        inline void set_edge_to_parent(
            vertex_descriptor v, edge_descriptor f_edge_to_parent)
        {
            BOOST_ASSERT(get(m_res_cap_map, f_edge_to_parent) > 0);
            put(m_pre_map, v, f_edge_to_parent);
            put(m_has_parent_map, v, true);
        }

        /**
         * removes the edge to parent of v (this is done by invalidating the
         * entry an additional map)
         */
        inline void set_no_parent(vertex_descriptor v)
        {
            put(m_has_parent_map, v, false);
        }

        /**
         * checks if vertex v has a connect to the sink-vertex (@var m_sink)
         * @param v the vertex which is checked
         * @return true if a path to the sink was found, false if not
         */
        inline bool has_sink_connect(vertex_descriptor v)
        {
            tDistanceVal current_distance = 0;
            vertex_descriptor current_vertex = v;
            while (true)
            {
                if (get(m_time_map, current_vertex) == m_time)
                {
                    // we found a node which was already checked this round. use
                    // it for distance calculations
                    current_distance += get(m_dist_map, current_vertex);
                    break;
                }
                if (current_vertex == m_sink)
                {
                    put(m_time_map, m_sink, m_time);
                    break;
                }
                if (has_parent(current_vertex))
                {
                    // it has a parent, so get it
                    current_vertex
                        = target(get_edge_to_parent(current_vertex), m_g);
                    ++current_distance;
                }
                else
                {
                    // no path found
                    return false;
                }
            }
            current_vertex = v;
            while (get(m_time_map, current_vertex) != m_time)
            {
                put(m_dist_map, current_vertex, current_distance);
                --current_distance;
                put(m_time_map, current_vertex, m_time);
                current_vertex
                    = target(get_edge_to_parent(current_vertex), m_g);
            }
            return true;
        }

        /**
         * checks if vertex v has a connect to the source-vertex (@var m_source)
         * @param v the vertex which is checked
         * @return true if a path to the source was found, false if not
         */
        inline bool has_source_connect(vertex_descriptor v)
        {
            tDistanceVal current_distance = 0;
            vertex_descriptor current_vertex = v;
            while (true)
            {
                if (get(m_time_map, current_vertex) == m_time)
                {
                    // we found a node which was already checked this round. use
                    // it for distance calculations
                    current_distance += get(m_dist_map, current_vertex);
                    break;
                }
                if (current_vertex == m_source)
                {
                    put(m_time_map, m_source, m_time);
                    break;
                }
                if (has_parent(current_vertex))
                {
                    // it has a parent, so get it
                    current_vertex
                        = source(get_edge_to_parent(current_vertex), m_g);
                    ++current_distance;
                }
                else
                {
                    // no path found
                    return false;
                }
            }
            current_vertex = v;
            while (get(m_time_map, current_vertex) != m_time)
            {
                put(m_dist_map, current_vertex, current_distance);
                --current_distance;
                put(m_time_map, current_vertex, m_time);
                current_vertex
                    = source(get_edge_to_parent(current_vertex), m_g);
            }
            return true;
        }

        /**
         * returns true, if p is closer to a terminal than q
         */
        inline bool is_closer_to_terminal(
            vertex_descriptor p, vertex_descriptor q)
        {
            // checks the timestamps first, to build no cycles, and after that
            // the real distance
            return (get(m_time_map, q) <= get(m_time_map, p)
                && get(m_dist_map, q) > get(m_dist_map, p) + 1);
        }

        ////////
        // member vars
        ////////
        Graph& m_g;
        IndexMap m_index_map;
        EdgeCapacityMap m_cap_map;
        ResidualCapacityEdgeMap m_res_cap_map;
        ReverseEdgeMap m_rev_edge_map;
        PredecessorMap m_pre_map; // stores paths found in the growth stage
        ColorMap m_tree_map; // maps each vertex into one of the two search tree
                             // or none (gray())
        DistanceMap m_dist_map; // stores distance to source/sink nodes
        vertex_descriptor m_source;
        vertex_descriptor m_sink;

        tQueue m_active_nodes;
        std::vector< bool > m_in_active_list_vec;
        iterator_property_map< std::vector< bool >::iterator, IndexMap >
            m_in_active_list_map;

        std::list< vertex_descriptor > m_orphans;
        tQueue m_child_orphans; // we use a second queuqe for child orphans, as
                                // they are FIFO processed

        std::vector< bool > m_has_parent_vec;
        iterator_property_map< std::vector< bool >::iterator, IndexMap >
            m_has_parent_map;

        std::vector< long > m_time_vec; // timestamp of each node, used for
                                        // sink/source-path calculations
        iterator_property_map< std::vector< long >::iterator, IndexMap >
            m_time_map;
        tEdgeVal m_flow;
        long m_time;
        vertex_descriptor m_last_grow_vertex;
        out_edge_iterator m_last_grow_edge_it;
        out_edge_iterator m_last_grow_edge_end;
    };

} // namespace boost::detail

/**
 * non-named-parameter version, given everything
 * this is the catch all version
 */
template < class Graph, class CapacityEdgeMap, class ResidualCapacityEdgeMap,
    class ReverseEdgeMap, class PredecessorMap, class ColorMap,
    class DistanceMap, class IndexMap >
typename property_traits< CapacityEdgeMap >::value_type
boykov_kolmogorov_max_flow(Graph& g, CapacityEdgeMap cap,
    ResidualCapacityEdgeMap res_cap, ReverseEdgeMap rev_map,
    PredecessorMap pre_map, ColorMap color, DistanceMap dist, IndexMap idx,
    typename graph_traits< Graph >::vertex_descriptor src,
    typename graph_traits< Graph >::vertex_descriptor sink)
{
    typedef typename graph_traits< Graph >::vertex_descriptor vertex_descriptor;
    typedef typename graph_traits< Graph >::edge_descriptor edge_descriptor;

    // as this method is the last one before we instantiate the solver, we do
    // the concept checks here
    BOOST_CONCEPT_ASSERT(
        (VertexListGraphConcept< Graph >)); // to have vertices(),
                                            // num_vertices(),
    BOOST_CONCEPT_ASSERT((EdgeListGraphConcept< Graph >)); // to have edges()
    BOOST_CONCEPT_ASSERT(
        (IncidenceGraphConcept< Graph >)); // to have source(), target() and
                                           // out_edges()
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< CapacityEdgeMap,
        edge_descriptor >)); // read flow-values from edges
    BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept< ResidualCapacityEdgeMap,
        edge_descriptor >)); // write flow-values to residuals
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< ReverseEdgeMap,
        edge_descriptor >)); // read out reverse edges
    BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept< PredecessorMap,
        vertex_descriptor >)); // store predecessor there
    BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept< ColorMap,
        vertex_descriptor >)); // write corresponding tree
    BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept< DistanceMap,
        vertex_descriptor >)); // write distance to source/sink
    BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< IndexMap,
        vertex_descriptor >)); // get index 0...|V|-1
    BOOST_ASSERT(num_vertices(g) >= 2 && src != sink);

    detail::bk_max_flow< Graph, CapacityEdgeMap, ResidualCapacityEdgeMap,
        ReverseEdgeMap, PredecessorMap, ColorMap, DistanceMap, IndexMap >
        algo(g, cap, res_cap, rev_map, pre_map, color, dist, idx, src, sink);

    return algo.max_flow();
}

/**
 * non-named-parameter version, given capacity, residucal_capacity,
 * reverse_edges, and an index map.
 */
template < class Graph, class CapacityEdgeMap, class ResidualCapacityEdgeMap,
    class ReverseEdgeMap, class IndexMap >
typename property_traits< CapacityEdgeMap >::value_type
boykov_kolmogorov_max_flow(Graph& g, CapacityEdgeMap cap,
    ResidualCapacityEdgeMap res_cap, ReverseEdgeMap rev, IndexMap idx,
    typename graph_traits< Graph >::vertex_descriptor src,
    typename graph_traits< Graph >::vertex_descriptor sink)
{
    typename graph_traits< Graph >::vertices_size_type n_verts
        = num_vertices(g);
    std::vector< typename graph_traits< Graph >::edge_descriptor >
        predecessor_vec(n_verts);
    std::vector< default_color_type > color_vec(n_verts);
    std::vector< typename graph_traits< Graph >::vertices_size_type >
        distance_vec(n_verts);
    return boykov_kolmogorov_max_flow(g, cap, res_cap, rev,
        make_iterator_property_map(predecessor_vec.begin(), idx),
        make_iterator_property_map(color_vec.begin(), idx),
        make_iterator_property_map(distance_vec.begin(), idx), idx, src, sink);
}

/**
 * non-named-parameter version, some given: capacity, residual_capacity,
 * reverse_edges, color_map and an index map. Use this if you are interested in
 * the minimum cut, as the color map provides that info.
 */
template < class Graph, class CapacityEdgeMap, class ResidualCapacityEdgeMap,
    class ReverseEdgeMap, class ColorMap, class IndexMap >
typename property_traits< CapacityEdgeMap >::value_type
boykov_kolmogorov_max_flow(Graph& g, CapacityEdgeMap cap,
    ResidualCapacityEdgeMap res_cap, ReverseEdgeMap rev, ColorMap color,
    IndexMap idx, typename graph_traits< Graph >::vertex_descriptor src,
    typename graph_traits< Graph >::vertex_descriptor sink)
{
    typename graph_traits< Graph >::vertices_size_type n_verts
        = num_vertices(g);
    std::vector< typename graph_traits< Graph >::edge_descriptor >
        predecessor_vec(n_verts);
    std::vector< typename graph_traits< Graph >::vertices_size_type >
        distance_vec(n_verts);
    return boykov_kolmogorov_max_flow(g, cap, res_cap, rev,
        make_iterator_property_map(predecessor_vec.begin(), idx), color,
        make_iterator_property_map(distance_vec.begin(), idx), idx, src, sink);
}

/**
 * named-parameter version, some given
 */
template < class Graph, class P, class T, class R >
typename property_traits<
    typename property_map< Graph, edge_capacity_t >::const_type >::value_type
boykov_kolmogorov_max_flow(Graph& g,
    typename graph_traits< Graph >::vertex_descriptor src,
    typename graph_traits< Graph >::vertex_descriptor sink,
    const bgl_named_params< P, T, R >& params)
{
    return boykov_kolmogorov_max_flow(g,
        choose_const_pmap(get_param(params, edge_capacity), g, edge_capacity),
        choose_pmap(get_param(params, edge_residual_capacity), g,
            edge_residual_capacity),
        choose_const_pmap(get_param(params, edge_reverse), g, edge_reverse),
        choose_pmap(
            get_param(params, vertex_predecessor), g, vertex_predecessor),
        choose_pmap(get_param(params, vertex_color), g, vertex_color),
        choose_pmap(get_param(params, vertex_distance), g, vertex_distance),
        choose_const_pmap(get_param(params, vertex_index), g, vertex_index),
        src, sink);
}

/**
 * named-parameter version, none given
 */
template < class Graph >
typename property_traits<
    typename property_map< Graph, edge_capacity_t >::const_type >::value_type
boykov_kolmogorov_max_flow(Graph& g,
    typename graph_traits< Graph >::vertex_descriptor src,
    typename graph_traits< Graph >::vertex_descriptor sink)
{
    bgl_named_params< int, buffer_param_t > params(0); // bogus empty param
    return boykov_kolmogorov_max_flow(g, src, sink, params);
}

} // namespace boost

#endif // BOOST_BOYKOV_KOLMOGOROV_MAX_FLOW_HPP
