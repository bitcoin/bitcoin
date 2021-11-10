// Copyright (C) 2007 Douglas Gregor 

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file contains code for the distributed adjacency list's
// initializations. It should not be included directly by users.

#ifndef BOOST_GRAPH_DISTRIBUTED_ADJLIST_INITIALIZE_HPP
#define BOOST_GRAPH_DISTRIBUTED_ADJLIST_INITIALIZE_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

namespace boost { 

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
template<typename EdgeIterator>
void
PBGL_DISTRIB_ADJLIST_TYPE::
initialize(EdgeIterator first, EdgeIterator last,
           vertices_size_type, const base_distribution_type& distribution, 
           vecS)
{
  process_id_type id = process_id(process_group_);
  while (first != last) {
    if ((process_id_type)distribution(first->first) == id) {
      vertex_descriptor source(id, distribution.local(first->first));
      vertex_descriptor target(distribution(first->second),
                               distribution.local(first->second));
      add_edge(source, target, *this);
    }
    ++first;
  }

  synchronize(process_group_);
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
template<typename EdgeIterator, typename EdgePropertyIterator>
void
PBGL_DISTRIB_ADJLIST_TYPE::
initialize(EdgeIterator first, EdgeIterator last,
           EdgePropertyIterator ep_iter,
           vertices_size_type, const base_distribution_type& distribution, 
           vecS)
{
  process_id_type id = process_id(process_group_);
  while (first != last) {
    if (static_cast<process_id_type>(distribution(first->first)) == id) {
      vertex_descriptor source(id, distribution.local(first->first));
      vertex_descriptor target(distribution(first->second),
                               distribution.local(first->second));
      add_edge(source, target, *ep_iter, *this);
    }
    ++first;
    ++ep_iter;
  }

  synchronize(process_group_);
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
template<typename EdgeIterator, typename EdgePropertyIterator,
         typename VertexListS>
void
PBGL_DISTRIB_ADJLIST_TYPE::
initialize(EdgeIterator first, EdgeIterator last,
           EdgePropertyIterator ep_iter,
           vertices_size_type n, const base_distribution_type& distribution,
           VertexListS)
{
  using boost::parallel::inplace_all_to_all;

  typedef vertices_size_type vertex_number_t;
  typedef typename std::iterator_traits<EdgePropertyIterator>::value_type
    edge_property_init_t;

  typedef std::pair<vertex_descriptor, vertex_number_t>
    st_pair;
  typedef std::pair<st_pair, edge_property_init_t> delayed_edge_t;

  process_group_type pg = process_group();
  process_id_type id = process_id(pg);

  // Vertex indices
  std::vector<local_vertex_descriptor> index_to_vertex;
  index_to_vertex.reserve(num_vertices(*this));
  BGL_FORALL_VERTICES_T(v, base(), inherited)
    index_to_vertex.push_back(v);

  // The list of edges we can't add immediately.
  std::vector<delayed_edge_t> delayed_edges;

  std::vector<std::vector<vertex_number_t> > descriptor_requests;
  descriptor_requests.resize(num_processes(pg));

  // Add all of the edges we can, up to the point where we run
  // into a descriptor we don't know.
  while (first != last) {
    if (distribution(first->first) == id) {
      if (distribution(first->second) != id) break;
      vertex_descriptor source
        (id, index_to_vertex[distribution.local(first->first)]);
      vertex_descriptor target
        (distribution(first->second),
         index_to_vertex[distribution.local(first->second)]);
      add_edge(source, target, *ep_iter, *this);
    }
    ++first;
    ++ep_iter;
  }

  // Queue all of the remaining edges and determine the set of
  // descriptors we need to know about.
  while (first != last) {
    if (distribution(first->first) == id) {
      vertex_descriptor source
        (id, index_to_vertex[distribution.local(first->first)]);
      process_id_type dest = distribution(first->second);
      if (dest != id) {
        descriptor_requests[dest]
          .push_back(distribution.local(first->second));
        // Compact request list if we need to
        if (descriptor_requests[dest].size() >
              distribution.block_size(dest, n)) {
          std::sort(descriptor_requests[dest].begin(),
                    descriptor_requests[dest].end());
          descriptor_requests[dest].erase(
            std::unique(descriptor_requests[dest].begin(),
                        descriptor_requests[dest].end()),
            descriptor_requests[dest].end());
        }
      }

      // Save the edge for later
      delayed_edges.push_back
        (delayed_edge_t(st_pair(source, first->second), *ep_iter));
    }
    ++first;
    ++ep_iter;
  }

  // Compact descriptor requests
  for (process_id_type dest = 0; dest < num_processes(pg); ++dest) {
    std::sort(descriptor_requests[dest].begin(),
              descriptor_requests[dest].end());
    descriptor_requests[dest].erase(
      std::unique(descriptor_requests[dest].begin(),
                  descriptor_requests[dest].end()),
      descriptor_requests[dest].end());
  }

  // Send out all of the descriptor requests
  std::vector<std::vector<vertex_number_t> > in_descriptor_requests;
  in_descriptor_requests.resize(num_processes(pg));
  inplace_all_to_all(pg, descriptor_requests, in_descriptor_requests);

  // Reply to all of the descriptor requests
  std::vector<std::vector<local_vertex_descriptor> >
    descriptor_responses;
  descriptor_responses.resize(num_processes(pg));
  for (process_id_type dest = 0; dest < num_processes(pg); ++dest) {
    for (std::size_t i = 0; i < in_descriptor_requests[dest].size(); ++i) {
      local_vertex_descriptor v =
        index_to_vertex[in_descriptor_requests[dest][i]];
      descriptor_responses[dest].push_back(v);
    }
    in_descriptor_requests[dest].clear();
  }
  in_descriptor_requests.clear();
  inplace_all_to_all(pg, descriptor_responses);

  // Add the queued edges
  for(typename std::vector<delayed_edge_t>::iterator i
        = delayed_edges.begin(); i != delayed_edges.end(); ++i) {
    process_id_type dest = distribution(i->first.second);
    local_vertex_descriptor tgt_local;
    if (dest == id) {
      tgt_local = index_to_vertex[distribution.local(i->first.second)];
    } else {
      std::vector<vertex_number_t>& requests = descriptor_requests[dest];
      typename std::vector<vertex_number_t>::iterator pos =
        std::lower_bound(requests.begin(), requests.end(),
                         distribution.local(i->first.second));
      tgt_local = descriptor_responses[dest][pos - requests.begin()];
    }
    add_edge(i->first.first, vertex_descriptor(dest, tgt_local),
             i->second, *this);
  }
  synchronize(process_group_);
}

template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
template<typename EdgeIterator, typename VertexListS>
void
PBGL_DISTRIB_ADJLIST_TYPE::
initialize(EdgeIterator first, EdgeIterator last,
           vertices_size_type n, const base_distribution_type& distribution,
           VertexListS)
{
  using boost::parallel::inplace_all_to_all;

  typedef vertices_size_type vertex_number_t;

  typedef std::pair<vertex_descriptor, vertex_number_t> delayed_edge_t;

  process_group_type pg = process_group();
  process_id_type id = process_id(pg);

  // Vertex indices
  std::vector<local_vertex_descriptor> index_to_vertex;
  index_to_vertex.reserve(num_vertices(*this));
  BGL_FORALL_VERTICES_T(v, base(), inherited)
    index_to_vertex.push_back(v);

  // The list of edges we can't add immediately.
  std::vector<delayed_edge_t> delayed_edges;

  std::vector<std::vector<vertex_number_t> > descriptor_requests;
  descriptor_requests.resize(num_processes(pg));

  // Add all of the edges we can, up to the point where we run
  // into a descriptor we don't know.
  while (first != last) {
    if (distribution(first->first) == id) {
      if (distribution(first->second) != id) break;
      vertex_descriptor source
        (id, index_to_vertex[distribution.local(first->first)]);
      vertex_descriptor target
        (distribution(first->second),
         index_to_vertex[distribution.local(first->second)]);
      add_edge(source, target, *this);
    }
    ++first;
  }

  // Queue all of the remaining edges and determine the set of
  // descriptors we need to know about.
  while (first != last) {
    if (distribution(first->first) == id) {
      vertex_descriptor source
        (id, index_to_vertex[distribution.local(first->first)]);
      process_id_type dest = distribution(first->second);
      if (dest != id) {
        descriptor_requests[dest]
          .push_back(distribution.local(first->second));
        // Compact request list if we need to
        if (descriptor_requests[dest].size() >
              distribution.block_size(dest, n)) {
          std::sort(descriptor_requests[dest].begin(),
                    descriptor_requests[dest].end());
          descriptor_requests[dest].erase(
            std::unique(descriptor_requests[dest].begin(),
                        descriptor_requests[dest].end()),
            descriptor_requests[dest].end());
        }
      }

      // Save the edge for later
      delayed_edges.push_back(delayed_edge_t(source, first->second));
    }
    ++first;
  }

  // Compact descriptor requests
  for (process_id_type dest = 0; dest < num_processes(pg); ++dest) {
    std::sort(descriptor_requests[dest].begin(),
              descriptor_requests[dest].end());
    descriptor_requests[dest].erase(
      std::unique(descriptor_requests[dest].begin(),
                  descriptor_requests[dest].end()),
      descriptor_requests[dest].end());
  }

  // Send out all of the descriptor requests
  std::vector<std::vector<vertex_number_t> > in_descriptor_requests;
  in_descriptor_requests.resize(num_processes(pg));
  inplace_all_to_all(pg, descriptor_requests, in_descriptor_requests);

  // Reply to all of the descriptor requests
  std::vector<std::vector<local_vertex_descriptor> >
    descriptor_responses;
  descriptor_responses.resize(num_processes(pg));
  for (process_id_type dest = 0; dest < num_processes(pg); ++dest) {
    for (std::size_t i = 0; i < in_descriptor_requests[dest].size(); ++i) {
      local_vertex_descriptor v =
        index_to_vertex[in_descriptor_requests[dest][i]];
      descriptor_responses[dest].push_back(v);
    }
    in_descriptor_requests[dest].clear();
  }
  in_descriptor_requests.clear();
  inplace_all_to_all(pg, descriptor_responses);

  // Add the queued edges
  for(typename std::vector<delayed_edge_t>::iterator i
        = delayed_edges.begin(); i != delayed_edges.end(); ++i) {
    process_id_type dest = distribution(i->second);
    local_vertex_descriptor tgt_local;
    if (dest == id) {
      tgt_local = index_to_vertex[distribution.local(i->second)];
    } else {
      std::vector<vertex_number_t>& requests = descriptor_requests[dest];
      typename std::vector<vertex_number_t>::iterator pos =
        std::lower_bound(requests.begin(), requests.end(),
                         distribution.local(i->second));
      tgt_local = descriptor_responses[dest][pos - requests.begin()];
    }
    add_edge(i->first, vertex_descriptor(dest, tgt_local), *this);
  }
  synchronize(process_group_);
}

}   // end namespace boost

#endif // BOOST_GRAPH_DISTRIBUTED_ADJLIST_INITIALIZE_HPP
