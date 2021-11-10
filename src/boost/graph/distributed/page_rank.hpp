// Copyright (C) 2004-2006 The Trustees of Indiana University.
// Copyright (C) 2002 Brad King and Douglas Gregor

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
//           Brian Barrett
#ifndef BOOST_PARALLEL_GRAPH_PAGE_RANK_HPP
#define BOOST_PARALLEL_GRAPH_PAGE_RANK_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/assert.hpp>
#include <boost/graph/overloading.hpp>
#include <boost/graph/page_rank.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/property_map/parallel/caching_property_map.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/graph/parallel/container_traits.hpp>

// #define WANT_MPI_ONESIDED 1

namespace boost { namespace graph { namespace distributed {

namespace detail {
#ifdef WANT_MPI_ONESIDED
  template<typename Graph, typename RankMap, typename owner_map_t>
  void page_rank_step(const Graph& g, RankMap from_rank, MPI_Win to_win, 
                      typename property_traits<RankMap>::value_type damping,
                      owner_map_t owner)
  {
    typedef typename property_traits<RankMap>::value_type rank_type;
    int me, ret;
    MPI_Comm_rank(MPI_COMM_WORLD, &me);

    // MPI_Accumulate is not required to store the value of the data
    // being sent, only the address.  The value of the memory location
    // must not change until the end of the access epoch, meaning the
    // call to MPI_Fence.  We therefore store the updated value back
    // into the from_rank map before the accumulate rather than using
    // a temporary.  We're going to reset the values in the from_rank
    // before the end of page_rank_step() anyway, so this isn't a huge
    // deal.  But MPI-2 One-sided is an abomination.
    BGL_FORALL_VERTICES_T(u, g, Graph) {
      put(from_rank, u, (damping * get(from_rank, u) / out_degree(u, g)));
      BGL_FORALL_ADJ_T(u, v, g, Graph) {
        ret = MPI_Accumulate(&(from_rank[u]),
                             1, MPI_DOUBLE,
                             get(owner, v), local(v), 
                             1, MPI_DOUBLE, MPI_SUM, to_win);
        BOOST_ASSERT(MPI_SUCCESS == ret);
      }
    }
    MPI_Win_fence(0, to_win);

    // Set new rank maps for the other map.  Do this now to get around
    // the stupid synchronization rules of MPI-2 One-sided
    BGL_FORALL_VERTICES_T(v, g, Graph) put(from_rank, v, rank_type(1 - damping));
  }
#endif

  template<typename T>
  struct rank_accumulate_reducer {
    BOOST_STATIC_CONSTANT(bool, non_default_resolver = true);

    template<typename K>
    T operator()(const K&) const { return T(0); }

    template<typename K>
    T operator()(const K&, const T& x, const T& y) const { return x + y; }
  };
} // end namespace detail

template<typename Graph, typename RankMap, typename Done, typename RankMap2>
void
page_rank_impl(const Graph& g, RankMap rank_map, Done done, 
               typename property_traits<RankMap>::value_type damping,
               typename graph_traits<Graph>::vertices_size_type n,
               RankMap2 rank_map2)
{
  typedef typename property_traits<RankMap>::value_type rank_type;

  int me;
  MPI_Comm_rank(MPI_COMM_WORLD, &me);

  typename property_map<Graph, vertex_owner_t>::const_type
    owner = get(vertex_owner, g);
  (void)owner;

  typedef typename boost::graph::parallel::process_group_type<Graph>
    ::type process_group_type;
  typedef typename process_group_type::process_id_type process_id_type;

  process_group_type pg = process_group(g);
  process_id_type id = process_id(pg);

  BOOST_ASSERT(me == id);

  rank_type initial_rank = rank_type(rank_type(1) / n);
  BGL_FORALL_VERTICES_T(v, g, Graph) put(rank_map, v, initial_rank);

#ifdef WANT_MPI_ONESIDED

  BOOST_ASSERT(sizeof(rank_type) == sizeof(double));

  bool to_map_2 = true;
  MPI_Win win, win2;

  MPI_Win_create(&(rank_map[*(vertices(g).first)]), 
                 sizeof(double) * num_vertices(g),
                 sizeof(double), 
                 MPI_INFO_NULL, MPI_COMM_WORLD, &win);
  MPI_Win_set_name(win, "rank_map_win");
  MPI_Win_create(&(rank_map2[*(vertices(g).first)]), 
                 sizeof(double) * num_vertices(g),
                 sizeof(double), 
                 MPI_INFO_NULL, MPI_COMM_WORLD, &win2);
  MPI_Win_set_name(win, "rank_map2_win");

  // set initial rank maps for the first iteration...
  BGL_FORALL_VERTICES_T(v, g, Graph) put(rank_map2, v, rank_type(1 - damping));

  MPI_Win_fence(0, win);
  MPI_Win_fence(0, win2);

  while ((to_map_2 && !done(rank_map, g)) ||
         (!to_map_2 && !done(rank_map2, g))) {
    if (to_map_2) {
      graph::distributed::detail::page_rank_step(g, rank_map, win2, damping, owner);
      to_map_2 = false;
    } else {
      graph::distributed::detail::page_rank_step(g, rank_map2, win, damping, owner);
      to_map_2 = true;
    }
  }
  synchronize(boost::graph::parallel::process_group(g));

  MPI_Win_free(&win);
  MPI_Win_free(&win2);

#else  
  // The ranks accumulate after each step.
  rank_map.set_reduce(detail::rank_accumulate_reducer<rank_type>());
  rank_map2.set_reduce(detail::rank_accumulate_reducer<rank_type>());
  rank_map.set_consistency_model(boost::parallel::cm_flush | boost::parallel::cm_reset);
  rank_map2.set_consistency_model(boost::parallel::cm_flush | boost::parallel::cm_reset);

  bool to_map_2 = true;
  while ((to_map_2 && !done(rank_map, g)) ||
         (!to_map_2 && !done(rank_map2, g))) {
    /**
     * PageRank can implemented slightly more efficiently on a
     * bidirectional graph than on an incidence graph. However,
     * distributed PageRank requires that we have the rank of the
     * source vertex available locally, so we force the incidence
     * graph implementation, which pushes rank from source to
     * target.
     */
    typedef incidence_graph_tag category;
    if (to_map_2) {
      graph::detail::page_rank_step(g, rank_map, rank_map2, damping,
                                    category());
      to_map_2 = false;
    } else {
      graph::detail::page_rank_step(g, rank_map2, rank_map, damping,
                                    category());
      to_map_2 = true;
    }
    using boost::graph::parallel::process_group;
    synchronize(process_group(g));
  }

  rank_map.reset();
#endif
      
  if (!to_map_2)
    BGL_FORALL_VERTICES_T(v, g, Graph) put(rank_map, v, get(rank_map2, v));
}

template<typename Graph, typename RankMap, typename Done, typename RankMap2>
void
page_rank(const Graph& g, RankMap rank_map, Done done, 
          typename property_traits<RankMap>::value_type damping,
          typename graph_traits<Graph>::vertices_size_type n,
          RankMap2 rank_map2
          BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph, distributed_graph_tag)) 
{
  (page_rank_impl)(g, rank_map, done, damping, n, rank_map2);
}

template<typename MutableGraph>
void
remove_dangling_links(MutableGraph& g
                      BOOST_GRAPH_ENABLE_IF_MODELS_PARM(MutableGraph, 
                                                        distributed_graph_tag))
{
  typename graph_traits<MutableGraph>::vertices_size_type old_n;
  do {
    old_n = num_vertices(g);

    typename graph_traits<MutableGraph>::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; /* in loop */) {
      typename graph_traits<MutableGraph>::vertex_descriptor v = *vi++;
      if (out_degree(v, g) == 0) {
        clear_vertex(v, g);
        remove_vertex(v, g);
      }
    }
  } while (num_vertices(g) < old_n);
}

} // end namespace distributed

using distributed::page_rank;
using distributed::remove_dangling_links;

} } // end namespace boost::graph

#endif // BOOST_PARALLEL_GRAPH_PAGE_RANK_HPP
