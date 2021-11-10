// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_LOCAL_SUBGRAPH_HPP
#define BOOST_GRAPH_LOCAL_SUBGRAPH_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/graph/parallel/container_traits.hpp>

namespace boost {

namespace graph { namespace detail {
  // Optionally, virtually derive from a base class
  template<bool Derive, typename Base> struct derive_from_if;
  template<typename Base> struct derive_from_if<true, Base> : virtual Base {};
  template<typename Base> struct derive_from_if<false, Base> {};

  template<typename NewBase, typename Tag, typename OldBase = NewBase>
  struct derive_from_if_tag_is : 
    derive_from_if<(is_base_and_derived<OldBase, Tag>::value
                    || is_same<OldBase, Tag>::value), 
                   NewBase>
  {
  };
} } // end namespace graph::detail

template<typename DistributedGraph>
class is_local_edge
{
public:
  typedef bool result_type;
  typedef typename graph_traits<DistributedGraph>::edge_descriptor
    argument_type;

  is_local_edge() : g(0) {}
  is_local_edge(DistributedGraph& g) : g(&g), owner(get(vertex_owner, g)) {}

  // Since either the source or target vertex must be local, the
  // equivalence of their owners indicates a local edge.
  result_type operator()(const argument_type& e) const
  { return get(owner, source(e, *g)) == get(owner, target(e, *g)); }

private:
  DistributedGraph* g;
  typename property_map<DistributedGraph, vertex_owner_t>::const_type owner;
};

template<typename DistributedGraph>
class is_local_vertex
{
public:
  typedef bool result_type;
  typedef typename graph_traits<DistributedGraph>::vertex_descriptor
    argument_type;

  is_local_vertex() : g(0) {}
  is_local_vertex(DistributedGraph& g) : g(&g), owner(get(vertex_owner, g)) { }

  // Since either the source or target vertex must be local, the
  // equivalence of their owners indicates a local edge.
  result_type operator()(const argument_type& v) const
  { 
    return get(owner, v) == process_id(process_group(*g)); 
  }

private:
  DistributedGraph* g;
  typename property_map<DistributedGraph, vertex_owner_t>::const_type owner;
};

template<typename DistributedGraph>
class local_subgraph 
  : public filtered_graph<DistributedGraph, 
                          is_local_edge<DistributedGraph>,
                          is_local_vertex<DistributedGraph> >
{
  typedef filtered_graph<DistributedGraph, 
                         is_local_edge<DistributedGraph>,
                         is_local_vertex<DistributedGraph> >
    inherited;
  typedef typename graph_traits<DistributedGraph>::traversal_category
    inherited_category;
  
public:
  struct traversal_category :
    graph::detail::derive_from_if_tag_is<incidence_graph_tag, 
                                         inherited_category>,
    graph::detail::derive_from_if_tag_is<adjacency_graph_tag, 
                                         inherited_category>,
    graph::detail::derive_from_if_tag_is<vertex_list_graph_tag, 
                                         inherited_category>,
    graph::detail::derive_from_if_tag_is<edge_list_graph_tag, 
                                         inherited_category>,
    graph::detail::derive_from_if_tag_is<vertex_list_graph_tag, 
                                         inherited_category,
                                         distributed_vertex_list_graph_tag>,
    graph::detail::derive_from_if_tag_is<edge_list_graph_tag, 
                                         inherited_category,
                                         distributed_edge_list_graph_tag>
  { };

  local_subgraph(DistributedGraph& g) 
    : inherited(g, 
                is_local_edge<DistributedGraph>(g),
                is_local_vertex<DistributedGraph>(g)), 
      g(g) 
  {
  }

  // Distributed Container
  typedef typename boost::graph::parallel::process_group_type<DistributedGraph>::type
    process_group_type;

  process_group_type&       process_group()       
  { 
    using boost::graph::parallel::process_group;
    return process_group(g); 
  }
  const process_group_type& process_group() const 
  { 
    using boost::graph::parallel::process_group;
    return boost::graph::parallel::process_group(g); 
  }
  
  DistributedGraph&         base()               { return g; }
  const DistributedGraph&   base() const         { return g; }

private:
  DistributedGraph& g;
};

template<typename DistributedGraph, typename PropertyTag>
class property_map<local_subgraph<DistributedGraph>, PropertyTag>
  : public property_map<DistributedGraph, PropertyTag> { };

template<typename DistributedGraph, typename PropertyTag>
class property_map<local_subgraph<const DistributedGraph>, PropertyTag>
{
 public:
  typedef typename property_map<DistributedGraph, PropertyTag>::const_type
    type;
  typedef type const_type;
};

template<typename PropertyTag, typename DistributedGraph>
inline typename property_map<local_subgraph<DistributedGraph>, PropertyTag>::type
get(PropertyTag p, local_subgraph<DistributedGraph>& g)
{ return get(p, g.base()); }

template<typename PropertyTag, typename DistributedGraph>
inline typename property_map<local_subgraph<DistributedGraph>, PropertyTag>
  ::const_type
get(PropertyTag p, const local_subgraph<DistributedGraph>& g)
{ return get(p, g.base()); } 

template<typename DistributedGraph>
inline local_subgraph<DistributedGraph> 
make_local_subgraph(DistributedGraph& g)
{ return local_subgraph<DistributedGraph>(g); }

} // end namespace boost

#endif // BOOST_GRAPH_LOCAL_SUBGRAPH_HPP
