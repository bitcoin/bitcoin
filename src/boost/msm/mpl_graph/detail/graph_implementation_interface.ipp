// Copyright 2008-2010 Gordon Woodhull
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_MPL_GRAPH_DETAIL_GRAPH_IMPLEMENTATION_INTERFACE_IPP_INCLUDED
#define BOOST_MSM_MPL_GRAPH_DETAIL_GRAPH_IMPLEMENTATION_INTERFACE_IPP_INCLUDED

// forward definitions of the producer metafunctions that need to be specialized for
// each graph representation

namespace boost {
namespace msm {
namespace mpl_graph {
namespace detail {
    
    // Edge->Target map for a Source for out_*, degree
    template<typename RepresentationTag, typename Source, typename GraphData>
    struct produce_out_map;

    // Edge->Source map for a Target for in_*, degree
    template<typename RepresentationTag, typename Target, typename GraphData>
    struct produce_in_map;
    
    // Edge->pair<Source,Target> map for source, target
    template<typename RepresentationTag, typename GraphData>
    struct produce_edge_st_map;
    
    // Vertex set for VertexListGraph
    template<typename RepresentationTag, typename GraphData>
    struct produce_vertex_set;
    
    // Edge set for EdgeListGraph
    template<typename RepresentationTag, typename GraphData>
    struct produce_edge_set;

} // namespaces   
}
}
}

#endif // BOOST_MSM_MPL_GRAPH_DETAIL_GRAPH_IMPLEMENTATION_INTERFACE_IPP_INCLUDED

