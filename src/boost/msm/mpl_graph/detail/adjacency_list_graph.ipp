// Copyright 2008-2010 Gordon Woodhull
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_MPL_GRAPH_DETAIL_ADJACENCY_LIST_GRAPH_IPP_INCLUDED
#define BOOST_MSM_MPL_GRAPH_DETAIL_ADJACENCY_LIST_GRAPH_IPP_INCLUDED

// implementation of a graph declared in adjacency list format
// sequence< pair< source_vertex, sequence< pair<edge, target_vertex> > > >

#include <boost/msm/mpl_graph/mpl_utils.hpp>
#include <boost/msm/mpl_graph/detail/incidence_list_graph.ipp>

#include <boost/mpl/copy.hpp>
#include <boost/mpl/inserter.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/push_back.hpp>

namespace boost {
namespace msm {
namespace mpl_graph {
namespace detail {
    
// tag identifying graph implementation as adjacency list (not defined)
struct adjacency_list_tag;

// outs map is really just the same data with the sequences turned into maps
// it might make sense to make another adjacency_map implementation for that case
template<typename AdjacencyList>
struct produce_al_outs_map :
    mpl::reverse_fold<AdjacencyList,
              mpl::map<>,
              mpl::insert<mpl::_1,
                          mpl::pair<mpl::first<mpl::_2>, mpl_utils::as_map<mpl::second<mpl::_2> > > > >
{};
                                    
// Edge->Target map for a Source for out_*, degree
template<typename Source, typename GraphData>
struct produce_out_map<adjacency_list_tag, Source, GraphData> : 
    mpl::at<typename produce_al_outs_map<GraphData>::type, Source>
{};

template<typename InsMap, typename Source, typename Adjacencies>
struct produce_in_adjacencies :
    mpl::reverse_fold<Adjacencies,
              InsMap,
              mpl::insert<mpl::_1,
                          mpl::pair<mpl::second<mpl::_2>,
                                    mpl::insert<mpl_utils::at_or_default<mpl::_1, mpl::second<mpl::_2>, mpl::map<> >,
                                                mpl::pair<mpl::first<mpl::_2>, Source> > > > >
{};
    
template<typename AdjacencyList>
struct produce_al_ins_map :
    mpl::reverse_fold<AdjacencyList,
              mpl::map<>,
              produce_in_adjacencies<mpl::_1, mpl::first<mpl::_2>, mpl::second<mpl::_2> > >
{};

// Edge->Source map for a Target for in_*, degree
template<typename Target, typename GraphData>
struct produce_in_map<adjacency_list_tag, Target, GraphData> :
    mpl::at<typename produce_al_ins_map<GraphData>::type, Target>
{};

// for everything else to do with edges, 
// just produce an incidence list and forward to that graph implementation
// (produce_out_map could, and produce_in_map probably should, be implemented this way too)
template<typename Incidences, typename Source, typename Adjacencies>
struct produce_adjacencies_incidences : // adjacencies' 
    mpl::reverse_fold<Adjacencies,
              Incidences,
              mpl::push_back<mpl::_1,
                             mpl::vector3<mpl::first<mpl::_2>, Source, mpl::second<mpl::_2> > > >
{};
    
template<typename AdjacencyList>
struct produce_incidence_list_from_adjacency_list :
    mpl::reverse_fold<AdjacencyList,
              mpl::vector<>,
              produce_adjacencies_incidences<mpl::_1, mpl::first<mpl::_2>, mpl::second<mpl::_2> > >
{};


// Edge->pair<Source,Target> map for source, target
template<typename GraphData>
struct produce_edge_st_map<adjacency_list_tag, GraphData> :
    produce_edge_st_map<incidence_list_tag,
                        typename produce_incidence_list_from_adjacency_list<GraphData>::type>
{};
             

// adjacency list supports zero-degree vertices, which incidence list does not
template<typename VertexSet, typename Adjacencies>
struct insert_adjacencies_targets : // adjacencies' 
    mpl::reverse_fold<Adjacencies,
              VertexSet,
              mpl::insert<mpl::_1, mpl::second<mpl::_2> > >
{};

template<typename GraphData>
struct produce_vertex_set<adjacency_list_tag, GraphData> :
    mpl::reverse_fold<GraphData,
              mpl::set<>,
              insert_adjacencies_targets<mpl::insert<mpl::_1, mpl::first<mpl::_2> >,
                                         mpl::second<mpl::_2> > >
{};
                                        

// Edge set for EdgeListGraph
template<typename GraphData>
struct produce_edge_set<adjacency_list_tag, GraphData> :
    produce_edge_set<incidence_list_tag,
                     typename produce_incidence_list_from_adjacency_list<GraphData>::type>
{};


} // namespaces   
}
}
}

#endif // BOOST_MSM_MPL_GRAPH_DETAIL_ADJACENCY_LIST_GRAPH_IPP_INCLUDED

