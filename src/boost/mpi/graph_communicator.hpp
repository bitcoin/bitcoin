// Copyright (C) 2007 Trustees of Indiana University

// Authors: Douglas Gregor
//          Andrew Lumsdaine

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** @file graph_communicator.hpp
 *
 *  This header defines facilities to support MPI communicators with
 *  graph topologies, using the graph interface defined by the Boost
 *  Graph Library. One can construct a communicator whose topology is
 *  described by any graph meeting the requirements of the Boost Graph
 *  Library's graph concepts. Likewise, any communicator that has a
 *  graph topology can be viewed as a graph by the Boost Graph
 *  Library, permitting one to use the BGL's graph algorithms on the
 *  process topology.
 */
#ifndef BOOST_MPI_GRAPH_COMMUNICATOR_HPP
#define BOOST_MPI_GRAPH_COMMUNICATOR_HPP

#include <boost/mpi/communicator.hpp>
#include <vector>
#include <utility>

// Headers required to implement graph topologies
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/shared_array.hpp>
#include <boost/assert.hpp>

namespace boost { namespace mpi {

/**
 * @brief An MPI communicator with a graph topology.
 *
 * A @c graph_communicator is a communicator whose topology is
 * expressed as a graph. Graph communicators have the same
 * functionality as (intra)communicators, but also allow one to query
 * the relationships among processes. Those relationships are
 * expressed via a graph, using the interface defined by the Boost
 * Graph Library. The @c graph_communicator class meets the
 * requirements of the BGL Graph, Incidence Graph, Adjacency Graph,
 * Vertex List Graph, and Edge List Graph concepts.
 */
class BOOST_MPI_DECL graph_communicator : public communicator
{
  friend class communicator;

  /**
   * INTERNAL ONLY
   *
   * Construct a graph communicator given a shared pointer to the
   * underlying MPI_Comm. This operation is used for "casting" from a
   * communicator to a graph communicator.
   */
  explicit graph_communicator(const shared_ptr<MPI_Comm>& comm_ptr)
  {
#ifndef BOOST_DISABLE_ASSERTS
    int status;
    BOOST_MPI_CHECK_RESULT(MPI_Topo_test, ((MPI_Comm)*this, &status));
    BOOST_ASSERT(status == MPI_GRAPH);
#endif
    this->comm_ptr = comm_ptr;
  }

public:
  /**
   * Build a new Boost.MPI graph communicator based on the MPI
   * communicator @p comm with graph topology.
   *
   * @p comm may be any valid MPI communicator. If @p comm is
   * MPI_COMM_NULL, an empty communicator (that cannot be used for
   * communication) is created and the @p kind parameter is
   * ignored. Otherwise, the @p kind parameter determines how the
   * Boost.MPI communicator will be related to @p comm:
   *
   *   - If @p kind is @c comm_duplicate, duplicate @c comm to create
   *   a new communicator. This new communicator will be freed when
   *   the Boost.MPI communicator (and all copies of it) is
   *   destroyed. This option is only permitted if the underlying MPI
   *   implementation supports MPI 2.0; duplication of
   *   intercommunicators is not available in MPI 1.x.
   *
   *   - If @p kind is @c comm_take_ownership, take ownership of @c
   *   comm. It will be freed automatically when all of the Boost.MPI
   *   communicators go out of scope.
   *
   *   - If @p kind is @c comm_attach, this Boost.MPI communicator
   *   will reference the existing MPI communicator @p comm but will
   *   not free @p comm when the Boost.MPI communicator goes out of
   *   scope. This option should only be used when the communicator is
   *   managed by the user.
   */
  graph_communicator(const MPI_Comm& comm, comm_create_kind kind)
    : communicator(comm, kind)
  { 
#ifndef BOOST_DISABLE_ASSERTS
    int status;
    BOOST_MPI_CHECK_RESULT(MPI_Topo_test, ((MPI_Comm)*this, &status));
    BOOST_ASSERT(status == MPI_GRAPH);
#endif
  }

  /**
   *  Create a new communicator whose topology is described by the
   *  given graph. The indices of the vertices in the graph will be
   *  assumed to be the ranks of the processes within the
   *  communicator. There may be fewer vertices in the graph than
   *  there are processes in the communicator; in this case, the
   *  resulting communicator will be a NULL communicator.
   *
   *  @param comm The communicator that the new, graph communicator
   *  will be based on. 
   * 
   *  @param graph Any type that meets the requirements of the
   *  Incidence Graph and Vertex List Graph concepts from the Boost Graph
   *  Library. This structure of this graph will become the topology
   *  of the communicator that is returned.
   *
   *  @param reorder Whether MPI is permitted to re-order the process
   *  ranks within the returned communicator, to better optimize
   *  communication. If false, the ranks of each process in the
   *  returned process will match precisely the rank of that process
   *  within the original communicator.
   */
  template<typename Graph>
  explicit 
  graph_communicator(const communicator& comm, const Graph& graph, 
                     bool reorder = false);

  /**
   *  Create a new communicator whose topology is described by the
   *  given graph. The rank map (@p rank) gives the mapping from
   *  vertices in the graph to ranks within the communicator. There
   *  may be fewer vertices in the graph than there are processes in
   *  the communicator; in this case, the resulting communicator will
   *  be a NULL communicator.
   *
   *  @param comm The communicator that the new, graph communicator
   *  will be based on. The ranks in @c rank refer to the processes in
   *  this communicator.
   * 
   *  @param graph Any type that meets the requirements of the
   *  Incidence Graph and Vertex List Graph concepts from the Boost Graph
   *  Library. This structure of this graph will become the topology
   *  of the communicator that is returned.
   *
   *  @param rank This map translates vertices in the @c graph into
   *  ranks within the current communicator. It must be a Readable
   *  Property Map (see the Boost Property Map library) whose key type
   *  is the vertex type of the @p graph and whose value type is @c
   *  int.
   *
   *  @param reorder Whether MPI is permitted to re-order the process
   *  ranks within the returned communicator, to better optimize
   *  communication. If false, the ranks of each process in the
   *  returned process will match precisely the rank of that process
   *  within the original communicator.
   */
  template<typename Graph, typename RankMap>
  explicit 
  graph_communicator(const communicator& comm, const Graph& graph, 
                     RankMap rank, bool reorder = false);

protected:
  /**
   * INTERNAL ONLY
   *
   * Used by the constructors to create the new communicator with a
   * graph topology.
   */
  template<typename Graph, typename RankMap>
  void
  setup_graph(const communicator& comm, const Graph& graph, RankMap rank, 
              bool reorder);
};

/****************************************************************************
 *  Implementation Details                                                  *
 ****************************************************************************/

template<typename Graph>
graph_communicator::graph_communicator(const communicator& comm, 
                                       const Graph& graph, 
                                       bool reorder)
{
  this->setup_graph(comm, graph, get(vertex_index, graph), reorder);
}

template<typename Graph, typename RankMap>
graph_communicator::graph_communicator(const communicator& comm, 
                                       const Graph& graph, 
                                       RankMap rank, bool reorder)
{
  this->setup_graph(comm, graph, rank, reorder);
}


template<typename Graph, typename RankMap>
void
graph_communicator::setup_graph(const communicator& comm, const Graph& graph, 
                                RankMap rank, bool reorder)
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  // Build a mapping from ranks to vertices
  std::vector<vertex_descriptor> vertex_with_rank(num_vertices(graph));
  if (vertex_with_rank.empty())
    return;

  BGL_FORALL_VERTICES_T(v, graph, Graph)
    vertex_with_rank[get(rank, v)] = v;

  // Build the representation of the graph required by
  // MPI_Graph_create.
  std::vector<int> indices(num_vertices(graph));
  std::vector<int> edges;
  int nvertices = indices.size();
  for (int vertex_index = 0; vertex_index < nvertices; ++vertex_index) {
    vertex_descriptor v = vertex_with_rank[vertex_index];

    BGL_FORALL_OUTEDGES_T(v, e, graph, Graph)
      edges.push_back(get(rank, target(e, graph)));

    indices[vertex_index] = edges.size();
  }

  // Create the new communicator
  MPI_Comm newcomm;
  BOOST_MPI_CHECK_RESULT(MPI_Graph_create,
                         ((MPI_Comm)comm, 
                          nvertices,
                          detail::c_data(indices),
                          detail::c_data(edges),
                          reorder,
                          &newcomm));
  this->comm_ptr.reset(new MPI_Comm(newcomm), comm_free());
}

/****************************************************************************
 *  Communicator with Graph Topology as BGL Graph                           *
 ****************************************************************************/
namespace detail {
  /**
   *  INTERNAL ONLY
   *
   *  The iterator used to access the outgoing edges within a
   *  communicator's graph topology.
   */
  class comm_out_edge_iterator
    : public iterator_facade<comm_out_edge_iterator, 
                             std::pair<int, int>,
                             random_access_traversal_tag,
                             const std::pair<int, int>&, 
                             int>
  {
  public:
    comm_out_edge_iterator() { }

    comm_out_edge_iterator(int source, shared_array<int> neighbors, int index)
      : edge(source, -1), neighbors(neighbors), index(index) { }

  protected:
    friend class boost::iterator_core_access;

    const std::pair<int, int>& dereference() const
    {
      edge.second = neighbors[index];
      return edge;
    }

    bool equal(const comm_out_edge_iterator& other) const
    {
      return (edge.first == other.edge.first
              && index == other.index);
    }

    void increment() { ++index; }

    void decrement() { --index; }

    void advance(int n) { index += n; }

    int distance_to(const comm_out_edge_iterator& other) const
    {
      return other.index - index;
    }

    mutable std::pair<int, int> edge;
    shared_array<int> neighbors;
    int index;
  };

  /**
   *  INTERNAL ONLY
   *
   *  The iterator used to access the adjacent vertices within a
   *  communicator's graph topology.
   */
  class comm_adj_iterator
    : public iterator_facade<comm_adj_iterator, 
                             int,
                             random_access_traversal_tag,
                             int, 
                             int>
  {
  public:
    comm_adj_iterator() { }

    comm_adj_iterator(shared_array<int> neighbors, int index)
      : neighbors(neighbors), index(index) { }

  protected:
    friend class boost::iterator_core_access;

    int dereference() const { return neighbors[index]; }

    bool equal(const comm_adj_iterator& other) const
    {
      return (neighbors == other.neighbors
              && index == other.index);
    }

    void increment() { ++index; }

    void decrement() { --index; }

    void advance(int n) { index += n; }

    int distance_to(const comm_adj_iterator& other) const
    {
      return other.index - index;
    }

    shared_array<int> neighbors;
    int index;
  };

  /**
   *  INTERNAL ONLY
   *
   *  The iterator used to access the edges in a communicator's graph
   *  topology.
   */
  class comm_edge_iterator
    : public iterator_facade<comm_edge_iterator, 
                             std::pair<int, int>,
                             forward_traversal_tag,
                             const std::pair<int, int>&, 
                             int>
  {
  public:
    comm_edge_iterator() { }

    /// Constructor for a past-the-end iterator
    comm_edge_iterator(int nedges) : edge_index(nedges) { }

    comm_edge_iterator(shared_array<int> indices, shared_array<int> edges)
      : indices(indices), edges(edges), edge_index(0), edge(0, 0)
    { }

  protected:
    friend class boost::iterator_core_access;

    const std::pair<int, int>& dereference() const
    {
      while (edge_index == indices[edge.first])
        ++edge.first;
      edge.second = edges[edge_index];
      return edge;
    }

    bool equal(const comm_edge_iterator& other) const
    {
      return edge_index == other.edge_index;
    }

    void increment() 
    { 
      ++edge_index; 
    }

    shared_array<int> indices;
    shared_array<int> edges;
    int edge_index;
    mutable std::pair<int, int> edge;
  };

} // end namespace detail

// Incidence Graph requirements

/**
 * @brief Returns the source vertex from an edge in the graph topology
 * of a communicator.
 */
inline int source(const std::pair<int, int>& edge, const graph_communicator&)
{
  return edge.first;
}

/**
 * @brief Returns the target vertex from an edge in the graph topology
 * of a communicator.
 */
inline int target(const std::pair<int, int>& edge, const graph_communicator&)
{
  return edge.second;
}

/**
 * @brief Returns an iterator range containing all of the edges
 * outgoing from the given vertex in a graph topology of a
 * communicator.
 */
std::pair<detail::comm_out_edge_iterator, detail::comm_out_edge_iterator>
out_edges(int vertex, const graph_communicator& comm);


/**
 * @brief Returns the out-degree of a vertex in the graph topology of
 * a communicator.
 */
int out_degree(int vertex, const graph_communicator& comm);

// Adjacency Graph requirements

/**
 * @brief Returns an iterator range containing all of the neighbors of
 * the given vertex in the communicator's graph topology.
 */
std::pair<detail::comm_adj_iterator, detail::comm_adj_iterator>
adjacent_vertices(int vertex, const graph_communicator& comm);

// Vertex List Graph requirements

/**
 * @brief Returns an iterator range that contains all of the vertices
 * with the communicator's graph topology, i.e., all of the process
 * ranks in the communicator.
 */
inline std::pair<counting_iterator<int>, counting_iterator<int> >
vertices(const graph_communicator& comm)
{
  return std::make_pair(counting_iterator<int>(0),
                        counting_iterator<int>(comm.size()));
}

/**
 *  @brief Returns the number of vertices within the graph topology of
 *  the communicator, i.e., the number of processes in the
 *  communicator.
 */
inline int num_vertices(const graph_communicator& comm) { return comm.size(); }

// Edge List Graph requirements

/**
 * @brief Returns an iterator range that contains all of the edges
 * with the communicator's graph topology.
 */
std::pair<detail::comm_edge_iterator, detail::comm_edge_iterator>
edges(const graph_communicator& comm);

/**
 * @brief Returns the number of edges in the communicator's graph
 * topology.
 */
int num_edges(const graph_communicator& comm);

// Property Graph requirements

/**
 *  @brief Returns a property map that maps from vertices in a
 *  communicator's graph topology to their index values. 
 *
 *  Since the vertices are ranks in the communicator, the returned
 *  property map is the identity property map.
 */
inline identity_property_map get(vertex_index_t, const graph_communicator&)
{
  return identity_property_map();
}

/**
 * @brief Returns the index of a vertex in the communicator's graph
 * topology.
 *
 * Since the vertices are ranks in the communicator, this is the
 *  identity function.
 */
inline int get(vertex_index_t, const graph_communicator&, int vertex)
{
  return vertex;
}

} } // end namespace boost::mpi

namespace boost {

/**
 * @brief Traits structure that allows a communicator with graph
 * topology to be view as a graph by the Boost Graph Library.
 *
 * The specialization of @c graph_traits for an MPI communicator
 * allows a communicator with graph topology to be viewed as a
 * graph. An MPI communicator with graph topology meets the
 * requirements of the Graph, Incidence Graph, Adjacency Graph, Vertex
 * List Graph, and Edge List Graph concepts from the Boost Graph
 * Library.
 */
template<>
struct graph_traits<mpi::graph_communicator> {
  // Graph concept requirements
  typedef int                        vertex_descriptor;
  typedef std::pair<int, int>        edge_descriptor;
  typedef directed_tag               directed_category;
  typedef disallow_parallel_edge_tag edge_parallel_category;
  
  /**
   * INTERNAL ONLY
   */
  struct traversal_category
    : incidence_graph_tag, 
      adjacency_graph_tag, 
      vertex_list_graph_tag, 
      edge_list_graph_tag 
  { 
  };

  /**
   * @brief Returns a vertex descriptor that can never refer to any
   * valid vertex.
   */
  static vertex_descriptor null_vertex() { return -1; }

  // Incidence Graph requirements
  typedef mpi::detail::comm_out_edge_iterator out_edge_iterator;
  typedef int degree_size_type;

  // Adjacency Graph requirements
  typedef mpi::detail::comm_adj_iterator adjacency_iterator;

  // Vertex List Graph requirements
  typedef counting_iterator<int> vertex_iterator;
  typedef int                    vertices_size_type;

  // Edge List Graph requirements
  typedef mpi::detail::comm_edge_iterator edge_iterator;
  typedef int                             edges_size_type;
};

// Property Graph requirements

/**
 * INTERNAL ONLY
 */
template<>
struct property_map<mpi::graph_communicator, vertex_index_t>
{
  typedef identity_property_map type;
  typedef identity_property_map const_type;
};

} // end namespace boost



#endif // BOOST_MPI_GRAPH_COMMUNICATOR_HPP
