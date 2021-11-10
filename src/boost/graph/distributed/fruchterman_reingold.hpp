// Copyright (C) 2005-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DISTRIBUTED_FRUCHTERMAN_REINGOLD_HPP
#define BOOST_GRAPH_DISTRIBUTED_FRUCHTERMAN_REINGOLD_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/fruchterman_reingold.hpp>

namespace boost { namespace graph { namespace distributed {

class simple_tiling
{
 public:
  simple_tiling(int columns, int rows, bool flip = true) 
    : columns(columns), rows(rows), flip(flip)
  {
  }

  // Convert from a position (x, y) in the tiled display into a
  // processor ID number
  int operator()(int x, int y) const
  {
    return flip? (rows - y - 1) * columns + x : y * columns + x;
  }

  // Convert from a process ID to a position (x, y) in the tiled
  // display
  std::pair<int, int> operator()(int id)
  {
    int my_col = id % columns;
    int my_row = flip? rows - (id / columns) - 1 : id / columns;
    return std::make_pair(my_col, my_row);
  }

  int columns, rows;

 private:
  bool flip;
};

// Force pairs function object that does nothing
struct no_force_pairs
{
  template<typename Graph, typename ApplyForce>
  void operator()(const Graph&, const ApplyForce&)
  {
  }
};

// Computes force pairs in the distributed case.
template<typename PositionMap, typename DisplacementMap, typename LocalForces,
         typename NonLocalForces = no_force_pairs>
class distributed_force_pairs_proxy
{
 public:
  distributed_force_pairs_proxy(const PositionMap& position, 
                                const DisplacementMap& displacement,
                                const LocalForces& local_forces,
                                const NonLocalForces& nonlocal_forces = NonLocalForces())
    : position(position), displacement(displacement), 
      local_forces(local_forces), nonlocal_forces(nonlocal_forces)
  {
  }

  template<typename Graph, typename ApplyForce>
  void operator()(const Graph& g, ApplyForce apply_force)
  {
    // Flush remote displacements
    displacement.flush();

    // Receive updated positions for all of our neighbors
    synchronize(position);

    // Reset remote displacements 
    displacement.reset();

    // Compute local repulsive forces
    local_forces(g, apply_force);

    // Compute neighbor repulsive forces
    nonlocal_forces(g, apply_force);
  }

 protected:
  PositionMap position;
  DisplacementMap displacement;
  LocalForces local_forces;
  NonLocalForces nonlocal_forces;
};

template<typename PositionMap, typename DisplacementMap, typename LocalForces>
inline 
distributed_force_pairs_proxy<PositionMap, DisplacementMap, LocalForces>
make_distributed_force_pairs(const PositionMap& position, 
                             const DisplacementMap& displacement,
                             const LocalForces& local_forces)
{
  typedef 
    distributed_force_pairs_proxy<PositionMap, DisplacementMap, LocalForces>
    result_type;
  return result_type(position, displacement, local_forces);
}

template<typename PositionMap, typename DisplacementMap, typename LocalForces,
         typename NonLocalForces>
inline 
distributed_force_pairs_proxy<PositionMap, DisplacementMap, LocalForces,
                              NonLocalForces>
make_distributed_force_pairs(const PositionMap& position, 
                             const DisplacementMap& displacement,
                             const LocalForces& local_forces,
                             const NonLocalForces& nonlocal_forces)
{
  typedef 
    distributed_force_pairs_proxy<PositionMap, DisplacementMap, LocalForces,
                                  NonLocalForces>
      result_type;
  return result_type(position, displacement, local_forces, nonlocal_forces);
}

// Compute nonlocal force pairs based on the shared borders with
// adjacent tiles.
template<typename PositionMap>
class neighboring_tiles_force_pairs
{
 public:
  typedef typename property_traits<PositionMap>::value_type Point;
  typedef typename point_traits<Point>::component_type Dim;

  enum bucket_position { left, top, right, bottom, end_position };
  
  neighboring_tiles_force_pairs(PositionMap position, Point origin,
                                Point extent, simple_tiling tiling)
    : position(position), origin(origin), extent(extent), tiling(tiling)
  {
  }

  template<typename Graph, typename ApplyForce>
  void operator()(const Graph& g, ApplyForce apply_force)
  {
    // TBD: Do this some smarter way
    if (tiling.columns == 1 && tiling.rows == 1)
      return;

    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
#ifndef BOOST_NO_STDC_NAMESPACE
    using std::sqrt;
#endif // BOOST_NO_STDC_NAMESPACE

    // TBD: num_vertices(g) should be the global number of vertices?
    Dim two_k = Dim(2) * sqrt(extent[0] * extent[1] / num_vertices(g));

    std::vector<vertex_descriptor> my_vertices[4];
    std::vector<vertex_descriptor> neighbor_vertices[4];
   
    // Compute cutoff positions
    Dim cutoffs[4];
    cutoffs[left] = origin[0] + two_k;
    cutoffs[top] = origin[1] + two_k;
    cutoffs[right] = origin[0] + extent[0] - two_k;
    cutoffs[bottom] = origin[1] + extent[1] - two_k;

    // Compute neighbors
    typename PositionMap::process_group_type pg = position.process_group();
    std::pair<int, int> my_tile = tiling(process_id(pg));
    int neighbors[4] = { -1, -1, -1, -1 } ;
    if (my_tile.first > 0)
      neighbors[left] = tiling(my_tile.first - 1, my_tile.second);
    if (my_tile.second > 0)
      neighbors[top] = tiling(my_tile.first, my_tile.second - 1);
    if (my_tile.first < tiling.columns - 1)
      neighbors[right] = tiling(my_tile.first + 1, my_tile.second);
    if (my_tile.second < tiling.rows - 1)
      neighbors[bottom] = tiling(my_tile.first, my_tile.second + 1);

    // Sort vertices along the edges into buckets
    BGL_FORALL_VERTICES_T(v, g, Graph) {
      if (position[v][0] <= cutoffs[left]) my_vertices[left].push_back(v); 
      if (position[v][1] <= cutoffs[top]) my_vertices[top].push_back(v); 
      if (position[v][0] >= cutoffs[right]) my_vertices[right].push_back(v); 
      if (position[v][1] >= cutoffs[bottom]) my_vertices[bottom].push_back(v); 
    }

    // Send vertices to neighbors, and gather our neighbors' vertices
    bucket_position pos;
    for (pos = left; pos < end_position; pos = bucket_position(pos + 1)) {
      if (neighbors[pos] != -1) {
        send(pg, neighbors[pos], 0, my_vertices[pos].size());
        if (!my_vertices[pos].empty())
          send(pg, neighbors[pos], 1, 
               &my_vertices[pos].front(), my_vertices[pos].size());
      }
    }

    // Pass messages around
    synchronize(pg);
    
    // Receive neighboring vertices
    for (pos = left; pos < end_position; pos = bucket_position(pos + 1)) {
      if (neighbors[pos] != -1) {
        std::size_t incoming_vertices;
        receive(pg, neighbors[pos], 0, incoming_vertices);

        if (incoming_vertices) {
          neighbor_vertices[pos].resize(incoming_vertices);
          receive(pg, neighbors[pos], 1, &neighbor_vertices[pos].front(),
                  incoming_vertices);
        }
      }
    }

    // For each neighboring vertex, we need to get its current position
    for (pos = left; pos < end_position; pos = bucket_position(pos + 1))
      for (typename std::vector<vertex_descriptor>::iterator i = 
             neighbor_vertices[pos].begin(); 
           i != neighbor_vertices[pos].end();
           ++i)
        request(position, *i);
    synchronize(position);

    // Apply forces in adjacent bins. This is O(n^2) in the worst
    // case. Oh well.
    for (pos = left; pos < end_position; pos = bucket_position(pos + 1)) {
      for (typename std::vector<vertex_descriptor>::iterator i = 
             my_vertices[pos].begin(); 
           i != my_vertices[pos].end();
           ++i)
        for (typename std::vector<vertex_descriptor>::iterator j = 
               neighbor_vertices[pos].begin(); 
             j != neighbor_vertices[pos].end();
             ++j)
          apply_force(*i, *j);
    }
  }

 protected:
  PositionMap position;
  Point origin;
  Point extent;
  simple_tiling tiling;
};

template<typename PositionMap>
inline neighboring_tiles_force_pairs<PositionMap>
make_neighboring_tiles_force_pairs
 (PositionMap position, 
  typename property_traits<PositionMap>::value_type origin,
  typename property_traits<PositionMap>::value_type extent,
  simple_tiling tiling)
{
  return neighboring_tiles_force_pairs<PositionMap>(position, origin, extent,
                                                    tiling);
}

template<typename DisplacementMap, typename Cooling>
class distributed_cooling_proxy
{
 public:
  typedef typename Cooling::result_type result_type;

  distributed_cooling_proxy(const DisplacementMap& displacement,
                            const Cooling& cooling)
    : displacement(displacement), cooling(cooling)
  {
  }

  result_type operator()()
  {
    // Accumulate displacements computed on each processor
    synchronize(displacement.data->process_group);

    // Allow the underlying cooling to occur
    return cooling();
  }

 protected:
  DisplacementMap displacement;
  Cooling cooling;
};

template<typename DisplacementMap, typename Cooling>
inline distributed_cooling_proxy<DisplacementMap, Cooling>
make_distributed_cooling(const DisplacementMap& displacement,
                         const Cooling& cooling)
{
  typedef distributed_cooling_proxy<DisplacementMap, Cooling> result_type;
  return result_type(displacement, cooling);
}

template<typename Point>
struct point_accumulating_reducer {
  BOOST_STATIC_CONSTANT(bool, non_default_resolver = true);

  template<typename K>
  Point operator()(const K&) const { return Point(); }

  template<typename K>
  Point operator()(const K&, const Point& p1, const Point& p2) const 
  { return Point(p1[0] + p2[0], p1[1] + p2[1]); }
};

template<typename Graph, typename PositionMap, 
         typename AttractiveForce, typename RepulsiveForce,
         typename ForcePairs, typename Cooling, typename DisplacementMap>
void
fruchterman_reingold_force_directed_layout
 (const Graph&    g,
  PositionMap     position,
  typename property_traits<PositionMap>::value_type const& origin,
  typename property_traits<PositionMap>::value_type const& extent,
  AttractiveForce attractive_force,
  RepulsiveForce  repulsive_force,
  ForcePairs      force_pairs,
  Cooling         cool,
  DisplacementMap displacement)
{
  typedef typename property_traits<PositionMap>::value_type Point;

  // Reduction in the displacement map involves summing the forces
  displacement.set_reduce(point_accumulating_reducer<Point>());

  // We need to track the positions of all of our neighbors
  BGL_FORALL_VERTICES_T(u, g, Graph)
    BGL_FORALL_ADJ_T(u, v, g, Graph)
      request(position, v);

  // Invoke the "sequential" Fruchterman-Reingold implementation
  boost::fruchterman_reingold_force_directed_layout
    (g, position, origin, extent,
     attractive_force, repulsive_force,
     make_distributed_force_pairs(position, displacement, force_pairs),
     make_distributed_cooling(displacement, cool),
     displacement);
}

template<typename Graph, typename PositionMap, 
         typename AttractiveForce, typename RepulsiveForce,
         typename ForcePairs, typename Cooling, typename DisplacementMap>
void
fruchterman_reingold_force_directed_layout
 (const Graph&    g,
  PositionMap     position,
  typename property_traits<PositionMap>::value_type const& origin,
  typename property_traits<PositionMap>::value_type const& extent,
  AttractiveForce attractive_force,
  RepulsiveForce  repulsive_force,
  ForcePairs      force_pairs,
  Cooling         cool,
  DisplacementMap displacement,
  simple_tiling   tiling)
{
  typedef typename property_traits<PositionMap>::value_type Point;

  // Reduction in the displacement map involves summing the forces
  displacement.set_reduce(point_accumulating_reducer<Point>());

  // We need to track the positions of all of our neighbors
  BGL_FORALL_VERTICES_T(u, g, Graph)
    BGL_FORALL_ADJ_T(u, v, g, Graph)
      request(position, v);

  // Invoke the "sequential" Fruchterman-Reingold implementation
  boost::fruchterman_reingold_force_directed_layout
    (g, position, origin, extent,
     attractive_force, repulsive_force,
     make_distributed_force_pairs
      (position, displacement, force_pairs,
       make_neighboring_tiles_force_pairs(position, origin, extent, tiling)),
     make_distributed_cooling(displacement, cool),
     displacement);
}

} } } // end namespace boost::graph::distributed

#endif // BOOST_GRAPH_DISTRIBUTED_FRUCHTERMAN_REINGOLD_HPP
