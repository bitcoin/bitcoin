// Copyright 2008-2010 Gordon Woodhull
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// mpl_graph - defines a metadata implementation of the BGL immutable graph concepts

// (c) 2008 Gordon Woodhull 
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSEmpl::_1_0.txt or copy at
// http://www.boost.org/LICENSEmpl::_1_0.txt)

#ifndef BOOST_MSM_MPL_GRAPH_MPL_GRAPH_HPP_INCLUDED
#define BOOST_MSM_MPL_GRAPH_MPL_GRAPH_HPP_INCLUDED

#include <boost/msm/mpl_graph/detail/graph_implementation_interface.ipp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/back_inserter.hpp>

namespace boost {
namespace msm {
namespace mpl_graph {

// Boost Graph concepts, MPL style

// The metafunctions of the public interface rely 
// metafunctions in the graph implementation to transform the input 
// into the maps which are required to deliver results.  Since the
// maps are produced lazily and are memoized, all of the graph
// concepts can be supported with no cost until they are actually
// used.

// Each of these dispatch to the correct producer metafunctions based
// on the representation inner type tag



// IncidenceGraph
template<typename Edge, typename Graph>
struct source : 
    mpl::first<typename mpl::at<typename detail::produce_edge_st_map<typename Graph::representation, typename Graph::data>::type,Edge>::type> 
{};
template<typename Edge, typename Graph>
struct target : 
    mpl::second<typename mpl::at<typename detail::produce_edge_st_map<typename Graph::representation, typename Graph::data>::type,Edge>::type> 
{};
template<typename Vertex, typename Graph>
struct out_edges :
    mpl::fold<typename detail::produce_out_map<typename Graph::representation, Vertex, typename Graph::data>::type,
         mpl::vector<>,
         mpl::push_back<mpl::_1, mpl::first<mpl::_2> > >
{};
template<typename Vertex, typename Graph>
struct out_degree : 
    mpl::size<typename out_edges<Vertex, Graph>::type>
{};

// BidirectionalGraph
template<typename Vertex, typename Graph>
struct in_edges :
    mpl::fold<typename detail::produce_in_map<typename Graph::representation, Vertex, typename Graph::data>::type,
         mpl::vector<>,
         mpl::push_back<mpl::_1, mpl::first<mpl::_2> > >
{};
template<typename Vertex, typename Graph>
struct in_degree :
    mpl::size<typename in_edges<Vertex, Graph>::type>
{};
template<typename Vertex, typename Graph>
struct degree :
    mpl::plus<typename out_degree<Vertex, Graph>::type,typename in_degree<Vertex, Graph>::type>
{};

// AdjacencyGraph 
template<typename Vertex, typename Graph>
struct adjacent_vertices :
    mpl::transform<typename detail::produce_out_map<typename Graph::representation, Vertex, typename Graph::data>::type,
              mpl::second<mpl::_1>,
              mpl::back_inserter<mpl::vector<> > >
{};

// VertexListGraph
template<typename Graph>
struct vertices :
    detail::produce_vertex_set<typename Graph::representation, typename Graph::data>
{};
template<typename Graph>
struct num_vertices :
    mpl::size<typename vertices<Graph>::type>
{};

// EdgeListGraph
template<typename Graph>
struct edges :
    detail::produce_edge_set<typename Graph::representation, typename Graph::data>
{};
template<typename Graph>
struct num_edges :
    mpl::size<typename edges<Graph>::type>
{};
// source and target are defined in IncidenceGraph

} // mpl_graph
} // msm
} // boost

#endif // BOOST_MSM_MPL_GRAPH_MPL_GRAPH_HPP_INCLUDED
