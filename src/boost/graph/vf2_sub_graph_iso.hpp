//=======================================================================
// Copyright (C) 2012 Flavio De Lorenzi (fdlorenzi@gmail.com)
// Copyright (C) 2013 Jakob Lykke Andersen, University of Southern Denmark
// (jlandersen@imada.sdu.dk)
//
// The algorithm implemented here is derived from original ideas by
// Pasquale Foggia and colaborators. For further information see
// e.g. Cordella et al. 2001, 2004.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

// Revision History:
//   8 April 2013: Fixed a typo in vf2_print_callback. (Flavio De Lorenzi)

#ifndef BOOST_VF2_SUB_GRAPH_ISO_HPP
#define BOOST_VF2_SUB_GRAPH_ISO_HPP

#include <iostream>
#include <iomanip>
#include <iterator>
#include <vector>
#include <utility>

#include <boost/assert.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/mcgregor_common_subgraphs.hpp> // for always_equivalent
#include <boost/graph/named_function_params.hpp>
#include <boost/type_traits/has_less.hpp>
#include <boost/mpl/int.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/utility/enable_if.hpp>

#ifndef BOOST_GRAPH_ITERATION_MACROS_HPP
#define BOOST_ISO_INCLUDED_ITER_MACROS // local macro, see bottom of file
#include <boost/graph/iteration_macros.hpp>
#endif

namespace boost
{

// Default print_callback
template < typename Graph1, typename Graph2 > struct vf2_print_callback
{

    vf2_print_callback(const Graph1& graph1, const Graph2& graph2)
    : graph1_(graph1), graph2_(graph2)
    {
    }

    template < typename CorrespondenceMap1To2, typename CorrespondenceMap2To1 >
    bool operator()(CorrespondenceMap1To2 f, CorrespondenceMap2To1) const
    {

        // Print (sub)graph isomorphism map
        BGL_FORALL_VERTICES_T(v, graph1_, Graph1)
        std::cout << '(' << get(vertex_index_t(), graph1_, v) << ", "
                  << get(vertex_index_t(), graph2_, get(f, v)) << ") ";

        std::cout << std::endl;

        return true;
    }

private:
    const Graph1& graph1_;
    const Graph2& graph2_;
};

namespace detail
{

    // State associated with a single graph (graph_this)
    template < typename GraphThis, typename GraphOther, typename IndexMapThis,
        typename IndexMapOther >
    class base_state
    {

        typedef typename graph_traits< GraphThis >::vertex_descriptor
            vertex_this_type;
        typedef typename graph_traits< GraphOther >::vertex_descriptor
            vertex_other_type;

        typedef
            typename graph_traits< GraphThis >::vertices_size_type size_type;

        const GraphThis& graph_this_;
        const GraphOther& graph_other_;

        IndexMapThis index_map_this_;
        IndexMapOther index_map_other_;

        std::vector< vertex_other_type > core_vec_;
        typedef iterator_property_map<
            typename std::vector< vertex_other_type >::iterator, IndexMapThis,
            vertex_other_type, vertex_other_type& >
            core_map_type;
        core_map_type core_;

        std::vector< size_type > in_vec_, out_vec_;
        typedef iterator_property_map<
            typename std::vector< size_type >::iterator, IndexMapThis,
            size_type, size_type& >
            in_out_map_type;
        in_out_map_type in_, out_;

        size_type term_in_count_, term_out_count_, term_both_count_,
            core_count_;

        // Forbidden
        base_state(const base_state&);
        base_state& operator=(const base_state&);

    public:
        base_state(const GraphThis& graph_this, const GraphOther& graph_other,
            IndexMapThis index_map_this, IndexMapOther index_map_other)
        : graph_this_(graph_this)
        , graph_other_(graph_other)
        , index_map_this_(index_map_this)
        , index_map_other_(index_map_other)
        , core_vec_(num_vertices(graph_this_),
              graph_traits< GraphOther >::null_vertex())
        , core_(core_vec_.begin(), index_map_this_)
        , in_vec_(num_vertices(graph_this_), 0)
        , out_vec_(num_vertices(graph_this_), 0)
        , in_(in_vec_.begin(), index_map_this_)
        , out_(out_vec_.begin(), index_map_this_)
        , term_in_count_(0)
        , term_out_count_(0)
        , term_both_count_(0)
        , core_count_(0)
        {
        }

        // Adds a vertex pair to the state of graph graph_this
        void push(
            const vertex_this_type& v_this, const vertex_other_type& v_other)
        {

            ++core_count_;

            put(core_, v_this, v_other);

            if (!get(in_, v_this))
            {
                put(in_, v_this, core_count_);
                ++term_in_count_;
                if (get(out_, v_this))
                    ++term_both_count_;
            }

            if (!get(out_, v_this))
            {
                put(out_, v_this, core_count_);
                ++term_out_count_;
                if (get(in_, v_this))
                    ++term_both_count_;
            }

            BGL_FORALL_INEDGES_T(v_this, e, graph_this_, GraphThis)
            {
                vertex_this_type w = source(e, graph_this_);
                if (!get(in_, w))
                {
                    put(in_, w, core_count_);
                    ++term_in_count_;
                    if (get(out_, w))
                        ++term_both_count_;
                }
            }

            BGL_FORALL_OUTEDGES_T(v_this, e, graph_this_, GraphThis)
            {
                vertex_this_type w = target(e, graph_this_);
                if (!get(out_, w))
                {
                    put(out_, w, core_count_);
                    ++term_out_count_;
                    if (get(in_, w))
                        ++term_both_count_;
                }
            }
        }

        // Removes vertex pair from state of graph_this
        void pop(const vertex_this_type& v_this, const vertex_other_type&)
        {

            if (!core_count_)
                return;

            if (get(in_, v_this) == core_count_)
            {
                put(in_, v_this, 0);
                --term_in_count_;
                if (get(out_, v_this))
                    --term_both_count_;
            }

            BGL_FORALL_INEDGES_T(v_this, e, graph_this_, GraphThis)
            {
                vertex_this_type w = source(e, graph_this_);
                if (get(in_, w) == core_count_)
                {
                    put(in_, w, 0);
                    --term_in_count_;
                    if (get(out_, w))
                        --term_both_count_;
                }
            }

            if (get(out_, v_this) == core_count_)
            {
                put(out_, v_this, 0);
                --term_out_count_;
                if (get(in_, v_this))
                    --term_both_count_;
            }

            BGL_FORALL_OUTEDGES_T(v_this, e, graph_this_, GraphThis)
            {
                vertex_this_type w = target(e, graph_this_);
                if (get(out_, w) == core_count_)
                {
                    put(out_, w, 0);
                    --term_out_count_;
                    if (get(in_, w))
                        --term_both_count_;
                }
            }
            put(core_, v_this, graph_traits< GraphOther >::null_vertex());

            --core_count_;
        }

        // Returns true if the in-terminal set is not empty
        bool term_in() const { return core_count_ < term_in_count_; }

        // Returns true if vertex belongs to the in-terminal set
        bool term_in(const vertex_this_type& v) const
        {
            return (get(in_, v) > 0)
                && (get(core_, v) == graph_traits< GraphOther >::null_vertex());
        }

        // Returns true if the out-terminal set is not empty
        bool term_out() const { return core_count_ < term_out_count_; }

        // Returns true if vertex belongs to the out-terminal set
        bool term_out(const vertex_this_type& v) const
        {
            return (get(out_, v) > 0)
                && (get(core_, v) == graph_traits< GraphOther >::null_vertex());
        }

        // Returns true of both (in- and out-terminal) sets are not empty
        bool term_both() const { return core_count_ < term_both_count_; }

        // Returns true if vertex belongs to both (in- and out-terminal) sets
        bool term_both(const vertex_this_type& v) const
        {
            return (get(in_, v) > 0) && (get(out_, v) > 0)
                && (get(core_, v) == graph_traits< GraphOther >::null_vertex());
        }

        // Returns true if vertex belongs to the core map, i.e. it is in the
        // present mapping
        bool in_core(const vertex_this_type& v) const
        {
            return get(core_, v) != graph_traits< GraphOther >::null_vertex();
        }

        // Returns the number of vertices in the mapping
        size_type count() const { return core_count_; }

        // Returns the image (in graph_other) of vertex v (in graph_this)
        vertex_other_type core(const vertex_this_type& v) const
        {
            return get(core_, v);
        }

        // Returns the mapping
        core_map_type get_map() const { return core_; }

        // Returns the "time" (or depth) when vertex was added to the
        // in-terminal set
        size_type in_depth(const vertex_this_type& v) const
        {
            return get(in_, v);
        }

        // Returns the "time" (or depth) when vertex was added to the
        // out-terminal set
        size_type out_depth(const vertex_this_type& v) const
        {
            return get(out_, v);
        }

        // Returns the terminal set counts
        boost::tuple< size_type, size_type, size_type > term_set() const
        {
            return boost::make_tuple(
                term_in_count_, term_out_count_, term_both_count_);
        }
    };

    // Function object that checks whether a valid edge
    // exists. For multi-graphs matched edges are excluded
    template < typename Graph, typename Enable = void >
    struct equivalent_edge_exists
    {
        typedef
            typename boost::graph_traits< Graph >::edge_descriptor edge_type;

        BOOST_CONCEPT_ASSERT((LessThanComparable< edge_type >));

        template < typename EdgePredicate >
        bool operator()(typename graph_traits< Graph >::vertex_descriptor s,
            typename graph_traits< Graph >::vertex_descriptor t,
            EdgePredicate is_valid_edge, const Graph& g)
        {

            BGL_FORALL_OUTEDGES_T(s, e, g, Graph)
            {
                if ((target(e, g) == t) && is_valid_edge(e)
                    && (matched_edges_.find(e) == matched_edges_.end()))
                {
                    matched_edges_.insert(e);
                    return true;
                }
            }

            return false;
        }

    private:
        std::set< edge_type > matched_edges_;
    };

    template < typename Graph >
    struct equivalent_edge_exists< Graph,
        typename boost::disable_if< is_multigraph< Graph > >::type >
    {
        template < typename EdgePredicate >
        bool operator()(typename graph_traits< Graph >::vertex_descriptor s,
            typename graph_traits< Graph >::vertex_descriptor t,
            EdgePredicate is_valid_edge, const Graph& g)
        {

            typename graph_traits< Graph >::edge_descriptor e;
            bool found;
            boost::tie(e, found) = edge(s, t, g);
            if (!found)
                return false;
            else if (is_valid_edge(e))
                return true;

            return false;
        }
    };

    // Generates a predicate for edge e1 given  a binary predicate and a
    // fixed edge e2
    template < typename Graph1, typename Graph2,
        typename EdgeEquivalencePredicate >
    struct edge1_predicate
    {

        edge1_predicate(EdgeEquivalencePredicate edge_comp,
            typename graph_traits< Graph2 >::edge_descriptor e2)
        : edge_comp_(edge_comp), e2_(e2)
        {
        }

        bool operator()(typename graph_traits< Graph1 >::edge_descriptor e1)
        {
            return edge_comp_(e1, e2_);
        }

        EdgeEquivalencePredicate edge_comp_;
        typename graph_traits< Graph2 >::edge_descriptor e2_;
    };

    // Generates a predicate for edge e2 given given a binary predicate and a
    // fixed edge e1
    template < typename Graph1, typename Graph2,
        typename EdgeEquivalencePredicate >
    struct edge2_predicate
    {

        edge2_predicate(EdgeEquivalencePredicate edge_comp,
            typename graph_traits< Graph1 >::edge_descriptor e1)
        : edge_comp_(edge_comp), e1_(e1)
        {
        }

        bool operator()(typename graph_traits< Graph2 >::edge_descriptor e2)
        {
            return edge_comp_(e1_, e2);
        }

        EdgeEquivalencePredicate edge_comp_;
        typename graph_traits< Graph1 >::edge_descriptor e1_;
    };

    enum problem_selector
    {
        subgraph_mono,
        subgraph_iso,
        isomorphism
    };

    // The actual state associated with both graphs
    template < typename Graph1, typename Graph2, typename IndexMap1,
        typename IndexMap2, typename EdgeEquivalencePredicate,
        typename VertexEquivalencePredicate, typename SubGraphIsoMapCallback,
        problem_selector problem_selection >
    class state
    {

        typedef typename graph_traits< Graph1 >::vertex_descriptor vertex1_type;
        typedef typename graph_traits< Graph2 >::vertex_descriptor vertex2_type;

        typedef typename graph_traits< Graph1 >::edge_descriptor edge1_type;
        typedef typename graph_traits< Graph2 >::edge_descriptor edge2_type;

        typedef typename graph_traits< Graph1 >::vertices_size_type
            graph1_size_type;
        typedef typename graph_traits< Graph2 >::vertices_size_type
            graph2_size_type;

        const Graph1& graph1_;
        const Graph2& graph2_;

        IndexMap1 index_map1_;

        EdgeEquivalencePredicate edge_comp_;
        VertexEquivalencePredicate vertex_comp_;

        base_state< Graph1, Graph2, IndexMap1, IndexMap2 > state1_;
        base_state< Graph2, Graph1, IndexMap2, IndexMap1 > state2_;

        // Three helper functions used in Feasibility and Valid functions to
        // test terminal set counts when testing for:
        // - graph sub-graph monomorphism, or
        inline bool comp_term_sets(graph1_size_type a, graph2_size_type b,
            boost::mpl::int_< subgraph_mono >) const
        {
            return a <= b;
        }

        // - graph sub-graph isomorphism, or
        inline bool comp_term_sets(graph1_size_type a, graph2_size_type b,
            boost::mpl::int_< subgraph_iso >) const
        {
            return a <= b;
        }

        // - graph isomorphism
        inline bool comp_term_sets(graph1_size_type a, graph2_size_type b,
            boost::mpl::int_< isomorphism >) const
        {
            return a == b;
        }

        // Forbidden
        state(const state&);
        state& operator=(const state&);

    public:
        state(const Graph1& graph1, const Graph2& graph2, IndexMap1 index_map1,
            IndexMap2 index_map2, EdgeEquivalencePredicate edge_comp,
            VertexEquivalencePredicate vertex_comp)
        : graph1_(graph1)
        , graph2_(graph2)
        , index_map1_(index_map1)
        , edge_comp_(edge_comp)
        , vertex_comp_(vertex_comp)
        , state1_(graph1, graph2, index_map1, index_map2)
        , state2_(graph2, graph1, index_map2, index_map1)
        {
        }

        // Add vertex pair to the state
        void push(const vertex1_type& v, const vertex2_type& w)
        {
            state1_.push(v, w);
            state2_.push(w, v);
        }

        // Remove vertex pair from state
        void pop(const vertex1_type& v, const vertex2_type&)
        {
            vertex2_type w = state1_.core(v);
            state1_.pop(v, w);
            state2_.pop(w, v);
        }

        // Checks the feasibility of a new vertex pair
        bool feasible(const vertex1_type& v_new, const vertex2_type& w_new)
        {

            if (!vertex_comp_(v_new, w_new))
                return false;

            // graph1
            graph1_size_type term_in1_count = 0, term_out1_count = 0,
                             rest1_count = 0;

            {
                equivalent_edge_exists< Graph2 > edge2_exists;

                BGL_FORALL_INEDGES_T(v_new, e1, graph1_, Graph1)
                {
                    vertex1_type v = source(e1, graph1_);

                    if (state1_.in_core(v) || (v == v_new))
                    {
                        vertex2_type w = w_new;
                        if (v != v_new)
                            w = state1_.core(v);
                        if (!edge2_exists(w, w_new,
                                edge2_predicate< Graph1, Graph2,
                                    EdgeEquivalencePredicate >(edge_comp_, e1),
                                graph2_))
                            return false;
                    }
                    else
                    {
                        if (0 < state1_.in_depth(v))
                            ++term_in1_count;
                        if (0 < state1_.out_depth(v))
                            ++term_out1_count;
                        if ((state1_.in_depth(v) == 0)
                            && (state1_.out_depth(v) == 0))
                            ++rest1_count;
                    }
                }
            }

            {
                equivalent_edge_exists< Graph2 > edge2_exists;

                BGL_FORALL_OUTEDGES_T(v_new, e1, graph1_, Graph1)
                {
                    vertex1_type v = target(e1, graph1_);
                    if (state1_.in_core(v) || (v == v_new))
                    {
                        vertex2_type w = w_new;
                        if (v != v_new)
                            w = state1_.core(v);

                        if (!edge2_exists(w_new, w,
                                edge2_predicate< Graph1, Graph2,
                                    EdgeEquivalencePredicate >(edge_comp_, e1),
                                graph2_))
                            return false;
                    }
                    else
                    {
                        if (0 < state1_.in_depth(v))
                            ++term_in1_count;
                        if (0 < state1_.out_depth(v))
                            ++term_out1_count;
                        if ((state1_.in_depth(v) == 0)
                            && (state1_.out_depth(v) == 0))
                            ++rest1_count;
                    }
                }
            }

            // graph2
            graph2_size_type term_out2_count = 0, term_in2_count = 0,
                             rest2_count = 0;

            {
                equivalent_edge_exists< Graph1 > edge1_exists;

                BGL_FORALL_INEDGES_T(w_new, e2, graph2_, Graph2)
                {
                    vertex2_type w = source(e2, graph2_);
                    if (state2_.in_core(w) || (w == w_new))
                    {
                        if (problem_selection != subgraph_mono)
                        {
                            vertex1_type v = v_new;
                            if (w != w_new)
                                v = state2_.core(w);

                            if (!edge1_exists(v, v_new,
                                    edge1_predicate< Graph1, Graph2,
                                        EdgeEquivalencePredicate >(
                                        edge_comp_, e2),
                                    graph1_))
                                return false;
                        }
                    }
                    else
                    {
                        if (0 < state2_.in_depth(w))
                            ++term_in2_count;
                        if (0 < state2_.out_depth(w))
                            ++term_out2_count;
                        if ((state2_.in_depth(w) == 0)
                            && (state2_.out_depth(w) == 0))
                            ++rest2_count;
                    }
                }
            }

            {
                equivalent_edge_exists< Graph1 > edge1_exists;

                BGL_FORALL_OUTEDGES_T(w_new, e2, graph2_, Graph2)
                {
                    vertex2_type w = target(e2, graph2_);
                    if (state2_.in_core(w) || (w == w_new))
                    {
                        if (problem_selection != subgraph_mono)
                        {
                            vertex1_type v = v_new;
                            if (w != w_new)
                                v = state2_.core(w);

                            if (!edge1_exists(v_new, v,
                                    edge1_predicate< Graph1, Graph2,
                                        EdgeEquivalencePredicate >(
                                        edge_comp_, e2),
                                    graph1_))
                                return false;
                        }
                    }
                    else
                    {
                        if (0 < state2_.in_depth(w))
                            ++term_in2_count;
                        if (0 < state2_.out_depth(w))
                            ++term_out2_count;
                        if ((state2_.in_depth(w) == 0)
                            && (state2_.out_depth(w) == 0))
                            ++rest2_count;
                    }
                }
            }

            if (problem_selection != subgraph_mono)
            { // subgraph_iso and isomorphism
                return comp_term_sets(term_in1_count, term_in2_count,
                           boost::mpl::int_< problem_selection >())
                    && comp_term_sets(term_out1_count, term_out2_count,
                        boost::mpl::int_< problem_selection >())
                    && comp_term_sets(rest1_count, rest2_count,
                        boost::mpl::int_< problem_selection >());
            }
            else
            { // subgraph_mono
                return comp_term_sets(term_in1_count, term_in2_count,
                           boost::mpl::int_< problem_selection >())
                    && comp_term_sets(term_out1_count, term_out2_count,
                        boost::mpl::int_< problem_selection >())
                    && comp_term_sets(
                        term_in1_count + term_out1_count + rest1_count,
                        term_in2_count + term_out2_count + rest2_count,
                        boost::mpl::int_< problem_selection >());
            }
        }

        // Returns true if vertex v in graph1 is a possible candidate to
        // be added to the current state
        bool possible_candidate1(const vertex1_type& v) const
        {
            if (state1_.term_both() && state2_.term_both())
                return state1_.term_both(v);
            else if (state1_.term_out() && state2_.term_out())
                return state1_.term_out(v);
            else if (state1_.term_in() && state2_.term_in())
                return state1_.term_in(v);
            else
                return !state1_.in_core(v);
        }

        // Returns true if vertex w in graph2 is a possible candidate to
        // be added to the current state
        bool possible_candidate2(const vertex2_type& w) const
        {
            if (state1_.term_both() && state2_.term_both())
                return state2_.term_both(w);
            else if (state1_.term_out() && state2_.term_out())
                return state2_.term_out(w);
            else if (state1_.term_in() && state2_.term_in())
                return state2_.term_in(w);
            else
                return !state2_.in_core(w);
        }

        // Returns true if a mapping was found
        bool success() const
        {
            return state1_.count() == num_vertices(graph1_);
        }

        // Returns true if a state is valid
        bool valid() const
        {
            boost::tuple< graph1_size_type, graph1_size_type, graph1_size_type >
                term1;
            boost::tuple< graph2_size_type, graph2_size_type, graph2_size_type >
                term2;

            term1 = state1_.term_set();
            term2 = state2_.term_set();

            return comp_term_sets(boost::get< 0 >(term1),
                       boost::get< 0 >(term2),
                       boost::mpl::int_< problem_selection >())
                && comp_term_sets(boost::get< 1 >(term1),
                    boost::get< 1 >(term2),
                    boost::mpl::int_< problem_selection >())
                && comp_term_sets(boost::get< 2 >(term1),
                    boost::get< 2 >(term2),
                    boost::mpl::int_< problem_selection >());
        }

        // Calls the user_callback with a graph (sub)graph mapping
        bool call_back(SubGraphIsoMapCallback user_callback) const
        {
            return user_callback(state1_.get_map(), state2_.get_map());
        }
    };

    // Data structure to keep info used for back tracking during
    // matching process
    template < typename Graph1, typename Graph2, typename VertexOrder1 >
    struct vf2_match_continuation
    {
        typename VertexOrder1::const_iterator graph1_verts_iter;
        typename graph_traits< Graph2 >::vertex_iterator graph2_verts_iter;
    };

    // Non-recursive method that explores state space using a depth-first
    // search strategy.  At each depth possible pairs candidate are compute
    // and tested for feasibility to extend the mapping. If a complete
    // mapping is found, the mapping is output to user_callback in the form
    // of a correspondence map (graph1 to graph2). Returning false from the
    // user_callback will terminate the search. Function match will return
    // true if the entire search space was explored.
    template < typename Graph1, typename Graph2, typename IndexMap1,
        typename IndexMap2, typename VertexOrder1,
        typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
        typename SubGraphIsoMapCallback, problem_selector problem_selection >
    bool match(const Graph1& graph1, const Graph2& graph2,
        SubGraphIsoMapCallback user_callback, const VertexOrder1& vertex_order1,
        state< Graph1, Graph2, IndexMap1, IndexMap2, EdgeEquivalencePredicate,
            VertexEquivalencePredicate, SubGraphIsoMapCallback,
            problem_selection >& s)
    {

        typename VertexOrder1::const_iterator graph1_verts_iter;

        typedef typename graph_traits< Graph2 >::vertex_iterator
            vertex2_iterator_type;
        vertex2_iterator_type graph2_verts_iter, graph2_verts_iter_end;

        typedef vf2_match_continuation< Graph1, Graph2, VertexOrder1 >
            match_continuation_type;
        std::vector< match_continuation_type > k;
        bool found_match = false;

    recur:
        if (s.success())
        {
            if (!s.call_back(user_callback))
                return true;
            found_match = true;

            goto back_track;
        }

        if (!s.valid())
            goto back_track;

        graph1_verts_iter = vertex_order1.begin();
        while (graph1_verts_iter != vertex_order1.end()
            && !s.possible_candidate1(*graph1_verts_iter))
        {
            ++graph1_verts_iter;
        }

        boost::tie(graph2_verts_iter, graph2_verts_iter_end) = vertices(graph2);
        while (graph2_verts_iter != graph2_verts_iter_end)
        {
            if (s.possible_candidate2(*graph2_verts_iter))
            {
                if (s.feasible(*graph1_verts_iter, *graph2_verts_iter))
                {
                    match_continuation_type kk;
                    kk.graph1_verts_iter = graph1_verts_iter;
                    kk.graph2_verts_iter = graph2_verts_iter;
                    k.push_back(kk);

                    s.push(*graph1_verts_iter, *graph2_verts_iter);
                    goto recur;
                }
            }
        graph2_loop:
            ++graph2_verts_iter;
        }

    back_track:
        if (k.empty())
            return found_match;

        const match_continuation_type kk = k.back();
        graph1_verts_iter = kk.graph1_verts_iter;
        graph2_verts_iter = kk.graph2_verts_iter;
        k.pop_back();

        s.pop(*graph1_verts_iter, *graph2_verts_iter);

        goto graph2_loop;
    }

    // Used to sort nodes by in/out degrees
    template < typename Graph > struct vertex_in_out_degree_cmp
    {
        typedef typename graph_traits< Graph >::vertex_descriptor vertex_type;

        vertex_in_out_degree_cmp(const Graph& graph) : graph_(graph) {}

        bool operator()(const vertex_type& v, const vertex_type& w) const
        {
            // lexicographical comparison
            return std::make_pair(in_degree(v, graph_), out_degree(v, graph_))
                < std::make_pair(in_degree(w, graph_), out_degree(w, graph_));
        }

        const Graph& graph_;
    };

    // Used to sort nodes by multiplicity of in/out degrees
    template < typename Graph, typename FrequencyMap >
    struct vertex_frequency_degree_cmp
    {
        typedef typename graph_traits< Graph >::vertex_descriptor vertex_type;

        vertex_frequency_degree_cmp(const Graph& graph, FrequencyMap freq)
        : graph_(graph), freq_(freq)
        {
        }

        bool operator()(const vertex_type& v, const vertex_type& w) const
        {
            // lexicographical comparison
            return std::make_pair(
                       freq_[v], in_degree(v, graph_) + out_degree(v, graph_))
                < std::make_pair(
                    freq_[w], in_degree(w, graph_) + out_degree(w, graph_));
        }

        const Graph& graph_;
        FrequencyMap freq_;
    };

    // Sorts vertices of a graph by multiplicity of in/out degrees
    template < typename Graph, typename IndexMap, typename VertexOrder >
    void sort_vertices(
        const Graph& graph, IndexMap index_map, VertexOrder& order)
    {
        typedef typename graph_traits< Graph >::vertices_size_type size_type;

        boost::range::sort(order, vertex_in_out_degree_cmp< Graph >(graph));

        std::vector< size_type > freq_vec(num_vertices(graph), 0);
        typedef iterator_property_map<
            typename std::vector< size_type >::iterator, IndexMap, size_type,
            size_type& >
            frequency_map_type;

        frequency_map_type freq
            = make_iterator_property_map(freq_vec.begin(), index_map);

        typedef typename VertexOrder::iterator order_iterator;

        for (order_iterator order_iter = order.begin();
             order_iter != order.end();)
        {
            size_type count = 0;
            for (order_iterator count_iter = order_iter;
                 (count_iter != order.end())
                 && (in_degree(*order_iter, graph)
                     == in_degree(*count_iter, graph))
                 && (out_degree(*order_iter, graph)
                     == out_degree(*count_iter, graph));
                 ++count_iter)
                ++count;

            for (size_type i = 0; i < count; ++i)
            {
                freq[*order_iter] = count;
                ++order_iter;
            }
        }

        boost::range::sort(order,
            vertex_frequency_degree_cmp< Graph, frequency_map_type >(
                graph, freq));
    }

    // Enumerates all graph sub-graph mono-/iso-morphism mappings between graphs
    // graph_small and graph_large. Continues until user_callback returns true
    // or the search space has been fully explored.
    template < problem_selector problem_selection, typename GraphSmall,
        typename GraphLarge, typename IndexMapSmall, typename IndexMapLarge,
        typename VertexOrderSmall, typename EdgeEquivalencePredicate,
        typename VertexEquivalencePredicate, typename SubGraphIsoMapCallback >
    bool vf2_subgraph_morphism(const GraphSmall& graph_small,
        const GraphLarge& graph_large, SubGraphIsoMapCallback user_callback,
        IndexMapSmall index_map_small, IndexMapLarge index_map_large,
        const VertexOrderSmall& vertex_order_small,
        EdgeEquivalencePredicate edge_comp,
        VertexEquivalencePredicate vertex_comp)
    {

        // Graph requirements
        BOOST_CONCEPT_ASSERT((BidirectionalGraphConcept< GraphSmall >));
        BOOST_CONCEPT_ASSERT((VertexListGraphConcept< GraphSmall >));
        BOOST_CONCEPT_ASSERT((EdgeListGraphConcept< GraphSmall >));
        BOOST_CONCEPT_ASSERT((AdjacencyMatrixConcept< GraphSmall >));

        BOOST_CONCEPT_ASSERT((BidirectionalGraphConcept< GraphLarge >));
        BOOST_CONCEPT_ASSERT((VertexListGraphConcept< GraphLarge >));
        BOOST_CONCEPT_ASSERT((EdgeListGraphConcept< GraphLarge >));
        BOOST_CONCEPT_ASSERT((AdjacencyMatrixConcept< GraphLarge >));

        typedef typename graph_traits< GraphSmall >::vertex_descriptor
            vertex_small_type;
        typedef typename graph_traits< GraphLarge >::vertex_descriptor
            vertex_large_type;

        typedef typename graph_traits< GraphSmall >::vertices_size_type
            size_type_small;
        typedef typename graph_traits< GraphLarge >::vertices_size_type
            size_type_large;

        // Property map requirements
        BOOST_CONCEPT_ASSERT(
            (ReadablePropertyMapConcept< IndexMapSmall, vertex_small_type >));
        typedef typename property_traits< IndexMapSmall >::value_type
            IndexMapSmallValue;
        BOOST_STATIC_ASSERT(
            (is_convertible< IndexMapSmallValue, size_type_small >::value));

        BOOST_CONCEPT_ASSERT(
            (ReadablePropertyMapConcept< IndexMapLarge, vertex_large_type >));
        typedef typename property_traits< IndexMapLarge >::value_type
            IndexMapLargeValue;
        BOOST_STATIC_ASSERT(
            (is_convertible< IndexMapLargeValue, size_type_large >::value));

        // Edge & vertex requirements
        typedef typename graph_traits< GraphSmall >::edge_descriptor
            edge_small_type;
        typedef typename graph_traits< GraphLarge >::edge_descriptor
            edge_large_type;

        BOOST_CONCEPT_ASSERT((BinaryPredicateConcept< EdgeEquivalencePredicate,
            edge_small_type, edge_large_type >));

        BOOST_CONCEPT_ASSERT(
            (BinaryPredicateConcept< VertexEquivalencePredicate,
                vertex_small_type, vertex_large_type >));

        // Vertex order requirements
        BOOST_CONCEPT_ASSERT((ContainerConcept< VertexOrderSmall >));
        typedef typename VertexOrderSmall::value_type order_value_type;
        BOOST_STATIC_ASSERT(
            (is_same< vertex_small_type, order_value_type >::value));
        BOOST_ASSERT(num_vertices(graph_small) == vertex_order_small.size());

        if (num_vertices(graph_small) > num_vertices(graph_large))
            return false;

        typename graph_traits< GraphSmall >::edges_size_type num_edges_small
            = num_edges(graph_small);
        typename graph_traits< GraphLarge >::edges_size_type num_edges_large
            = num_edges(graph_large);

        // Double the number of edges for undirected graphs: each edge counts as
        // in-edge and out-edge
        if (is_undirected(graph_small))
            num_edges_small *= 2;
        if (is_undirected(graph_large))
            num_edges_large *= 2;
        if (num_edges_small > num_edges_large)
            return false;

        detail::state< GraphSmall, GraphLarge, IndexMapSmall, IndexMapLarge,
            EdgeEquivalencePredicate, VertexEquivalencePredicate,
            SubGraphIsoMapCallback, problem_selection >
            s(graph_small, graph_large, index_map_small, index_map_large,
                edge_comp, vertex_comp);

        return detail::match(
            graph_small, graph_large, user_callback, vertex_order_small, s);
    }

} // namespace detail

// Returns vertex order (vertices sorted by multiplicity of in/out degrees)
template < typename Graph >
std::vector< typename graph_traits< Graph >::vertex_descriptor >
vertex_order_by_mult(const Graph& graph)
{

    std::vector< typename graph_traits< Graph >::vertex_descriptor >
        vertex_order;
    std::copy(vertices(graph).first, vertices(graph).second,
        std::back_inserter(vertex_order));

    detail::sort_vertices(graph, get(vertex_index, graph), vertex_order);
    return vertex_order;
}

// Enumerates all graph sub-graph monomorphism mappings between graphs
// graph_small and graph_large. Continues until user_callback returns true or
// the search space has been fully explored.
template < typename GraphSmall, typename GraphLarge, typename IndexMapSmall,
    typename IndexMapLarge, typename VertexOrderSmall,
    typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
    typename SubGraphIsoMapCallback >
bool vf2_subgraph_mono(const GraphSmall& graph_small,
    const GraphLarge& graph_large, SubGraphIsoMapCallback user_callback,
    IndexMapSmall index_map_small, IndexMapLarge index_map_large,
    const VertexOrderSmall& vertex_order_small,
    EdgeEquivalencePredicate edge_comp, VertexEquivalencePredicate vertex_comp)
{
    return detail::vf2_subgraph_morphism< detail::subgraph_mono >(graph_small,
        graph_large, user_callback, index_map_small, index_map_large,
        vertex_order_small, edge_comp, vertex_comp);
}

// All default interface for vf2_subgraph_iso
template < typename GraphSmall, typename GraphLarge,
    typename SubGraphIsoMapCallback >
bool vf2_subgraph_mono(const GraphSmall& graph_small,
    const GraphLarge& graph_large, SubGraphIsoMapCallback user_callback)
{
    return vf2_subgraph_mono(graph_small, graph_large, user_callback,
        get(vertex_index, graph_small), get(vertex_index, graph_large),
        vertex_order_by_mult(graph_small), always_equivalent(),
        always_equivalent());
}

// Named parameter interface of vf2_subgraph_iso
template < typename GraphSmall, typename GraphLarge, typename VertexOrderSmall,
    typename SubGraphIsoMapCallback, typename Param, typename Tag,
    typename Rest >
bool vf2_subgraph_mono(const GraphSmall& graph_small,
    const GraphLarge& graph_large, SubGraphIsoMapCallback user_callback,
    const VertexOrderSmall& vertex_order_small,
    const bgl_named_params< Param, Tag, Rest >& params)
{
    return vf2_subgraph_mono(graph_small, graph_large, user_callback,
        choose_const_pmap(
            get_param(params, vertex_index1), graph_small, vertex_index),
        choose_const_pmap(
            get_param(params, vertex_index2), graph_large, vertex_index),
        vertex_order_small,
        choose_param(
            get_param(params, edges_equivalent_t()), always_equivalent()),
        choose_param(
            get_param(params, vertices_equivalent_t()), always_equivalent()));
}

// Enumerates all graph sub-graph isomorphism mappings between graphs
// graph_small and graph_large. Continues until user_callback returns true or
// the search space has been fully explored.
template < typename GraphSmall, typename GraphLarge, typename IndexMapSmall,
    typename IndexMapLarge, typename VertexOrderSmall,
    typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
    typename SubGraphIsoMapCallback >
bool vf2_subgraph_iso(const GraphSmall& graph_small,
    const GraphLarge& graph_large, SubGraphIsoMapCallback user_callback,
    IndexMapSmall index_map_small, IndexMapLarge index_map_large,
    const VertexOrderSmall& vertex_order_small,
    EdgeEquivalencePredicate edge_comp, VertexEquivalencePredicate vertex_comp)
{
    return detail::vf2_subgraph_morphism< detail::subgraph_iso >(graph_small,
        graph_large, user_callback, index_map_small, index_map_large,
        vertex_order_small, edge_comp, vertex_comp);
}

// All default interface for vf2_subgraph_iso
template < typename GraphSmall, typename GraphLarge,
    typename SubGraphIsoMapCallback >
bool vf2_subgraph_iso(const GraphSmall& graph_small,
    const GraphLarge& graph_large, SubGraphIsoMapCallback user_callback)
{

    return vf2_subgraph_iso(graph_small, graph_large, user_callback,
        get(vertex_index, graph_small), get(vertex_index, graph_large),
        vertex_order_by_mult(graph_small), always_equivalent(),
        always_equivalent());
}

// Named parameter interface of vf2_subgraph_iso
template < typename GraphSmall, typename GraphLarge, typename VertexOrderSmall,
    typename SubGraphIsoMapCallback, typename Param, typename Tag,
    typename Rest >
bool vf2_subgraph_iso(const GraphSmall& graph_small,
    const GraphLarge& graph_large, SubGraphIsoMapCallback user_callback,
    const VertexOrderSmall& vertex_order_small,
    const bgl_named_params< Param, Tag, Rest >& params)
{

    return vf2_subgraph_iso(graph_small, graph_large, user_callback,
        choose_const_pmap(
            get_param(params, vertex_index1), graph_small, vertex_index),
        choose_const_pmap(
            get_param(params, vertex_index2), graph_large, vertex_index),
        vertex_order_small,
        choose_param(
            get_param(params, edges_equivalent_t()), always_equivalent()),
        choose_param(
            get_param(params, vertices_equivalent_t()), always_equivalent()));
}

// Enumerates all isomorphism mappings between graphs graph1_ and graph2_.
// Continues until user_callback returns true or the search space has been
// fully explored.
template < typename Graph1, typename Graph2, typename IndexMap1,
    typename IndexMap2, typename VertexOrder1,
    typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate,
    typename GraphIsoMapCallback >
bool vf2_graph_iso(const Graph1& graph1, const Graph2& graph2,
    GraphIsoMapCallback user_callback, IndexMap1 index_map1,
    IndexMap2 index_map2, const VertexOrder1& vertex_order1,
    EdgeEquivalencePredicate edge_comp, VertexEquivalencePredicate vertex_comp)
{

    // Graph requirements
    BOOST_CONCEPT_ASSERT((BidirectionalGraphConcept< Graph1 >));
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph1 >));
    BOOST_CONCEPT_ASSERT((EdgeListGraphConcept< Graph1 >));
    BOOST_CONCEPT_ASSERT((AdjacencyMatrixConcept< Graph1 >));

    BOOST_CONCEPT_ASSERT((BidirectionalGraphConcept< Graph2 >));
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< Graph2 >));
    BOOST_CONCEPT_ASSERT((EdgeListGraphConcept< Graph2 >));
    BOOST_CONCEPT_ASSERT((AdjacencyMatrixConcept< Graph2 >));

    typedef typename graph_traits< Graph1 >::vertex_descriptor vertex1_type;
    typedef typename graph_traits< Graph2 >::vertex_descriptor vertex2_type;

    typedef typename graph_traits< Graph1 >::vertices_size_type size_type1;
    typedef typename graph_traits< Graph2 >::vertices_size_type size_type2;

    // Property map requirements
    BOOST_CONCEPT_ASSERT(
        (ReadablePropertyMapConcept< IndexMap1, vertex1_type >));
    typedef typename property_traits< IndexMap1 >::value_type IndexMap1Value;
    BOOST_STATIC_ASSERT((is_convertible< IndexMap1Value, size_type1 >::value));

    BOOST_CONCEPT_ASSERT(
        (ReadablePropertyMapConcept< IndexMap2, vertex2_type >));
    typedef typename property_traits< IndexMap2 >::value_type IndexMap2Value;
    BOOST_STATIC_ASSERT((is_convertible< IndexMap2Value, size_type2 >::value));

    // Edge & vertex requirements
    typedef typename graph_traits< Graph1 >::edge_descriptor edge1_type;
    typedef typename graph_traits< Graph2 >::edge_descriptor edge2_type;

    BOOST_CONCEPT_ASSERT((BinaryPredicateConcept< EdgeEquivalencePredicate,
        edge1_type, edge2_type >));

    BOOST_CONCEPT_ASSERT((BinaryPredicateConcept< VertexEquivalencePredicate,
        vertex1_type, vertex2_type >));

    // Vertex order requirements
    BOOST_CONCEPT_ASSERT((ContainerConcept< VertexOrder1 >));
    typedef typename VertexOrder1::value_type order_value_type;
    BOOST_STATIC_ASSERT((is_same< vertex1_type, order_value_type >::value));
    BOOST_ASSERT(num_vertices(graph1) == vertex_order1.size());

    if (num_vertices(graph1) != num_vertices(graph2))
        return false;

    typename graph_traits< Graph1 >::edges_size_type num_edges1
        = num_edges(graph1);
    typename graph_traits< Graph2 >::edges_size_type num_edges2
        = num_edges(graph2);

    // Double the number of edges for undirected graphs: each edge counts as
    // in-edge and out-edge
    if (is_undirected(graph1))
        num_edges1 *= 2;
    if (is_undirected(graph2))
        num_edges2 *= 2;
    if (num_edges1 != num_edges2)
        return false;

    detail::state< Graph1, Graph2, IndexMap1, IndexMap2,
        EdgeEquivalencePredicate, VertexEquivalencePredicate,
        GraphIsoMapCallback, detail::isomorphism >
        s(graph1, graph2, index_map1, index_map2, edge_comp, vertex_comp);

    return detail::match(graph1, graph2, user_callback, vertex_order1, s);
}

// All default interface for vf2_graph_iso
template < typename Graph1, typename Graph2, typename GraphIsoMapCallback >
bool vf2_graph_iso(const Graph1& graph1, const Graph2& graph2,
    GraphIsoMapCallback user_callback)
{

    return vf2_graph_iso(graph1, graph2, user_callback,
        get(vertex_index, graph1), get(vertex_index, graph2),
        vertex_order_by_mult(graph1), always_equivalent(), always_equivalent());
}

// Named parameter interface of vf2_graph_iso
template < typename Graph1, typename Graph2, typename VertexOrder1,
    typename GraphIsoMapCallback, typename Param, typename Tag, typename Rest >
bool vf2_graph_iso(const Graph1& graph1, const Graph2& graph2,
    GraphIsoMapCallback user_callback, const VertexOrder1& vertex_order1,
    const bgl_named_params< Param, Tag, Rest >& params)
{

    return vf2_graph_iso(graph1, graph2, user_callback,
        choose_const_pmap(
            get_param(params, vertex_index1), graph1, vertex_index),
        choose_const_pmap(
            get_param(params, vertex_index2), graph2, vertex_index),
        vertex_order1,
        choose_param(
            get_param(params, edges_equivalent_t()), always_equivalent()),
        choose_param(
            get_param(params, vertices_equivalent_t()), always_equivalent()));
}

// Verifies a graph (sub)graph isomorphism map
template < typename Graph1, typename Graph2, typename CorresponenceMap1To2,
    typename EdgeEquivalencePredicate, typename VertexEquivalencePredicate >
inline bool verify_vf2_subgraph_iso(const Graph1& graph1, const Graph2& graph2,
    const CorresponenceMap1To2 f, EdgeEquivalencePredicate edge_comp,
    VertexEquivalencePredicate vertex_comp)
{

    BOOST_CONCEPT_ASSERT((EdgeListGraphConcept< Graph1 >));
    BOOST_CONCEPT_ASSERT((AdjacencyMatrixConcept< Graph2 >));

    detail::equivalent_edge_exists< Graph2 > edge2_exists;

    BGL_FORALL_EDGES_T(e1, graph1, Graph1)
    {
        typename graph_traits< Graph1 >::vertex_descriptor s1, t1;
        typename graph_traits< Graph2 >::vertex_descriptor s2, t2;

        s1 = source(e1, graph1);
        t1 = target(e1, graph1);
        s2 = get(f, s1);
        t2 = get(f, t1);

        if (!vertex_comp(s1, s2) || !vertex_comp(t1, t2))
            return false;

        typename graph_traits< Graph2 >::edge_descriptor e2;

        if (!edge2_exists(s2, t2,
                detail::edge2_predicate< Graph1, Graph2,
                    EdgeEquivalencePredicate >(edge_comp, e1),
                graph2))
            return false;
    }

    return true;
}

// Variant of verify_subgraph_iso with all default parameters
template < typename Graph1, typename Graph2, typename CorresponenceMap1To2 >
inline bool verify_vf2_subgraph_iso(
    const Graph1& graph1, const Graph2& graph2, const CorresponenceMap1To2 f)
{
    return verify_vf2_subgraph_iso(
        graph1, graph2, f, always_equivalent(), always_equivalent());
}

} // namespace boost

#ifdef BOOST_ISO_INCLUDED_ITER_MACROS
#undef BOOST_ISO_INCLUDED_ITER_MACROS
#include <boost/graph/iteration_macros_undef.hpp>
#endif

#endif // BOOST_VF2_SUB_GRAPH_ISO_HPP
