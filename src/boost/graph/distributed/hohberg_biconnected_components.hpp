// Copyright 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

// An implementation of Walter Hohberg's distributed biconnected
// components algorithm, from:
//
//   Walter Hohberg. How to Find Biconnected Components in Distributed
//   Networks. J. Parallel Distrib. Comput., 9(4):374-386, 1990.
//
#ifndef BOOST_GRAPH_DISTRIBUTED_HOHBERG_BICONNECTED_COMPONENTS_HPP
#define BOOST_GRAPH_DISTRIBUTED_HOHBERG_BICONNECTED_COMPONENTS_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

/* You can define PBGL_HOHBERG_DEBUG to an integer value (1, 2, or 3)
 * to enable debugging information. 1 includes only the phases of the
 * algorithm and messages as their are received. 2 and 3 add
 * additional levels of detail about internal data structures related
 * to the algorithm itself.
 *
 * #define PBGL_HOHBERG_DEBUG 1
*/

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/parallel/container_traits.hpp>
#include <boost/graph/parallel/process_group.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpi/operations.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/optional.hpp>
#include <utility> // for std::pair
#include <boost/assert.hpp>
#include <algorithm> // for std::find, std::mismatch
#include <vector>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/graph/distributed/connected_components.hpp>
#include <boost/concept/assert.hpp>

namespace boost { namespace graph { namespace distributed {

namespace hohberg_detail {
  enum message_kind {
    /* A header for the PATH message, stating which edge the message
       is coming on and how many vertices will be following. The data
       structure communicated will be a path_header. */
    msg_path_header,
    /* A message containing the vertices that make up a path. It will
       always follow a msg_path_header and will contain vertex
       descriptors, only. */
    msg_path_vertices,
    /* A header for the TREE message, stating the value of gamma and
       the number of vertices to come in the following
       msg_tree_vertices. */
    msg_tree_header,
    /* A message containing the vertices that make up the set of
       vertices in the same bicomponent as the sender. It will always
       follow a msg_tree_header and will contain vertex descriptors,
       only. */
    msg_tree_vertices,
    /* Provides a name for the biconnected component of the edge. */
    msg_name
  };

  // Payload for a msg_path_header message.
  template<typename EdgeDescriptor>
  struct path_header
  {
    // The edge over which the path is being communicated
    EdgeDescriptor edge;

    // The length of the path, i.e., the number of vertex descriptors
    // that will be coming in the next msg_path_vertices message.
    std::size_t    path_length;

    template<typename Archiver>
    void serialize(Archiver& ar, const unsigned int /*version*/)
    {
      ar & edge & path_length;
    }
  };

  // Payload for a msg_tree_header message.
  template<typename Vertex, typename Edge>
  struct tree_header
  {
    // The edge over which the tree is being communicated
    Edge edge;

    // Gamma, which is the eta of the sender.
    Vertex gamma;

    // The length of the list of vertices in the bicomponent, i.e.,
    // the number of vertex descriptors that will be coming in the
    // next msg_tree_vertices message.
    std::size_t    bicomp_length;

    template<typename Archiver>
    void serialize(Archiver& ar, const unsigned int /*version*/)
    {
      ar & edge & gamma & bicomp_length;
    }
  };

  // Payload for the msg_name message.
  template<typename EdgeDescriptor>
  struct name_header
  {
    // The edge being communicated and named.
    EdgeDescriptor edge;

    // The 0-based name of the component
    std::size_t name;

    template<typename Archiver>
    void serialize(Archiver& ar, const unsigned int /*version*/)
    {
      ar & edge & name;
    }
  };

  /* Computes the branch point between two paths. The branch point is
     the last position at which both paths are equivalent, beyond
     which the paths diverge. Both paths must have length > 0 and the
     initial elements of the paths must be equal. This is guaranteed
     in Hohberg's algorithm because all paths start at the
     leader. Returns the value at the branch point. */
  template<typename T>
  T branch_point(const std::vector<T>& p1, const std::vector<T>& p2)
  {
    BOOST_ASSERT(!p1.empty());
    BOOST_ASSERT(!p2.empty());
    BOOST_ASSERT(p1.front() == p2.front());

    typedef typename std::vector<T>::const_iterator iterator;

    iterator mismatch_pos;
    if (p1.size() <= p2.size())
      mismatch_pos = std::mismatch(p1.begin(), p1.end(), p2.begin()).first;
    else
      mismatch_pos = std::mismatch(p2.begin(), p2.end(), p1.begin()).first;
    --mismatch_pos;
    return *mismatch_pos;
  }

  /* Computes the infimum of vertices a and b in the given path. The
     infimum is the largest element that is on the paths from a to the
     root and from b to the root. */
  template<typename T>
  T infimum(const std::vector<T>& parent_path, T a, T b)
  {
    using std::swap;

    typedef typename std::vector<T>::const_iterator iterator;
    iterator first = parent_path.begin(), last = parent_path.end();

#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 2
    std::cerr << "infimum(";
    for (iterator i = first; i != last; ++i) {
      if (i != first) std::cerr << ' ';
      std::cerr << local(*i) << '@' << owner(*i);
    }
    std::cerr << ", " << local(a) << '@' << owner(a) << ", "
              << local(b) << '@' << owner(b) << ") = ";
#endif

    if (a == b) {
#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 2
      std::cerr << local(a) << '@' << owner(a) << std::endl;
#endif
      return a;
    }

    // Try to find a or b, whichever is closest to the end
    --last;
    while (*last != a) {
      // If we match b, swap the two so we'll be looking for b later.
      if (*last == b) { swap(a,b); break; }

      if (last == first) {
#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 2
        std::cerr << local(*first) << '@' << owner(*first) << std::endl;
#endif
        return *first;
      }
      else --last;
    }

    // Try to find b (which may originally have been a)
    while (*last != b) {
      if (last == first) {
#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 2
        std::cerr << local(*first) << '@' << owner(*first) << std::endl;
#endif
        return *first;
      }
      else --last;
    }

#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 2
    std::cerr << local(*last) << '@' << owner(*last) << std::endl;
#endif
    // We've found b; it's the infimum.
    return *last;
  }
} // end namespace hohberg_detail

/* A message that will be stored for each edge by Hohberg's algorithm. */
template<typename Graph>
struct hohberg_message
{
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::edge_descriptor   Edge;

  // Assign from a path message
  void assign(const std::vector<Vertex>& path)
  {
    gamma = graph_traits<Graph>::null_vertex();
    path_or_bicomp = path;
  }

  // Assign from a tree message
  void assign(Vertex gamma, const std::vector<Vertex>& in_same_bicomponent)
  {
    this->gamma = gamma;
    path_or_bicomp = in_same_bicomponent;
  }

  bool is_path() const { return gamma == graph_traits<Graph>::null_vertex(); }
  bool is_tree() const { return gamma != graph_traits<Graph>::null_vertex(); }

  /// The "gamma" of a tree message, or null_vertex() for a path message
  Vertex gamma;

  // Either the path for a path message or the in_same_bicomponent
  std::vector<Vertex> path_or_bicomp;
};


/* An abstraction of a vertex processor in Hohberg's algorithm. The
   hohberg_vertex_processor class is responsible for processing
   messages transmitted to it via its edges. */
template<typename Graph>
class hohberg_vertex_processor
{
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef typename graph_traits<Graph>::edge_descriptor   Edge;
  typedef typename graph_traits<Graph>::degree_size_type  degree_size_type;
  typedef typename graph_traits<Graph>::edges_size_type   edges_size_type;
  typedef typename boost::graph::parallel::process_group_type<Graph>::type
    ProcessGroup;
  typedef std::vector<Vertex> path_t;
  typedef typename path_t::iterator path_iterator;

 public:
  hohberg_vertex_processor()
    : phase(1),
      parent(graph_traits<Graph>::null_vertex()),
      eta(graph_traits<Graph>::null_vertex())
  {
  }

  // Called to initialize a leader in the algorithm, which involves
  // sending out the initial path messages and being ready to receive
  // them.
  void initialize_leader(Vertex alpha, const Graph& g);

  /// Handle a path message on edge e. The path will be destroyed by
  /// this operation.
  void
  operator()(Edge e, path_t& path, const Graph& g);

  /// Handle a tree message on edge e. in_same_bicomponent will be
  /// destroyed by this operation.
  void
  operator()(Edge e, Vertex gamma, path_t& in_same_bicomponent,
             const Graph& g);

  // Handle a name message.
  void operator()(Edge e, edges_size_type name, const Graph& g);

  // Retrieve the phase
  unsigned char get_phase() const { return phase; }

  // Start the naming phase. The current phase must be 3 (echo), and
  // the offset contains the offset at which this processor should
  // begin when labelling its bicomponents. The offset is just a
  // parallel prefix sum of the number of bicomponents in each
  // processor that precedes it (globally).
  void
  start_naming_phase(Vertex alpha, const Graph& g, edges_size_type offset);

  /* Determine the number of bicomponents that we will be naming
   * ourselves.
   */
  edges_size_type num_starting_bicomponents(Vertex alpha, const Graph& g);

  // Fill in the edge component map with biconnected component
  // numbers.
  template<typename ComponentMap>
  void fill_edge_map(Vertex alpha, const Graph& g, ComponentMap& component);

 protected:
  /* Start the echo phase (phase 3) where we propagate information up
     the tree. */
  void echo_phase(Vertex alpha, const Graph& g);

  /* Retrieve the index of edge in the out-edges list of target(e, g). */
  std::size_t get_edge_index(Edge e, const Graph& g);

  /* Retrieve the index of the edge incidence on v in the out-edges
     list of vertex u. */
  std::size_t get_incident_edge_index(Vertex u, Vertex v, const Graph& g);

  /* Keeps track of which phase of the algorithm we are in. There are
   * four phases plus the "finished" phase:
   *
   *   1) Building the spanning tree
   *   2) Discovering cycles
   *   3) Echoing back up the spanning tree
   *   4) Labelling biconnected components
   *   5) Finished
   */
  unsigned char phase;

  /* The parent of this vertex in the spanning tree. This value will
     be graph_traits<Graph>::null_vertex() for the leader. */
  Vertex parent;

  /* The farthest ancestor up the tree that resides in the same
     biconnected component as we do. This information is approximate:
     we might not know about the actual farthest ancestor, but this is
     the farthest one we've seen so far. */
  Vertex eta;

  /* The number of edges that have not yet transmitted any messages to
     us. This starts at the degree of the vertex and decreases as we
     receive messages. When this counter hits zero, we're done with
     the second phase of the algorithm. In Hohberg's paper, the actual
     remaining edge set E is stored with termination when all edges
     have been removed from E, but we only need to detect termination
     so the set E need not be explicitly represented. */
  degree_size_type num_edges_not_transmitted;

  /* The path from the root of the spanning tree to this vertex. This
     vector will always store only the parts of the path leading up to
     this vertex, and not the vertex itself. Thus, it will be empty
     for the leader. */
  std::vector<Vertex> path_from_root;

  /* Structure containing all of the extra data we need to keep around
     PER EDGE. This information can not be contained within a property
     map, because it can't be shared among vertices without breaking
     the algorithm. Decreasing the size of this structure will drastically */
  struct per_edge_data
  {
    hohberg_message<Graph> msg;
    std::vector<Vertex> M;
    bool is_tree_edge;
    degree_size_type partition;
  };

  /* Data for each edge in the graph. This structure will be indexed
     by the position of the edge in the out_edges() list. */
  std::vector<per_edge_data> edge_data;

  /* The mapping from local partition numbers (0..n-1) to global
     partition numbers. */
  std::vector<edges_size_type> local_to_global_partitions;

  friend class boost::serialization::access;

  // We cannot actually serialize a vertex processor, nor would we
  // want to. However, the fact that we're putting instances into a
  // distributed_property_map means that we need to have a serialize()
  // function available.
  template<typename Archiver>
  void serialize(Archiver&, const unsigned int /*version*/)
  {
    BOOST_ASSERT(false);
  }
};

template<typename Graph>
void
hohberg_vertex_processor<Graph>::initialize_leader(Vertex alpha,
                                                   const Graph& g)
{
  using namespace hohberg_detail;

  ProcessGroup pg = process_group(g);

  typename property_map<Graph, vertex_owner_t>::const_type
    owner = get(vertex_owner, g);

  path_header<Edge> header;
  header.path_length = 1;
  BGL_FORALL_OUTEDGES_T(alpha, e, g, Graph) {
    header.edge = e;
    send(pg, get(owner, target(e, g)), msg_path_header, header);
    send(pg, get(owner, target(e, g)), msg_path_vertices, alpha);
  }

  num_edges_not_transmitted = degree(alpha, g);
  edge_data.resize(num_edges_not_transmitted);
  phase = 2;
}

template<typename Graph>
void
hohberg_vertex_processor<Graph>::operator()(Edge e, path_t& path,
                                            const Graph& g)
{
  using namespace hohberg_detail;

  typename property_map<Graph, vertex_owner_t>::const_type
    owner = get(vertex_owner, g);

#ifdef PBGL_HOHBERG_DEBUG
//  std::cerr << local(source(e, g)) << '@' << owner(source(e, g)) << " -> "
//            << local(target(e, g)) << '@' << owner(target(e, g)) << ": path(";
//  for (std::size_t i = 0; i < path.size(); ++i) {
//    if (i > 0) std::cerr << ' ';
//    std::cerr << local(path[i]) << '@' << owner(path[i]);
//  }
  std::cerr << "), phase = " << (int)phase << std::endl;
#endif

  // Get access to edge-specific data
  if (edge_data.empty())
    edge_data.resize(degree(target(e, g), g));
  per_edge_data& edata = edge_data[get_edge_index(e, g)];

  // Record the message. We'll need it in phase 3.
  edata.msg.assign(path);

  // Note: "alpha" refers to the vertex "processor" receiving the
  // message.
  Vertex alpha = target(e, g);

  switch (phase) {
  case 1:
    {
      num_edges_not_transmitted = degree(alpha, g) - 1;
      edata.is_tree_edge = true;
      parent = path.back();
      eta = parent;
      edata.M.clear(); edata.M.push_back(parent);

      // Broadcast the path from the root to our potential children in
      // the spanning tree.
      path.push_back(alpha);
      path_header<Edge> header;
      header.path_length = path.size();
      ProcessGroup pg = process_group(g);
      BGL_FORALL_OUTEDGES_T(alpha, oe, g, Graph) {
        // Skip the tree edge we just received
        if (target(oe, g) != source(e, g)) {
          header.edge = oe;
          send(pg, get(owner, target(oe, g)), msg_path_header, header);
          send(pg, get(owner, target(oe, g)), msg_path_vertices, &path[0],
               header.path_length);
        }
      }
      path.pop_back();

      // Swap the old path in, to save some extra copying. Nobody
      path_from_root.swap(path);

      // Once we have received our place in the spanning tree, move on
      // to phase 2.
      phase = 2;

      // If we only had only edge, skip to phase 3.
      if (num_edges_not_transmitted == 0)
        echo_phase(alpha, g);
      return;
    }

  case 2:
    {
      --num_edges_not_transmitted;
      edata.is_tree_edge = false;

      // Determine if alpha (our vertex) is in the path
      path_iterator pos = std::find(path.begin(), path.end(), alpha);
      if (pos != path.end()) {
        // Case A: There is a cycle alpha beta ... gamma alpha
        // M(e) <- {beta, gammar}
        edata.M.clear();
        ++pos;
        // If pos == path.end(), we have a self-loop
        if (pos != path.end()) {
          // Add beta
          edata.M.push_back(*pos);
          ++pos;
        }
        // If pos == path.end(), we have a self-loop or beta == gamma
        // (parallel edge). Otherwise, add gamma.
        if (pos != path.end()) edata.M.push_back(path.back());
      } else {
        // Case B: There is a cycle but we haven't seen alpha yet.
        // M(e) = {parent, path.back()}
        edata.M.clear();
        edata.M.push_back(path.back());
        if (parent != path.back()) edata.M.push_back(parent);

        // eta = inf(eta, bra(pi_t, pi))
        eta = infimum(path_from_root, eta, branch_point(path_from_root, path));
      }
      if (num_edges_not_transmitted == 0)
        echo_phase(alpha, g);
      break;
    }

  default:
//    std::cerr << "Phase is " << int(phase) << "\n";
    BOOST_ASSERT(false);
  }
}

template<typename Graph>
void
hohberg_vertex_processor<Graph>::operator()(Edge e, Vertex gamma,
                                            path_t& in_same_bicomponent,
                                            const Graph& g)
{
  using namespace hohberg_detail;

#ifdef PBGL_HOHBERG_DEBUG
  std::cerr << local(source(e, g)) << '@' << owner(source(e, g)) << " -> "
            << local(target(e, g)) << '@' << owner(target(e, g)) << ": tree("
            << local(gamma) << '@' << owner(gamma) << ", ";
  for (std::size_t i = 0; i < in_same_bicomponent.size(); ++i) {
    if (i > 0) std::cerr << ' ';
    std::cerr << local(in_same_bicomponent[i]) << '@'
              << owner(in_same_bicomponent[i]);
  }
  std::cerr << ", " << local(source(e, g)) << '@' << owner(source(e, g))
            << "), phase = " << (int)phase << std::endl;
#endif

  // Get access to edge-specific data
  per_edge_data& edata = edge_data[get_edge_index(e, g)];

  // Record the message. We'll need it in phase 3.
  edata.msg.assign(gamma, in_same_bicomponent);

  // Note: "alpha" refers to the vertex "processor" receiving the
  // message.
  Vertex alpha = target(e, g);
  Vertex beta = source(e, g);

  switch (phase) {
  case 2:
    --num_edges_not_transmitted;
    edata.is_tree_edge = true;

    if (gamma == alpha) {
      // Case C
      edata.M.swap(in_same_bicomponent);
    } else {
      // Case D
      edata.M.clear();
      edata.M.push_back(parent);
      if (beta != parent) edata.M.push_back(beta);
      eta = infimum(path_from_root, eta, gamma);
    }
    if (num_edges_not_transmitted == 0)
      echo_phase(alpha, g);
    break;

  default:
    BOOST_ASSERT(false);
  }
}

template<typename Graph>
void
hohberg_vertex_processor<Graph>::operator()(Edge e, edges_size_type name,
                                            const Graph& g)
{
  using namespace hohberg_detail;

#ifdef PBGL_HOHBERG_DEBUG
  std::cerr << local(source(e, g)) << '@' << owner(source(e, g)) << " -> "
            << local(target(e, g)) << '@' << owner(target(e, g)) << ": name("
            << name << "), phase = " << (int)phase << std::endl;
#endif

  BOOST_ASSERT(phase == 4);

  typename property_map<Graph, vertex_owner_t>::const_type
    owner = get(vertex_owner, g);

  // Send name messages along the spanning tree edges that are in the
  // same bicomponent as the edge to our parent.
  ProcessGroup pg = process_group(g);

  Vertex alpha = target(e, g);

  std::size_t idx = 0;
  BGL_FORALL_OUTEDGES_T(alpha, e, g, Graph) {
    per_edge_data& edata = edge_data[idx++];
    if (edata.is_tree_edge
        && find(edata.M.begin(), edata.M.end(), parent) != edata.M.end()
        && target(e, g) != parent) {
      // Notify our children in the spanning tree of this name
      name_header<Edge> header;
      header.edge = e;
      header.name = name;
      send(pg, get(owner, target(e, g)), msg_name, header);
    } else if (target(e, g) == parent) {
      // Map from local partition numbers to global bicomponent numbers
      local_to_global_partitions[edata.partition] = name;
    }
  }

  // Final stage
  phase = 5;
}

template<typename Graph>
typename hohberg_vertex_processor<Graph>::edges_size_type
hohberg_vertex_processor<Graph>::
num_starting_bicomponents(Vertex alpha, const Graph& g)
{
  edges_size_type not_mapped = (std::numeric_limits<edges_size_type>::max)();

  edges_size_type result = 0;
  std::size_t idx = 0;
  BGL_FORALL_OUTEDGES_T(alpha, e, g, Graph) {
    per_edge_data& edata = edge_data[idx++];
    if (edata.is_tree_edge
        && find(edata.M.begin(), edata.M.end(), parent) == edata.M.end()) {
      // Map from local partition numbers to global bicomponent numbers
      if (local_to_global_partitions[edata.partition] == not_mapped)
        local_to_global_partitions[edata.partition] = result++;
    }
  }

#ifdef PBGL_HOHBERG_DEBUG
  std::cerr << local(alpha) << '@' << owner(alpha) << " has " << result
            << " bicomponents originating at it." << std::endl;
#endif

  return result;
}

template<typename Graph>
template<typename ComponentMap>
void
hohberg_vertex_processor<Graph>::
fill_edge_map(Vertex alpha, const Graph& g, ComponentMap& component)
{
  std::size_t idx = 0;
  BGL_FORALL_OUTEDGES_T(alpha, e, g, Graph) {
    per_edge_data& edata = edge_data[idx++];
    local_put(component, e, local_to_global_partitions[edata.partition]);

#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 2
    std::cerr << "component("
              << local(source(e, g)) << '@' << owner(source(e, g)) << " -> "
              << local(target(e, g)) << '@' << owner(target(e, g)) << ") = "
              << local_to_global_partitions[edata.partition]
              << " (partition = " << edata.partition << " of "
              << local_to_global_partitions.size() << ")" << std::endl;
#endif
  }
}

template<typename Graph>
void
hohberg_vertex_processor<Graph>::
start_naming_phase(Vertex alpha, const Graph& g, edges_size_type offset)
{
  using namespace hohberg_detail;

  BOOST_ASSERT(phase == 4);

  typename property_map<Graph, vertex_owner_t>::const_type
    owner = get(vertex_owner, g);

  // Send name messages along the spanning tree edges of the
  // components that we get to number.
  ProcessGroup pg = process_group(g);

  bool has_more_children_to_name = false;

  // Map from local partition numbers to global bicomponent numbers
  edges_size_type not_mapped = (std::numeric_limits<edges_size_type>::max)();
  for (std::size_t i = 0; i < local_to_global_partitions.size(); ++i) {
    if (local_to_global_partitions[i] != not_mapped)
      local_to_global_partitions[i] += offset;
  }

  std::size_t idx = 0;
  BGL_FORALL_OUTEDGES_T(alpha, e, g, Graph) {
    per_edge_data& edata = edge_data[idx++];
    if (edata.is_tree_edge
        && find(edata.M.begin(), edata.M.end(), parent) == edata.M.end()) {
      // Notify our children in the spanning tree of this new name
      name_header<Edge> header;
      header.edge = e;
      header.name = local_to_global_partitions[edata.partition];
      send(pg, get(owner, target(e, g)), msg_name, header);
    } else if (edata.is_tree_edge) {
      has_more_children_to_name = true;
    }
#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 2
    std::cerr << "M[" << local(source(e, g)) << '@' << owner(source(e, g))
              << " -> " << local(target(e, g)) << '@' << owner(target(e, g))
              << "] = ";
    for (std::size_t i = 0; i < edata.M.size(); ++i) {
      std::cerr << local(edata.M[i]) << '@' << owner(edata.M[i]) << ' ';
    }
    std::cerr << std::endl;
#endif
  }

  // See if we're done.
  if (!has_more_children_to_name)
    // Final stage
    phase = 5;
}

template<typename Graph>
void
hohberg_vertex_processor<Graph>::echo_phase(Vertex alpha, const Graph& g)
{
  using namespace hohberg_detail;

  typename property_map<Graph, vertex_owner_t>::const_type
    owner = get(vertex_owner, g);

  /* We're entering the echo phase. */
  phase = 3;

  if (parent != graph_traits<Graph>::null_vertex()) {
    Edge edge_to_parent;

#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 1
     std::cerr << local(alpha) << '@' << owner(alpha) << " echo: parent = "
               << local(parent) << '@' << owner(parent) << ", eta = "
               << local(eta) << '@' << owner(eta) << ", Gamma = ";
#endif

    std::vector<Vertex> bicomp;
    std::size_t e_index = 0;
    BGL_FORALL_OUTEDGES_T(alpha, e, g, Graph) {
      if (target(e, g) == parent && parent == eta) {
        edge_to_parent = e;
        if (find(bicomp.begin(), bicomp.end(), alpha) == bicomp.end()) {
#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 1
          std::cerr << local(alpha) << '@' << owner(alpha) << ' ';
#endif
          bicomp.push_back(alpha);
        }
      } else {
        if (target(e, g) == parent) edge_to_parent = e;

        per_edge_data& edata = edge_data[e_index];

        if (edata.msg.is_path()) {
          path_iterator pos = std::find(edata.msg.path_or_bicomp.begin(),
                                        edata.msg.path_or_bicomp.end(),
                                        eta);
          if (pos != edata.msg.path_or_bicomp.end()) {
            ++pos;
            if (pos != edata.msg.path_or_bicomp.end()
                && find(bicomp.begin(), bicomp.end(), *pos) == bicomp.end()) {
#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 1
              std::cerr << local(*pos) << '@' << owner(*pos) << ' ';
#endif
              bicomp.push_back(*pos);
            }
          }
        } else if (edata.msg.is_tree() && edata.msg.gamma == eta) {
          for (path_iterator i = edata.msg.path_or_bicomp.begin();
               i != edata.msg.path_or_bicomp.end(); ++i) {
            if (find(bicomp.begin(), bicomp.end(), *i) == bicomp.end()) {
#if defined(PBGL_HOHBERG_DEBUG) && PBGL_HOHBERG_DEBUG > 1
              std::cerr << local(*i) << '@' << owner(*i) << ' ';
#endif
              bicomp.push_back(*i);
            }
          }
        }
      }
      ++e_index;
    }
#ifdef PBGL_HOHBERG_DEBUG
    std::cerr << std::endl;
#endif

    // Send tree(eta, bicomp) to parent
    tree_header<Vertex, Edge> header;
    header.edge = edge_to_parent;
    header.gamma = eta;
    header.bicomp_length = bicomp.size();
    ProcessGroup pg = process_group(g);
    send(pg, get(owner, parent), msg_tree_header, header);
    send(pg, get(owner, parent), msg_tree_vertices, &bicomp[0],
         header.bicomp_length);
  }

  // Compute the partition of edges such that iff two edges e1 and e2
  // are in different subsets then M(e1) is disjoint from M(e2).

  // Start by putting each edge in a different partition
  std::vector<degree_size_type> parent_vec(edge_data.size());
  degree_size_type idx = 0;
  for (idx = 0; idx < edge_data.size(); ++idx)
    parent_vec[idx] = idx;

  // Go through each edge e, performing a union() on the edges
  // incident on all vertices in M[e].
  idx = 0;
  BGL_FORALL_OUTEDGES_T(alpha, e, g, Graph) {
    per_edge_data& edata = edge_data[idx++];

    // Compute union of vertices in M
    if (!edata.M.empty()) {
      degree_size_type e1 = get_incident_edge_index(alpha, edata.M.front(), g);
      while (parent_vec[e1] != e1) e1 = parent_vec[e1];

      for (std::size_t i = 1; i < edata.M.size(); ++i) {
        degree_size_type e2 = get_incident_edge_index(alpha, edata.M[i], g);
        while (parent_vec[e2] != e2) e2 = parent_vec[e2];
        parent_vec[e2] = e1;
      }
    }
  }

  edges_size_type not_mapped = (std::numeric_limits<edges_size_type>::max)();

  // Determine the number of partitions
  for (idx = 0; idx < parent_vec.size(); ++idx) {
    if (parent_vec[idx] == idx) {
      edge_data[idx].partition = local_to_global_partitions.size();
      local_to_global_partitions.push_back(not_mapped);
    }
  }

  // Assign partition numbers to each edge
  for (idx = 0; idx < parent_vec.size(); ++idx) {
    degree_size_type rep = parent_vec[idx];
    while (rep != parent_vec[rep]) rep = parent_vec[rep];
    edge_data[idx].partition = edge_data[rep].partition;
  }

  // Enter the naming phase (but don't send anything yet).
  phase = 4;
}

template<typename Graph>
std::size_t
hohberg_vertex_processor<Graph>::get_edge_index(Edge e, const Graph& g)
{
  std::size_t result = 0;
  BGL_FORALL_OUTEDGES_T(target(e, g), oe, g, Graph) {
    if (source(e, g) == target(oe, g)) return result;
    ++result;
  }
  BOOST_ASSERT(false);
}

template<typename Graph>
std::size_t
hohberg_vertex_processor<Graph>::get_incident_edge_index(Vertex u, Vertex v,
                                                         const Graph& g)
{
  std::size_t result = 0;
  BGL_FORALL_OUTEDGES_T(u, e, g, Graph) {
    if (target(e, g) == v) return result;
    ++result;
  }
  BOOST_ASSERT(false);
}

template<typename Graph, typename InputIterator, typename ComponentMap,
         typename VertexProcessorMap>
typename graph_traits<Graph>::edges_size_type
hohberg_biconnected_components
  (const Graph& g,
   ComponentMap component,
   InputIterator first, InputIterator last,
   VertexProcessorMap vertex_processor)
{
  using namespace boost::graph::parallel;
  using namespace hohberg_detail;
  using boost::parallel::all_reduce;

  typename property_map<Graph, vertex_owner_t>::const_type
    owner = get(vertex_owner, g);

  // The graph must be undirected
  BOOST_STATIC_ASSERT(
    (is_convertible<typename graph_traits<Graph>::directed_category,
                    undirected_tag>::value));

  // The graph must model Incidence Graph
  BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept<Graph> ));

  typedef typename graph_traits<Graph>::edges_size_type edges_size_type;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;

  // Retrieve the process group we will use for communication
  typedef typename process_group_type<Graph>::type process_group_type;
  process_group_type pg = process_group(g);

  // Keeps track of the edges that we know to be tree edges.
  std::vector<edge_descriptor> tree_edges;

  // The leaders send out a path message to initiate the algorithm
  while (first != last) {
    vertex_descriptor leader = *first;
    if (process_id(pg) == get(owner, leader))
      vertex_processor[leader].initialize_leader(leader, g);
    ++first;
  }
  synchronize(pg);

  // Will hold the number of bicomponents in the graph.
  edges_size_type num_bicomponents = 0;

  // Keep track of the path length that we should expect, based on the
  // level in the breadth-first search tree. At present, this is only
  // used as a sanity check. TBD: This could be used to decrease the
  // amount of communication required per-edge (by about 4 bytes).
  std::size_t path_length = 1;

  typedef std::vector<vertex_descriptor> path_t;

  unsigned char minimum_phase = 5;
  do {
    while (optional<std::pair<int, int> > msg = probe(pg)) {
      switch (msg->second) {
      case msg_path_header:
        {
          // Receive the path header
          path_header<edge_descriptor> header;
          receive(pg, msg->first, msg->second, header);
          BOOST_ASSERT(path_length == header.path_length);

          // Receive the path itself
          path_t path(path_length);
          receive(pg, msg->first, msg_path_vertices, &path[0], path_length);

          edge_descriptor e = header.edge;
          vertex_processor[target(e, g)](e, path, g);
        }
        break;

      case msg_path_vertices:
        // Should be handled in msg_path_header case, unless we're going
        // stateless.
        BOOST_ASSERT(false);
        break;

      case msg_tree_header:
        {
          // Receive the tree header
          tree_header<vertex_descriptor, edge_descriptor> header;
          receive(pg, msg->first, msg->second, header);

          // Receive the tree itself
          path_t in_same_bicomponent(header.bicomp_length);
          receive(pg, msg->first, msg_tree_vertices, &in_same_bicomponent[0],
                  header.bicomp_length);

          edge_descriptor e = header.edge;
          vertex_processor[target(e, g)](e, header.gamma, in_same_bicomponent,
                                         g);
        }
        break;

      case msg_tree_vertices:
        // Should be handled in msg_tree_header case, unless we're
        // going stateless.
        BOOST_ASSERT(false);
        break;

      case msg_name:
        {
          name_header<edge_descriptor> header;
          receive(pg, msg->first, msg->second, header);
          edge_descriptor e = header.edge;
          vertex_processor[target(e, g)](e, header.name, g);
        }
        break;

      default:
        BOOST_ASSERT(false);
      }
    }
    ++path_length;

    // Compute minimum phase locally
    minimum_phase = 5;
    unsigned char maximum_phase = 1;
    BGL_FORALL_VERTICES_T(v, g, Graph) {
      minimum_phase = (std::min)(minimum_phase, vertex_processor[v].get_phase());
      maximum_phase = (std::max)(maximum_phase, vertex_processor[v].get_phase());
    }

#ifdef PBGL_HOHBERG_DEBUG
    if (process_id(pg) == 0)
      std::cerr << "<---------End of stage------------->" << std::endl;
#endif
    // Compute minimum phase globally
    minimum_phase = all_reduce(pg, minimum_phase, boost::mpi::minimum<char>());

#ifdef PBGL_HOHBERG_DEBUG
    if (process_id(pg) == 0)
      std::cerr << "Minimum phase = " << (int)minimum_phase << std::endl;
#endif

    if (minimum_phase == 4
        && all_reduce(pg, maximum_phase, boost::mpi::maximum<char>()) == 4) {

#ifdef PBGL_HOHBERG_DEBUG
      if (process_id(pg) == 0)
        std::cerr << "<---------Naming phase------------->" << std::endl;
#endif
      // Compute the biconnected component number offsets for each
      // vertex.
      std::vector<edges_size_type> local_offsets;
      local_offsets.reserve(num_vertices(g));
      edges_size_type num_local_bicomponents = 0;
      BGL_FORALL_VERTICES_T(v, g, Graph) {
        local_offsets.push_back(num_local_bicomponents);
        num_local_bicomponents +=
          vertex_processor[v].num_starting_bicomponents(v, g);
      }

      synchronize(pg);

      // Find our the number of bicomponent names that will originate
      // from each process. This tells us how many bicomponents are in
      // the entire graph and what our global offset is for computing
      // our own biconnected component names.
      std::vector<edges_size_type> all_bicomponents(num_processes(pg));
      all_gather(pg, &num_local_bicomponents, &num_local_bicomponents + 1,
                 all_bicomponents);
      num_bicomponents = 0;
      edges_size_type my_global_offset = 0;
      for (std::size_t i = 0; i < all_bicomponents.size(); ++i) {
        if (i == (std::size_t)process_id(pg)) 
          my_global_offset = num_bicomponents;
        num_bicomponents += all_bicomponents[i];
      }

      std::size_t index = 0;
      BGL_FORALL_VERTICES_T(v, g, Graph) {
        edges_size_type offset = my_global_offset + local_offsets[index++];
        vertex_processor[v].start_naming_phase(v, g, offset);
      }
    }

    synchronize(pg);
  } while (minimum_phase < 5);

  // Number the edges appropriately.
  BGL_FORALL_VERTICES_T(v, g, Graph)
    vertex_processor[v].fill_edge_map(v, g, component);

  return num_bicomponents;
}

template<typename Graph, typename ComponentMap, typename InputIterator>
typename graph_traits<Graph>::edges_size_type
hohberg_biconnected_components
  (const Graph& g, ComponentMap component,
   InputIterator first, InputIterator last)

{
  std::vector<hohberg_vertex_processor<Graph> >
    vertex_processors(num_vertices(g));
  return hohberg_biconnected_components
           (g, component, first, last,
            make_iterator_property_map(vertex_processors.begin(),
                                       get(vertex_index, g)));
}

template<typename Graph, typename ComponentMap, typename ParentMap>
typename graph_traits<Graph>::edges_size_type
hohberg_biconnected_components(const Graph& g, ComponentMap component,
                               ParentMap parent)
{
  // We need the connected components of the graph, but we don't care
  // about component numbers.
  connected_components(g, dummy_property_map(), parent);

  // Each root in the parent map is a leader
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  std::vector<vertex_descriptor> leaders;
  BGL_FORALL_VERTICES_T(v, g, Graph)
    if (get(parent, v) == v) leaders.push_back(v);

  return hohberg_biconnected_components(g, component,
                                        leaders.begin(), leaders.end());
}

template<typename Graph, typename ComponentMap>
typename graph_traits<Graph>::edges_size_type
hohberg_biconnected_components(const Graph& g, ComponentMap component)
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  std::vector<vertex_descriptor> parents(num_vertices(g));
  return hohberg_biconnected_components
           (g, component, make_iterator_property_map(parents.begin(),
                                                     get(vertex_index, g)));
}

} } } // end namespace boost::graph::distributed

#endif // BOOST_GRAPH_DISTRIBUTED_HOHBERG_BICONNECTED_COMPONENTS_HPP
