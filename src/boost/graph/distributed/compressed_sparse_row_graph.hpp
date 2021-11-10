// Copyright (C) 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Jeremiah Willcock
//           Andrew Lumsdaine

// Distributed compressed sparse row graph type

#ifndef BOOST_GRAPH_DISTRIBUTED_CSR_HPP
#define BOOST_GRAPH_DISTRIBUTED_CSR_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/assert.hpp>
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/distributed/selector.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/graph/parallel/properties.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <boost/property_map/parallel/local_property_map.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>

namespace boost {

// Distributed and sequential inplace ctors have the same signature so
// we need a separate tag for distributed inplace ctors
enum distributed_construct_inplace_from_sources_and_targets_t 
  {distributed_construct_inplace_from_sources_and_targets};

// The number of bits we reserve for the processor ID. 
// DPG TBD: This is a hack. It will eventually be a run-time quantity.
static const int processor_bits = 8;

// Tag class for a distributed CSR graph
struct distributed_csr_tag
  : public virtual distributed_graph_tag,
    public virtual distributed_vertex_list_graph_tag,
    public virtual distributed_edge_list_graph_tag,
    public virtual incidence_graph_tag,
    public virtual adjacency_graph_tag {};

template<typename VertexProperty, typename EdgeProperty,
         typename GraphProperty, typename ProcessGroup, typename InVertex,
         typename InDistribution, typename InEdgeIndex>
class compressed_sparse_row_graph<
         directedS, VertexProperty, EdgeProperty, GraphProperty,
         distributedS<ProcessGroup, InVertex, InDistribution>,
         InEdgeIndex>
{
  typedef compressed_sparse_row_graph self_type;

 private:
  /**
   *  Determine the type used to represent vertices in the graph. If
   *  the user has overridden the default, use the user's
   *  parameter. Otherwise, fall back to std::size_t.
   */
  typedef typename mpl::if_<is_same<InVertex, defaultS>,
                            std::size_t,
                            InVertex>::type Vertex;

  /**
   *  Determine the type used to represent edges in the graph. If
   *  the user has overridden the default (which is to be the same as
   *  the distributed vertex selector type), use the user's
   *  parameter. Otherwise, fall back to the value of @c Vertex.
   */
  typedef typename mpl::if_<is_same<InEdgeIndex,
                                    distributedS<ProcessGroup, InVertex,
                                                 InDistribution> >,
                            Vertex,
                            InEdgeIndex>::type EdgeIndex;

 public:
  /**
   * The type of the CSR graph that will be stored locally.
   */
  typedef compressed_sparse_row_graph<directedS, VertexProperty, EdgeProperty,
                                      GraphProperty, Vertex, EdgeIndex>
    base_type;

  // -----------------------------------------------------------------
  // Graph concept requirements
  typedef Vertex vertex_descriptor;
  typedef typename graph_traits<base_type>::edge_descriptor edge_descriptor;
  typedef directed_tag directed_category;
  typedef allow_parallel_edge_tag edge_parallel_category;
  typedef distributed_csr_tag traversal_category;
  static vertex_descriptor null_vertex();

  // -----------------------------------------------------------------
  // Distributed Vertex List Graph concept requirements
  typedef Vertex vertices_size_type;
  class vertex_iterator;

  // -----------------------------------------------------------------
  // Distributed Edge List Graph concept requirements
  typedef EdgeIndex edges_size_type;
  class edge_iterator;

  // -----------------------------------------------------------------
  // Incidence Graph concept requirements
  typedef typename graph_traits<base_type>::out_edge_iterator
    out_edge_iterator;
  typedef typename graph_traits<base_type>::degree_size_type
    degree_size_type;

  // -----------------------------------------------------------------
  // Adjacency Graph concept requirements
  typedef typename graph_traits<base_type>::adjacency_iterator
    adjacency_iterator;

  // Note: This graph type does not model Bidirectional Graph.
  // However, this typedef is required to satisfy graph_traits.
  typedef void in_edge_iterator;

  // -----------------------------------------------------------------
  // Distributed Container concept requirements
  typedef ProcessGroup process_group_type;
  typedef boost::parallel::variant_distribution<process_group_type, Vertex>
    distribution_type;

  // -----------------------------------------------------------------
  // Workarounds
  // NOTE: This graph type does not have old-style graph properties. It only
  // accepts bundles.
  typedef no_property vertex_property_type;
  typedef no_property edge_property_type;
  typedef no_property graph_property_type;
  typedef typename mpl::if_<is_void<VertexProperty>,
                            void****,
                            VertexProperty>::type vertex_bundled;
  typedef typename mpl::if_<is_void<EdgeProperty>,
                            void****,
                            EdgeProperty>::type edge_bundled;
  typedef typename mpl::if_<is_void<GraphProperty>,
                            void****,
                            GraphProperty>::type graph_bundled;

  // -----------------------------------------------------------------
  // Useful types
  typedef typename ProcessGroup::process_id_type process_id_type;

  // -----------------------------------------------------------------
  // Graph constructors
  compressed_sparse_row_graph(const ProcessGroup& pg = ProcessGroup())
    : m_process_group(pg), m_distribution(parallel::block(pg, 0)) {}

  compressed_sparse_row_graph(const GraphProperty& prop,
                              const ProcessGroup& pg = ProcessGroup())
    : m_process_group(pg), m_distribution(parallel::block(pg, 0)) {}

  compressed_sparse_row_graph(vertices_size_type numverts,
                              const ProcessGroup& pg = ProcessGroup())
    : m_process_group(pg), m_distribution(parallel::block(pg, 0)),
      m_base(numverts) 
  {}

  compressed_sparse_row_graph(vertices_size_type numverts,
                              const GraphProperty& prop,
                              const ProcessGroup& pg = ProcessGroup())
    : m_process_group(pg), m_distribution(parallel::block(pg, 0)),
      m_base(numverts) 
  {}

  template <typename Distribution>
  compressed_sparse_row_graph(vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist)
    : m_process_group(pg), m_distribution(dist), m_base(numverts) {}

  template <typename Distribution>
  compressed_sparse_row_graph(vertices_size_type numverts,
                              const GraphProperty& prop,
                              const ProcessGroup& pg,
                              const Distribution& dist)
    : m_process_group(pg), m_distribution(dist), m_base(numverts) {}

  template <typename InputIterator>
  compressed_sparse_row_graph(edges_are_unsorted_t,
                              InputIterator edge_begin, InputIterator edge_end,
                              vertices_size_type numverts,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template <typename InputIterator, typename Distribution>
  compressed_sparse_row_graph(edges_are_unsorted_t,
                              InputIterator edge_begin, InputIterator edge_end,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  template <typename InputIterator, typename EdgePropertyIterator>
  compressed_sparse_row_graph(edges_are_unsorted_t,
                              InputIterator edge_begin, InputIterator edge_end,
                              EdgePropertyIterator ep_iter,
                              vertices_size_type numverts,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template <typename InputIterator, typename EdgePropertyIterator,
            typename Distribution>
  compressed_sparse_row_graph(edges_are_unsorted_t,
                              InputIterator edge_begin, InputIterator edge_end,
                              EdgePropertyIterator ep_iter,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  template <typename InputIterator>
  compressed_sparse_row_graph(edges_are_sorted_t,
                              InputIterator edge_begin, InputIterator edge_end,
                              vertices_size_type numverts,
                              edges_size_type numedges = 0,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template <typename InputIterator, typename Distribution>
  compressed_sparse_row_graph(edges_are_sorted_t,
                              InputIterator edge_begin, InputIterator edge_end,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  template <typename InputIterator, typename EdgePropertyIterator>
  compressed_sparse_row_graph(edges_are_sorted_t,
                              InputIterator edge_begin, InputIterator edge_end,
                              EdgePropertyIterator ep_iter,
                              vertices_size_type numverts,
                              edges_size_type numedges = 0,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template <typename InputIterator, typename EdgePropertyIterator,
            typename Distribution>
  compressed_sparse_row_graph(edges_are_sorted_t,
                              InputIterator edge_begin, InputIterator edge_end,
                              EdgePropertyIterator ep_iter,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  template <typename MultiPassInputIterator>
  compressed_sparse_row_graph(edges_are_unsorted_multi_pass_t,
                              MultiPassInputIterator edge_begin, 
                              MultiPassInputIterator edge_end,
                              vertices_size_type numverts,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template <typename MultiPassInputIterator, typename Distribution>
  compressed_sparse_row_graph(edges_are_unsorted_multi_pass_t,
                              MultiPassInputIterator edge_begin, 
                              MultiPassInputIterator edge_end,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  template <typename MultiPassInputIterator, typename EdgePropertyIterator>
  compressed_sparse_row_graph(edges_are_unsorted_multi_pass_t,
                              MultiPassInputIterator edge_begin, 
                              MultiPassInputIterator edge_end,
                              EdgePropertyIterator ep_iter,
                              vertices_size_type numverts,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template <typename MultiPassInputIterator, typename EdgePropertyIterator,
            typename Distribution>
  compressed_sparse_row_graph(edges_are_unsorted_multi_pass_t,
                              MultiPassInputIterator edge_begin, 
                              MultiPassInputIterator edge_end,
                              EdgePropertyIterator ep_iter,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  template <typename Source>
  compressed_sparse_row_graph(distributed_construct_inplace_from_sources_and_targets_t,
                              std::vector<Source>& sources,
                              std::vector<vertex_descriptor>& targets,
                              vertices_size_type numverts,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template <typename Distribution, typename Source> 
  compressed_sparse_row_graph(distributed_construct_inplace_from_sources_and_targets_t,
                              std::vector<Source>& sources,
                              std::vector<vertex_descriptor>& targets,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  template <typename Source>
  compressed_sparse_row_graph(distributed_construct_inplace_from_sources_and_targets_t,
                              std::vector<Source>& sources,
                              std::vector<vertex_descriptor>& targets,
                              std::vector<edge_bundled>& edge_props,
                              vertices_size_type numverts,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template <typename Distribution, typename Source>
  compressed_sparse_row_graph(distributed_construct_inplace_from_sources_and_targets_t,
                              std::vector<Source>& sources,
                              std::vector<vertex_descriptor>& targets,
                              std::vector<edge_bundled>& edge_props,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  template<typename InputIterator>
  compressed_sparse_row_graph(InputIterator edge_begin, InputIterator edge_end,
                              vertices_size_type numverts,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template<typename InputIterator, typename EdgePropertyIterator>
  compressed_sparse_row_graph(InputIterator edge_begin, InputIterator edge_end,
                              EdgePropertyIterator ep_iter,
                              vertices_size_type numverts,
                              const ProcessGroup& pg = ProcessGroup(),
                              const GraphProperty& prop = GraphProperty());

  template<typename InputIterator, typename Distribution>
  compressed_sparse_row_graph(InputIterator edge_begin, InputIterator edge_end,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  template<typename InputIterator, typename EdgePropertyIterator, 
           typename Distribution>
  compressed_sparse_row_graph(InputIterator edge_begin, InputIterator edge_end,
                              EdgePropertyIterator ep_iter,
                              vertices_size_type numverts,
                              const ProcessGroup& pg,
                              const Distribution& dist,
                              const GraphProperty& prop = GraphProperty());

  base_type&       base()       { return m_base; }
  const base_type& base() const { return m_base; }

  process_group_type process_group() const { return m_process_group.base(); }

  distribution_type&       distribution()       { return m_distribution; }
  const distribution_type& distribution() const { return m_distribution; }

  // Directly access a vertex or edge bundle
  vertex_bundled& operator[](vertex_descriptor v)
  {
    return get(vertex_bundle, *this, v);
  }

  const vertex_bundled& operator[](vertex_descriptor v) const
  {
    return get(vertex_bundle, *this, v);
  }

  edge_bundled& operator[](edge_descriptor e)
  {
    return get(edge_bundle, *this, e);
  }

  const edge_bundled& operator[](edge_descriptor e) const
  {
    return get(edge_bundle, *this, e);
  }

  // Create a vertex descriptor from a process ID and a local index.
  vertex_descriptor 
  make_vertex_descriptor(process_id_type p, vertex_descriptor v) const
  {
    vertex_descriptor vertex_local_index_bits = 
      sizeof(vertex_descriptor) * CHAR_BIT - processor_bits;
    return v | ((vertex_descriptor)p << vertex_local_index_bits);
  }

  // Convert a local vertex descriptor into a global vertex descriptor
  vertex_descriptor local_to_global_vertex(vertex_descriptor v) const
  {
    return make_vertex_descriptor(process_id(m_process_group), v);
  }

  // Structural modification
  vertex_descriptor add_vertex()
  {
    typename graph_traits<base_type>::vertex_descriptor v 
      = boost::add_vertex(m_base);

    return make_vertex_descriptor(process_id(m_process_group), v);
  }

  vertex_descriptor add_vertex(const vertex_bundled& p)
  {
    typename graph_traits<base_type>::vertex_descriptor v 
      = boost::add_vertex(m_base, p);

    return make_vertex_descriptor(process_id(m_process_group), v);
  }

  vertex_descriptor add_vertices(vertices_size_type count)
  {
    typename graph_traits<base_type>::vertex_descriptor v 
      = boost::add_vertices(count, m_base);

    return make_vertex_descriptor(process_id(m_process_group), v);
  }

  template <typename InputIterator>
  void 
  add_edges(InputIterator first, InputIterator last)
  { boost::add_edges_global(first, last, get(vertex_local, *this), m_base); }

  template <typename InputIterator, typename EdgePropertyIterator>
  void 
  add_edges(InputIterator first, InputIterator last,
            EdgePropertyIterator ep_iter,
            EdgePropertyIterator ep_iter_end)
  { boost::add_edges_global(first, last, ep_iter, ep_iter_end, 
                            get(vertex_local, *this), m_base); }

  template <typename InputIterator>
  void 
  add_edges_sorted(InputIterator first, InputIterator last)
  { boost::add_edges_sorted_global(first, last, 
                                   get(vertex_local, *this), m_base); }

  template <typename InputIterator, typename EdgePropertyIterator>
  void 
  add_edges_sorted(InputIterator first_sorted, InputIterator last_sorted,
                   EdgePropertyIterator ep_iter_sorted)
  { boost::add_edges_sorted_global(first_sorted, last_sorted, ep_iter_sorted, 
                                   get(vertex_local, *this), m_base); }

 protected:
  ProcessGroup m_process_group;
  distribution_type m_distribution;
  base_type m_base;
};

/** @brief Helper macro containing the template parameters for the
 *   distributed CSR graph.
 *
 *  This macro contains all of the template parameters needed for the
 *  distributed compressed_sparse_row graph type. It is used to reduce
 *  the amount of typing required to declare free functions for this
 *  graph type.
 */
#define BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS                          \
  typename VertexProperty, typename EdgeProperty,    \
  typename GraphProperty, typename ProcessGroup, typename InVertex,     \
  typename InDistribution, typename InEdgeIndex

/** @brief Helper macro containing the typical instantiation of the
 *   distributed CSR graph.
 *
 *  This macro contains an instantiation of the distributed CSR graph
 *  type using the typical template parameters names (e.g., those
 *  provided by the macro @c
 *  BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS). It is used to reduce
 *  the amount of typing required to declare free functions for this
 *  graph type.
 */
#define BOOST_DISTRIB_CSR_GRAPH_TYPE                            \
  compressed_sparse_row_graph<                                  \
    directedS, VertexProperty, EdgeProperty, GraphProperty,      \
    distributedS<ProcessGroup, InVertex, InDistribution>,       \
    InEdgeIndex>

// -----------------------------------------------------------------
// Graph concept operations
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
BOOST_DISTRIB_CSR_GRAPH_TYPE::null_vertex()
{
  return graph_traits<base_type>::null_vertex();
}

// -----------------------------------------------------------------
// Incidence Graph concept operations
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
source(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor e,
       const BOOST_DISTRIB_CSR_GRAPH_TYPE&)
{ return e.src; }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
target(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor e,
       const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{ return target(e, g.base()); }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline std::pair<typename BOOST_DISTRIB_CSR_GRAPH_TYPE::out_edge_iterator,
                 typename BOOST_DISTRIB_CSR_GRAPH_TYPE::out_edge_iterator>
out_edges(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor u,
          const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edges_size_type
    edges_size_type;
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor ed;
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::out_edge_iterator it;
  edges_size_type u_local = get(vertex_local, g, u);
  edges_size_type u_row_start = g.base().m_forward.m_rowstart[u_local];
  edges_size_type next_row_start = g.base().m_forward.m_rowstart[u_local + 1];
  return std::make_pair(it(ed(u, u_row_start)),
                        it(ed(u, (std::max)(u_row_start, next_row_start))));
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::degree_size_type
out_degree(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor u,
           const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  return out_degree(get(vertex_local, g, u), g.base());
}

// -----------------------------------------------------------------
// DistributedGraph concept requirements
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
void synchronize(const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  synchronize(g.process_group());
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS> 
ProcessGroup
process_group(const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{ return g.process_group(); }


// -----------------------------------------------------------------
// Adjacency Graph concept requirements
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline std::pair<typename BOOST_DISTRIB_CSR_GRAPH_TYPE::adjacency_iterator,
                 typename BOOST_DISTRIB_CSR_GRAPH_TYPE::adjacency_iterator>
adjacent_vertices(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor u,
                  const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  return adjacent_vertices(get(vertex_local, g, u), g.base());
}

// -----------------------------------------------------------------
// Distributed Vertex List Graph concept operations
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
class BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_iterator
  : public iterator_adaptor<vertex_iterator,
                            counting_iterator<Vertex>,
                            Vertex,
                            random_access_traversal_tag,
                            Vertex>
{
  typedef iterator_adaptor<vertex_iterator,
                           counting_iterator<Vertex>,
                           Vertex,
                           random_access_traversal_tag,
                           Vertex> inherited;
 public:
  vertex_iterator() {}

  explicit vertex_iterator(Vertex v, const self_type* graph)
    : inherited(counting_iterator<Vertex>(v)), graph(graph) { }

  Vertex dereference() const
  {
    return graph->local_to_global_vertex(*(this->base_reference()));
  }

  friend class iterator_core_access;

 private:
  const self_type* graph;
};

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::degree_size_type
num_vertices(const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  return num_vertices(g.base());
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline std::pair<typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_iterator,
                 typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_iterator>
vertices(const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_iterator
    vertex_iterator;
  return std::make_pair(vertex_iterator(0, &g),
                        vertex_iterator(num_vertices(g), &g));
}

// -----------------------------------------------------------------
// Distributed Edge List Graph concept operations
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
class BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_iterator
{
 public:
  typedef std::forward_iterator_tag iterator_category;
  typedef edge_descriptor value_type;

  typedef const edge_descriptor* pointer;

  typedef edge_descriptor reference;
  typedef typename int_t<CHAR_BIT * sizeof(EdgeIndex)>::fast difference_type;

  edge_iterator() : graph(0), current_edge(), end_of_this_vertex(0) {}

  edge_iterator(const compressed_sparse_row_graph& graph,
                edge_descriptor current_edge,
                EdgeIndex end_of_this_vertex)
    : graph(&graph), local_src(current_edge.src), current_edge(current_edge),
      end_of_this_vertex(end_of_this_vertex)
  {
    // The edge that comes in has a local source vertex. Make it global.
    current_edge.src = graph.local_to_global_vertex(current_edge.src);
  }

  // From InputIterator
  reference operator*() const { return current_edge; }
  pointer operator->() const { return &current_edge; }

  bool operator==(const edge_iterator& o) const {
    return current_edge == o.current_edge;
  }
  bool operator!=(const edge_iterator& o) const {
    return current_edge != o.current_edge;
  }

  edge_iterator& operator++()
  {
    ++current_edge.idx;
    while (current_edge.idx == end_of_this_vertex && local_src < num_vertices(*graph)-1) {
      ++local_src;
      current_edge.src = graph->local_to_global_vertex(local_src);
      end_of_this_vertex = graph->base().m_forward.m_rowstart[local_src + 1];
    }
    return *this;
  }

  edge_iterator operator++(int) {
    edge_iterator temp = *this;
    ++*this;
    return temp;
  }

 private:
  const compressed_sparse_row_graph* graph;
  EdgeIndex local_src;
  edge_descriptor current_edge;
  EdgeIndex end_of_this_vertex;
};

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edges_size_type
num_edges(const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  return g.base().m_forward.m_column.size();
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
std::pair<typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_iterator,
          typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_iterator>
edges(const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor Vertex;
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_iterator ei;
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor edgedesc;
  if (g.base().m_forward.m_rowstart.size() == 1 ||
      g.base().m_forward.m_column.empty()) {
    return std::make_pair(ei(), ei());
  } else {
    // Find the first vertex that has outgoing edges
    Vertex src = 0;
    while (g.base().m_forward.m_rowstart[src + 1] == 0) ++src;
    return std::make_pair(ei(g, edgedesc(src, 0), g.base().m_forward.m_rowstart[src + 1]),
                          ei(g, edgedesc(num_vertices(g), g.base().m_forward.m_column.size()), 0));
  }
}

// -----------------------------------------------------------------
// Graph constructors

// Returns true if a vertex belongs to a process according to a distribution
template <typename OwnerMap, typename ProcessId>
struct local_vertex {

  local_vertex(OwnerMap owner, ProcessId id) 
    : owner(owner), id(id) {}

  template <typename Vertex>
  bool operator()(Vertex x) 
  { return get(owner, x) == id; }

  template <typename Vertex>
  bool operator()(Vertex x) const
  { return get(owner, x) == id; }

private:
  OwnerMap owner;
  ProcessId id;
};

// Returns true if a vertex belongs to a process according to a distribution
template <typename OwnerMap, typename ProcessId>
struct local_edge {

  local_edge(OwnerMap owner, ProcessId id) 
    : owner(owner), id(id) {}

  template <typename Vertex>
  bool operator()(std::pair<Vertex, Vertex>& x) 
  { return get(owner, x.first) == id; }

  template <typename Vertex>
  bool operator()(const std::pair<Vertex, Vertex>& x) const
  { return get(owner, x.first) == id; }

private:
  OwnerMap owner;
  ProcessId id;
};

// Turns an index iterator into a vertex iterator
template<typename IndexIterator, typename Graph>
class index_to_vertex_iterator {
  
public:
  typedef std::input_iterator_tag iterator_category;
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  typedef std::pair<Vertex, Vertex> value_type;
  typedef const value_type& reference;
  typedef const value_type* pointer;
  typedef void difference_type;

  index_to_vertex_iterator(IndexIterator index,
                           const Graph& g) 
    : index(index), g(g), current(to_edge(*index)) {}

  reference operator*() { current = to_edge(*index); return current; }
  pointer operator->() { current = to_edge(*index); return &current; }

  index_to_vertex_iterator& operator++()
  {
    ++index;
    return *this;
  }

  index_to_vertex_iterator operator++(int)
  {
    index_to_vertex_iterator temp(*this);
    ++(*this);
    return temp;
  }

  bool operator==(const index_to_vertex_iterator& other) const
  { return index == other.index; }
  
  bool operator!=(const index_to_vertex_iterator& other) const
  { return !(*this == other); }

private:
  value_type to_edge(const typename std::iterator_traits<IndexIterator>::value_type& x)
  { return std::make_pair(vertex(x.first, g), vertex(x.second, g)); }

  IndexIterator index;
  const Graph& g;  
  value_type current;
};

template <typename Distribution, typename Graph>
struct index_to_vertex_func {

  typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename boost::graph_traits<Graph>::vertices_size_type vertices_size_type;
  typedef std::pair<vertex_descriptor, vertex_descriptor> result_type;
  typedef std::pair<vertices_size_type, vertices_size_type> base_iterator_type;

  index_to_vertex_func(const Distribution& dist, const Graph& g)
    : dist(dist), g(g) {}


  result_type operator()(const base_iterator_type& p) const 
  {
    return std::make_pair(vertex(p.first, g), vertex(p.second, g));
  }

private:
  const Distribution& dist;
  const Graph& g;
};

// NGE: This method only works with iterators that have a difference_type,
// the index_to_vertex_iterator class above is retained for compatibility
// with BGL generators which have no difference_type
template <typename IndexIterator, typename Distribution, typename Graph>
boost::transform_iterator<index_to_vertex_func<Distribution, Graph>, IndexIterator>
make_index_to_vertex_iterator(IndexIterator it, const Distribution& dist, 
                              const Graph& g) {
  return boost::make_transform_iterator(
    it, index_to_vertex_func<Distribution, Graph>(dist, g));
}

// Forward declaration of csr_vertex_owner_map
template<typename ProcessID, typename Key> class csr_vertex_owner_map;

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename InputIterator>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_unsorted_t,
                            InputIterator edge_begin, InputIterator edge_end,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(edges_are_unsorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template <typename InputIterator, typename Distribution>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_unsorted_t,
                            InputIterator edge_begin, InputIterator edge_end,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(edges_are_unsorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename InputIterator, typename EdgePropertyIterator>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_unsorted_t,
                            InputIterator edge_begin, InputIterator edge_end,
                            EdgePropertyIterator ep_iter,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(edges_are_unsorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           ep_iter,
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template <typename InputIterator, typename EdgePropertyIterator,
          typename Distribution>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_unsorted_t,
                            InputIterator edge_begin, InputIterator edge_end,
                            EdgePropertyIterator ep_iter,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(edges_are_unsorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           ep_iter,
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename InputIterator>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_sorted_t,
                            InputIterator edge_begin, InputIterator edge_end,
                            vertices_size_type numverts,
                            edges_size_type numedges, // This is not used as there is no appropriate BGL ctor
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(edges_are_sorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           m_distribution.block_size(process_id(m_process_group), numverts),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template <typename InputIterator, typename Distribution>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_sorted_t,
                            InputIterator edge_begin, InputIterator edge_end,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(edges_are_sorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           m_distribution.block_size(process_id(m_process_group), numverts),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename InputIterator, typename EdgePropertyIterator>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_sorted_t,
                            InputIterator edge_begin, InputIterator edge_end,
                            EdgePropertyIterator ep_iter,
                            vertices_size_type numverts,
                            edges_size_type numedges, // This is not used as there is no appropriate BGL ctor
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(edges_are_sorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           ep_iter,
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           m_distribution.block_size(process_id(m_process_group), numverts),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename InputIterator, typename EdgePropertyIterator,
         typename Distribution>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_sorted_t,
                            InputIterator edge_begin, InputIterator edge_end,
                            EdgePropertyIterator ep_iter,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(edges_are_sorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           ep_iter,
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           m_distribution.block_size(process_id(m_process_group), numverts),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename MultiPassInputIterator>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_unsorted_multi_pass_t,
                            MultiPassInputIterator edge_begin, 
                            MultiPassInputIterator edge_end,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(edges_are_unsorted_multi_pass_global,
           make_index_to_vertex_iterator(edge_begin, parallel::block(m_process_group, numverts), *this),
           make_index_to_vertex_iterator(edge_end, parallel::block(m_process_group, numverts), *this),
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template <typename MultiPassInputIterator, typename Distribution>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_unsorted_multi_pass_t,
                            MultiPassInputIterator edge_begin, 
                            MultiPassInputIterator edge_end,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(edges_are_unsorted_multi_pass_global,
           make_index_to_vertex_iterator(edge_begin, parallel::block(m_process_group, numverts), *this),
           make_index_to_vertex_iterator(edge_end, parallel::block(m_process_group, numverts), *this),
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{ }


template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename MultiPassInputIterator, typename EdgePropertyIterator>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_unsorted_multi_pass_t,
                            MultiPassInputIterator edge_begin, 
                            MultiPassInputIterator edge_end,
                            EdgePropertyIterator ep_iter,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(edges_are_unsorted_multi_pass_global,
           make_index_to_vertex_iterator(edge_begin, parallel::block(m_process_group, numverts), *this),
           make_index_to_vertex_iterator(edge_end, parallel::block(m_process_group, numverts), *this),
           ep_iter,
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template <typename MultiPassInputIterator, typename EdgePropertyIterator,
          typename Distribution>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(edges_are_unsorted_multi_pass_t,
                            MultiPassInputIterator edge_begin, 
                            MultiPassInputIterator edge_end,
                            EdgePropertyIterator ep_iter,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(edges_are_unsorted_multi_pass_global,
           make_index_to_vertex_iterator(edge_begin, parallel::block(m_process_group, numverts), *this),
           make_index_to_vertex_iterator(edge_end, parallel::block(m_process_group, numverts), *this),
           ep_iter,
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{ }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename Source>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(distributed_construct_inplace_from_sources_and_targets_t,
                            std::vector<Source>& sources,
                            std::vector<vertex_descriptor>& targets,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(m_distribution.block_size(process_id(m_process_group), numverts))
{ 
  // Convert linear indices to global indices
  for (edges_size_type i = 0; i < sources.size(); ++i) {
    sources[i] = m_distribution.local(sources[i]);
    targets[i] = make_vertex_descriptor(m_distribution(targets[i]), 
                                        m_distribution.local(targets[i]));
  }

  m_base.assign_sources_and_targets_global(
    sources, targets, m_distribution.block_size(process_id(m_process_group), numverts),
    identity_property_map());

  // TODO: set property on m_base?
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template <typename Distribution, typename Source> 
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(distributed_construct_inplace_from_sources_and_targets_t,
                            std::vector<Source>& sources,
                            std::vector<vertex_descriptor>& targets,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(m_distribution.block_size(process_id(m_process_group), numverts))
{ 
  // Convert linear indices to global indices
  for (edges_size_type i = 0; i < sources.size(); ++i) {
    sources[i] = m_distribution.local(sources[i]);
    targets[i] = make_vertex_descriptor(m_distribution(targets[i]), 
                                        m_distribution.local(targets[i]));
  }

  m_base.assign_sources_and_targets_global(
    sources, targets, m_distribution.block_size(process_id(m_process_group), numverts),
    identity_property_map());

  // TODO: set property on m_base?
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename Source>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(distributed_construct_inplace_from_sources_and_targets_t,
                            std::vector<Source>& sources,
                            std::vector<vertex_descriptor>& targets,
                            std::vector<edge_bundled>& edge_props,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(m_distribution.block_size(process_id(m_process_group), numverts))
{ 
  // Convert linear indices to global indices
  for (edges_size_type i = 0; i < sources.size(); ++i) {
    sources[i] = m_distribution.local(sources[i]);
    targets[i] = make_vertex_descriptor(m_distribution(targets[i]), 
                                        m_distribution.local(targets[i]));
  }

  m_base.assign_sources_and_targets_global(
    sources, targets, edge_props, 
    m_distribution.block_size(process_id(m_process_group), numverts),
    identity_property_map());

  // TODO: set property on m_base?
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template <typename Distribution, typename Source> 
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(distributed_construct_inplace_from_sources_and_targets_t,
                            std::vector<Source>& sources,
                            std::vector<vertex_descriptor>& targets,
                            std::vector<edge_bundled>& edge_props,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(m_distribution.block_size(process_id(m_process_group), numverts))
{ 
  // Convert linear indices to global indices
  for (edges_size_type i = 0; i < sources.size(); ++i) {
    sources[i] = m_distribution.local(sources[i]);
    targets[i] = make_vertex_descriptor(m_distribution(targets[i]), 
                                        m_distribution.local(targets[i]));
  }

  m_base.assign_sources_and_targets_global(
    sources, targets, edge_props, 
    m_distribution.block_size(process_id(m_process_group), numverts),
    identity_property_map());

  // TODO: set property on m_base?
}

//
// Old (untagged) ctors, these default to the unsorted sequential ctors
//
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename InputIterator>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(InputIterator edge_begin, InputIterator edge_end,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(edges_are_unsorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
           
{
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename InputIterator, typename EdgePropertyIterator>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(InputIterator edge_begin, InputIterator edge_end,
                            EdgePropertyIterator ep_iter,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const GraphProperty& prop)
  : m_process_group(pg),

    m_distribution(parallel::block(m_process_group, numverts)),
    m_base(edges_are_unsorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           ep_iter,
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename InputIterator, typename Distribution>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(InputIterator edge_begin, InputIterator edge_end,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(edges_are_unsorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
template<typename InputIterator, typename EdgePropertyIterator, 
         typename Distribution>
BOOST_DISTRIB_CSR_GRAPH_TYPE::
compressed_sparse_row_graph(InputIterator edge_begin, InputIterator edge_end,
                            EdgePropertyIterator ep_iter,
                            vertices_size_type numverts,
                            const ProcessGroup& pg,
                            const Distribution& dist,
                            const GraphProperty& prop)
  : m_process_group(pg),
    m_distribution(dist),
    m_base(edges_are_unsorted_global,
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_begin, *this),
           index_to_vertex_iterator<InputIterator, BOOST_DISTRIB_CSR_GRAPH_TYPE>(edge_end, *this),
           m_distribution.block_size(process_id(m_process_group), numverts),
           get(vertex_local, *this),
           local_vertex<csr_vertex_owner_map<process_id_type, vertex_descriptor>, 
                        process_id_type> (get(vertex_owner, *this), process_id(pg)),
           prop)
{
}

// -----------------------------------------------------------------
// Vertex Global Property Map
template<typename ProcessID, typename Key>
class csr_vertex_global_map
{
 public:
  // -----------------------------------------------------------------
  // Readable Property Map concept requirements
  typedef std::pair<ProcessID, Key> value_type;
  typedef value_type reference;
  typedef Key key_type;
  typedef readable_property_map_tag category;
};

template<typename ProcessID, typename Key>
inline std::pair<ProcessID, Key>
get(csr_vertex_global_map<ProcessID, Key>,
    typename csr_vertex_global_map<ProcessID, Key>::key_type k)
{
  const int local_index_bits = sizeof(Key) * CHAR_BIT - processor_bits;
  const Key local_index_mask = Key(-1) >> processor_bits;

  return std::pair<ProcessID, Key>(k >> local_index_bits,
                                   k & local_index_mask);
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
class property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_global_t>
{
 public:
  typedef csr_vertex_global_map<
            typename ProcessGroup::process_id_type,
            typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor> type;
  typedef type const_type;
};

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_global_t>::type
get(vertex_global_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_global_t>
    ::type result_type;
  return result_type();
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
std::pair<typename ProcessGroup::process_id_type,
          typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor>
get(vertex_global_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  return get(vertex_global, 
             const_cast<const BOOST_DISTRIB_CSR_GRAPH_TYPE&>(g), 
             k);
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_global_t>::const_type
get(vertex_global_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_global_t>
    ::const_type result_type;
  return result_type();
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
std::pair<typename ProcessGroup::process_id_type,
          typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor>
get(vertex_global_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
    vertex_descriptor;
  typedef std::pair<typename ProcessGroup::process_id_type, vertex_descriptor>
    result_type;
  const int local_index_bits = 
    sizeof(vertex_descriptor) * CHAR_BIT - processor_bits;
  const vertex_descriptor local_index_mask = 
    vertex_descriptor(-1) >> processor_bits;

  return result_type(k >> local_index_bits, k & local_index_mask);
}

// -----------------------------------------------------------------
// Extra, common functions
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
vertex(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertices_size_type i,
       const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  return g.make_vertex_descriptor(g.distribution()(i), 
                                  g.distribution().local(i));
}

// Unlike for an adjacency_matrix, edge_range and edge take lg(out_degree(i))
// time
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline std::pair<typename BOOST_DISTRIB_CSR_GRAPH_TYPE::out_edge_iterator,
                 typename BOOST_DISTRIB_CSR_GRAPH_TYPE::out_edge_iterator>
edge_range(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor i,
           typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor j,
           const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor Vertex;
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edges_size_type EdgeIndex;
  typedef typename std::vector<Vertex>::const_iterator adj_iter;
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::out_edge_iterator out_edge_iter;
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor edge_desc;
  std::pair<adj_iter, adj_iter> raw_adjacencies = adjacent_vertices(i, g);
  std::pair<adj_iter, adj_iter> adjacencies =
    std::equal_range(raw_adjacencies.first, raw_adjacencies.second, j);
  EdgeIndex idx_begin = adjacencies.first - g.base().m_forward.m_column.begin();
  EdgeIndex idx_end = adjacencies.second - g.base().m_forward.m_column.begin();
  return std::make_pair(out_edge_iter(edge_desc(i, idx_begin)),
                        out_edge_iter(edge_desc(i, idx_end)));
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline std::pair<typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor, bool>
edge(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor i,
     typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor j,
     const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::out_edge_iterator out_edge_iter;
  std::pair<out_edge_iter, out_edge_iter> range = edge_range(i, j, g);
  if (range.first == range.second)
    return std::make_pair(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor(),
                          false);
  else
    return std::make_pair(*range.first, true);
}

// A helper that turns requests for property maps for const graphs
// into property maps for non-const graphs.
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS, typename Property>
class property_map<const BOOST_DISTRIB_CSR_GRAPH_TYPE, Property>
{
 public:
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, Property>
    ::const_type type;
  typedef type const_type;
};

// -----------------------------------------------------------------
// Structural modifiers

#if 0
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
add_vertex(BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{ return g.add_vertex(); }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
add_vertex(const typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_bundled& p, 
           BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{ return g.add_vertex(p); }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
add_vertices(typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertices_size_type count, 
             BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{ return g.add_vertices(count); }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS, typename InputIterator>
void 
add_edges(InputIterator first, InputIterator last,
          BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{ g.add_edges(first, last); }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS, typename InputIterator, 
         typename EdgePropertyIterator>
void 
add_edges(InputIterator first, InputIterator last,
          EdgePropertyIterator ep_iter,
          EdgePropertyIterator ep_iter_end,
          BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{ return g.add_edges(first, last, ep_iter, ep_iter_end); }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS, typename InputIterator>
void 
add_edges_sorted(InputIterator first, InputIterator last,
                 BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{ return g.add_edges_sorted(first, last); }

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS, typename InputIterator, 
         typename EdgePropertyIterator>
void 
add_edges_sorted(InputIterator first_sorted, InputIterator last_sorted,
                 EdgePropertyIterator ep_iter_sorted,
                 BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{ g.add_edges_sorted(first_sorted, last_sorted, ep_iter_sorted); }
#endif

// -----------------------------------------------------------------
// Vertex Owner Property Map
template<typename ProcessID, typename Key>
class csr_vertex_owner_map
{
 public:
  // -----------------------------------------------------------------
  // Readable Property Map concept requirements
  typedef ProcessID value_type;
  typedef value_type reference;
  typedef Key key_type;
  typedef readable_property_map_tag category;
};

template<typename ProcessID, typename Key>
inline ProcessID
get(csr_vertex_owner_map<ProcessID, Key> pm,
    typename csr_vertex_owner_map<ProcessID, Key>::key_type k)
{
  const int local_index_bits = sizeof(Key) * CHAR_BIT - processor_bits;
  return k >> local_index_bits;
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
class property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_owner_t>
{
 public:
  typedef csr_vertex_owner_map<
            typename ProcessGroup::process_id_type,
            typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor> type;
  typedef type const_type;
};

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_owner_t>::type
get(vertex_owner_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_owner_t>
    ::type result_type;
  return result_type();
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename ProcessGroup::process_id_type
get(vertex_owner_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  return get(vertex_owner, 
             const_cast<const BOOST_DISTRIB_CSR_GRAPH_TYPE&>(g),
             k);
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_owner_t>::const_type
get(vertex_owner_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_owner_t>
    ::const_type result_type;
  return result_type();
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename ProcessGroup::process_id_type
get(vertex_owner_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
    vertex_descriptor;
  const int local_index_bits = 
    sizeof(vertex_descriptor) * CHAR_BIT - processor_bits;
  return k >> local_index_bits;
}

// -----------------------------------------------------------------
// Vertex Local Property Map
template<typename Key>
class csr_vertex_local_map
{
 public:
  // -----------------------------------------------------------------
  // Readable Property Map concept requirements
  typedef Key value_type;
  typedef value_type reference;
  typedef Key key_type;
  typedef readable_property_map_tag category;
};

template<typename Key>
inline Key
get(csr_vertex_local_map<Key> pm,
    typename csr_vertex_local_map<Key>::key_type k)
{
  const Key local_index_mask = Key(-1) >> processor_bits;
  return k & local_index_mask;
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
class property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_t>
{
 public:
  typedef csr_vertex_local_map<
            typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor> type;
  typedef type const_type;
};

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_t>::type
get(vertex_local_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_t>
    ::type result_type;
  return result_type();
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
get(vertex_local_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  return get(vertex_local, 
             const_cast<const BOOST_DISTRIB_CSR_GRAPH_TYPE&>(g),
             k);
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_t>::const_type
get(vertex_local_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_t>
    ::const_type result_type;
  return result_type();
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
get(vertex_local_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor 
    vertex_descriptor;
  const vertex_descriptor local_index_mask = 
    vertex_descriptor(-1) >> processor_bits;
  return k & local_index_mask;
}

// -----------------------------------------------------------------
// Vertex Index Property Map
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
class property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_index_t>
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, 
                                vertex_global_t>::const_type
    global_map;
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::process_group_type
    process_group_type;

  typedef property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_t> local;

public:
  typedef local_property_map<process_group_type, 
                             global_map, 
                             typename local::type> type;
  typedef local_property_map<process_group_type, 
                             global_map, 
                             typename local::const_type> const_type;
};

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_index_t>::type
get(vertex_index_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_index_t>
    ::type result_type;

  return result_type(g.process_group(), get(vertex_global, g),
                     get(vertex_local, g));
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertices_size_type
get(vertex_index_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  return get(vertex_local, g, k);
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_index_t>::const_type
get(vertex_index_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_index_t>
    ::const_type result_type;
  return result_type(g.process_group(), get(vertex_global, g),
                     get(vertex_local, g));
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertices_size_type
get(vertex_index_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  return get(vertex_local, g, k);
}

// -----------------------------------------------------------------
// Vertex Local Index Property Map
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
class property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_index_t>
  : public property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_t> { };

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_index_t>::type
get(vertex_local_index_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  return get(vertex_local, g);
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertices_size_type
get(vertex_local_index_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  return get(vertex_local, g, k);
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, vertex_local_index_t>::const_type
get(vertex_local_index_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  return get(vertex_local, g);
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertices_size_type
get(vertex_local_index_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor k)
{
  return get(vertex_local, g, k);
}

// -----------------------------------------------------------------
// Edge Global Property Map
template<typename ProcessID, typename Vertex, typename EdgeIndex>
class csr_edge_global_map
{
 public:
  // -----------------------------------------------------------------
  // Readable Property Map concept requirements
  typedef detail::csr_edge_descriptor<Vertex, EdgeIndex> key_type;
  typedef std::pair<ProcessID, detail::csr_edge_descriptor<Vertex, EdgeIndex> > value_type;
  typedef value_type reference;
  typedef readable_property_map_tag category;
};

template<typename ProcessID, typename Vertex, typename EdgeIndex>
inline std::pair<ProcessID, detail::csr_edge_descriptor<Vertex, EdgeIndex> >
get(csr_edge_global_map<ProcessID, Vertex, EdgeIndex> pm,
    typename csr_edge_global_map<ProcessID, Vertex, EdgeIndex>::key_type k)
{
  const int local_index_bits = sizeof(Vertex) * CHAR_BIT - processor_bits;
  const Vertex local_index_mask = Vertex(-1) >> processor_bits;
  return std::pair<ProcessID, detail::csr_edge_descriptor<Vertex, EdgeIndex> >
           ((k.src >> local_index_bits),
            detail::csr_edge_descriptor<Vertex, EdgeIndex>(k.src & local_index_mask, k.idx));
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
class property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_global_t>
{
 public:
  typedef csr_edge_global_map<
            typename ProcessGroup::process_id_type,
            typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor,
            typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edges_size_type> type;
  typedef type const_type;
};

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_global_t>::type
get(edge_global_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_global_t>
    ::type result_type;
  return result_type();
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
std::pair<typename ProcessGroup::process_id_type,
          typename BOOST_DISTRIB_CSR_GRAPH_TYPE::base_type::edge_descriptor>
get(edge_global_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor k)
{
  return get(edge_global, 
             const_cast<const BOOST_DISTRIB_CSR_GRAPH_TYPE&>(g),
             k);
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_global_t>::const_type
get(edge_global_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_global_t>
    ::const_type result_type;
  return result_type();
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
std::pair<typename ProcessGroup::process_id_type,
          typename BOOST_DISTRIB_CSR_GRAPH_TYPE::base_type::edge_descriptor>
get(edge_global_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor k)
{
  typedef typename BOOST_DISTRIB_CSR_GRAPH_TYPE::vertex_descriptor
    vertex_descriptor;

  const int local_index_bits = 
    sizeof(vertex_descriptor) * CHAR_BIT - processor_bits;
  const typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edges_size_type local_index_mask =
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edges_size_type(-1) >> processor_bits;
  
  typedef std::pair<typename ProcessGroup::process_id_type,
                    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::base_type::edge_descriptor>
    result_type;

  return result_type(k.src >> local_index_bits,
                     typename BOOST_DISTRIB_CSR_GRAPH_TYPE::base_type::edge_descriptor
                       (k.src & local_index_mask, k.idx));
}

// -----------------------------------------------------------------
// Edge Index Property Map
template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
class property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_index_t>
{
   typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_global_t>
    ::type global_map;

 public:
  typedef local_property_map<
            typename BOOST_DISTRIB_CSR_GRAPH_TYPE::process_group_type,
            global_map,
            typename property_map<typename BOOST_DISTRIB_CSR_GRAPH_TYPE::base_type, edge_index_t>::type
          > type;
  typedef type const_type;
};

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_index_t>::type
get(edge_index_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_index_t>
    ::type result_type;
  return result_type(g.process_group(), get(edge_global, g),
                     get(edge_index, g.base()));
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edges_size_type
get(edge_index_t, BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor k)
{
  return k.idx;
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_index_t>::const_type
get(edge_index_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, edge_index_t>
    ::const_type result_type;
  return result_type(g.process_group(), get(edge_global, g),
                     get(edge_index, g.base()));
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS>
inline typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edges_size_type
get(edge_index_t, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g,
    typename BOOST_DISTRIB_CSR_GRAPH_TYPE::edge_descriptor k)
{
  return k.idx;
}

template <BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS, typename Tag>
class property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, Tag> {
  typedef BOOST_DISTRIB_CSR_GRAPH_TYPE graph_type;
  typedef typename graph_type::process_group_type process_group_type;
  typedef typename graph_type::base_type base_graph_type;
  typedef typename property_map<base_graph_type, Tag>::type
    local_pmap;
  typedef typename property_map<base_graph_type, Tag>::const_type
    local_const_pmap;

  typedef graph_traits<graph_type> traits;
  typedef typename graph_traits<base_graph_type>::vertex_descriptor local_vertex;
  typedef typename property_traits<local_pmap>::key_type local_key_type;

  typedef typename property_traits<local_pmap>::value_type value_type;

  typedef typename property_map<graph_type, vertex_global_t>::const_type
    vertex_global_map;
  typedef typename property_map<graph_type, edge_global_t>::const_type
    edge_global_map;

  typedef typename mpl::if_<is_same<typename detail::property_kind_from_graph<base_graph_type, Tag>::type,
                                    vertex_property_tag>,
                            vertex_global_map, edge_global_map>::type
    global_map;

public:
  typedef ::boost::parallel::distributed_property_map<
            process_group_type, global_map, local_pmap> type;

  typedef ::boost::parallel::distributed_property_map<
            process_group_type, global_map, local_const_pmap> const_type;
};

template <BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS, typename Tag>
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, Tag>::type
get(Tag tag, BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef BOOST_DISTRIB_CSR_GRAPH_TYPE Graph;
  typedef typename property_map<Graph, Tag>::type result_type;
  typedef typename property_traits<result_type>::value_type value_type;
  typedef typename property_reduce<Tag>::template apply<value_type>
    reduce;

  typedef typename mpl::if_<is_same<typename detail::property_kind_from_graph<Graph, Tag>::type,
                                    vertex_property_tag>,
                            vertex_global_t, edge_global_t>::type
    global_map_t;

  return result_type(g.process_group(), get(global_map_t(), g),
                     get(tag, g.base()), reduce());
}

template<BOOST_DISTRIB_CSR_GRAPH_TEMPLATE_PARMS, typename Tag>
typename property_map<BOOST_DISTRIB_CSR_GRAPH_TYPE, Tag>::const_type
get(Tag tag, const BOOST_DISTRIB_CSR_GRAPH_TYPE& g)
{
  typedef BOOST_DISTRIB_CSR_GRAPH_TYPE Graph;
  typedef typename property_map<Graph, Tag>::const_type result_type;
  typedef typename property_traits<result_type>::value_type value_type;
  typedef typename property_reduce<Tag>::template apply<value_type>
    reduce;

  typedef typename property_traits<result_type>::key_type descriptor;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef typename mpl::if_<is_same<descriptor, vertex_descriptor>,
                            vertex_global_t, edge_global_t>::type
    global_map_t;

  return result_type(g.process_group(), get(global_map_t(), g),
                     get(tag, g.base()), reduce());
}

namespace mpi {
  template<typename Vertex, typename EdgeIndex>
  struct is_mpi_datatype<boost::detail::csr_edge_descriptor<Vertex, EdgeIndex> >
    : mpl::true_ { };
}

namespace serialization {
  template<typename Vertex, typename EdgeIndex>
  struct is_bitwise_serializable<boost::detail::csr_edge_descriptor<Vertex, EdgeIndex> >
    : mpl::true_ { };

  template<typename Vertex, typename EdgeIndex>
  struct implementation_level<boost::detail::csr_edge_descriptor<Vertex, EdgeIndex> >
   : mpl::int_<object_serializable> {} ;

  template<typename Vertex, typename EdgeIndex>
  struct tracking_level<boost::detail::csr_edge_descriptor<Vertex, EdgeIndex> >
   : mpl::int_<track_never> {} ;

}

} // end namespace boost

#endif // BOOST_GRAPH_DISTRIBUTED_CSR_HPP
