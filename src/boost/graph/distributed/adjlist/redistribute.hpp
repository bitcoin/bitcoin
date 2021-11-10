// Copyright (C) 2005-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

//
// Implements redistribution of vertices for a distributed adjacency
// list. This file should not be included by users. It will be
// included by the distributed adjacency list header.
//

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/pending/container_traits.hpp>

namespace boost { namespace detail { namespace parallel {

/* This structure contains a (vertex or edge) descriptor that is being
   moved from one processor to another. It contains the properties for
   that descriptor (if any).
 */
template<typename Descriptor, typename DescriptorProperty>
struct redistributed_descriptor : maybe_store_property<DescriptorProperty>
{
  typedef maybe_store_property<DescriptorProperty> inherited;

  redistributed_descriptor() { }

  redistributed_descriptor(const Descriptor& v, const DescriptorProperty& p)
    : inherited(p), descriptor(v) { }

  Descriptor descriptor;

private:
  friend class boost::serialization::access;

  template<typename Archiver>
  void serialize(Archiver& ar, unsigned int /*version*/)
  {
    ar & boost::serialization::base_object<inherited>(*this) 
       & unsafe_serialize(descriptor);
  }
};

/* Predicate that returns true if the target has migrated. */
template<typename VertexProcessorMap, typename Graph>
struct target_migrated_t
{
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::edge_descriptor Edge;

  target_migrated_t(VertexProcessorMap vertex_to_processor, const Graph& g)
    : vertex_to_processor(vertex_to_processor), g(g) { }

  bool operator()(Edge e) const
  {
    typedef global_descriptor<Vertex> DVertex;
    processor_id_type owner = get(edge_target_processor_id, g, e);
    return get(vertex_to_processor, DVertex(owner, target(e, g))) != owner;
  }

private:
  VertexProcessorMap vertex_to_processor;
  const Graph& g;
};

template<typename VertexProcessorMap, typename Graph>
inline target_migrated_t<VertexProcessorMap, Graph>
target_migrated(VertexProcessorMap vertex_to_processor, const Graph& g)
{ return target_migrated_t<VertexProcessorMap, Graph>(vertex_to_processor, g); }

/* Predicate that returns true if the source of an in-edge has migrated. */
template<typename VertexProcessorMap, typename Graph>
struct source_migrated_t
{
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::edge_descriptor Edge;

  source_migrated_t(VertexProcessorMap vertex_to_processor, const Graph& g)
    : vertex_to_processor(vertex_to_processor), g(g) { }

  bool operator()(stored_in_edge<Edge> e) const
  {
    return get(vertex_to_processor, DVertex(e.source_processor, source(e.e, g)))
      != e.source_processor;
  }

private:
  VertexProcessorMap vertex_to_processor;
  const Graph& g;
};

template<typename VertexProcessorMap, typename Graph>
inline source_migrated_t<VertexProcessorMap, Graph>
source_migrated(VertexProcessorMap vertex_to_processor, const Graph& g)
{ return source_migrated_t<VertexProcessorMap, Graph>(vertex_to_processor, g); }

/* Predicate that returns true if the target has migrated. */
template<typename VertexProcessorMap, typename Graph>
struct source_or_target_migrated_t
{
  typedef typename graph_traits<Graph>::edge_descriptor Edge;

  source_or_target_migrated_t(VertexProcessorMap vertex_to_processor,
                              const Graph& g)
    : vertex_to_processor(vertex_to_processor), g(g) { }

  bool operator()(Edge e) const
  {
    return get(vertex_to_processor, source(e, g)) != source(e, g).owner
      || get(vertex_to_processor, target(e, g)) != target(e, g).owner;
  }

private:
  VertexProcessorMap vertex_to_processor;
  const Graph& g;
};

template<typename VertexProcessorMap, typename Graph>
inline source_or_target_migrated_t<VertexProcessorMap, Graph>
source_or_target_migrated(VertexProcessorMap vertex_to_processor,
const Graph& g)
{
  typedef source_or_target_migrated_t<VertexProcessorMap, Graph> result_type;
  return result_type(vertex_to_processor, g);
}

} } // end of namespace detail::parallel

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
template<typename VertexProcessorMap>
void
PBGL_DISTRIB_ADJLIST_TYPE
::request_in_neighbors(vertex_descriptor v,
                       VertexProcessorMap vertex_to_processor,
                       bidirectionalS)
{
  BGL_FORALL_INEDGES_T(v, e, *this, graph_type)
    request(vertex_to_processor, source(e, *this));
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
template<typename VertexProcessorMap>
void
PBGL_DISTRIB_ADJLIST_TYPE
::remove_migrated_in_edges(vertex_descriptor v,
                           VertexProcessorMap vertex_to_processor,
                           bidirectionalS)
{
  graph_detail::erase_if(get(vertex_in_edges, base())[v.local],
                         source_migrated(vertex_to_processor, base()));
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
template<typename VertexProcessorMap>
void
PBGL_DISTRIB_ADJLIST_TYPE
::redistribute(VertexProcessorMap vertex_to_processor)
{
  using boost::parallel::inplace_all_to_all;

  // When we have stable descriptors, we only move those descriptors
  // that actually need to be moved. Otherwise, we essentially have to
  // regenerate the entire graph.
  const bool has_stable_descriptors =
    is_same<typename config_type::vertex_list_selector, listS>::value
    || is_same<typename config_type::vertex_list_selector, setS>::value
    || is_same<typename config_type::vertex_list_selector, multisetS>::value;

  typedef detail::parallel::redistributed_descriptor<vertex_descriptor, 
                                                     vertex_property_type>
    redistributed_vertex;
  typedef detail::parallel::redistributed_descriptor<edge_descriptor, 
                                                     edge_property_type>
    redistributed_edge;

  vertex_iterator vi, vi_end;
  edge_iterator ei, ei_end;

  process_group_type pg = process_group();

  // Initial synchronization makes sure that we have all of our ducks
  // in a row. We don't want any outstanding add/remove messages
  // coming in mid-redistribution!
  synchronize(process_group_);

  // We cannot cope with eviction of ghost cells
  vertex_to_processor.set_max_ghost_cells(0);

  process_id_type p = num_processes(pg);

  // Send vertices and edges to the processor where they will
  // actually reside.  This requires O(|V| + |E|) communication
  std::vector<std::vector<redistributed_vertex> > redistributed_vertices(p);
  std::vector<std::vector<redistributed_edge> > redistributed_edges(p);

  // Build the sets of relocated vertices for each process and then do
  // an all-to-all transfer.
  for (boost::tie(vi, vi_end) = vertices(*this); vi != vi_end; ++vi) {
    if (!has_stable_descriptors
        || get(vertex_to_processor, *vi) != vi->owner) {
      redistributed_vertices[get(vertex_to_processor, *vi)]
        .push_back(redistributed_vertex(*vi, get(vertex_all_t(), base(),
                                                 vi->local)));
    }

    // When our descriptors are stable, we need to determine which
    // adjacent descriptors are stable to determine which edges will
    // be removed.
    if (has_stable_descriptors) {
      BGL_FORALL_OUTEDGES_T(*vi, e, *this, graph_type)
        request(vertex_to_processor, target(e, *this));
      request_in_neighbors(*vi, vertex_to_processor, directed_selector());
    }
  }

  inplace_all_to_all(pg, redistributed_vertices);

  // If we have stable descriptors, we need to know where our neighbor
  // vertices are moving.
  if (has_stable_descriptors)
    synchronize(vertex_to_processor);

  // Build the sets of relocated edges for each process and then do
  // an all-to-all transfer.
  for (boost::tie(ei, ei_end) = edges(*this); ei != ei_end; ++ei) {
    vertex_descriptor src = source(*ei, *this);
    vertex_descriptor tgt = target(*ei, *this);
    if (!has_stable_descriptors
        || get(vertex_to_processor, src) != src.owner
        || get(vertex_to_processor, tgt) != tgt.owner)
      redistributed_edges[get(vertex_to_processor, source(*ei, *this))]
        .push_back(redistributed_edge(*ei, split_edge_property(get(edge_all_t(), base(),
                                                                   ei->local))));
  }
  inplace_all_to_all(pg, redistributed_edges);

  // A mapping from old vertex descriptors to new vertex
  // descriptors. This is an STL map partly because I'm too lazy to
  // build a real property map (which is hard in the general case) but
  // also because it won't try to look in the graph itself, because
  // the keys are all vertex descriptors that have been invalidated.
  std::map<vertex_descriptor, vertex_descriptor> old_to_new_vertex_map;

  if (has_stable_descriptors) {
    // Clear out all vertices and edges that will have moved. There
    // are several stages to this.

    // First, eliminate all outgoing edges from the (local) vertices
    // that have been moved or whose targets have been moved.
    BGL_FORALL_VERTICES_T(v, *this, graph_type) {
      if (get(vertex_to_processor, v) != v.owner) {
        clear_out_edges(v.local, base());
        clear_in_edges_local(v, directed_selector());
      } else {
        remove_out_edge_if(v.local,
                           target_migrated(vertex_to_processor, base()),
                           base());
        remove_migrated_in_edges(v, vertex_to_processor, directed_selector());
      }
    }

    // Next, eliminate locally-stored edges that have migrated (for
    // undirected graphs).
    graph_detail::erase_if(local_edges_,
                           source_or_target_migrated(vertex_to_processor, *this));

    // Eliminate vertices that have migrated
    for (boost::tie(vi, vi_end) = vertices(*this); vi != vi_end; /* in loop */) {
      if (get(vertex_to_processor, *vi) != vi->owner)
        remove_vertex((*vi++).local, base());
      else {
        // Add the identity relation for vertices that have not migrated
        old_to_new_vertex_map[*vi] = *vi;
        ++vi;
      }
    }
  } else {
    // Clear out the local graph: the entire graph is in transit
    clear();
  }

  // Add the new vertices to the graph. When we do so, update the old
  // -> new vertex mapping both locally and for the owner of the "old"
  // vertex.
  {
    typedef std::pair<vertex_descriptor, vertex_descriptor> mapping_pair;
    std::vector<std::vector<mapping_pair> > mappings(p);

    for (process_id_type src = 0; src < p; ++src) {
      for (typename std::vector<redistributed_vertex>::iterator vi =
             redistributed_vertices[src].begin();
           vi != redistributed_vertices[src].end(); ++vi) {
        vertex_descriptor new_vertex =
            add_vertex(vi->get_property(), *this);
        old_to_new_vertex_map[vi->descriptor] = new_vertex;
        mappings[vi->descriptor.owner].push_back(mapping_pair(vi->descriptor,
                                                              new_vertex));
      }

      redistributed_vertices[src].clear();
    }

    inplace_all_to_all(pg, mappings);

    // Add the mappings we were sent into the old->new map.
    for (process_id_type src = 0; src < p; ++src)
      old_to_new_vertex_map.insert(mappings[src].begin(), mappings[src].end());
  }

  // Get old->new vertex mappings for all of the vertices we need to
  // know about.

  // TBD: An optimization here might involve sending the
  // request-response pairs without an explicit request step (for
  // bidirectional and undirected graphs). However, it may not matter
  // all that much given the cost of redistribution.
  {
    std::vector<std::vector<vertex_descriptor> > vertex_map_requests(p);
    std::vector<std::vector<vertex_descriptor> > vertex_map_responses(p);

    // We need to know about all of the vertices incident on edges
    // that have been relocated to this processor. Tell each processor
    // what each other processor needs to know.
    for (process_id_type src = 0; src < p; ++src)
      for (typename std::vector<redistributed_edge>::iterator ei =
             redistributed_edges[src].begin();
           ei != redistributed_edges[src].end(); ++ei) {
        vertex_descriptor need_vertex = target(ei->descriptor, *this);
        if (old_to_new_vertex_map.find(need_vertex)
            == old_to_new_vertex_map.end())
          {
            old_to_new_vertex_map[need_vertex] = need_vertex;
            vertex_map_requests[need_vertex.owner].push_back(need_vertex);
          }
      }
    inplace_all_to_all(pg,
                       vertex_map_requests,
                       vertex_map_responses);

    // Process the requests made for vertices we own. Then perform yet
    // another all-to-all swap. This one matches the requests we've
    // made to the responses we were given.
    for (process_id_type src = 0; src < p; ++src)
      for (typename std::vector<vertex_descriptor>::iterator vi =
             vertex_map_responses[src].begin();
           vi != vertex_map_responses[src].end(); ++vi)
        *vi = old_to_new_vertex_map[*vi];
    inplace_all_to_all(pg, vertex_map_responses);

    // Matching the requests to the responses, update the old->new
    // vertex map for all of the vertices we will need to know.
    for (process_id_type src = 0; src < p; ++src) {
      typedef typename std::vector<vertex_descriptor>::size_type size_type;
      for (size_type i = 0; i < vertex_map_requests[src].size(); ++i) {
        old_to_new_vertex_map[vertex_map_requests[src][i]] =
          vertex_map_responses[src][i];
      }
    }
  }

  // Add edges to the graph by mapping the source and target.
  for (process_id_type src = 0; src < p; ++src) {
    for (typename std::vector<redistributed_edge>::iterator ei =
           redistributed_edges[src].begin();
         ei != redistributed_edges[src].end(); ++ei) {
      add_edge(old_to_new_vertex_map[source(ei->descriptor, *this)],
               old_to_new_vertex_map[target(ei->descriptor, *this)],
               ei->get_property(),
               *this);
    }

    redistributed_edges[src].clear();
  }

  // Be sure that edge-addition messages are received now, completing
  // the graph.
  synchronize(process_group_);

  this->distribution().clear();

  detail::parallel::maybe_initialize_vertex_indices(vertices(base()), 
                                                    get(vertex_index, base()));
}

} // end namespace boost
