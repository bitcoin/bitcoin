// Copyright 2004 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_PARALLEL_BRANDES_BETWEENNESS_CENTRALITY_HPP
#define BOOST_GRAPH_PARALLEL_BRANDES_BETWEENNESS_CENTRALITY_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

// #define COMPUTE_PATH_COUNTS_INLINE

#include <boost/graph/betweenness_centrality.hpp>
#include <boost/graph/overloading.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/config.hpp>
#include <boost/assert.hpp>

// For additive_reducer
#include <boost/graph/distributed/distributed_graph_utility.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/named_function_params.hpp>

#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/graph/distributed/detail/dijkstra_shortest_paths.hpp>
#include <boost/tuple/tuple.hpp>

// NGE - Needed for minstd_rand at L807, should pass vertex list
//       or generator instead 
#include <boost/random/linear_congruential.hpp>

#include <algorithm>
#include <stack>
#include <vector>

// Appending reducer
template <typename T>
struct append_reducer {
  BOOST_STATIC_CONSTANT(bool, non_default_resolver = true);
      
  template<typename K>
  T operator()(const K&) const { return T(); }
      
  template<typename K>
  T operator()(const K&, const T& x, const T& y) const 
  { 
    T z(x.begin(), x.end());
    for (typename T::const_iterator iter = y.begin(); iter != y.end(); ++iter)
      if (std::find(z.begin(), z.end(), *iter) == z.end())
        z.push_back(*iter);
    
    return z;
  }
};

namespace boost {

  namespace serialization {

    // TODO(nge): Write generalized serialization for tuples
    template<typename Archive, typename T1, typename T2, typename T3, 
             typename T4>
    void serialize(Archive & ar,
                   boost::tuple<T1,T2,T3, T4>& t,
                   const unsigned int)
    {
      ar & boost::tuples::get<0>(t);
      ar & boost::tuples::get<1>(t);
      ar & boost::tuples::get<2>(t);
      ar & boost::tuples::get<3>(t);
    }

  } // serialization

  template <typename OwnerMap, typename Tuple>
  class get_owner_of_first_tuple_element {

  public:
    typedef typename property_traits<OwnerMap>::value_type owner_type;
    
    get_owner_of_first_tuple_element(OwnerMap owner) : owner(owner) { }

    owner_type get_owner(Tuple t) { return get(owner, boost::tuples::get<0>(t)); }

  private:
    OwnerMap owner;
  };

  template <typename OwnerMap, typename Tuple>
  typename get_owner_of_first_tuple_element<OwnerMap, Tuple>::owner_type
  get(get_owner_of_first_tuple_element<OwnerMap, Tuple> o, Tuple t)
  { return o.get_owner(t); } 

  template <typename OwnerMap>
  class get_owner_of_first_pair_element {

  public:
    typedef typename property_traits<OwnerMap>::value_type owner_type;
    
    get_owner_of_first_pair_element(OwnerMap owner) : owner(owner) { }

    template <typename Vertex, typename T>
    owner_type get_owner(std::pair<Vertex, T> p) { return get(owner, p.first); }

  private:
    OwnerMap owner;
  };

  template <typename OwnerMap, typename Vertex, typename T>
  typename get_owner_of_first_pair_element<OwnerMap>::owner_type
  get(get_owner_of_first_pair_element<OwnerMap> o, std::pair<Vertex, T> p)
  { return o.get_owner(p); } 

  namespace graph { namespace parallel { namespace detail {

  template<typename DistanceMap, typename IncomingMap>
  class betweenness_centrality_msg_value
  {
    typedef typename property_traits<DistanceMap>::value_type distance_type;
    typedef typename property_traits<IncomingMap>::value_type incoming_type;
    typedef typename incoming_type::value_type incoming_value_type;

  public:
    typedef std::pair<distance_type, incoming_value_type> type;
    
    static type create(distance_type dist, incoming_value_type source)
    { return std::make_pair(dist, source); }
  };


  /************************************************************************/
  /* Delta-stepping Betweenness Centrality                                */
  /************************************************************************/

  template<typename Graph, typename DistanceMap, typename IncomingMap, 
           typename EdgeWeightMap, typename PathCountMap
#ifdef COMPUTE_PATH_COUNTS_INLINE
           , typename IsSettledMap, typename VertexIndexMap
#endif
           >
  class betweenness_centrality_delta_stepping_impl { 
    // Could inherit from delta_stepping_impl to get run() method
    // but for the time being it's just reproduced here

    typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename graph_traits<Graph>::degree_size_type Degree;
    typedef typename property_traits<EdgeWeightMap>::value_type Dist;
    typedef typename property_traits<IncomingMap>::value_type IncomingType;
    typedef typename boost::graph::parallel::process_group_type<Graph>::type 
      ProcessGroup;
    
    typedef std::list<Vertex> Bucket;
    typedef typename Bucket::iterator BucketIterator;
    typedef typename std::vector<Bucket*>::size_type BucketIndex;

    typedef betweenness_centrality_msg_value<DistanceMap, IncomingMap> 
      MessageValue;
    
    enum { 
      // Relax a remote vertex. The message contains a pair<Vertex,
      // MessageValue>, the first part of which is the vertex whose
      // tentative distance is being relaxed and the second part
      // contains either the new distance (if there is no predecessor
      // map) or a pair with the distance and predecessor.
      msg_relax 
    };

  public:

    // Must supply delta, ctor that guesses delta removed 
    betweenness_centrality_delta_stepping_impl(const Graph& g,
                                               DistanceMap distance, 
                                               IncomingMap incoming,
                                               EdgeWeightMap weight,
                                               PathCountMap path_count,
#ifdef COMPUTE_PATH_COUNTS_INLINE
                                               IsSettledMap is_settled,
                                               VertexIndexMap vertex_index,
#endif
                                               Dist delta);
    
    void run(Vertex s);

  private:
    // Relax the edge (u, v), creating a new best path of distance x.
    void relax(Vertex u, Vertex v, Dist x);

    // Synchronize all of the processes, by receiving all messages that
    // have not yet been received.
    void synchronize()
    {
      using boost::parallel::synchronize;
      synchronize(pg);
    }
    
    // Setup triggers for msg_relax messages
    void setup_triggers()
    {
      using boost::parallel::simple_trigger;
      simple_trigger(pg, msg_relax, this, 
                     &betweenness_centrality_delta_stepping_impl::handle_msg_relax);
    }

    void handle_msg_relax(int /*source*/, int /*tag*/,
                          const std::pair<Vertex, typename MessageValue::type>& data,
                          boost::parallel::trigger_receive_context)
    { relax(data.second.second, data.first, data.second.first); }

    const Graph& g;
    IncomingMap incoming;
    DistanceMap distance;
    EdgeWeightMap weight;
    PathCountMap path_count;
#ifdef COMPUTE_PATH_COUNTS_INLINE
    IsSettledMap is_settled;
    VertexIndexMap vertex_index;
#endif
    Dist delta;
    ProcessGroup pg;
    typename property_map<Graph, vertex_owner_t>::const_type owner;
    typename property_map<Graph, vertex_local_t>::const_type local;
    
    // A "property map" that contains the position of each vertex in
    // whatever bucket it resides in.
    std::vector<BucketIterator> position_in_bucket;
    
    // Bucket data structure. The ith bucket contains all local vertices
    // with (tentative) distance in the range [i*delta,
    // (i+1)*delta). 
    std::vector<Bucket*> buckets;
    
    // This "dummy" list is used only so that we can initialize the
    // position_in_bucket property map with non-singular iterators. This
    // won't matter for most implementations of the C++ Standard
    // Library, but it avoids undefined behavior and allows us to run
    // with library "debug modes".
    std::list<Vertex> dummy_list;
    
    // A "property map" that states which vertices have been deleted
    // from the bucket in this iteration.
    std::vector<bool> vertex_was_deleted;
  };

  template<typename Graph, typename DistanceMap, typename IncomingMap, 
           typename EdgeWeightMap, typename PathCountMap
#ifdef COMPUTE_PATH_COUNTS_INLINE
           , typename IsSettledMap, typename VertexIndexMap
#endif
           >
  betweenness_centrality_delta_stepping_impl<
    Graph, DistanceMap, IncomingMap, EdgeWeightMap, PathCountMap
#ifdef COMPUTE_PATH_COUNTS_INLINE
           , IsSettledMap, VertexIndexMap
#endif
    >::
  betweenness_centrality_delta_stepping_impl(const Graph& g,
                                             DistanceMap distance,
                                             IncomingMap incoming,
                                             EdgeWeightMap weight,
                                             PathCountMap path_count,
#ifdef COMPUTE_PATH_COUNTS_INLINE
                                             IsSettledMap is_settled,
                                             VertexIndexMap vertex_index,
#endif
                                             Dist delta)
    : g(g),
      incoming(incoming),
      distance(distance),
      weight(weight),
      path_count(path_count),
#ifdef COMPUTE_PATH_COUNTS_INLINE
      is_settled(is_settled),
      vertex_index(vertex_index),
#endif
      delta(delta),
      pg(boost::graph::parallel::process_group_adl(g), boost::parallel::attach_distributed_object()),
      owner(get(vertex_owner, g)),
      local(get(vertex_local, g))

  { setup_triggers(); }

  template<typename Graph, typename DistanceMap, typename IncomingMap, 
           typename EdgeWeightMap, typename PathCountMap
#ifdef COMPUTE_PATH_COUNTS_INLINE
           , typename IsSettledMap, typename VertexIndexMap
#endif
           >
  void
  betweenness_centrality_delta_stepping_impl<
    Graph, DistanceMap, IncomingMap, EdgeWeightMap, PathCountMap
#ifdef COMPUTE_PATH_COUNTS_INLINE
           , IsSettledMap, VertexIndexMap
#endif
    >::
  run(Vertex s)
  {
    typedef typename boost::graph::parallel::process_group_type<Graph>::type 
      process_group_type;
    typename process_group_type::process_id_type id = process_id(pg);

    Dist inf = (std::numeric_limits<Dist>::max)();
    
    // None of the vertices are stored in the bucket.
    position_in_bucket.clear();
    position_in_bucket.resize(num_vertices(g), dummy_list.end());
    
    // None of the vertices have been deleted
    vertex_was_deleted.clear();
    vertex_was_deleted.resize(num_vertices(g), false);
    
    // No path from s to any other vertex, yet
    BGL_FORALL_VERTICES_T(v, g, Graph)
      put(distance, v, inf);
    
    // The distance to the starting node is zero
    if (get(owner, s) == id) 
      // Put "s" into its bucket (bucket 0)
      relax(s, s, 0);
    else
      // Note that we know the distance to s is zero
      cache(distance, s, 0);
    
#ifdef COMPUTE_PATH_COUNTS_INLINE
    // Synchronize here to deliver initial relaxation since we don't
    // synchronize at the beginning of the inner loop any more
    synchronize(); 

    // Incoming edge count map is an implementation detail and should
    // be freed as soon as possible so build it here
    typedef typename graph_traits<Graph>::edges_size_type edges_size_type;

    std::vector<edges_size_type> incoming_edge_countS(num_vertices(g));
    iterator_property_map<typename std::vector<edges_size_type>::iterator, VertexIndexMap> 
      incoming_edge_count(incoming_edge_countS.begin(), vertex_index);
#endif

    BucketIndex max_bucket = (std::numeric_limits<BucketIndex>::max)();
    BucketIndex current_bucket = 0;
    do {
#ifdef COMPUTE_PATH_COUNTS_INLINE
      // We need to clear the outgoing map after every bucket so just build it here
      std::vector<IncomingType> outgoingS(num_vertices(g));
      IncomingMap outgoing(outgoingS.begin(), vertex_index);
      
      outgoing.set_reduce(append_reducer<IncomingType>());
#else
      // Synchronize with all of the other processes.
      synchronize();
#endif    
  
      // Find the next bucket that has something in it.
      while (current_bucket < buckets.size() 
             && (!buckets[current_bucket] || buckets[current_bucket]->empty()))
        ++current_bucket;
      if (current_bucket >= buckets.size())
        current_bucket = max_bucket;
      
      // Find the smallest bucket (over all processes) that has vertices
      // that need to be processed.
      using boost::parallel::all_reduce;
      using boost::parallel::minimum;
      current_bucket = all_reduce(pg, current_bucket, minimum<BucketIndex>());
      
      if (current_bucket == max_bucket)
        // There are no non-empty buckets in any process; exit. 
        break;
      
      // Contains the set of vertices that have been deleted in the
      // relaxation of "light" edges. Note that we keep track of which
      // vertices were deleted with the property map
      // "vertex_was_deleted".
      std::vector<Vertex> deleted_vertices;
      
      // Repeatedly relax light edges
      bool nonempty_bucket;
      do {
        // Someone has work to do in this bucket.
        
        if (current_bucket < buckets.size() && buckets[current_bucket]) {
          Bucket& bucket = *buckets[current_bucket];
          // For each element in the bucket
          while (!bucket.empty()) {
            Vertex u = bucket.front();
            
            // Remove u from the front of the bucket
            bucket.pop_front();
            
            // Insert u into the set of deleted vertices, if it hasn't
            // been done already.
            if (!vertex_was_deleted[get(local, u)]) {
              vertex_was_deleted[get(local, u)] = true;
              deleted_vertices.push_back(u);
            }
            
            // Relax each light edge. 
            Dist u_dist = get(distance, u);
            BGL_FORALL_OUTEDGES_T(u, e, g, Graph)
              if (get(weight, e) <= delta) // light edge 
                relax(u, target(e, g), u_dist + get(weight, e));
          }
        }

        // Synchronize with all of the other processes.
        synchronize();
        
        // Is the bucket empty now?
        nonempty_bucket = (current_bucket < buckets.size() 
                           && buckets[current_bucket]
                           && !buckets[current_bucket]->empty());
      } while (all_reduce(pg, nonempty_bucket, std::logical_or<bool>()));
      
      // Relax heavy edges for each of the vertices that we previously
      // deleted.
      for (typename std::vector<Vertex>::iterator iter = deleted_vertices.begin();
           iter != deleted_vertices.end(); ++iter) {
        // Relax each heavy edge. 
        Vertex u = *iter;
        Dist u_dist = get(distance, u);
        BGL_FORALL_OUTEDGES_T(u, e, g, Graph)
          if (get(weight, e) > delta) // heavy edge
            relax(u, target(e, g), u_dist + get(weight, e)); 

#ifdef COMPUTE_PATH_COUNTS_INLINE
        // Set outgoing paths
        IncomingType in = get(incoming, u);
        for (typename IncomingType::iterator pred = in.begin(); pred != in.end(); ++pred) 
          if (get(owner, *pred) == id) {
            IncomingType x = get(outgoing, *pred);
            if (std::find(x.begin(), x.end(), u) == x.end())
              x.push_back(u);
            put(outgoing, *pred, x);
          } else {
            IncomingType in;
            in.push_back(u);
            put(outgoing, *pred, in);
          }

        // Set incoming edge counts
        put(incoming_edge_count, u, in.size());
#endif
      }

#ifdef COMPUTE_PATH_COUNTS_INLINE
      synchronize();  // Deliver heavy edge relaxations and outgoing paths

      // Build Queue
      typedef typename property_traits<PathCountMap>::value_type PathCountType;
      typedef std::pair<Vertex, PathCountType> queue_value_type;
      typedef typename property_map<Graph, vertex_owner_t>::const_type OwnerMap;
      typedef typename get_owner_of_first_pair_element<OwnerMap> IndirectOwnerMap;

      typedef boost::queue<queue_value_type> local_queue_type;
      typedef boost::graph::distributed::distributed_queue<process_group_type,
                                                           IndirectOwnerMap,
                                                           local_queue_type> dist_queue_type;

      IndirectOwnerMap indirect_owner(owner);
      dist_queue_type Q(pg, indirect_owner);

      // Find sources to initialize queue
      BGL_FORALL_VERTICES_T(v, g, Graph) {
        if (get(is_settled, v) && !(get(outgoing, v).empty())) {
          put(incoming_edge_count, v, 1); 
          Q.push(std::make_pair(v, 0)); // Push this vertex with no additional path count
        }
      }

      // Set path counts for vertices in this bucket
      while (!Q.empty()) {
        queue_value_type t = Q.top(); Q.pop();
        Vertex v = t.first;
        PathCountType p = t.second;

        put(path_count, v, get(path_count, v) + p);
        put(incoming_edge_count, v, get(incoming_edge_count, v) - 1);

        if (get(incoming_edge_count, v) == 0) {
          IncomingType out = get(outgoing, v);
          for (typename IncomingType::iterator iter = out.begin(); iter != out.end(); ++iter)
            Q.push(std::make_pair(*iter, get(path_count, v)));
        }
      }

      // Mark the vertices in this bucket settled 
      for (typename std::vector<Vertex>::iterator iter = deleted_vertices.begin();
           iter != deleted_vertices.end(); ++iter) 
        put(is_settled, *iter, true);

      // No need to clear path count map as it is never read/written remotely
      // No need to clear outgoing map as it is re-alloced every bucket 
#endif
      
      // Go to the next bucket: the current bucket must already be empty.
      ++current_bucket;
    } while (true);
    
    // Delete all of the buckets.
    for (typename std::vector<Bucket*>::iterator iter = buckets.begin();
         iter != buckets.end(); ++iter) {
      if (*iter) {
        delete *iter;
        *iter = 0;
      }
    }
  }
        
  template<typename Graph, typename DistanceMap, typename IncomingMap, 
           typename EdgeWeightMap, typename PathCountMap
#ifdef COMPUTE_PATH_COUNTS_INLINE
           , typename IsSettledMap, typename VertexIndexMap
#endif
           >
  void
  betweenness_centrality_delta_stepping_impl<
    Graph, DistanceMap, IncomingMap, EdgeWeightMap, PathCountMap
#ifdef COMPUTE_PATH_COUNTS_INLINE
           , IsSettledMap, VertexIndexMap
#endif
    >::
  relax(Vertex u, Vertex v, Dist x)
  {

    if (x <= get(distance, v)) {
      
      // We're relaxing the edge to vertex v.
      if (get(owner, v) == process_id(pg)) {
        if (x < get(distance, v)) {
          // Compute the new bucket index for v
          BucketIndex new_index = static_cast<BucketIndex>(x / delta);
        
          // Make sure there is enough room in the buckets data structure.
          if (new_index >= buckets.size()) buckets.resize(new_index + 1, 0);
        
          // Make sure that we have allocated the bucket itself.
          if (!buckets[new_index]) buckets[new_index] = new Bucket;
          
          if (get(distance, v) != (std::numeric_limits<Dist>::max)()
              && !vertex_was_deleted[get(local, v)]) {
            // We're moving v from an old bucket into a new one. Compute
            // the old index, then splice it in.
            BucketIndex old_index 
              = static_cast<BucketIndex>(get(distance, v) / delta);
            buckets[new_index]->splice(buckets[new_index]->end(),
                                       *buckets[old_index],
                                       position_in_bucket[get(local, v)]);
          } else {
            // We're inserting v into a bucket for the first time. Put it
            // at the end.
            buckets[new_index]->push_back(v);
          }
          
          // v is now at the last position in the new bucket
          position_in_bucket[get(local, v)] = buckets[new_index]->end();
          --position_in_bucket[get(local, v)];
          
          // Update tentative distance information and incoming, path_count
          if (u != v) put(incoming, v, IncomingType(1, u));
          put(distance, v, x);
        }        // u != v covers initial source relaxation and self-loops
        else if (x == get(distance, v) && u != v) {

          // Add incoming edge if it's not already in the list
          IncomingType in = get(incoming, v);
          if (std::find(in.begin(), in.end(), u) == in.end()) {
            in.push_back(u);
            put(incoming, v, in);
          }
        }
      } else {
        // The vertex is remote: send a request to the vertex's owner
        send(pg, get(owner, v), msg_relax, 
             std::make_pair(v, MessageValue::create(x, u)));

        // Cache tentative distance information
        cache(distance, v, x);
      }
    }
  }

  /************************************************************************/
  /* Shortest Paths function object for betweenness centrality            */
  /************************************************************************/

  template<typename WeightMap>
  struct brandes_shortest_paths {
    typedef typename property_traits<WeightMap>::value_type weight_type;

    brandes_shortest_paths() 
      : weight(1), delta(0)  { }
    brandes_shortest_paths(weight_type delta) 
      : weight(1), delta(delta)  { }
    brandes_shortest_paths(WeightMap w) 
      : weight(w), delta(0)  { }
    brandes_shortest_paths(WeightMap w, weight_type delta) 
      : weight(w), delta(delta)  { }

    template<typename Graph, typename IncomingMap, typename DistanceMap,
             typename PathCountMap
#ifdef COMPUTE_PATH_COUNTS_INLINE
             , typename IsSettledMap, typename VertexIndexMap
#endif

             > 
    void 
    operator()(Graph& g, 
               typename graph_traits<Graph>::vertex_descriptor s,
               IncomingMap incoming,
               DistanceMap distance,
               PathCountMap path_count
#ifdef COMPUTE_PATH_COUNTS_INLINE
               , IsSettledMap is_settled,
               VertexIndexMap vertex_index 
#endif
               )
    {  
      // The "distance" map needs to act like one, retrieving the default
      // value of infinity.
      set_property_map_role(vertex_distance, distance);
      
      // Only calculate delta the first time operator() is called
      // This presumes g is the same every time, but so does the fact
      // that we're reusing the weight map
      if (delta == 0)
        set_delta(g);
      
      // TODO (NGE): Restructure the code so we don't have to construct
      //             impl every time?
      betweenness_centrality_delta_stepping_impl<
          Graph, DistanceMap, IncomingMap, WeightMap, PathCountMap
#ifdef COMPUTE_PATH_COUNTS_INLINE
          , IsSettledMap, VertexIndexMap
#endif
            >
        impl(g, distance, incoming, weight, path_count, 
#ifdef COMPUTE_PATH_COUNTS_INLINE
             is_settled, vertex_index, 
#endif
             delta);

      impl.run(s);
    }

  private:

    template <typename Graph>
    void
    set_delta(const Graph& g)
    {
      using boost::parallel::all_reduce;
      using boost::parallel::maximum;
      using std::max;

      typedef typename graph_traits<Graph>::degree_size_type Degree;
      typedef weight_type Dist;

      // Compute the maximum edge weight and degree
      Dist max_edge_weight = 0;
      Degree max_degree = 0;
      BGL_FORALL_VERTICES_T(u, g, Graph) {
        max_degree = max BOOST_PREVENT_MACRO_SUBSTITUTION (max_degree, out_degree(u, g));
        BGL_FORALL_OUTEDGES_T(u, e, g, Graph)
          max_edge_weight = max BOOST_PREVENT_MACRO_SUBSTITUTION (max_edge_weight, get(weight, e));
      }
      
      max_edge_weight = all_reduce(process_group(g), max_edge_weight, maximum<Dist>());
      max_degree = all_reduce(process_group(g), max_degree, maximum<Degree>());
      
      // Take a guess at delta, based on what works well for random
      // graphs.
      delta = max_edge_weight / max_degree;
      if (delta == 0)
        delta = 1;
    }

    WeightMap     weight;
    weight_type   delta;
  };

  // Perform a single SSSP from the specified vertex and update the centrality map(s)
  template<typename Graph, typename CentralityMap, typename EdgeCentralityMap,
           typename IncomingMap, typename DistanceMap, typename DependencyMap, 
           typename PathCountMap, 
#ifdef COMPUTE_PATH_COUNTS_INLINE
           typename IsSettledMap,
#endif 
           typename VertexIndexMap, typename ShortestPaths> 
  void
  do_brandes_sssp(const Graph& g, 
                  CentralityMap centrality,     
                  EdgeCentralityMap edge_centrality_map,
                  IncomingMap incoming,
                  DistanceMap distance,
                  DependencyMap dependency,
                  PathCountMap path_count, 
#ifdef COMPUTE_PATH_COUNTS_INLINE
                  IsSettledMap is_settled,
#endif 
                  VertexIndexMap vertex_index,
                  ShortestPaths shortest_paths,
                  typename graph_traits<Graph>::vertex_descriptor s)
  {
    using boost::detail::graph::update_centrality;      
    using boost::graph::parallel::process_group;

    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename graph_traits<Graph>::edges_size_type edges_size_type;

    typedef typename property_traits<IncomingMap>::value_type incoming_type;
    typedef typename property_traits<DistanceMap>::value_type distance_type;
    typedef typename property_traits<DependencyMap>::value_type dependency_type;
    typedef typename property_traits<PathCountMap>::value_type path_count_type;

    typedef typename incoming_type::iterator incoming_iterator;

    typedef typename property_map<Graph, vertex_owner_t>::const_type OwnerMap;
    OwnerMap owner = get(vertex_owner, g);

    typedef typename boost::graph::parallel::process_group_type<Graph>::type 
      process_group_type;
    process_group_type pg = process_group(g);
    typename process_group_type::process_id_type id = process_id(pg);

    // TODO: Is it faster not to clear some of these maps?
    // Initialize for this iteration
    distance.clear();
    incoming.clear();
    path_count.clear();
    dependency.clear();
    BGL_FORALL_VERTICES_T(v, g, Graph) {
      put(path_count, v, 0);
      put(dependency, v, 0);
    }

    if (get(owner, s) == id) {
      put(incoming, s, incoming_type());
#ifdef COMPUTE_PATH_COUNTS_INLINE
      put(path_count, s, 1);
      put(is_settled, s, true);
#endif
    }

    // Execute the shortest paths algorithm. This will be either
    // a weighted or unweighted customized breadth-first search,
    shortest_paths(g, s, incoming, distance, path_count
#ifdef COMPUTE_PATH_COUNTS_INLINE
                   , is_settled, vertex_index
#endif 
                   );

#ifndef COMPUTE_PATH_COUNTS_INLINE

    //
    // TODO: Optimize case where source has no out-edges
    //
 
    // Count of incoming edges to tell when all incoming edges have been relaxed in 
    // the induced shortest paths DAG 
    std::vector<edges_size_type> incoming_edge_countS(num_vertices(g));
    iterator_property_map<typename std::vector<edges_size_type>::iterator, VertexIndexMap> 
      incoming_edge_count(incoming_edge_countS.begin(), vertex_index);

    BGL_FORALL_VERTICES_T(v, g, Graph) {
      put(incoming_edge_count, v, get(incoming, v).size());
    }

    if (get(owner, s) == id) {
      put(incoming_edge_count, s, 1);
      put(incoming, s, incoming_type());
    }

    std::vector<incoming_type> outgoingS(num_vertices(g));
    iterator_property_map<typename std::vector<incoming_type>::iterator, VertexIndexMap> 
      outgoing(outgoingS.begin(), vertex_index);

    outgoing.set_reduce(append_reducer<incoming_type>());

    // Mark forward adjacencies in DAG of shortest paths

    // TODO: It's possible to do this using edge flags but it's not currently done this way
    //       because during traversal of the DAG we would have to examine all out edges
    //       which would lead to more memory accesses and a larger cache footprint.
    //
    //       In the bidirectional graph case edge flags would be an excellent way of marking
    //       edges in the DAG of shortest paths  
    BGL_FORALL_VERTICES_T(v, g, Graph) {
      incoming_type i = get(incoming, v);
      for (typename incoming_type::iterator iter = i.begin(); iter != i.end(); ++iter) {
        if (get(owner, *iter) == id) {
          incoming_type x = get(outgoing, *iter);
          if (std::find(x.begin(), x.end(), v) == x.end())
            x.push_back(v);
          put(outgoing, *iter, x);
        } else {
          incoming_type in;
          in.push_back(v);
          put(outgoing, *iter, in);
        }
      }
    }

    synchronize(pg);

    // Traverse DAG induced by forward edges in dependency order and compute path counts
    {
      typedef std::pair<vertex_descriptor, path_count_type> queue_value_type;
      typedef get_owner_of_first_pair_element<OwnerMap> IndirectOwnerMap;

      typedef boost::queue<queue_value_type> local_queue_type;
      typedef boost::graph::distributed::distributed_queue<process_group_type,
                                                           IndirectOwnerMap,
                                                           local_queue_type> dist_queue_type;

      IndirectOwnerMap indirect_owner(owner);
      dist_queue_type Q(pg, indirect_owner);

      if (get(owner, s) == id)
        Q.push(std::make_pair(s, 1));

      while (!Q.empty()) {
        queue_value_type t = Q.top(); Q.pop();
        vertex_descriptor v = t.first;
        path_count_type p = t.second;

        put(path_count, v, get(path_count, v) + p);
        put(incoming_edge_count, v, get(incoming_edge_count, v) - 1);

        if (get(incoming_edge_count, v) == 0) {
          incoming_type out = get(outgoing, v);
          for (typename incoming_type::iterator iter = out.begin(); iter != out.end(); ++iter)
            Q.push(std::make_pair(*iter, get(path_count, v)));
        }
      }
    }

#endif // COMPUTE_PATH_COUNTS_INLINE

    //
    // Compute dependencies 
    //    


    // Build the distributed_queue
    // Value type consists of 1) target of update 2) source of update
    // 3) dependency of source 4) path count of source
    typedef boost::tuple<vertex_descriptor, vertex_descriptor, dependency_type, path_count_type>
      queue_value_type;
    typedef get_owner_of_first_tuple_element<OwnerMap, queue_value_type> IndirectOwnerMap;

    typedef boost::queue<queue_value_type> local_queue_type;
    typedef boost::graph::distributed::distributed_queue<process_group_type,
                                                         IndirectOwnerMap,
                                                         local_queue_type> dist_queue_type;

    IndirectOwnerMap indirect_owner(owner);
    dist_queue_type Q(pg, indirect_owner);

    // Calculate number of vertices each vertex depends on, when a vertex has been pushed
    // that number of times then we will update it
    // AND Request path counts of sources of incoming edges
    std::vector<dependency_type> dependency_countS(num_vertices(g), 0);
    iterator_property_map<typename std::vector<dependency_type>::iterator, VertexIndexMap> 
      dependency_count(dependency_countS.begin(), vertex_index);

    dependency_count.set_reduce(boost::graph::distributed::additive_reducer<dependency_type>());

    path_count.set_max_ghost_cells(0);

    BGL_FORALL_VERTICES_T(v, g, Graph) {
      if (get(distance, v) < (std::numeric_limits<distance_type>::max)()) {
        incoming_type el = get(incoming, v);
        for (incoming_iterator vw = el.begin(); vw != el.end(); ++vw) {
          if (get(owner, *vw) == id)
            put(dependency_count, *vw, get(dependency_count, *vw) + 1);
          else {
            put(dependency_count, *vw, 1);

            // Request path counts
            get(path_count, *vw); 
          }

          // request() doesn't work here, perhaps because we don't have a copy of this 
          // ghost cell already?
        }
      }
    }

    synchronize(pg);

    // Push vertices with non-zero distance/path count and zero dependency count
    BGL_FORALL_VERTICES_T(v, g, Graph) {
      if (get(distance, v) < (std::numeric_limits<distance_type>::max)()
          && get(dependency_count, v) == 0) 
        Q.push(boost::make_tuple(v, v, get(dependency, v), get(path_count, v)));
    }

    dependency.set_max_ghost_cells(0);
    while(!Q.empty()) {

      queue_value_type x = Q.top(); Q.pop();
      vertex_descriptor w = boost::tuples::get<0>(x);
      vertex_descriptor source = boost::tuples::get<1>(x);
      dependency_type dep = boost::tuples::get<2>(x);
      path_count_type pc = boost::tuples::get<3>(x);

      cache(dependency, source, dep);
      cache(path_count, source, pc);

      if (get(dependency_count, w) != 0)
        put(dependency_count, w, get(dependency_count, w) - 1);

      if (get(dependency_count, w) == 0) { 

        // Update dependency and centrality of sources of incoming edges
        incoming_type el = get(incoming, w);
        for (incoming_iterator vw = el.begin(); vw != el.end(); ++vw) {
          vertex_descriptor v = *vw;

          BOOST_ASSERT(get(path_count, w) != 0);

          dependency_type factor = dependency_type(get(path_count, v))
            / dependency_type(get(path_count, w));
          factor *= (dependency_type(1) + get(dependency, w));
          
          if (get(owner, v) == id)
            put(dependency, v, get(dependency, v) + factor);
          else
            put(dependency, v, factor);
          
          update_centrality(edge_centrality_map, v, factor);
        }
        
        if (w != s)
          update_centrality(centrality, w, get(dependency, w));

        // Push sources of edges in incoming edge list
        for (incoming_iterator vw = el.begin(); vw != el.end(); ++vw)
          Q.push(boost::make_tuple(*vw, w, get(dependency, w), get(path_count, w)));
      }
    }
  }

  template<typename Graph, typename CentralityMap, typename EdgeCentralityMap,
           typename IncomingMap, typename DistanceMap, typename DependencyMap, 
           typename PathCountMap, typename VertexIndexMap, typename ShortestPaths, 
           typename Buffer>
  void 
  brandes_betweenness_centrality_impl(const Graph& g, 
                                      CentralityMap centrality,     
                                      EdgeCentralityMap edge_centrality_map,
                                      IncomingMap incoming,
                                      DistanceMap distance,
                                      DependencyMap dependency,
                                      PathCountMap path_count, 
                                      VertexIndexMap vertex_index,
                                      ShortestPaths shortest_paths,
                                      Buffer sources)
  {
    using boost::detail::graph::init_centrality_map;
    using boost::detail::graph::divide_centrality_by_two;       
    using boost::graph::parallel::process_group;
    
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

    typedef typename property_traits<DistanceMap>::value_type distance_type;
    typedef typename property_traits<DependencyMap>::value_type dependency_type;

    // Initialize centrality
    init_centrality_map(vertices(g), centrality);
    init_centrality_map(edges(g), edge_centrality_map);

    // Set the reduction operation on the dependency map to be addition
    dependency.set_reduce(boost::graph::distributed::additive_reducer<dependency_type>()); 
    distance.set_reduce(boost::graph::distributed::choose_min_reducer<distance_type>());

    // Don't allow remote procs to write incoming or path_count maps
    // updating them is handled inside the betweenness_centrality_queue
    incoming.set_consistency_model(0);
    path_count.set_consistency_model(0);

    typedef typename boost::graph::parallel::process_group_type<Graph>::type 
      process_group_type;
    process_group_type pg = process_group(g);

#ifdef COMPUTE_PATH_COUNTS_INLINE
    // Build is_settled maps
    std::vector<bool> is_settledS(num_vertices(g));
    typedef iterator_property_map<std::vector<bool>::iterator, VertexIndexMap> 
      IsSettledMap;

    IsSettledMap is_settled(is_settledS.begin(), vertex_index);
#endif

    if (!sources.empty()) {
      // DO SSSPs
      while (!sources.empty()) {
        do_brandes_sssp(g, centrality, edge_centrality_map, incoming, distance,
                        dependency, path_count, 
#ifdef COMPUTE_PATH_COUNTS_INLINE
                        is_settled,
#endif 
                        vertex_index, shortest_paths, sources.top());
        sources.pop();
      }
    } else { // Exact Betweenness Centrality
      typedef typename graph_traits<Graph>::vertices_size_type vertices_size_type;
      vertices_size_type n = num_vertices(g);
      n = boost::parallel::all_reduce(pg, n, std::plus<vertices_size_type>());
      
      for (vertices_size_type i = 0; i < n; ++i) {
        vertex_descriptor v = vertex(i, g);

        do_brandes_sssp(g, centrality, edge_centrality_map, incoming, distance,
                        dependency, path_count, 
#ifdef COMPUTE_PATH_COUNTS_INLINE
                        is_settled,
#endif 
                        vertex_index, shortest_paths, v);
      }
    }

    typedef typename graph_traits<Graph>::directed_category directed_category;
    const bool is_undirected = 
      is_convertible<directed_category*, undirected_tag*>::value;
    if (is_undirected) {
      divide_centrality_by_two(vertices(g), centrality);
      divide_centrality_by_two(edges(g), edge_centrality_map);
    }
  }

  template<typename Graph, typename CentralityMap, typename EdgeCentralityMap,
           typename IncomingMap, typename DistanceMap, typename DependencyMap, 
           typename PathCountMap, typename VertexIndexMap, typename ShortestPaths,
           typename Stack>
  void
  do_sequential_brandes_sssp(const Graph& g, 
                             CentralityMap centrality,     
                             EdgeCentralityMap edge_centrality_map,
                             IncomingMap incoming,
                             DistanceMap distance,
                             DependencyMap dependency,
                             PathCountMap path_count, 
                             VertexIndexMap vertex_index,
                             ShortestPaths shortest_paths,
                             Stack& ordered_vertices,
                             typename graph_traits<Graph>::vertex_descriptor v)
  {
    using boost::detail::graph::update_centrality;

    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

    // Initialize for this iteration
    BGL_FORALL_VERTICES_T(w, g, Graph) {
      // put(path_count, w, 0);
      incoming[w].clear();
      put(dependency, w, 0);
    }

    put(path_count, v, 1);
    incoming[v].clear();

    // Execute the shortest paths algorithm. This will be either
    // Dijkstra's algorithm or a customized breadth-first search,
    // depending on whether the graph is weighted or unweighted.
    shortest_paths(g, v, ordered_vertices, incoming, distance,
                   path_count, vertex_index);
    
    while (!ordered_vertices.empty()) {
      vertex_descriptor w = ordered_vertices.top();
      ordered_vertices.pop();
      
      typedef typename property_traits<IncomingMap>::value_type
            incoming_type;
      typedef typename incoming_type::iterator incoming_iterator;
      typedef typename property_traits<DependencyMap>::value_type 
        dependency_type;
      
      for (incoming_iterator vw = incoming[w].begin();
           vw != incoming[w].end(); ++vw) {
        vertex_descriptor v = source(*vw, g);
        dependency_type factor = dependency_type(get(path_count, v))
          / dependency_type(get(path_count, w));
        factor *= (dependency_type(1) + get(dependency, w));
        put(dependency, v, get(dependency, v) + factor);
        update_centrality(edge_centrality_map, *vw, factor);
      }
      
      if (w != v) {
        update_centrality(centrality, w, get(dependency, w));
      }
    }
  }

  // Betweenness Centrality variant that duplicates graph across processors
  // and parallizes SSSPs
  // This function expects a non-distributed graph and property-maps
  template<typename ProcessGroup, typename Graph, 
           typename CentralityMap, typename EdgeCentralityMap,
           typename IncomingMap, typename DistanceMap, 
           typename DependencyMap, typename PathCountMap,
           typename VertexIndexMap, typename ShortestPaths,
           typename Buffer>
  void
  non_distributed_brandes_betweenness_centrality_impl(const ProcessGroup& pg,
                                                      const Graph& g,
                                                      CentralityMap centrality,
                                                      EdgeCentralityMap edge_centrality_map,
                                                      IncomingMap incoming, // P
                                                      DistanceMap distance,         // d
                                                      DependencyMap dependency,     // delta
                                                      PathCountMap path_count,      // sigma
                                                      VertexIndexMap vertex_index,
                                                      ShortestPaths shortest_paths,
                                                      Buffer sources)
  {
    using boost::detail::graph::init_centrality_map;
    using boost::detail::graph::divide_centrality_by_two;       
    using boost::graph::parallel::process_group;

    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

    typedef ProcessGroup process_group_type;

    typename process_group_type::process_id_type id = process_id(pg);
    typename process_group_type::process_size_type p = num_processes(pg);

    // Initialize centrality
    init_centrality_map(vertices(g), centrality);
    init_centrality_map(edges(g), edge_centrality_map);

    std::stack<vertex_descriptor> ordered_vertices;

    if (!sources.empty()) {
      std::vector<vertex_descriptor> local_sources;

      for (int i = 0; i < id; ++i) if (!sources.empty()) sources.pop();
      while (!sources.empty()) {
        local_sources.push_back(sources.top());

        for (int i = 0; i < p; ++i) if (!sources.empty()) sources.pop();
      }

      // DO SSSPs
      for(size_t i = 0; i < local_sources.size(); ++i)
        do_sequential_brandes_sssp(g, centrality, edge_centrality_map, incoming,
                                   distance, dependency, path_count, vertex_index,
                                   shortest_paths, ordered_vertices, local_sources[i]);

    } else { // Exact Betweenness Centrality
      typedef typename graph_traits<Graph>::vertices_size_type vertices_size_type;
      vertices_size_type n = num_vertices(g);
      
      for (vertices_size_type i = id; i < n; i += p) {
        vertex_descriptor v = vertex(i, g);

        do_sequential_brandes_sssp(g, centrality, edge_centrality_map, incoming,
                                   distance, dependency, path_count, vertex_index,
                                   shortest_paths, ordered_vertices, v);
      }
    }

    typedef typename graph_traits<Graph>::directed_category directed_category;
    const bool is_undirected = 
      is_convertible<directed_category*, undirected_tag*>::value;
    if (is_undirected) {
      divide_centrality_by_two(vertices(g), centrality);
      divide_centrality_by_two(edges(g), edge_centrality_map);
    }

    // Merge the centrality maps by summing the values at each vertex)
    // TODO(nge): this copy-out, reduce, copy-in is lame
    typedef typename property_traits<CentralityMap>::value_type centrality_type;
    typedef typename property_traits<EdgeCentralityMap>::value_type edge_centrality_type;

    std::vector<centrality_type> centrality_v(num_vertices(g));
    std::vector<edge_centrality_type> edge_centrality_v;
    edge_centrality_v.reserve(num_edges(g));

    BGL_FORALL_VERTICES_T(v, g, Graph) {
      centrality_v[get(vertex_index, v)] = get(centrality, v);
    }
    
    // Skip when EdgeCentralityMap is a dummy_property_map
    if (!is_same<EdgeCentralityMap, dummy_property_map>::value) {
      BGL_FORALL_EDGES_T(e, g, Graph) {
        edge_centrality_v.push_back(get(edge_centrality_map, e));
      }
      // NGE: If we trust that the order of elements in the vector isn't changed in the
      //      all_reduce below then this method avoids the need for an edge index map
    }

    using boost::parallel::all_reduce;

    all_reduce(pg, &centrality_v[0], &centrality_v[centrality_v.size()],
               &centrality_v[0], std::plus<centrality_type>());

    if (edge_centrality_v.size()) 
      all_reduce(pg, &edge_centrality_v[0], &edge_centrality_v[edge_centrality_v.size()],
                 &edge_centrality_v[0], std::plus<edge_centrality_type>());

    BGL_FORALL_VERTICES_T(v, g, Graph) {
      put(centrality, v, centrality_v[get(vertex_index, v)]);
    }

    // Skip when EdgeCentralityMap is a dummy_property_map
    if (!is_same<EdgeCentralityMap, dummy_property_map>::value) {
      int i = 0;
      BGL_FORALL_EDGES_T(e, g, Graph) {
        put(edge_centrality_map, e, edge_centrality_v[i]);
        ++i;
      }
    }
  }

} } } // end namespace graph::parallel::detail

template<typename Graph, typename CentralityMap, typename EdgeCentralityMap,
         typename IncomingMap, typename DistanceMap, typename DependencyMap, 
         typename PathCountMap, typename VertexIndexMap, typename Buffer>
void 
brandes_betweenness_centrality(const Graph& g, 
                               CentralityMap centrality,
                               EdgeCentralityMap edge_centrality_map,
                               IncomingMap incoming, 
                               DistanceMap distance, 
                               DependencyMap dependency,     
                               PathCountMap path_count,   
                               VertexIndexMap vertex_index,
                               Buffer sources,
                               typename property_traits<DistanceMap>::value_type delta
                               BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph,distributed_graph_tag))
{
  typedef typename property_traits<DistanceMap>::value_type distance_type;
  typedef static_property_map<distance_type> WeightMap;

  graph::parallel::detail::brandes_shortest_paths<WeightMap> 
    shortest_paths(delta);

  graph::parallel::detail::brandes_betweenness_centrality_impl(g, centrality, 
                                                               edge_centrality_map,
                                                               incoming, distance,
                                                               dependency, path_count,
                                                               vertex_index, 
                                                               shortest_paths,
                                                               sources);
}

template<typename Graph, typename CentralityMap, typename EdgeCentralityMap, 
         typename IncomingMap, typename DistanceMap, typename DependencyMap, 
         typename PathCountMap, typename VertexIndexMap, typename WeightMap, 
         typename Buffer>    
void 
brandes_betweenness_centrality(const Graph& g, 
                               CentralityMap centrality,
                               EdgeCentralityMap edge_centrality_map,
                               IncomingMap incoming, 
                               DistanceMap distance, 
                               DependencyMap dependency,
                               PathCountMap path_count, 
                               VertexIndexMap vertex_index,
                               Buffer sources,
                               typename property_traits<WeightMap>::value_type delta,
                               WeightMap weight_map
                               BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph,distributed_graph_tag))
{
  graph::parallel::detail::brandes_shortest_paths<WeightMap> shortest_paths(weight_map, delta);

  graph::parallel::detail::brandes_betweenness_centrality_impl(g, centrality, 
                                                               edge_centrality_map,
                                                               incoming, distance,
                                                               dependency, path_count,
                                                               vertex_index, 
                                                               shortest_paths,
                                                               sources);
}

namespace graph { namespace parallel { namespace detail {
  template<typename Graph, typename CentralityMap, typename EdgeCentralityMap,
           typename WeightMap, typename VertexIndexMap, typename Buffer>
  void 
  brandes_betweenness_centrality_dispatch2(const Graph& g,
                                           CentralityMap centrality,
                                           EdgeCentralityMap edge_centrality_map,
                                           WeightMap weight_map,
                                           VertexIndexMap vertex_index,
                                           Buffer sources,
                                           typename property_traits<WeightMap>::value_type delta)
  {
    typedef typename graph_traits<Graph>::degree_size_type degree_size_type;
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename mpl::if_c<(is_same<CentralityMap, 
                                        dummy_property_map>::value),
                                         EdgeCentralityMap, 
                               CentralityMap>::type a_centrality_map;
    typedef typename property_traits<a_centrality_map>::value_type 
      centrality_type;

    typename graph_traits<Graph>::vertices_size_type V = num_vertices(g);

    std::vector<std::vector<vertex_descriptor> > incoming(V);
    std::vector<centrality_type> distance(V);
    std::vector<centrality_type> dependency(V);
    std::vector<degree_size_type> path_count(V);

    brandes_betweenness_centrality(
      g, centrality, edge_centrality_map,
      make_iterator_property_map(incoming.begin(), vertex_index),
      make_iterator_property_map(distance.begin(), vertex_index),
      make_iterator_property_map(dependency.begin(), vertex_index),
      make_iterator_property_map(path_count.begin(), vertex_index),
      vertex_index, unwrap_ref(sources), delta,
      weight_map);
  }
  
  // TODO: Should the type of the distance and dependency map depend on the 
  //       value type of the centrality map?
  template<typename Graph, typename CentralityMap, typename EdgeCentralityMap,
           typename VertexIndexMap, typename Buffer>
  void 
  brandes_betweenness_centrality_dispatch2(const Graph& g,
                                           CentralityMap centrality,
                                           EdgeCentralityMap edge_centrality_map,
                                           VertexIndexMap vertex_index,
                                           Buffer sources,
                                           typename graph_traits<Graph>::edges_size_type delta)
  {
    typedef typename graph_traits<Graph>::degree_size_type degree_size_type;
    typedef typename graph_traits<Graph>::edges_size_type edges_size_type;
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

    typename graph_traits<Graph>::vertices_size_type V = num_vertices(g);
    
    std::vector<std::vector<vertex_descriptor> > incoming(V);
    std::vector<edges_size_type> distance(V);
    std::vector<edges_size_type> dependency(V);
    std::vector<degree_size_type> path_count(V);

    brandes_betweenness_centrality(
      g, centrality, edge_centrality_map,
      make_iterator_property_map(incoming.begin(), vertex_index),
      make_iterator_property_map(distance.begin(), vertex_index),
      make_iterator_property_map(dependency.begin(), vertex_index),
      make_iterator_property_map(path_count.begin(), vertex_index),
      vertex_index, unwrap_ref(sources), delta); 
  }

  template<typename WeightMap>
  struct brandes_betweenness_centrality_dispatch1
  {
    template<typename Graph, typename CentralityMap, typename EdgeCentralityMap, 
             typename VertexIndexMap, typename Buffer>
    static void 
    run(const Graph& g, CentralityMap centrality, EdgeCentralityMap edge_centrality_map, 
        VertexIndexMap vertex_index, Buffer sources,
        typename property_traits<WeightMap>::value_type delta, WeightMap weight_map) 
    {
      boost::graph::parallel::detail::brandes_betweenness_centrality_dispatch2(
       g, centrality, edge_centrality_map, weight_map, vertex_index, sources, delta);
    }
  };

  template<>
  struct brandes_betweenness_centrality_dispatch1<boost::param_not_found> 
  {
    template<typename Graph, typename CentralityMap, typename EdgeCentralityMap, 
             typename VertexIndexMap, typename Buffer>
    static void 
    run(const Graph& g, CentralityMap centrality, EdgeCentralityMap edge_centrality_map, 
        VertexIndexMap vertex_index, Buffer sources,
        typename graph_traits<Graph>::edges_size_type delta,
        boost::param_not_found)
    {
      boost::graph::parallel::detail::brandes_betweenness_centrality_dispatch2(
       g, centrality, edge_centrality_map, vertex_index, sources, delta);
    }
  };

} } } // end namespace graph::parallel::detail

template<typename Graph, typename Param, typename Tag, typename Rest>
void 
brandes_betweenness_centrality(const Graph& g, 
                               const bgl_named_params<Param,Tag,Rest>& params
                               BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph,distributed_graph_tag))
{
  typedef bgl_named_params<Param,Tag,Rest> named_params;

  typedef queue<typename graph_traits<Graph>::vertex_descriptor> queue_t;
  queue_t q;

  typedef typename get_param_type<edge_weight_t, named_params>::type ew_param;
  typedef typename detail::choose_impl_result<mpl::true_, Graph, ew_param, edge_weight_t>::type ew;
  graph::parallel::detail::brandes_betweenness_centrality_dispatch1<ew>::run(
    g, 
    choose_param(get_param(params, vertex_centrality), 
                 dummy_property_map()),
    choose_param(get_param(params, edge_centrality), 
                 dummy_property_map()),
    choose_const_pmap(get_param(params, vertex_index), g, vertex_index),
    choose_param(get_param(params, buffer_param_t()), boost::ref(q)),
    choose_param(get_param(params, lookahead_t()), 0),
    choose_const_pmap(get_param(params, edge_weight), g, edge_weight));
}

template<typename Graph, typename CentralityMap>
void 
brandes_betweenness_centrality(const Graph& g, CentralityMap centrality
                               BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph,distributed_graph_tag))
{
  typedef queue<typename graph_traits<Graph>::vertex_descriptor> queue_t;
  queue_t q;

  boost::graph::parallel::detail::brandes_betweenness_centrality_dispatch2(
    g, centrality, dummy_property_map(), get(vertex_index, g), boost::ref(q), 0);
}

template<typename Graph, typename CentralityMap, typename EdgeCentralityMap>
void 
brandes_betweenness_centrality(const Graph& g, CentralityMap centrality,
                               EdgeCentralityMap edge_centrality_map
                               BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph,distributed_graph_tag))
{
  typedef queue<int> queue_t;
  queue_t q;

  boost::graph::parallel::detail::brandes_betweenness_centrality_dispatch2(
    g, centrality, edge_centrality_map, get(vertex_index, g), boost::ref(q), 0);
}
  
template<typename ProcessGroup, typename Graph, typename CentralityMap, 
         typename EdgeCentralityMap, typename IncomingMap, typename DistanceMap, 
         typename DependencyMap, typename PathCountMap, typename VertexIndexMap, 
         typename Buffer>
void 
non_distributed_brandes_betweenness_centrality(const ProcessGroup& pg,
                                               const Graph& g, 
                                               CentralityMap centrality,
                                               EdgeCentralityMap edge_centrality_map,
                                               IncomingMap incoming, 
                                               DistanceMap distance, 
                                               DependencyMap dependency,     
                                               PathCountMap path_count,      
                                               VertexIndexMap vertex_index,
                                               Buffer sources)
{
  detail::graph::brandes_unweighted_shortest_paths shortest_paths;
  
  graph::parallel::detail::non_distributed_brandes_betweenness_centrality_impl(pg, g, centrality, 
                                                                               edge_centrality_map,
                                                                               incoming, distance,
                                                                               dependency, path_count,
                                                                               vertex_index, 
                                                                               shortest_paths,
                                                                               sources);
}
  
template<typename ProcessGroup, typename Graph, typename CentralityMap, 
         typename EdgeCentralityMap, typename IncomingMap, typename DistanceMap, 
         typename DependencyMap, typename PathCountMap, typename VertexIndexMap, 
         typename WeightMap, typename Buffer>
void 
non_distributed_brandes_betweenness_centrality(const ProcessGroup& pg,
                                               const Graph& g, 
                                               CentralityMap centrality,
                                               EdgeCentralityMap edge_centrality_map,
                                               IncomingMap incoming, 
                                               DistanceMap distance, 
                                               DependencyMap dependency,
                                               PathCountMap path_count, 
                                               VertexIndexMap vertex_index,
                                               WeightMap weight_map,
                                               Buffer sources)
{
  detail::graph::brandes_dijkstra_shortest_paths<WeightMap> shortest_paths(weight_map);

  graph::parallel::detail::non_distributed_brandes_betweenness_centrality_impl(pg, g, centrality, 
                                                                               edge_centrality_map,
                                                                               incoming, distance,
                                                                               dependency, path_count,
                                                                               vertex_index, 
                                                                               shortest_paths,
                                                                               sources);
}

namespace detail { namespace graph {
  template<typename ProcessGroup, typename Graph, typename CentralityMap, 
           typename EdgeCentralityMap, typename WeightMap, typename VertexIndexMap,
           typename Buffer>
  void 
  non_distributed_brandes_betweenness_centrality_dispatch2(const ProcessGroup& pg,
                                                           const Graph& g,
                                                           CentralityMap centrality,
                                                           EdgeCentralityMap edge_centrality_map,
                                                           WeightMap weight_map,
                                                           VertexIndexMap vertex_index,
                                                           Buffer sources)
  {
    typedef typename graph_traits<Graph>::degree_size_type degree_size_type;
    typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
    typedef typename mpl::if_c<(is_same<CentralityMap, 
                                        dummy_property_map>::value),
                                         EdgeCentralityMap, 
                               CentralityMap>::type a_centrality_map;
    typedef typename property_traits<a_centrality_map>::value_type 
      centrality_type;

    typename graph_traits<Graph>::vertices_size_type V = num_vertices(g);
    
    std::vector<std::vector<edge_descriptor> > incoming(V);
    std::vector<centrality_type> distance(V);
    std::vector<centrality_type> dependency(V);
    std::vector<degree_size_type> path_count(V);

    non_distributed_brandes_betweenness_centrality(
      pg, g, centrality, edge_centrality_map,
      make_iterator_property_map(incoming.begin(), vertex_index),
      make_iterator_property_map(distance.begin(), vertex_index),
      make_iterator_property_map(dependency.begin(), vertex_index),
      make_iterator_property_map(path_count.begin(), vertex_index),
      vertex_index, weight_map, unwrap_ref(sources));
  }
  

  template<typename ProcessGroup, typename Graph, typename CentralityMap, 
           typename EdgeCentralityMap, typename VertexIndexMap, typename Buffer>
  void 
  non_distributed_brandes_betweenness_centrality_dispatch2(const ProcessGroup& pg,
                                                           const Graph& g,
                                                           CentralityMap centrality,
                                                           EdgeCentralityMap edge_centrality_map,
                                                           VertexIndexMap vertex_index,
                                                           Buffer sources)
  {
    typedef typename graph_traits<Graph>::degree_size_type degree_size_type;
    typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
    typedef typename mpl::if_c<(is_same<CentralityMap, 
                                        dummy_property_map>::value),
                                         EdgeCentralityMap, 
                               CentralityMap>::type a_centrality_map;
    typedef typename property_traits<a_centrality_map>::value_type 
      centrality_type;

    typename graph_traits<Graph>::vertices_size_type V = num_vertices(g);
    
    std::vector<std::vector<edge_descriptor> > incoming(V);
    std::vector<centrality_type> distance(V);
    std::vector<centrality_type> dependency(V);
    std::vector<degree_size_type> path_count(V);

    non_distributed_brandes_betweenness_centrality(
      pg, g, centrality, edge_centrality_map,
      make_iterator_property_map(incoming.begin(), vertex_index),
      make_iterator_property_map(distance.begin(), vertex_index),
      make_iterator_property_map(dependency.begin(), vertex_index),
      make_iterator_property_map(path_count.begin(), vertex_index),
      vertex_index, unwrap_ref(sources));
  }

  template<typename WeightMap>
  struct non_distributed_brandes_betweenness_centrality_dispatch1
  {
    template<typename ProcessGroup, typename Graph, typename CentralityMap, 
             typename EdgeCentralityMap, typename VertexIndexMap, typename Buffer>
    static void 
    run(const ProcessGroup& pg, const Graph& g, CentralityMap centrality, 
        EdgeCentralityMap edge_centrality_map, VertexIndexMap vertex_index,
        Buffer sources, WeightMap weight_map)
    {
      non_distributed_brandes_betweenness_centrality_dispatch2(pg, g, centrality, edge_centrality_map,
                                                               weight_map, vertex_index, sources);
    }
  };

  template<>
  struct non_distributed_brandes_betweenness_centrality_dispatch1<param_not_found>
  {
    template<typename ProcessGroup, typename Graph, typename CentralityMap, 
             typename EdgeCentralityMap, typename VertexIndexMap, typename Buffer>
    static void 
    run(const ProcessGroup& pg, const Graph& g, CentralityMap centrality, 
        EdgeCentralityMap edge_centrality_map, VertexIndexMap vertex_index,
        Buffer sources, param_not_found)
    {
      non_distributed_brandes_betweenness_centrality_dispatch2(pg, g, centrality, edge_centrality_map,
                                                               vertex_index, sources);
    }
  };

} } // end namespace detail::graph

template<typename ProcessGroup, typename Graph, typename Param, typename Tag, typename Rest>
void 
non_distributed_brandes_betweenness_centrality(const ProcessGroup& pg, const Graph& g, 
                                               const bgl_named_params<Param,Tag,Rest>& params)
{
  typedef bgl_named_params<Param,Tag,Rest> named_params;

  typedef queue<int> queue_t;
  queue_t q;

  typedef typename get_param_type<edge_weight_t, named_params>::type ew_param;
  typedef typename detail::choose_impl_result<mpl::true_, Graph, ew_param, edge_weight_t>::type ew;
  detail::graph::non_distributed_brandes_betweenness_centrality_dispatch1<ew>::run(
    pg, g, 
    choose_param(get_param(params, vertex_centrality), 
                 dummy_property_map()),
    choose_param(get_param(params, edge_centrality), 
                 dummy_property_map()),
    choose_const_pmap(get_param(params, vertex_index), g, vertex_index),
    choose_param(get_param(params, buffer_param_t()),  boost::ref(q)),
    choose_const_pmap(get_param(params, edge_weight), g, edge_weight));
}

template<typename ProcessGroup, typename Graph, typename CentralityMap>
void 
non_distributed_brandes_betweenness_centrality(const ProcessGroup& pg, const Graph& g, 
                                               CentralityMap centrality)
{
  typedef queue<int> queue_t;
  queue_t q;

  detail::graph::non_distributed_brandes_betweenness_centrality_dispatch2(
    pg, g, centrality, dummy_property_map(), get(vertex_index, g), boost::ref(q));
}

template<typename ProcessGroup, typename Graph, typename CentralityMap, 
         typename Buffer>
void 
non_distributed_brandes_betweenness_centrality(const ProcessGroup& pg, const Graph& g, 
                                               CentralityMap centrality, Buffer sources)
{
  detail::graph::non_distributed_brandes_betweenness_centrality_dispatch2(
    pg, g, centrality, dummy_property_map(), get(vertex_index, g), sources);
}

template<typename ProcessGroup, typename Graph, typename CentralityMap, 
         typename EdgeCentralityMap, typename Buffer>
void 
non_distributed_brandes_betweenness_centrality(const ProcessGroup& pg, const Graph& g, 
                                               CentralityMap centrality,
                                               EdgeCentralityMap edge_centrality_map, 
                                               Buffer sources)
{
  detail::graph::non_distributed_brandes_betweenness_centrality_dispatch2(
    pg, g, centrality, edge_centrality_map, get(vertex_index, g), sources);
}

// Compute the central point dominance of a graph.
// TODO: Make sure central point dominance works in parallel case
template<typename Graph, typename CentralityMap>
typename property_traits<CentralityMap>::value_type
central_point_dominance(const Graph& g, CentralityMap centrality
                        BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph,distributed_graph_tag))
{
  using std::max;

  typedef typename graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef typename property_traits<CentralityMap>::value_type centrality_type;
  typedef typename graph_traits<Graph>::vertices_size_type vertices_size_type;

  typedef typename boost::graph::parallel::process_group_type<Graph>::type 
    process_group_type;
  process_group_type pg = boost::graph::parallel::process_group(g);

  vertices_size_type n = num_vertices(g);

  using boost::parallel::all_reduce;  
  n = all_reduce(pg, n, std::plus<vertices_size_type>());

  // Find max centrality
  centrality_type max_centrality(0);
  vertex_iterator v, v_end;
  for (boost::tie(v, v_end) = vertices(g); v != v_end; ++v) {
    max_centrality = (max)(max_centrality, get(centrality, *v));
  }

  // All reduce to get global max centrality
  max_centrality = all_reduce(pg, max_centrality, boost::parallel::maximum<centrality_type>());

  // Compute central point dominance
  centrality_type sum(0);
  for (boost::tie(v, v_end) = vertices(g); v != v_end; ++v) {
    sum += (max_centrality - get(centrality, *v));
  }

  sum = all_reduce(pg, sum, std::plus<centrality_type>());

  return sum/(n-1);
}

} // end namespace boost

#endif // BOOST_GRAPH_PARALLEL_BRANDES_BETWEENNESS_CENTRALITY_HPP
