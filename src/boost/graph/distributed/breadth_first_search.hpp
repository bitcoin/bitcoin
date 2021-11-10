// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_PARALLEL_BFS_HPP
#define BOOST_GRAPH_PARALLEL_BFS_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/overloading.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/distributed/detail/filtered_queue.hpp>
#include <boost/graph/distributed/queue.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/pending/queue.hpp>
#include <boost/graph/parallel/properties.hpp>
#include <boost/graph/parallel/container_traits.hpp>

namespace boost {
  namespace detail {
    /** @brief A unary predicate that decides when to push into a
     *         breadth-first search queue.
     *
     *  This predicate stores a color map that is used to determine
     *  when to push. If it is provided with a key for which the color
     *  is white, it darkens the color to gray and returns true (so
     *  that the value will be pushed appropriately); if the color is
     *  not white, it returns false so that the vertex will be
     *  ignored.
     */
    template<typename ColorMap>
    struct darken_and_push
    {
      typedef typename property_traits<ColorMap>::key_type argument_type;
      typedef bool result_type;

      explicit darken_and_push(const ColorMap& color) : color(color) { }

      bool operator()(const argument_type& value) const
      {
        typedef color_traits<typename property_traits<ColorMap>::value_type>
          Color;
        if (get(color, value) == Color::white()) {
          put(color, value, Color::gray());
          return true;
        } else {
          return false;
        }
      }

      ColorMap color;
    };

    template<typename IndexMap>
    struct has_not_been_seen
    {
      typedef bool result_type;

      has_not_been_seen() { }

      has_not_been_seen(std::size_t n, IndexMap index_map)
        : seen(n), index_map(index_map) {}

      template<typename Key>
      result_type operator()(Key key)
      {
        bool result = seen[get(index_map, key)];
        seen[get(index_map, key)] = true;
        return !result;
      }

      void swap(has_not_been_seen& other)
      {
        using std::swap;
        swap(seen, other.seen);
        swap(index_map, other.index_map);
      }

    private:
      dynamic_bitset<> seen;
      IndexMap index_map;
    };

    template<typename IndexMap>
    inline void
    swap(has_not_been_seen<IndexMap>& x, has_not_been_seen<IndexMap>& y)
    {
      x.swap(y);
    }

    template <class DistributedGraph, class ColorMap, class BFSVisitor,
              class BufferRef, class VertexIndexMap>
    inline void
    parallel_bfs_helper
      (DistributedGraph& g,
       typename graph_traits<DistributedGraph>::vertex_descriptor s,
       ColorMap color,
       BFSVisitor vis,
       BufferRef Q,
       VertexIndexMap)
    {
      set_property_map_role(vertex_color, color);
      color.set_consistency_model(0);
      breadth_first_search(g, s, Q.ref, vis, color);
    }

    template <class DistributedGraph, class ColorMap, class BFSVisitor,
              class VertexIndexMap>
    void parallel_bfs_helper
      (DistributedGraph& g,
       typename graph_traits<DistributedGraph>::vertex_descriptor s,
       ColorMap color,
       BFSVisitor vis,
       boost::param_not_found,
       VertexIndexMap vertex_index)
    {
      using boost::graph::parallel::process_group;

      typedef graph_traits<DistributedGraph> Traits;
      typedef typename Traits::vertex_descriptor Vertex;
      typedef typename boost::graph::parallel::process_group_type<DistributedGraph>::type 
        process_group_type;

      set_property_map_role(vertex_color, color);
      color.set_consistency_model(0);

      // Buffer default
      typedef typename property_map<DistributedGraph, vertex_owner_t>
        ::const_type vertex_owner_map;
      typedef boost::graph::distributed::distributed_queue<
                process_group_type, vertex_owner_map, queue<Vertex>, 
                detail::darken_and_push<ColorMap> > queue_t;
      queue_t Q(process_group(g),
                get(vertex_owner, g),
                detail::darken_and_push<ColorMap>(color));
      breadth_first_search(g, s, Q, vis, color);
    }

    template <class DistributedGraph, class ColorMap, class BFSVisitor,
              class P, class T, class R>
    void bfs_helper
      (DistributedGraph& g,
       typename graph_traits<DistributedGraph>::vertex_descriptor s,
       ColorMap color,
       BFSVisitor vis,
       const bgl_named_params<P, T, R>& params,
       boost::mpl::true_)
        {
            parallel_bfs_helper
        (g, s, color, vis, get_param(params, buffer_param_t()),
         choose_const_pmap(get_param(params, vertex_index),  g, vertex_index));
        }
  }
}

#endif // BOOST_GRAPH_PARALLEL_BFS_HPP
