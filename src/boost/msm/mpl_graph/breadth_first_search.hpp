// Copyright 2008-2010 Gordon Woodhull
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_MPL_GRAPH_BREADTH_FIRST_SEARCH_HPP_INCLUDED
#define BOOST_MSM_MPL_GRAPH_BREADTH_FIRST_SEARCH_HPP_INCLUDED

#include <boost/msm/mpl_graph/mpl_graph.hpp>

#include <boost/mpl/has_key.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/remove.hpp>

#include "search_colors.hpp"

namespace boost {
namespace msm {
namespace mpl_graph {

// bfs takes a visitor which has all the bgl-like metafunctions encapsulated in an 
// "operations" member class, and also a state.  the operations are expected to return a new state
struct bfs_default_visitor_operations {
    template<typename Vertex, typename Graph, typename State>
    struct initialize_vertex {
        typedef State type;       
    };
    
    template<typename Vertex, typename Graph, typename State>
    struct discover_vertex {
        typedef State type;       
    };
    
    template<typename Vertex, typename Graph, typename State>
    struct examine_vertex {
        typedef State type;       
    };
    
    template<typename Vertex, typename Graph, typename State>
    struct examine_edge {
        typedef State type;       
    };
        
    template<typename Edge, typename Graph, typename State>
    struct tree_edge {
        typedef State type;
    };
    
    template<typename Edge, typename Graph, typename State>
    struct non_tree_edge {
        typedef State type;
    };
    
    template<typename Edge, typename Graph, typename State>
    struct gray_target {
        typedef State type;
    };  
    
    template<typename Edge, typename Graph, typename State>
    struct black_target {
        typedef State type;
    };  
    
    template<typename Vertex, typename Graph, typename State>
    struct finish_vertex {
        typedef State type;       
    };
};

namespace detail {

template<typename Graph, typename VisitorOps, typename VCQState, typename Edge>
struct bfs_run_queue_examine_edge {
    typedef typename VisitorOps::template examine_edge<Edge, Graph, typename mpl::at_c<VCQState, 0>::type>::type visitor_state;
    typedef typename mpl::at_c<VCQState, 1>::type color_state;
    typedef typename mpl::at_c<VCQState, 2>::type vertex_queue;

    typedef typename mpl::if_<typename boost::is_same<typename search_color_map_ops::template get_color<typename mpl_graph::target<Edge, Graph>::type, color_state>::type, search_colors::White>::type,
         // unseen target: tree edge, discover target, paint it gray, and enqueue
         mpl::vector<typename VisitorOps::template discover_vertex<typename mpl_graph::target<Edge, Graph>::type, Graph,
                                                                            typename VisitorOps::template tree_edge<Edge, Graph, visitor_state>::type>::type,
                     typename search_color_map_ops::template set_color<typename mpl_graph::target<Edge, Graph>::type, search_colors::Gray, color_state>::type,
                     typename mpl::push_back<vertex_queue, typename mpl_graph::target<Edge, Graph>::type >::type >,
         // seen
         mpl::vector<typename mpl::if_<typename boost::is_same<typename search_color_map_ops::template get_color<mpl_graph::target<Edge, Graph>, color_state>, 
                                             search_colors::Gray>::type,
                              typename VisitorOps::template gray_target<Edge, Graph, visitor_state>::type,
                              typename VisitorOps::template black_target<Edge, Graph, visitor_state>::type>::type,
                     color_state,
                     vertex_queue> 
         >::type type; 
};

// runs bfs on a queue, passing the new queue forward on recursion
// returns pair<visitor_state, color_state>
template<typename Graph, typename VertexQueue, typename VisitorOps, typename VisitorState, typename ColorMap>
struct bfs_run_queue {
    // enter vertex
    typedef typename mpl::front<VertexQueue>::type Vertex;
    typedef typename mpl::pop_front<VertexQueue>::type Tail;
    typedef typename VisitorOps::template examine_vertex<Vertex, Graph, VisitorState>::type examined_state;
    
    // loop over out edges
    typedef typename mpl::template 
        fold<typename mpl_graph::out_edges<Vertex, Graph>::type, 
             mpl::vector<examined_state, ColorMap, Tail>,
             bfs_run_queue_examine_edge<Graph, VisitorOps, mpl::_1, mpl::_2>
            >::type did_edges;
            
    typedef typename VisitorOps::template 
        finish_vertex<Vertex, Graph, typename mpl::at_c<did_edges, 0>::type>::type 
            finished_vertex; 
    // does map insert always overwrite?  i seem to remember this not working on msvc once
    typedef typename search_color_map_ops::template 
        set_color<Vertex, search_colors::Black, typename mpl::at_c<did_edges, 1>::type>::type 
            colored_vertex;
    typedef typename mpl::at_c<did_edges, 2>::type queued_targets;

    typedef typename 
        mpl::if_<typename mpl::empty<queued_targets>::type,
                 mpl::pair<finished_vertex, colored_vertex>,
                 bfs_run_queue<Graph, queued_targets,
                               VisitorOps, finished_vertex,
                               colored_vertex> >::type::type type;
};

} // namespace detail

template<typename Graph, typename VisitorOps, typename VisitorState, 
         typename Vertex, 
         typename ColorMap = create_search_color_map::type >
struct breadth_first_search {
    typedef typename VisitorOps::template 
        discover_vertex<Vertex, Graph, VisitorState>::type 
            discovered_state;
    typedef typename search_color_map_ops::template 
        set_color<Vertex, search_colors::Gray, ColorMap>::type 
            discovered_colors;
    typedef typename detail::
        bfs_run_queue<Graph, mpl::vector<Vertex>, 
                      VisitorOps, discovered_state, 
                      discovered_colors>::type type;
};

template<typename Graph, typename VisitorOps, typename VisitorState,
         typename FirstVertex = typename mpl::front<typename mpl_graph::vertices<Graph>::type>::type,
         typename ColorMap = create_search_color_map::type>
struct breadth_first_search_all : // visit "first" first, then visit any still white
    mpl::fold<typename mpl_graph::vertices<Graph>::type,
              typename breadth_first_search<Graph, VisitorOps, VisitorState, FirstVertex, ColorMap>::type,
              mpl::if_<boost::is_same<search_color_map_ops::template get_color<mpl::_2, mpl::second<mpl::_1> >,
                                      search_colors::White>,
                       breadth_first_search<Graph, VisitorOps, mpl::first<mpl::_1>,
                                            mpl::_2, mpl::second<mpl::_1> >,
                       mpl::_1> >   
{};

} // namespace mpl_graph
} // namespace msm
} // namespace boost


#endif // BOOST_MSM_MPL_GRAPH_BREADTH_FIRST_SEARCH_HPP_INCLUDED
