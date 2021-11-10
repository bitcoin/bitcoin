// Copyright 2008-2010 Gordon Woodhull
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_MPL_GRAPH_INCIDENCE_LIST_GRAPH_HPP_INCLUDED
#define BOOST_MSM_MPL_GRAPH_INCIDENCE_LIST_GRAPH_HPP_INCLUDED

// graph implementation based on a an mpl sequence of sequences <Edge,Source,Target> 

// incidence_list_graph labels such a sequence as manipulable by the metafunctions
// in the corresponding implementation header detail/incidence_list_graph.ipp
// to produce the metadata structures needed by mpl_graph.hpp 

// the public interface
#include <boost/msm/mpl_graph/mpl_graph.hpp>

// the implementation
#include <boost/msm/mpl_graph/detail/incidence_list_graph.ipp>

namespace boost {
namespace msm {
namespace mpl_graph {
   
template<typename EdgeSequence>
struct incidence_list_graph {
    typedef detail::incidence_list_tag representation;
    typedef EdgeSequence data;
};

}
}
}

#endif // BOOST_MSM_MPL_GRAPH_INCIDENCE_LIST_GRAPH_HPP_INCLUDED
