// Copyright (C) 2004-2008 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Nick Edmonds
//           Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DISTRIBUTED_SCC_HPP
#define BOOST_GRAPH_DISTRIBUTED_SCC_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

// #define PBGL_SCC_DEBUG

#include <boost/assert.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/property_map/parallel/caching_property_map.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/graph/parallel/process_group.hpp>
#include <boost/graph/distributed/queue.hpp>
#include <boost/graph/distributed/filtered_graph.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/overloading.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/distributed/local_subgraph.hpp>
#include <boost/graph/parallel/properties.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/distributed/reverse_graph.hpp>
#include <boost/optional.hpp>
#include <boost/graph/distributed/detail/filtered_queue.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#ifdef PBGL_SCC_DEBUG
  #include <iostream>
  #include <cstdlib>
  #include <iomanip>
  #include <sys/time.h>
  #include <boost/graph/distributed/graphviz.hpp> // for ostringstream
#endif
#include <vector>
#include <map>
#include <boost/graph/parallel/container_traits.hpp>

#ifdef PBGL_SCC_DEBUG
#  include <boost/graph/accounting.hpp>
#endif /* PBGL_SCC_DEBUG */

// If your graph is likely to have large numbers of small strongly connected
// components then running the sequential SCC algorithm on the local subgraph
// and filtering components with no remote edges may increase performance
// #define FILTER_LOCAL_COMPONENTS

namespace boost { namespace graph { namespace distributed { namespace detail {

template<typename vertex_descriptor>
struct v_sets{
  std::vector<vertex_descriptor> pred, succ, intersect, ps_union;
};

/* Serialize vertex set */
template<typename Graph>
void
marshal_set( std::vector<std::vector<typename graph_traits<Graph>::vertex_descriptor> > in,
             std::vector<typename graph_traits<Graph>::vertex_descriptor>& out )
{
  for( std::size_t i = 0; i < in.size(); ++i ) {
    out.insert( out.end(), graph_traits<Graph>::null_vertex() );
    out.insert( out.end(), in[i].begin(), in[i].end() );
  }
}

/* Un-serialize vertex set */
template<typename Graph>
void
unmarshal_set( std::vector<typename graph_traits<Graph>::vertex_descriptor> in,
               std::vector<std::vector<typename graph_traits<Graph>::vertex_descriptor> >& out )
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  while( !in.empty() ) {
    typename std::vector<vertex_descriptor>::iterator end 
      = std::find( in.begin(), in.end(), graph_traits<Graph>::null_vertex() );

    if( end == in.begin() )
      in.erase( in.begin() );
    else {
      out.push_back(std::vector<vertex_descriptor>());
      out[out.size() - 1].insert( out[out.size() - 1].end(), in.begin(), end );
      in.erase( in.begin(), end );
    }
  }
}

/* Determine if vertex is in subset */
template <typename Set>
struct in_subset {
  in_subset() : m_s(0) { }
  in_subset(const Set& s) : m_s(&s) { }

  template <typename Elt>
  bool operator()(const Elt& x) const {
    return ((*m_s).find(x) != (*m_s).end());
  }

private:
  const Set* m_s;
};

template<typename T>
struct vertex_identity_property_map
  : public boost::put_get_helper<T, vertex_identity_property_map<T> >
{
  typedef T key_type;
  typedef T value_type;
  typedef T reference;
  typedef boost::readable_property_map_tag category;

  inline value_type operator[](const key_type& v) const { return v; }
  inline void clear() { }
};

template <typename T>
inline void synchronize( vertex_identity_property_map<T> & ) { }

/* BFS visitor for SCC */
template<typename Graph, typename SourceMap>
struct scc_discovery_visitor : bfs_visitor<>
{
  scc_discovery_visitor(SourceMap& sourceM)
    : sourceM(sourceM) {}

  template<typename Edge>
  void tree_edge(Edge e, const Graph& g)
  {
    put(sourceM, target(e,g), get(sourceM, source(e,g)));
  }

 private:
  SourceMap& sourceM;
};

} } } } /* End namespace boost::graph::distributed::detail */

namespace boost { namespace graph { namespace distributed {
    enum fhp_message_tags { fhp_edges_size_msg, fhp_add_edges_msg, fhp_pred_size_msg, 
                            fhp_pred_msg, fhp_succ_size_msg, fhp_succ_msg };

    template<typename Graph, typename ReverseGraph,
             typename VertexComponentMap, typename IsoMapFR, typename IsoMapRF,
             typename VertexIndexMap>
    void
    fleischer_hendrickson_pinar_strong_components(const Graph& g,
                                                  VertexComponentMap c,
                                                  const ReverseGraph& gr,
                                                  IsoMapFR fr, IsoMapRF rf,
                                                  VertexIndexMap vertex_index_map)
    {
      typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
      typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
      typedef typename graph_traits<ReverseGraph>::vertex_iterator rev_vertex_iterator;
      typedef typename graph_traits<ReverseGraph>::vertex_descriptor rev_vertex_descriptor;
      typedef typename boost::graph::parallel::process_group_type<Graph>::type
        process_group_type;
      typedef typename process_group_type::process_id_type process_id_type;
      typedef iterator_property_map<typename std::vector<vertex_descriptor>::iterator,
                                    VertexIndexMap> ParentMap;
      typedef iterator_property_map<typename std::vector<default_color_type>::iterator,
                                    VertexIndexMap> ColorMap;
      typedef iterator_property_map<typename std::vector<vertex_descriptor>::iterator,
                                    VertexIndexMap> Rev_ParentMap;
      typedef std::vector<std::pair<vertex_descriptor, vertex_descriptor> > VertexPairVec;

      typedef typename property_map<Graph, vertex_owner_t>::const_type
        OwnerMap;

      OwnerMap owner = get(vertex_owner, g);

      using boost::graph::parallel::process_group;
      process_group_type pg = process_group(g);
      process_id_type id = process_id(pg);
      int num_procs = num_processes(pg);
      int n = 0;

      int my_n = num_vertices(g);
      all_reduce(pg, &my_n, &my_n+1, &n, std::plus<int>());

      //
      // Initialization
      //

#ifdef PBGL_SCC_DEBUG
  accounting::time_type start = accounting::get_time();
#endif

      vertex_iterator vstart, vend;
      rev_vertex_iterator rev_vstart, rev_vend;
      std::vector<std::vector<vertex_descriptor> > vertex_sets, new_vertex_sets;

      vertex_sets.push_back(std::vector<vertex_descriptor>());

      // Remove vertices that do not have at least one in edge and one out edge
      new_vertex_sets.push_back(std::vector<vertex_descriptor>());
      for( boost::tie(vstart, vend) = vertices(g); vstart != vend; vstart++ )
        if( out_degree( get(fr, *vstart), gr) > 0 && out_degree(*vstart, g) > 0 )
          new_vertex_sets[0].push_back( *vstart );

      // Perform sequential SCC on local subgraph, filter all components with external
      // edges, mark remaining components and remove them from vertex_sets
#ifdef FILTER_LOCAL_COMPONENTS  
      // This doesn't actually speed up SCC in connected graphs it seems, but it does work
      // and may help in the case where there are lots of small strong components.
      {
        local_subgraph<const Graph> ls(g);
        typedef typename property_map<local_subgraph<const Graph>, vertex_index_t>::type
          local_index_map_type;
        local_index_map_type local_index = get(vertex_index, ls);

        std::vector<int> ls_components_vec(num_vertices(ls));
        typedef iterator_property_map<std::vector<int>::iterator, local_index_map_type>
          ls_components_map_type;
        ls_components_map_type ls_component(ls_components_vec.begin(), local_index);
        int num_comp = boost::strong_components(ls, ls_component);

        // Create map of components
        std::map<int, std::vector<vertex_descriptor> > local_comp_map;
        typedef typename graph_traits<local_subgraph<const Graph> >::vertex_iterator ls_vertex_iterator;
        ls_vertex_iterator vstart, vend;
        for( boost::tie(vstart,vend) = vertices(ls); vstart != vend; vstart++ )
          local_comp_map[get(ls_component, *vstart)].push_back( *vstart );

        // Filter components that have no non-local edges
        typedef typename graph_traits<Graph>::adjacency_iterator adjacency_iterator;
        typedef typename graph_traits<ReverseGraph>::adjacency_iterator rev_adjacency_iterator;
        adjacency_iterator abegin, aend;
        rev_adjacency_iterator rev_abegin, rev_aend;
        for( std::size_t i = 0; i < num_comp; ++i ) {
          bool local = true;
          for( std::size_t j = 0; j < local_comp_map[i].size(); j++ ) {
            for( boost::tie(abegin,aend) = adjacent_vertices(local_comp_map[i][j], g);
                 abegin != aend; abegin++ )
              if( get(owner, *abegin) != id ) {
                local = false;
                break;
              }

            if( local )
              for( boost::tie(rev_abegin,rev_aend) = adjacent_vertices(get(fr, local_comp_map[i][j]), gr);
                   rev_abegin != rev_aend; rev_abegin++ )
                if( get(owner, *rev_abegin) != id ) {
                  local = false;
                  break;
                }

            if( !local ) break;
          }

          if( local ) // Mark and remove from new_vertex_sets
            for( std::size_t j = 0; j < local_comp_map[i].size(); j++ ) {
              put( c, local_comp_map[i][j], local_comp_map[i][0] );
              typename std::vector<vertex_descriptor>::iterator pos =
                std::find(new_vertex_sets[0].begin(), new_vertex_sets[0].end(), local_comp_map[i][j]);
              if( pos != new_vertex_sets[0].end() )
                new_vertex_sets[0].erase(pos);
            }
        }
      }
#endif // FILTER_LOCAL_COMPONENTS

      all_gather( pg, new_vertex_sets[0].begin(), new_vertex_sets[0].end(), vertex_sets[0] );
      new_vertex_sets.clear();

#ifdef PBGL_SCC_DEBUG
  accounting::time_type end = accounting::get_time();
  if(id == 0)
    std::cerr << "Trim local SCCs time = " << accounting::print_time(end - start) << " seconds.\n";
#endif

      if( vertex_sets[0].empty() ) return;

      //
      // Recursively determine SCCs
      //

#ifdef PBGL_SCC_DEBUG
  int iterations = 0;
#endif

      // Only need to be able to map starting vertices for BFS from now on
      fr.clear();

      do {

#ifdef PBGL_SCC_DEBUG
  if(id == 0) {
    printf("\n\nIteration %d:\n\n", iterations++);

    if( iterations > 1 ) {
      end = accounting::get_time();
      std::cerr << "Running main loop destructors time = " << accounting::print_time(end - start) << " seconds.\n";
    }

    start = accounting::get_time();
  }
#endif

        // Get forward->reverse mappings for BFS start vertices
       for (std::size_t i = 0; i < vertex_sets.size(); ++i)
          get(fr, vertex_sets[i][0]);
        synchronize(fr);

        // Determine local vertices to start BFS from
        std::vector<vertex_descriptor> local_start;
        for( std::size_t i = id; i < vertex_sets.size(); i += num_procs )
          local_start.push_back(vertex_sets[i][0]);

        if( local_start.empty() )
          local_start.push_back(vertex_sets[0][0]);


        // Make filtered graphs
        typedef std::set<vertex_descriptor> VertexSet;
        typedef std::set<rev_vertex_descriptor> Rev_VertexSet;

        VertexSet filter_set_g;
        Rev_VertexSet filter_set_gr;
        typename VertexSet::iterator fs;

        int active_vertices = 0;
        for (std::size_t i = 0; i < vertex_sets.size(); i++)
          active_vertices += vertex_sets[i].size();

        // This is a completely random bound
        if ( active_vertices < 0.05*n ) {
          // TODO: This set insertion is ridiculously inefficient, make it an in-place-merge?
          for (std::size_t i = 0; i < vertex_sets.size(); i++)
            filter_set_g.insert(vertex_sets[i].begin(), vertex_sets[i].end());

          for (fs = filter_set_g.begin(); fs != filter_set_g.end(); ++fs )
            filter_set_gr.insert(get(fr, *fs));
        }

        filtered_graph<const Graph, keep_all, detail::in_subset<VertexSet> >
          fg(g, keep_all(), detail::in_subset<VertexSet>(filter_set_g));

        filtered_graph<const ReverseGraph, keep_all, detail::in_subset<VertexSet> >
          fgr(gr, keep_all(), detail::in_subset<VertexSet>(filter_set_gr));

        // Add additional starting vertices to BFS queue
        typedef filtered_queue<queue<vertex_descriptor>, boost::detail::has_not_been_seen<VertexIndexMap> >
          local_queue_type;
        typedef boost::graph::distributed::distributed_queue<process_group_type, OwnerMap, local_queue_type>
          queue_t;

        typedef typename property_map<ReverseGraph, vertex_owner_t>::const_type
          RevOwnerMap;

        typedef filtered_queue<queue<rev_vertex_descriptor>, boost::detail::has_not_been_seen<VertexIndexMap> >
          rev_local_queue_type;
        typedef boost::graph::distributed::distributed_queue<process_group_type, RevOwnerMap, rev_local_queue_type>
          rev_queue_t;

        queue_t Q(process_group(g),
                  owner,
                  make_filtered_queue(queue<vertex_descriptor>(),
                                      boost::detail::has_not_been_seen<VertexIndexMap>
                                      (num_vertices(g), vertex_index_map)),
                  false);

        rev_queue_t Qr(process_group(gr),
                       get(vertex_owner, gr),
                       make_filtered_queue(queue<rev_vertex_descriptor>(),
                                           boost::detail::has_not_been_seen<VertexIndexMap>
                                           (num_vertices(gr), vertex_index_map)),
                       false);

        for( std::size_t i = 1; i < local_start.size(); ++i ) {
          Q.push(local_start[i]);
          Qr.push(get(fr, local_start[i]));
        }

#ifdef PBGL_SCC_DEBUG
  end = accounting::get_time();
  if(id == 0)
    std::cerr << "  Initialize BFS time = " << accounting::print_time(end - start) << " seconds.\n";
  start = accounting::get_time();
#endif

#ifdef PBGL_SCC_DEBUG
  accounting::time_type start2 = accounting::get_time();
#endif

        // Forward BFS
        std::vector<default_color_type> color_map_s(num_vertices(g));
        ColorMap color_map(color_map_s.begin(), vertex_index_map);
        std::vector<vertex_descriptor> succ_map_s(num_vertices(g), graph_traits<Graph>::null_vertex());
        ParentMap succ_map(succ_map_s.begin(), vertex_index_map);

        for( std::size_t i = 0; i < vertex_sets.size(); ++i )
          put(succ_map, vertex_sets[i][0], vertex_sets[i][0]);

#ifdef PBGL_SCC_DEBUG
  accounting::time_type end2 = accounting::get_time();
  if(id == 0)
    std::cerr << "  Initialize forward BFS time = " << accounting::print_time(end2 - start2) << " seconds.\n";
#endif

        if (active_vertices < 0.05*n)
          breadth_first_search(fg, local_start[0], Q,
                               detail::scc_discovery_visitor<filtered_graph<const Graph, keep_all,
                                                                            detail::in_subset<VertexSet> >, ParentMap>
                               (succ_map),
                               color_map);
        else
          breadth_first_search(g, local_start[0], Q,
                               detail::scc_discovery_visitor<const Graph, ParentMap>(succ_map),
                               color_map);

#ifdef PBGL_SCC_DEBUG
  start2 = accounting::get_time();
#endif

        // Reverse BFS
        color_map.clear(); // reuse color map since g and gr have same vertex index
        std::vector<vertex_descriptor> pred_map_s(num_vertices(gr), graph_traits<Graph>::null_vertex());
        Rev_ParentMap pred_map(pred_map_s.begin(), vertex_index_map);

        for( std::size_t i = 0; i < vertex_sets.size(); ++i )
          put(pred_map, get(fr, vertex_sets[i][0]), vertex_sets[i][0]);

#ifdef PBGL_SCC_DEBUG
  end2 = accounting::get_time();
  if(id == 0)
    std::cerr << "  Initialize reverse BFS time = " << accounting::print_time(end2 - start2) << " seconds.\n";
#endif

        if (active_vertices < 0.05*n)
          breadth_first_search(fgr, get(fr, local_start[0]),
                               Qr,
                               detail::scc_discovery_visitor<filtered_graph<const ReverseGraph, keep_all,
                                                                            detail::in_subset<Rev_VertexSet> >, Rev_ParentMap>
                               (pred_map),
                               color_map);
        else
          breadth_first_search(gr, get(fr, local_start[0]),
                               Qr,
                               detail::scc_discovery_visitor<const ReverseGraph, Rev_ParentMap>(pred_map),
                               color_map);

#ifdef PBGL_SCC_DEBUG
  end = accounting::get_time();
  if(id == 0)
    std::cerr << "  Perform forward and reverse BFS time = " << accounting::print_time(end - start) << " seconds.\n";
  start = accounting::get_time();
#endif

        // Send predecessors and successors discovered by this proc to the proc responsible for
        // this BFS tree
        typedef struct detail::v_sets<vertex_descriptor> Vsets;
        std::map<vertex_descriptor, Vsets> set_map;

        std::map<vertex_descriptor, int> dest_map;

        std::vector<VertexPairVec> successors(num_procs);
        std::vector<VertexPairVec> predecessors(num_procs);

        // Calculate destinations for messages
        for (std::size_t i = 0; i < vertex_sets.size(); ++i)
          dest_map[vertex_sets[i][0]] = i % num_procs;

        for( boost::tie(vstart, vend) = vertices(g); vstart != vend; vstart++ ) {
          vertex_descriptor v = get(succ_map, *vstart);
          if( v != graph_traits<Graph>::null_vertex() ) {
            if (dest_map[v] == id)
              set_map[v].succ.push_back(*vstart);
            else
              successors[dest_map[v]].push_back( std::make_pair(v, *vstart) );
          }
        }

        for( boost::tie(rev_vstart, rev_vend) = vertices(gr); rev_vstart != rev_vend; rev_vstart++ ) {
          vertex_descriptor v = get(pred_map, *rev_vstart);
          if( v != graph_traits<Graph>::null_vertex() ) {
            if (dest_map[v] == id)
              set_map[v].pred.push_back(get(rf, *rev_vstart));
            else
              predecessors[dest_map[v]].push_back( std::make_pair(v, get(rf, *rev_vstart)) );
          }
        }

        // Send predecessor and successor messages
        for (process_id_type i = 0; i < num_procs; ++i) {
          if (!successors[i].empty()) {
            send(pg, i, fhp_succ_size_msg, successors[i].size());
            send(pg, i, fhp_succ_msg, &successors[i][0], successors[i].size());
          }
          if (!predecessors[i].empty()) {
            send(pg, i, fhp_pred_size_msg, predecessors[i].size());
            send(pg, i, fhp_pred_msg, &predecessors[i][0], predecessors[i].size());
          }
        }
        synchronize(pg);

        // Receive predecessor and successor messages and handle them
        while (optional<std::pair<process_id_type, int> > m = probe(pg)) {
          BOOST_ASSERT(m->second == fhp_succ_size_msg || m->second == fhp_pred_size_msg);
          std::size_t num_requests;
          receive(pg, m->first, m->second, num_requests);
          VertexPairVec requests(num_requests);
          if (m->second == fhp_succ_size_msg) {
            receive(pg, m->first, fhp_succ_msg, &requests[0],
                    num_requests);

            std::map<vertex_descriptor, int> added;
            for (std::size_t i = 0; i < requests.size(); ++i) {
              set_map[requests[i].first].succ.push_back(requests[i].second);
              added[requests[i].first]++;
            }

            // If order of vertex traversal in vertices() is std::less<vertex_descriptor>,
            // then the successor sets will be in order
            for (std::size_t i = 0; i < local_start.size(); ++i)
              if (added[local_start[i]] > 0)
                  std::inplace_merge(set_map[local_start[i]].succ.begin(),
                                     set_map[local_start[i]].succ.end() - added[local_start[i]],
                                     set_map[local_start[i]].succ.end(),
                                     std::less<vertex_descriptor>());

          } else {
            receive(pg, m->first, fhp_pred_msg, &requests[0],
                    num_requests);

            std::map<vertex_descriptor, int> added;
            for (std::size_t i = 0; i < requests.size(); ++i) {
              set_map[requests[i].first].pred.push_back(requests[i].second);
              added[requests[i].first]++;
            }

            if (boost::is_same<detail::vertex_identity_property_map<vertex_descriptor>, IsoMapRF>::value)
              for (std::size_t i = 0; i < local_start.size(); ++i)
                if (added[local_start[i]] > 0)
                  std::inplace_merge(set_map[local_start[i]].pred.begin(),
                                     set_map[local_start[i]].pred.end() - added[local_start[i]],
                                     set_map[local_start[i]].pred.end(),
                                     std::less<vertex_descriptor>());
          }
        }

#ifdef PBGL_SCC_DEBUG
  end = accounting::get_time();
  if(id == 0)
    std::cerr << "  All gather successors and predecessors time = " << accounting::print_time(end - start) << " seconds.\n";
  start = accounting::get_time();
#endif

        //
        // Filter predecessor and successor sets and perform set arithmetic
        //
        new_vertex_sets.clear();

        if( std::size_t(id) < vertex_sets.size() ) { //If this proc has one or more unique starting points

          for( std::size_t i = 0; i < local_start.size(); ++i ) {

            vertex_descriptor v = local_start[i];

            // Replace this sort with an in-place merges during receive step if possible
            if (!boost::is_same<detail::vertex_identity_property_map<vertex_descriptor>, IsoMapRF>::value) 
              std::sort(set_map[v].pred.begin(), set_map[v].pred.end(), std::less<vertex_descriptor>());

            // Limit predecessor and successor sets to members of the original set
            std::vector<vertex_descriptor> temp;

            std::set_intersection( vertex_sets[id + i*num_procs].begin(), vertex_sets[id + i*num_procs].end(),
                                   set_map[v].pred.begin(), set_map[v].pred.end(),
                                   back_inserter(temp),
                                   std::less<vertex_descriptor>());
            set_map[v].pred.clear();
            std::swap(set_map[v].pred, temp);

            std::set_intersection( vertex_sets[id + i*num_procs].begin(), vertex_sets[id + i*num_procs].end(),
                                   set_map[v].succ.begin(), set_map[v].succ.end(),
                                   back_inserter(temp),
                                   std::less<vertex_descriptor>());
            set_map[v].succ.clear();
            std::swap(set_map[v].succ, temp);

            // Intersection(pred, succ)
            std::set_intersection(set_map[v].pred.begin(), set_map[v].pred.end(),
                                    set_map[v].succ.begin(), set_map[v].succ.end(),
                                    back_inserter(set_map[v].intersect),
                                    std::less<vertex_descriptor>());

            // Union(pred, succ)
            std::set_union(set_map[v].pred.begin(), set_map[v].pred.end(),
                           set_map[v].succ.begin(), set_map[v].succ.end(),
                           back_inserter(set_map[v].ps_union),
                           std::less<vertex_descriptor>());

            new_vertex_sets.push_back(std::vector<vertex_descriptor>());
            // Original set - Union(pred, succ)
            std::set_difference(vertex_sets[id + i*num_procs].begin(), vertex_sets[id + i*num_procs].end(),
                                set_map[v].ps_union.begin(), set_map[v].ps_union.end(),
                                back_inserter(new_vertex_sets[new_vertex_sets.size() - 1]),
                                std::less<vertex_descriptor>());

            set_map[v].ps_union.clear();

            new_vertex_sets.push_back(std::vector<vertex_descriptor>());
            // Pred - Intersect(pred, succ)
            std::set_difference(set_map[v].pred.begin(), set_map[v].pred.end(),
                                set_map[v].intersect.begin(), set_map[v].intersect.end(),
                                back_inserter(new_vertex_sets[new_vertex_sets.size() - 1]),
                                std::less<vertex_descriptor>());

            set_map[v].pred.clear();

            new_vertex_sets.push_back(std::vector<vertex_descriptor>());
            // Succ - Intersect(pred, succ)
            std::set_difference(set_map[v].succ.begin(), set_map[v].succ.end(),
                                set_map[v].intersect.begin(), set_map[v].intersect.end(),
                                back_inserter(new_vertex_sets[new_vertex_sets.size() - 1]),
                                std::less<vertex_descriptor>());

            set_map[v].succ.clear();

            // Label SCC just identified with the 'first' vertex in that SCC
            for( std::size_t j = 0; j < set_map[v].intersect.size(); j++ )
              put(c, set_map[v].intersect[j], set_map[v].intersect[0]);

            set_map[v].intersect.clear();
          }
        }

#ifdef PBGL_SCC_DEBUG
  end = accounting::get_time();
  if(id == 0)
    std::cerr << "  Perform set arithemetic time = " << accounting::print_time(end - start) << " seconds.\n";
  start = accounting::get_time();
#endif

        // Remove sets of size 1 from new_vertex_sets
        typename std::vector<std::vector<vertex_descriptor> >::iterator vviter;
        for( vviter = new_vertex_sets.begin(); vviter != new_vertex_sets.end(); /*in loop*/ )
          if( (*vviter).size() < 2 )
            vviter = new_vertex_sets.erase( vviter );
          else
            vviter++;

        // All gather new sets and recur (gotta marshal and unmarshal sets first)
        vertex_sets.clear();
        std::vector<vertex_descriptor> serial_sets, all_serial_sets;
        detail::marshal_set<Graph>( new_vertex_sets, serial_sets );
        all_gather( pg, serial_sets.begin(), serial_sets.end(), all_serial_sets );
        detail::unmarshal_set<Graph>( all_serial_sets, vertex_sets );

#ifdef PBGL_SCC_DEBUG
  end = accounting::get_time();
  if(id == 0) {
    std::cerr << "  Serialize and gather new vertex sets time = " << accounting::print_time(end - start) << " seconds.\n\n\n";

    printf("Vertex sets: %d\n", (int)vertex_sets.size() );
    for( std::size_t i = 0; i < vertex_sets.size(); ++i )
      printf("  %d: %d\n", i, (int)vertex_sets[i].size() );
  }
  start = accounting::get_time();
#endif

        // HACK!?!  --  This would be more properly implemented as a topological sort
        // Remove vertices without an edge to another vertex in the set and an edge from another
        // vertex in the set
       typedef typename graph_traits<Graph>::out_edge_iterator out_edge_iterator;
       out_edge_iterator estart, eend;
       typedef typename graph_traits<ReverseGraph>::out_edge_iterator r_out_edge_iterator;
       r_out_edge_iterator restart, reend;
       for (std::size_t i = 0; i < vertex_sets.size(); ++i) {
         std::vector<vertex_descriptor> new_set;
         for (std::size_t j = 0; j < vertex_sets[i].size(); ++j) {
           vertex_descriptor v = vertex_sets[i][j];
           if (get(owner, v) == id) {
             boost::tie(estart, eend) = out_edges(v, g);
             while (estart != eend && find(vertex_sets[i].begin(), vertex_sets[i].end(),
                                           target(*estart,g)) == vertex_sets[i].end()) estart++;
             if (estart != eend) {
               boost::tie(restart, reend) = out_edges(get(fr, v), gr);
               while (restart != reend && find(vertex_sets[i].begin(), vertex_sets[i].end(),
                                               get(rf, target(*restart,gr))) == vertex_sets[i].end()) restart++;
               if (restart != reend)
                 new_set.push_back(v);
             }
           }
         }
         vertex_sets[i].clear();
         all_gather(pg, new_set.begin(), new_set.end(), vertex_sets[i]);
         std::sort(vertex_sets[i].begin(), vertex_sets[i].end(), std::less<vertex_descriptor>());
       }
#ifdef PBGL_SCC_DEBUG
  end = accounting::get_time();
  if(id == 0)
    std::cerr << "  Trim vertex sets time = " << accounting::print_time(end - start) << " seconds.\n\n\n";
  start = accounting::get_time();
#endif

      } while ( !vertex_sets.empty() );


      // Label vertices not in a SCC as their own SCC
      for( boost::tie(vstart, vend) = vertices(g); vstart != vend; vstart++ )
        if( get(c, *vstart) == graph_traits<Graph>::null_vertex() )
          put(c, *vstart, *vstart);

      synchronize(c);
    }

    template<typename Graph, typename ReverseGraph, typename IsoMap>
    void
    build_reverse_graph( const Graph& g, ReverseGraph& gr, IsoMap& fr, IsoMap& rf )
    {
      typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
      typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
      typedef typename graph_traits<Graph>::out_edge_iterator out_edge_iterator;
      typedef typename boost::graph::parallel::process_group_type<Graph>::type process_group_type;
      typedef typename process_group_type::process_id_type process_id_type;
      typedef std::vector<std::pair<vertex_descriptor, vertex_descriptor> > VertexPairVec;

      typename property_map<Graph, vertex_owner_t>::const_type
        owner = get(vertex_owner, g);

      process_group_type pg = process_group(g);
      process_id_type id = process_id(pg);

      int n;
      vertex_iterator vstart, vend;
      int num_procs = num_processes(pg);

      vertex_descriptor v;
      out_edge_iterator oestart, oeend;
      for( boost::tie(vstart, vend) = vertices(g); vstart != vend; vstart++ )
        {
          v = add_vertex(gr);
          put(fr, *vstart, v);
          put(rf, v, *vstart);
        }

      gr.distribution() = g.distribution();

      int my_n = num_vertices(g);
      all_reduce(pg, &my_n, &my_n+1, &n, std::plus<int>());

      for (int i = 0; i < n; ++i)
        get(fr, vertex(i,g));
      synchronize(fr);

      // Add edges to gr
      std::vector<std::pair<vertex_descriptor, vertex_descriptor> > new_edges;
      for( boost::tie(vstart, vend) = vertices(g); vstart != vend; vstart++ )
        for( boost::tie(oestart, oeend) = out_edges(*vstart, g); oestart != oeend; oestart++ )
          new_edges.push_back( std::make_pair(get(fr, target(*oestart,g)), get(fr, source(*oestart, g))) );

      std::vector<VertexPairVec> edge_requests(num_procs);

      typename std::vector<std::pair<vertex_descriptor, vertex_descriptor> >::iterator iter;
      for( iter = new_edges.begin(); iter != new_edges.end(); iter++ ) {
        std::pair<vertex_descriptor, vertex_descriptor> p1 = *iter;
        if( get(owner,  p1.first ) == id )
          add_edge( p1.first, p1.second, gr );
        else
          edge_requests[get(owner, p1.first)].push_back(p1);
      }
      new_edges.clear();

      // Send edge addition requests
      for (process_id_type p = 0; p < num_procs; ++p) {
        if (!edge_requests[p].empty()) {
          VertexPairVec reqs(edge_requests[p].begin(), edge_requests[p].end());
          send(pg, p, fhp_edges_size_msg, reqs.size());
          send(pg, p, fhp_add_edges_msg, &reqs[0], reqs.size());
        }
      }
      synchronize(pg);

      // Receive edge addition requests and handle them
      while (optional<std::pair<process_id_type, int> > m = probe(pg)) {
        BOOST_ASSERT(m->second == fhp_edges_size_msg);
        std::size_t num_requests;
        receive(pg, m->first, m->second, num_requests);
        VertexPairVec requests(num_requests);
        receive(pg, m->first, fhp_add_edges_msg, &requests[0],
                num_requests);
        for( std::size_t i = 0; i < requests.size(); ++i )
          add_edge( requests[i].first, requests[i].second, gr );
      }
          synchronize(gr);
    }

    template<typename Graph, typename VertexComponentMap, typename ComponentMap>
    typename property_traits<ComponentMap>::value_type
    number_components(const Graph& g, VertexComponentMap r, ComponentMap c)
    {
      typedef typename boost::graph::parallel::process_group_type<Graph>::type process_group_type;
      typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
      typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
      typedef typename property_traits<ComponentMap>::value_type ComponentMapType;
      std::vector<vertex_descriptor> my_roots, all_roots;
      vertex_iterator vstart, vend;

      for( boost::tie(vstart, vend) = vertices(g); vstart != vend; vstart++ )
        if( find( my_roots.begin(), my_roots.end(), get(r, *vstart) ) == my_roots.end() )
          my_roots.push_back( get(r, *vstart) );

      all_gather( process_group(g), my_roots.begin(), my_roots.end(), all_roots );

      /* Number components */
      std::map<vertex_descriptor, ComponentMapType> comp_numbers;
      ComponentMapType c_num = 0;

      // Compute component numbers
      for (std::size_t i = 0; i < all_roots.size(); ++i )
        if ( comp_numbers.count(all_roots[i]) == 0 )
          comp_numbers[all_roots[i]] = c_num++;

      // Broadcast component numbers
      for( boost::tie(vstart, vend) = vertices(g); vstart != vend; vstart++ )
        put( c, *vstart, comp_numbers[get(r,*vstart)] );

      // Broadcast number of components
      if (process_id(process_group(g)) == 0) {
        typedef typename process_group_type::process_size_type
          process_size_type;
        for (process_size_type dest = 1, n = num_processes(process_group(g));
              dest != n; ++dest)
          send(process_group(g), dest, 0, c_num);
      }

      synchronize(process_group(g));

      if (process_id(process_group(g)) != 0) receive(process_group(g), 0, 0, c_num);

      synchronize(c);
      return c_num;
    }


    template<typename Graph, typename ComponentMap, typename VertexComponentMap,
             typename VertexIndexMap>
    typename property_traits<ComponentMap>::value_type
    fleischer_hendrickson_pinar_strong_components_impl
      (const Graph& g,
       ComponentMap c,
       VertexComponentMap r,
       VertexIndexMap vertex_index_map,
       incidence_graph_tag)
    {
      typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
      typedef iterator_property_map<typename std::vector<vertex_descriptor>::iterator,
                                    VertexIndexMap> IsoMap;
      typename boost::graph::parallel::process_group_type<Graph>::type pg = process_group(g);

#ifdef PBGL_SCC_DEBUG
  accounting::time_type start = accounting::get_time();
#endif

      typedef adjacency_list<listS,
                             distributedS<typename boost::graph::parallel::process_group_type<Graph>::type, vecS>,
                             directedS > ReverseGraph;

      ReverseGraph gr(0, pg);
      std::vector<vertex_descriptor> fr_s(num_vertices(g));
      std::vector<vertex_descriptor> rf_s(num_vertices(g));
      IsoMap fr(fr_s.begin(), vertex_index_map);  // fr = forward->reverse
      IsoMap rf(rf_s.begin(), vertex_index_map); // rf = reverse->forward

      build_reverse_graph(g, gr, fr, rf);

#ifdef PBGL_SCC_DEBUG
  accounting::time_type end = accounting::get_time();
  if(process_id(process_group(g)) == 0)
    std::cerr << "Reverse graph initialization time = " << accounting::print_time(end - start) << " seconds.\n";
#endif

  fleischer_hendrickson_pinar_strong_components(g, r, gr, fr, rf, 
                                                vertex_index_map);

      typename property_traits<ComponentMap>::value_type c_num = number_components(g, r, c);

      return c_num;
    }

    template<typename Graph, typename ComponentMap, typename VertexComponentMap,
             typename VertexIndexMap>
    typename property_traits<ComponentMap>::value_type
    fleischer_hendrickson_pinar_strong_components_impl
      (const Graph& g,
       ComponentMap c,
       VertexComponentMap r,
       VertexIndexMap vertex_index_map,
       bidirectional_graph_tag)
    {
      typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

      reverse_graph<Graph> gr(g);
      detail::vertex_identity_property_map<vertex_descriptor> fr, rf;

      fleischer_hendrickson_pinar_strong_components(g, r, gr, fr, rf, 
                                                    vertex_index_map);

      typename property_traits<ComponentMap>::value_type c_num
        = number_components(g, r, c);

      return c_num;
    }

    template<typename Graph, typename ComponentMap, typename VertexIndexMap>
    inline typename property_traits<ComponentMap>::value_type
    fleischer_hendrickson_pinar_strong_components
      (const Graph& g,
       ComponentMap c,
       VertexIndexMap vertex_index_map)
    {
      typedef typename graph_traits<Graph>::vertex_descriptor
        vertex_descriptor;
      typedef iterator_property_map<typename std::vector<vertex_descriptor>::iterator,
                                    VertexIndexMap> VertexComponentMap;
      typename boost::graph::parallel::process_group_type<Graph>::type pg 
        = process_group(g);

      if (num_processes(pg) == 1) {
        local_subgraph<const Graph> ls(g);
        return boost::strong_components(ls, c);
      }

      // Create a VertexComponentMap for intermediate labeling of SCCs
      std::vector<vertex_descriptor> r_s(num_vertices(g), graph_traits<Graph>::null_vertex());
      VertexComponentMap r(r_s.begin(), vertex_index_map);

      return fleischer_hendrickson_pinar_strong_components_impl
               (g, c, r, vertex_index_map,
                typename graph_traits<Graph>::traversal_category());
    }

    template<typename Graph, typename ComponentMap>
    inline typename property_traits<ComponentMap>::value_type
    fleischer_hendrickson_pinar_strong_components(const Graph& g,
                                                  ComponentMap c)
    {
      return fleischer_hendrickson_pinar_strong_components(g, c, get(vertex_index, g));
    }

}  // end namespace distributed

using distributed::fleischer_hendrickson_pinar_strong_components;

} // end namespace graph

template<class Graph, class ComponentMap, class P, class T, class R>
inline typename property_traits<ComponentMap>::value_type
strong_components
 (const Graph& g, ComponentMap comp,
  const bgl_named_params<P, T, R>&
  BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph, distributed_graph_tag))
{
  return graph::fleischer_hendrickson_pinar_strong_components(g, comp);
}

template<class Graph, class ComponentMap>
inline typename property_traits<ComponentMap>::value_type
strong_components
 (const Graph& g, ComponentMap comp
  BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph, distributed_graph_tag))
{
  return graph::fleischer_hendrickson_pinar_strong_components(g, comp);
}

} /* end namespace boost */

#endif // BOOST_GRAPH_DISTRIBUTED_SCC_HPP
