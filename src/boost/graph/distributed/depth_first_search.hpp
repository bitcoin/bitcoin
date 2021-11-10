// Copyright (C) 2004-2008 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DISTRIBUTED_DFS_HPP
#define BOOST_GRAPH_DISTRIBUTED_DFS_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/overloading.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/graph/parallel/process_group.hpp>
#include <boost/graph/parallel/container_traits.hpp>

namespace boost {
  namespace graph { namespace distributed { namespace detail {
    template<typename DistributedGraph, typename ColorMap, typename ParentMap,
             typename ExploreMap, typename VertexIndexMap, typename DFSVisitor>
    class parallel_dfs
    {
      typedef typename graph_traits<DistributedGraph>::vertex_iterator
        vertex_iterator;
      typedef typename graph_traits<DistributedGraph>::vertex_descriptor
        vertex_descriptor;
      typedef typename graph_traits<DistributedGraph>::out_edge_iterator
        out_edge_iterator;

      typedef typename boost::graph::parallel::process_group_type<DistributedGraph>
        ::type process_group_type;
      typedef typename process_group_type::process_id_type process_id_type;

      /**
       * The first vertex in the pair is the local node (i) and the
       * second vertex in the pair is the (possibly remote) node (j).
       */
      typedef boost::parallel::detail::untracked_pair<vertex_descriptor, vertex_descriptor> vertex_pair;

      typedef typename property_traits<ColorMap>::value_type color_type;
      typedef color_traits<color_type> Color;

      // Message types
      enum { discover_msg = 10, return_msg = 50, visited_msg = 100 , done_msg = 150};
        

    public:
      parallel_dfs(const DistributedGraph& g, ColorMap color,
                   ParentMap parent, ExploreMap explore,
                   VertexIndexMap index_map, DFSVisitor vis)
        : g(g), color(color), parent(parent), explore(explore),
          index_map(index_map), vis(vis), pg(process_group(g)),
          owner(get(vertex_owner, g)), next_out_edge(num_vertices(g))
      { }

      void run(vertex_descriptor s)
      {
        vertex_iterator vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
          put(color, *vi, Color::white());
          put(parent, *vi, *vi); 
          put(explore, *vi, *vi);
          next_out_edge[get(index_map, *vi)] = out_edges(*vi, g).first;
          vis.initialize_vertex(*vi, g);
        }

        vis.start_vertex(s, g);
        
        if (get(owner, s) == process_id(pg)) {
          send_oob(pg, get(owner, s), discover_msg, vertex_pair(s, s));
        }
        
        bool done = false;
        while (!done) {
          std::pair<process_id_type, int> msg = *pg.poll(true);

          switch (msg.second) {
          case discover_msg:
            {
              vertex_pair p;
              receive_oob(pg, msg.first, msg.second, p);

              if (p.first != p.second) {
                // delete j from nomessage(j)
                if (get(color, p.second) != Color::black())
                  local_put(color, p.second, Color::gray());

                if (recover(p)) break;
              }

              if (get(color, p.first) == Color::white()) {
                put(color, p.first, Color::gray());
                put(parent, p.first, p.second);

                vis.discover_vertex(p.first, g);

                if (shift_center_of_activity(p.first)) break;
                
                out_edge_iterator ei, ei_end;
                for (boost::tie(ei,ei_end) = out_edges(p.first, g); ei != ei_end; ++ei)
                {
                  // Notify everyone who may not know that the source
                  // vertex has been visited. They can then mark the
                  // corresponding color map entry gray.
                  if (get(parent, p.first) != target(*ei, g)
                      && get(explore, p.first) != target(*ei, g)) {
                    vertex_pair visit(target(*ei, g), p.first);
                    
                    send_oob(pg, get(owner, target(*ei, g)), visited_msg, visit);
                  }
                }
              }
            }
            break;
            
          case visited_msg:
            {
              vertex_pair p;
              receive_oob(pg, msg.first, msg.second, p);
              
              // delete j from nomessage(j)
              if (get(color, p.second) != Color::black())
                local_put(color, p.second, Color::gray());

              recover(p);
            }
            break;
            
          case return_msg:
            {
              vertex_pair p;
              receive_oob(pg, msg.first, msg.second, p);
              
              // delete j from nomessage(i)
              local_put(color, p.second, Color::black());

              shift_center_of_activity(p.first);
            }
            break;
            
          case done_msg:
            {
              receive_oob(pg, msg.first, msg.second, done);

              // Propagate done message downward in tree
              done = true;
              process_id_type id = process_id(pg);
              process_id_type left = 2*id + 1;
              process_id_type right = left + 1;
              if (left < num_processes(pg))
                send_oob(pg, left, done_msg, done);
              if (right < num_processes(pg))
                send_oob(pg, right, done_msg, done);
            }
            break;

          default:
            BOOST_ASSERT(false);
          }
        }
      }
      
    private:
      bool recover(const vertex_pair& p)
      {
        if (get(explore, p.first) == p.second) {
          return shift_center_of_activity(p.first);
        }
        else
          return false;
      }
      
      bool shift_center_of_activity(vertex_descriptor i)
      {
        for (out_edge_iterator ei = next_out_edge[get(index_map, i)],
                               ei_end = out_edges(i, g).second;
             ei != ei_end; ++ei) {
          vis.examine_edge(*ei, g);

          vertex_descriptor k = target(*ei, g);
          color_type target_color = get(color, k);
          if (target_color == Color::black()) vis.forward_or_cross_edge(*ei, g);
          else if (target_color == Color::gray()) vis.back_edge(*ei, g);
          else {
            put(explore, i, k);
            vis.tree_edge(*ei, g);
            vertex_pair p(k, i);
            send_oob(pg, get(owner, k), discover_msg, p);
            next_out_edge[get(index_map, i)] = ++ei;
            return false;
          } 
        }

        next_out_edge[get(index_map, i)] = out_edges(i, g).second;
        put(explore, i, i);
        put(color, i, Color::black());
        vis.finish_vertex(i, g);
          
        if (get(parent, i) == i) {
          send_oob(pg, 0, done_msg, true);
          return true;
        }
        else {
          vertex_pair ret(get(parent, i), i);
          send_oob(pg, get(owner, ret.first), return_msg, ret);
        }
        return false;
      }

      const DistributedGraph& g; 
      ColorMap color;
      ParentMap parent; 
      ExploreMap explore;
      VertexIndexMap index_map;
      DFSVisitor vis;
      process_group_type pg;
      typename property_map<DistributedGraph, vertex_owner_t>::const_type owner;
      std::vector<out_edge_iterator> next_out_edge; 
    };
  } // end namespace detail

  template<typename DistributedGraph, typename ColorMap, typename ParentMap,
           typename ExploreMap, typename VertexIndexMap, typename DFSVisitor>
  void
  tsin_depth_first_visit
    (const DistributedGraph& g,
     typename graph_traits<DistributedGraph>::vertex_descriptor s,
     DFSVisitor vis, ColorMap color, ParentMap parent, ExploreMap explore, 
     VertexIndexMap index_map)
  {
    typedef typename graph_traits<DistributedGraph>::directed_category
      directed_category;
    BOOST_STATIC_ASSERT(
      (is_convertible<directed_category, undirected_tag>::value));

    set_property_map_role(vertex_color, color);
    graph::distributed::detail::parallel_dfs
      <DistributedGraph, ColorMap, ParentMap, ExploreMap, VertexIndexMap, 
       DFSVisitor> do_dfs(g, color, parent, explore, index_map, vis);
    do_dfs.run(s);

    using boost::graph::parallel::process_group;
    synchronize(process_group(g));
  }
    
  template<typename DistributedGraph, typename DFSVisitor, 
           typename VertexIndexMap>
  void
  tsin_depth_first_visit
    (const DistributedGraph& g,
     typename graph_traits<DistributedGraph>::vertex_descriptor s,
     DFSVisitor vis,
     VertexIndexMap index_map)
  {
    typedef typename graph_traits<DistributedGraph>::vertex_descriptor
      vertex_descriptor;

    std::vector<default_color_type> colors(num_vertices(g));
    std::vector<vertex_descriptor> parent(num_vertices(g));
    std::vector<vertex_descriptor> explore(num_vertices(g));
    tsin_depth_first_visit
      (g, s,
       vis,
       make_iterator_property_map(colors.begin(), index_map),
       make_iterator_property_map(parent.begin(), index_map),
       make_iterator_property_map(explore.begin(), index_map),
       index_map);
  }

  template<typename DistributedGraph, typename DFSVisitor, 
           typename VertexIndexMap>
  void
  tsin_depth_first_visit
    (const DistributedGraph& g,
     typename graph_traits<DistributedGraph>::vertex_descriptor s,
     DFSVisitor vis)
  {
    tsin_depth_first_visit(g, s, vis, get(vertex_index, g));
  }
} // end namespace distributed

using distributed::tsin_depth_first_visit;

} // end namespace graph

template<typename DistributedGraph, typename DFSVisitor>
void
depth_first_visit
  (const DistributedGraph& g,
   typename graph_traits<DistributedGraph>::vertex_descriptor s,
   DFSVisitor vis)
{
  graph::tsin_depth_first_visit(g, s, vis, get(vertex_index, g));
}

} // end namespace boost

#endif // BOOST_GRAPH_DISTRIBUTED_DFS_HPP
