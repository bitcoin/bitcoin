// Copyright (C) 2007 Douglas Gregor 
// Copyright (C) 2007 Hartmut Kaiser

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// TODO:
//   - Cache (some) remote vertex names?
#ifndef BOOST_GRAPH_DISTRIBUTED_NAMED_GRAPH_HPP
#define BOOST_GRAPH_DISTRIBUTED_NAMED_GRAPH_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/assert.hpp>
#include <boost/core/uncaught_exceptions.hpp>
#include <boost/graph/named_graph.hpp>
#include <boost/functional/hash.hpp>
#include <boost/variant.hpp>
#include <boost/graph/parallel/simple_trigger.hpp>
#include <boost/graph/parallel/process_group.hpp>
#include <boost/graph/parallel/detail/property_holders.hpp>

namespace boost { namespace graph { namespace distributed {

using boost::parallel::trigger_receive_context;
using boost::detail::parallel::pair_with_property;

/*******************************************************************
 * Hashed distribution of named entities                           *
 *******************************************************************/

template<typename T>
struct hashed_distribution
{
  template<typename ProcessGroup>
  hashed_distribution(const ProcessGroup& pg, std::size_t /*num_vertices*/ = 0)
    : n(num_processes(pg)) { }

  int operator()(const T& value) const 
  {
    return hasher(value) % n;
  }

  std::size_t n;
  hash<T> hasher;
};

/// Specialization for named graphs
template <typename InDistribution, typename VertexProperty, typename VertexSize,
          typename ProcessGroup,
          typename ExtractName 
            = typename internal_vertex_name<VertexProperty>::type>
struct select_distribution
{
private:
  /// The type used to name vertices in the graph
  typedef typename remove_cv<
            typename remove_reference<
              typename ExtractName::result_type>::type>::type
    vertex_name_type;

public:
  /**
   *  The @c type field provides a distribution object that maps
   *  vertex names to processors. The distribution object will be
   *  constructed with the process group over which communication will
   *  occur. The distribution object shall also be a function
   *  object mapping from the type of the name to a processor number
   *  in @c [0, @c p) (for @c p processors). By default, the mapping
   *  function uses the @c boost::hash value of the name, modulo @c p.
   */
  typedef typename mpl::if_<is_same<InDistribution, defaultS>,
                            hashed_distribution<vertex_name_type>,
                            InDistribution>::type 
    type;

  /// for named graphs the default type is the same as the stored distribution 
  /// type
  typedef type default_type;
};

/// Specialization for non-named graphs
template <typename InDistribution, typename VertexProperty, typename VertexSize,
          typename ProcessGroup>
struct select_distribution<InDistribution, VertexProperty, VertexSize, 
                           ProcessGroup, void>
{
  /// the distribution type stored in the graph for non-named graphs should be
  /// the variant_distribution type
  typedef typename mpl::if_<is_same<InDistribution, defaultS>,
                            boost::parallel::variant_distribution<ProcessGroup, 
                                                                  VertexSize>,
                            InDistribution>::type type;

  /// default_type is used as the distribution functor for the
  /// adjacency_list, it should be parallel::block by default
  typedef typename mpl::if_<is_same<InDistribution, defaultS>,
                            boost::parallel::block, type>::type
    default_type;
};


/*******************************************************************
 * Named graph mixin                                               *
 *******************************************************************/

/**
 * named_graph is a mixin that provides names for the vertices of a
 * graph, including a mapping from names to vertices. Graph types that
 * may or may not be have vertex names (depending on the properties
 * supplied by the user) should use maybe_named_graph.
 *
 * Template parameters:
 *
 *   Graph: the graph type that derives from named_graph
 *
 *   Vertex: the type of a vertex descriptor in Graph. Note: we cannot
 *   use graph_traits here, because the Graph is not yet defined.
 *
 *   VertexProperty: the type of the property stored along with the
 *   vertex.
 *
 *   ProcessGroup: the process group over which the distributed name
 *   graph mixin will communicate.
 */
template<typename Graph, typename Vertex, typename Edge, typename Config>
class named_graph
{
public:
  /// Messages passed within the distributed named graph
  enum message_kind {
    /**
     * Requests the addition of a vertex on a remote processor. The
     * message data is a @c vertex_name_type.
     */
    msg_add_vertex_name,

    /**
     * Requests the addition of a vertex on a remote processor. The
     * message data is a @c vertex_name_type. The remote processor
     * will send back a @c msg_add_vertex_name_reply message
     * containing the vertex descriptor.
     */
    msg_add_vertex_name_with_reply,

    /**
     * Requests the vertex descriptor corresponding to the given
     * vertex name. The remote process will reply with a 
     * @c msg_find_vertex_reply message containing the answer.
     */
    msg_find_vertex,

    /**
     * Requests the addition of an edge on a remote processor. The
     * data stored in these messages is a @c pair<source, target>@,
     * where @c source and @c target may be either names (of type @c
     * vertex_name_type) or vertex descriptors, depending on what
     * information we have locally.  
     */
    msg_add_edge_name_name,
    msg_add_edge_vertex_name,
    msg_add_edge_name_vertex,

    /**
     * These messages are identical to msg_add_edge_*_*, except that
     * the process actually adding the edge will send back a @c
     * pair<edge_descriptor,bool>
     */
    msg_add_edge_name_name_with_reply,
    msg_add_edge_vertex_name_with_reply,
    msg_add_edge_name_vertex_with_reply,

    /**
     * Requests the addition of an edge with a property on a remote
     * processor. The data stored in these messages is a @c
     * pair<vertex_property_type, pair<source, target>>@, where @c
     * source and @c target may be either names (of type @c
     * vertex_name_type) or vertex descriptors, depending on what
     * information we have locally.
     */
    msg_add_edge_name_name_with_property,
    msg_add_edge_vertex_name_with_property,
    msg_add_edge_name_vertex_with_property,

    /**
     * These messages are identical to msg_add_edge_*_*_with_property,
     * except that the process actually adding the edge will send back
     * a @c pair<edge_descriptor,bool>.
     */
    msg_add_edge_name_name_with_reply_and_property,
    msg_add_edge_vertex_name_with_reply_and_property,
    msg_add_edge_name_vertex_with_reply_and_property
  };

  /// The vertex descriptor type
  typedef Vertex vertex_descriptor;
  
  /// The edge descriptor type
  typedef Edge edge_descriptor;

  /// The vertex property type
  typedef typename Config::vertex_property_type vertex_property_type;
  
  /// The vertex property type
  typedef typename Config::edge_property_type edge_property_type;
  
  /// The type used to extract names from the property structure
  typedef typename internal_vertex_name<vertex_property_type>::type
    extract_name_type;

  /// The type used to name vertices in the graph
  typedef typename remove_cv<
            typename remove_reference<
              typename extract_name_type::result_type>::type>::type
    vertex_name_type;

  /// The type used to distribute named vertices in the graph
  typedef typename Config::distribution_type distribution_type;
  typedef typename Config::base_distribution_type base_distribution_type;

  /// The type used for communication in the distributed structure
  typedef typename Config::process_group_type process_group_type;

  /// Type used to identify processes
  typedef typename process_group_type::process_id_type process_id_type;

  /// a reference to this class, which is used for disambiguation of the 
  //  add_vertex function
  typedef named_graph named_graph_type;
  
  /// Structure returned when adding a vertex by vertex name
  struct lazy_add_vertex;
  friend struct lazy_add_vertex;

  /// Structure returned when adding an edge by vertex name
  struct lazy_add_edge;
  friend struct lazy_add_edge;

  /// Structure returned when adding an edge by vertex name with a property
  struct lazy_add_edge_with_property;
  friend struct lazy_add_edge_with_property;

  explicit named_graph(const process_group_type& pg);

  named_graph(const process_group_type& pg, const base_distribution_type& distribution);

  /// Set up triggers, but only for the BSP process group
  void setup_triggers();

  /// Retrieve the derived instance
  Graph&       derived()       { return static_cast<Graph&>(*this); }
  const Graph& derived() const { return static_cast<const Graph&>(*this); }

  /// Retrieve the process group
  process_group_type&       process_group()       { return process_group_; }
  const process_group_type& process_group() const { return process_group_; }

  // Retrieve the named distribution
  distribution_type&       named_distribution()       { return distribution_; }
  const distribution_type& named_distribution() const { return distribution_; }

  /// Notify the named_graph that we have added the given vertex. This
  /// is a no-op.
  void added_vertex(Vertex) { }

  /// Notify the named_graph that we are removing the given
  /// vertex. This is a no-op.
  template <typename VertexIterStability>
  void removing_vertex(Vertex, VertexIterStability) { }

  /// Notify the named_graph that we are clearing the graph
  void clearing_graph() { }

  /// Retrieve the owner of a given vertex based on the properties
  /// associated with that vertex. This operation just returns the
  /// number of the local processor, adding all vertices locally.
  process_id_type owner_by_property(const vertex_property_type&);

protected:
  void 
  handle_add_vertex_name(int source, int tag, const vertex_name_type& msg,
                         trigger_receive_context);

  vertex_descriptor 
  handle_add_vertex_name_with_reply(int source, int tag, 
                                    const vertex_name_type& msg,
                                    trigger_receive_context);

  boost::parallel::detail::untracked_pair<vertex_descriptor, bool>
  handle_find_vertex(int source, int tag, const vertex_name_type& msg,
                     trigger_receive_context);

  template<typename U, typename V>
  void handle_add_edge(int source, int tag, const boost::parallel::detail::untracked_pair<U, V>& msg,
                       trigger_receive_context);

  template<typename U, typename V>
  boost::parallel::detail::untracked_pair<edge_descriptor, bool> 
  handle_add_edge_with_reply(int source, int tag, const boost::parallel::detail::untracked_pair<U, V>& msg,
                             trigger_receive_context);

  template<typename U, typename V>
  void 
  handle_add_edge_with_property
    (int source, int tag, 
     const pair_with_property<U, V, edge_property_type>& msg,
     trigger_receive_context);

  template<typename U, typename V>
  boost::parallel::detail::untracked_pair<edge_descriptor, bool> 
  handle_add_edge_with_reply_and_property
    (int source, int tag, 
     const pair_with_property<U, V, edge_property_type>& msg,
     trigger_receive_context);

  /// The process group for this distributed data structure
  process_group_type process_group_;

  /// The distribution we will use to map names to processors
  distribution_type distribution_;
};

/// Helper macro containing the template parameters of named_graph
#define BGL_NAMED_GRAPH_PARAMS \
  typename Graph, typename Vertex, typename Edge, typename Config
/// Helper macro containing the named_graph<...> instantiation
#define BGL_NAMED_GRAPH \
  named_graph<Graph, Vertex, Edge, Config>

/**
 * Data structure returned from add_vertex that will "lazily" add the
 * vertex, either when it is converted to a @c vertex_descriptor or
 * when the most recent copy has been destroyed.
 */
template<BGL_NAMED_GRAPH_PARAMS>
struct BGL_NAMED_GRAPH::lazy_add_vertex
{
  /// Construct a new lazyily-added vertex
  lazy_add_vertex(named_graph& self, const vertex_name_type& name)
    : self(self), name(name), committed(false) { }

  /// Transfer responsibility for adding the vertex from the source of
  /// the copy to the newly-constructed opbject.
  lazy_add_vertex(const lazy_add_vertex& other)
    : self(other.self), name(other.name), committed(other.committed)
  {
    other.committed = true;
  }

  /// If the vertex has not been added yet, add it
  ~lazy_add_vertex();

  /// Add the vertex and return its descriptor. This conversion can
  /// only occur once, and only when this object is responsible for
  /// creating the vertex.
  operator vertex_descriptor() const { return commit(); }

  /// Add the vertex and return its descriptor. This can only be
  /// called once, and only when this object is responsible for
  /// creating the vertex.
  vertex_descriptor commit() const;

protected:
  named_graph&     self;
  vertex_name_type name;
  mutable bool     committed;
};

template<BGL_NAMED_GRAPH_PARAMS>
BGL_NAMED_GRAPH::lazy_add_vertex::~lazy_add_vertex()
{
  typedef typename BGL_NAMED_GRAPH::process_id_type process_id_type;

  /// If this vertex has already been created or will be created by
  /// someone else, or if someone threw an exception, we will not
  /// create the vertex now.
  if (committed || boost::core::uncaught_exceptions() > 0)
    return;

  committed = true;

  process_id_type owner = self.named_distribution()(name);
  if (owner == process_id(self.process_group()))
    /// Add the vertex locally
    add_vertex(self.derived().base().vertex_constructor(name), self.derived()); 
  else
    /// Ask the owner of the vertex to add a vertex with this name
    send(self.process_group(), owner, msg_add_vertex_name, name);
}

template<BGL_NAMED_GRAPH_PARAMS>
typename BGL_NAMED_GRAPH::vertex_descriptor
BGL_NAMED_GRAPH::lazy_add_vertex::commit() const
{
  typedef typename BGL_NAMED_GRAPH::process_id_type process_id_type;
  BOOST_ASSERT (!committed);
  committed = true;

  process_id_type owner = self.named_distribution()(name);
  if (owner == process_id(self.process_group()))
    /// Add the vertex locally
    return add_vertex(self.derived().base().vertex_constructor(name),
                      self.derived()); 
  else {
    /// Ask the owner of the vertex to add a vertex with this name
    vertex_descriptor result;
    send_oob_with_reply(self.process_group(), owner, 
                        msg_add_vertex_name_with_reply, name, result);
    return result;
  }
}

/**
 * Data structure returned from add_edge that will "lazily" add the
 * edge, either when it is converted to a @c
 * pair<edge_descriptor,bool> or when the most recent copy has been
 * destroyed.
 */
template<BGL_NAMED_GRAPH_PARAMS>
struct BGL_NAMED_GRAPH::lazy_add_edge 
{
  /// The graph's edge descriptor
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;

  /// Add an edge for the edge (u, v) based on vertex names
  lazy_add_edge(BGL_NAMED_GRAPH& self, 
                const vertex_name_type& u_name,
                const vertex_name_type& v_name) 
    : self(self), u(u_name), v(v_name), committed(false) { }

  /// Add an edge for the edge (u, v) based on a vertex descriptor and name
  lazy_add_edge(BGL_NAMED_GRAPH& self,
                vertex_descriptor u,
                const vertex_name_type& v_name)
    : self(self), u(u), v(v_name), committed(false) { }

  /// Add an edge for the edge (u, v) based on a vertex name and descriptor
  lazy_add_edge(BGL_NAMED_GRAPH& self,
                const vertex_name_type& u_name,
                vertex_descriptor v)
    : self(self), u(u_name), v(v), committed(false) { }

  /// Add an edge for the edge (u, v) based on vertex descriptors
  lazy_add_edge(BGL_NAMED_GRAPH& self,
                vertex_descriptor u,
                vertex_descriptor v)
    : self(self), u(u), v(v), committed(false) { }

  /// Copy a lazy_add_edge structure, which transfers responsibility
  /// for adding the edge to the newly-constructed object.
  lazy_add_edge(const lazy_add_edge& other)
    : self(other.self), u(other.u), v(other.v), committed(other.committed)
  {
    other.committed = true;
  }

  /// If the edge has not yet been added, add the edge but don't wait
  /// for a reply.
  ~lazy_add_edge();

  /// Returns commit().
  operator std::pair<edge_descriptor, bool>() const { return commit(); }

  // Add the edge. This operation will block if a remote edge is
  // being added.
  std::pair<edge_descriptor, bool> commit() const;

protected:
  BGL_NAMED_GRAPH& self;
  mutable variant<vertex_descriptor, vertex_name_type> u;
  mutable variant<vertex_descriptor, vertex_name_type> v;
  mutable bool committed;

private:
  // No copy-assignment semantics
  void operator=(lazy_add_edge&);
};

template<BGL_NAMED_GRAPH_PARAMS>
BGL_NAMED_GRAPH::lazy_add_edge::~lazy_add_edge()
{
  using boost::parallel::detail::make_untracked_pair;

  /// If this edge has already been created or will be created by
  /// someone else, or if someone threw an exception, we will not
  /// create the edge now.
  if (committed || boost::core::uncaught_exceptions() > 0)
    return;

  committed = true;

  if (vertex_name_type* v_name = boost::get<vertex_name_type>(&v)) {
    // We haven't resolved the target vertex to a descriptor yet, so
    // it must not be local. Send a message to the owner of the target
    // of the edge. If the owner of the target does not happen to own
    // the source, it will resolve the target to a vertex descriptor
    // and pass the message along to the owner of the source. 
    if (vertex_name_type* u_name = boost::get<vertex_name_type>(&u))
      send(self.process_group(), self.distribution_(*v_name),
           BGL_NAMED_GRAPH::msg_add_edge_name_name,
           make_untracked_pair(*u_name, *v_name));
    else
      send(self.process_group(), self.distribution_(*v_name),
           BGL_NAMED_GRAPH::msg_add_edge_vertex_name,
           make_untracked_pair(boost::get<vertex_descriptor>(u), *v_name));
  } else {
    if (vertex_name_type* u_name = boost::get<vertex_name_type>(&u))
      // We haven't resolved the source vertex to a descriptor yet, so
      // it must not be local. Send a message to the owner of the
      // source vertex requesting the edge addition.
      send(self.process_group(), self.distribution_(*u_name),
           BGL_NAMED_GRAPH::msg_add_edge_name_vertex,
           make_untracked_pair(*u_name, boost::get<vertex_descriptor>(v)));
    else
      // We have descriptors for both of the vertices, either of which
      // may be remote or local. Tell the owner of the source vertex
      // to add the edge (it may be us!).
      add_edge(boost::get<vertex_descriptor>(u), 
               boost::get<vertex_descriptor>(v), 
               self.derived());
  }
}

template<BGL_NAMED_GRAPH_PARAMS>
std::pair<typename graph_traits<Graph>::edge_descriptor, bool>
BGL_NAMED_GRAPH::lazy_add_edge::commit() const
{
  typedef typename BGL_NAMED_GRAPH::process_id_type process_id_type;
  using boost::parallel::detail::make_untracked_pair;

  BOOST_ASSERT(!committed);
  committed = true;

  /// The result we will return, if we are sending a message to
  /// request that someone else add the edge.
  boost::parallel::detail::untracked_pair<edge_descriptor, bool> result;

  /// The owner of the vertex "u"
  process_id_type u_owner;

  process_id_type rank = process_id(self.process_group());
  if (const vertex_name_type* u_name = boost::get<vertex_name_type>(&u)) {
    /// We haven't resolved the source vertex to a descriptor yet, so
    /// it must not be local. 
    u_owner = self.named_distribution()(*u_name);

    /// Send a message to the remote vertex requesting that it add the
    /// edge. The message differs depending on whether we have a
    /// vertex name or a vertex descriptor for the target.
    if (const vertex_name_type* v_name = boost::get<vertex_name_type>(&v))
      send_oob_with_reply(self.process_group(), u_owner,
                          BGL_NAMED_GRAPH::msg_add_edge_name_name_with_reply,
                          make_untracked_pair(*u_name, *v_name), result);
    else
      send_oob_with_reply(self.process_group(), u_owner,
                          BGL_NAMED_GRAPH::msg_add_edge_name_vertex_with_reply,
                          make_untracked_pair(*u_name, 
                                         boost::get<vertex_descriptor>(v)),
                          result);
  } else {
    /// We have resolved the source vertex to a descriptor, which may
    /// either be local or remote.
    u_owner 
      = get(vertex_owner, self.derived(),
            boost::get<vertex_descriptor>(u));
    if (u_owner == rank) {
      /// The source is local. If we need to, resolve the target vertex.
      if (const vertex_name_type* v_name = boost::get<vertex_name_type>(&v))
        v = add_vertex(*v_name, self.derived());

      /// Add the edge using vertex descriptors
      return add_edge(boost::get<vertex_descriptor>(u), 
                      boost::get<vertex_descriptor>(v), 
                      self.derived());
    } else {
      /// The source is remote. Just send a message to its owner
      /// requesting that the owner add the new edge, either directly
      /// or via the derived class's add_edge function.
      if (const vertex_name_type* v_name = boost::get<vertex_name_type>(&v))
        send_oob_with_reply
          (self.process_group(), u_owner,
           BGL_NAMED_GRAPH::msg_add_edge_vertex_name_with_reply,
           make_untracked_pair(boost::get<vertex_descriptor>(u), *v_name),
           result);
      else
        return add_edge(boost::get<vertex_descriptor>(u), 
                        boost::get<vertex_descriptor>(v), 
                        self.derived());
    }
  }

  // If we get here, the edge has been added remotely and "result"
  // contains the result of that edge addition.
  return result;
}

/**
 * Data structure returned from add_edge that will "lazily" add the
 * edge with a property, either when it is converted to a @c
 * pair<edge_descriptor,bool> or when the most recent copy has been
 * destroyed.
 */
template<BGL_NAMED_GRAPH_PARAMS>
struct BGL_NAMED_GRAPH::lazy_add_edge_with_property 
{
  /// The graph's edge descriptor
  typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;

  /// The Edge property type for our graph
  typedef typename Config::edge_property_type edge_property_type;
  
  /// Add an edge for the edge (u, v) based on vertex names
  lazy_add_edge_with_property(BGL_NAMED_GRAPH& self, 
                              const vertex_name_type& u_name,
                              const vertex_name_type& v_name,
                              const edge_property_type& property) 
    : self(self), u(u_name), v(v_name), property(property), committed(false)
  { 
  }

  /// Add an edge for the edge (u, v) based on a vertex descriptor and name
  lazy_add_edge_with_property(BGL_NAMED_GRAPH& self,
                vertex_descriptor u,
                const vertex_name_type& v_name,
                                  const edge_property_type& property)
    : self(self), u(u), v(v_name), property(property), committed(false) { }

  /// Add an edge for the edge (u, v) based on a vertex name and descriptor
  lazy_add_edge_with_property(BGL_NAMED_GRAPH& self,
                              const vertex_name_type& u_name,
                              vertex_descriptor v,
                              const edge_property_type& property)
    : self(self), u(u_name), v(v), property(property), committed(false) { }

  /// Add an edge for the edge (u, v) based on vertex descriptors
  lazy_add_edge_with_property(BGL_NAMED_GRAPH& self,
                              vertex_descriptor u,
                              vertex_descriptor v,
                              const edge_property_type& property)
    : self(self), u(u), v(v), property(property), committed(false) { }

  /// Copy a lazy_add_edge_with_property structure, which transfers
  /// responsibility for adding the edge to the newly-constructed
  /// object.
  lazy_add_edge_with_property(const lazy_add_edge_with_property& other)
    : self(other.self), u(other.u), v(other.v), property(other.property), 
      committed(other.committed)
  {
    other.committed = true;
  }

  /// If the edge has not yet been added, add the edge but don't wait
  /// for a reply.
  ~lazy_add_edge_with_property();

  /// Returns commit().
  operator std::pair<edge_descriptor, bool>() const { return commit(); }

  // Add the edge. This operation will block if a remote edge is
  // being added.
  std::pair<edge_descriptor, bool> commit() const;

protected:
  BGL_NAMED_GRAPH& self;
  mutable variant<vertex_descriptor, vertex_name_type> u;
  mutable variant<vertex_descriptor, vertex_name_type> v;
  edge_property_type property;
  mutable bool committed;

private:
  // No copy-assignment semantics
  void operator=(lazy_add_edge_with_property&);
};

template<BGL_NAMED_GRAPH_PARAMS>
BGL_NAMED_GRAPH::lazy_add_edge_with_property::~lazy_add_edge_with_property()
{
  using boost::detail::parallel::make_pair_with_property;

  /// If this edge has already been created or will be created by
  /// someone else, or if someone threw an exception, we will not
  /// create the edge now.
  if (committed || boost::core::uncaught_exceptions() > 0)
    return;

  committed = true;

  if (vertex_name_type* v_name = boost::get<vertex_name_type>(&v)) {
    // We haven't resolved the target vertex to a descriptor yet, so
    // it must not be local. Send a message to the owner of the target
    // of the edge. If the owner of the target does not happen to own
    // the source, it will resolve the target to a vertex descriptor
    // and pass the message along to the owner of the source. 
    if (vertex_name_type* u_name = boost::get<vertex_name_type>(&u))
      send(self.process_group(), self.distribution_(*v_name),
           BGL_NAMED_GRAPH::msg_add_edge_name_name_with_property,
           make_pair_with_property(*u_name, *v_name, property));
    else
      send(self.process_group(), self.distribution_(*v_name),
           BGL_NAMED_GRAPH::msg_add_edge_vertex_name_with_property,
           make_pair_with_property(boost::get<vertex_descriptor>(u), *v_name, 
                                   property));
  } else {
    if (vertex_name_type* u_name = boost::get<vertex_name_type>(&u))
      // We haven't resolved the source vertex to a descriptor yet, so
      // it must not be local. Send a message to the owner of the
      // source vertex requesting the edge addition.
      send(self.process_group(), self.distribution_(*u_name),
           BGL_NAMED_GRAPH::msg_add_edge_name_vertex_with_property,
           make_pair_with_property(*u_name, boost::get<vertex_descriptor>(v), 
                                   property));
    else
      // We have descriptors for both of the vertices, either of which
      // may be remote or local. Tell the owner of the source vertex
      // to add the edge (it may be us!).
      add_edge(boost::get<vertex_descriptor>(u), 
               boost::get<vertex_descriptor>(v), 
               property,
               self.derived());
  }
}

template<BGL_NAMED_GRAPH_PARAMS>
std::pair<typename graph_traits<Graph>::edge_descriptor, bool>
BGL_NAMED_GRAPH::lazy_add_edge_with_property::commit() const
{
  using boost::detail::parallel::make_pair_with_property;
  typedef typename BGL_NAMED_GRAPH::process_id_type process_id_type;
  BOOST_ASSERT(!committed);
  committed = true;

  /// The result we will return, if we are sending a message to
  /// request that someone else add the edge.
  boost::parallel::detail::untracked_pair<edge_descriptor, bool> result;

  /// The owner of the vertex "u"
  process_id_type u_owner;

  process_id_type rank = process_id(self.process_group());
  if (const vertex_name_type* u_name = boost::get<vertex_name_type>(&u)) {
    /// We haven't resolved the source vertex to a descriptor yet, so
    /// it must not be local. 
    u_owner = self.named_distribution()(*u_name);

    /// Send a message to the remote vertex requesting that it add the
    /// edge. The message differs depending on whether we have a
    /// vertex name or a vertex descriptor for the target.
    if (const vertex_name_type* v_name = boost::get<vertex_name_type>(&v))
      send_oob_with_reply
        (self.process_group(), u_owner,
         BGL_NAMED_GRAPH::msg_add_edge_name_name_with_reply_and_property,
         make_pair_with_property(*u_name, *v_name, property),
         result);
    else
      send_oob_with_reply
        (self.process_group(), u_owner,
         BGL_NAMED_GRAPH::msg_add_edge_name_vertex_with_reply_and_property,
         make_pair_with_property(*u_name, 
                                 boost::get<vertex_descriptor>(v),
                                 property),
         result);
  } else {
    /// We have resolved the source vertex to a descriptor, which may
    /// either be local or remote.
    u_owner 
      = get(vertex_owner, self.derived(),
            boost::get<vertex_descriptor>(u));
    if (u_owner == rank) {
      /// The source is local. If we need to, resolve the target vertex.
      if (const vertex_name_type* v_name = boost::get<vertex_name_type>(&v))
        v = add_vertex(*v_name, self.derived());

      /// Add the edge using vertex descriptors
      return add_edge(boost::get<vertex_descriptor>(u), 
                      boost::get<vertex_descriptor>(v), 
                      property,
                      self.derived());
    } else {
      /// The source is remote. Just send a message to its owner
      /// requesting that the owner add the new edge, either directly
      /// or via the derived class's add_edge function.
      if (const vertex_name_type* v_name = boost::get<vertex_name_type>(&v))
        send_oob_with_reply
          (self.process_group(), u_owner,
           BGL_NAMED_GRAPH::msg_add_edge_vertex_name_with_reply_and_property,
           make_pair_with_property(boost::get<vertex_descriptor>(u), *v_name,
                                   property),
           result);
      else
        return add_edge(boost::get<vertex_descriptor>(u), 
                        boost::get<vertex_descriptor>(v), 
                        property,
                        self.derived());
    }
  }

  // If we get here, the edge has been added remotely and "result"
  // contains the result of that edge addition.
  return result;
}

/// Construct the named_graph with a particular process group
template<BGL_NAMED_GRAPH_PARAMS>
BGL_NAMED_GRAPH::named_graph(const process_group_type& pg)
  : process_group_(pg, boost::parallel::attach_distributed_object()),
    distribution_(pg)
{
  setup_triggers();
}

/// Construct the named_graph mixin with a particular process group
/// and distribution function
template<BGL_NAMED_GRAPH_PARAMS>
BGL_NAMED_GRAPH::named_graph(const process_group_type& pg,
                             const base_distribution_type& distribution)
  : process_group_(pg, boost::parallel::attach_distributed_object()),
    distribution_(pg, distribution)
{
  setup_triggers();
}

template<BGL_NAMED_GRAPH_PARAMS>
void
BGL_NAMED_GRAPH::setup_triggers()
{
  using boost::graph::parallel::simple_trigger;

  simple_trigger(process_group_, msg_add_vertex_name, this,
                 &named_graph::handle_add_vertex_name);
  simple_trigger(process_group_, msg_add_vertex_name_with_reply, this,
                 &named_graph::handle_add_vertex_name_with_reply);
  simple_trigger(process_group_, msg_find_vertex, this, 
                 &named_graph::handle_find_vertex);
  simple_trigger(process_group_, msg_add_edge_name_name, this, 
                 &named_graph::template handle_add_edge<vertex_name_type, 
                                                        vertex_name_type>);
  simple_trigger(process_group_, msg_add_edge_name_name_with_reply, this,
                 &named_graph::template handle_add_edge_with_reply
                   <vertex_name_type, vertex_name_type>);
  simple_trigger(process_group_, msg_add_edge_name_vertex, this,
                 &named_graph::template handle_add_edge<vertex_name_type, 
                                                        vertex_descriptor>);
  simple_trigger(process_group_, msg_add_edge_name_vertex_with_reply, this,
                 &named_graph::template handle_add_edge_with_reply
                   <vertex_name_type, vertex_descriptor>);
  simple_trigger(process_group_, msg_add_edge_vertex_name, this,
                 &named_graph::template handle_add_edge<vertex_descriptor, 
                                                        vertex_name_type>);
  simple_trigger(process_group_, msg_add_edge_vertex_name_with_reply, this,
                 &named_graph::template handle_add_edge_with_reply
                   <vertex_descriptor, vertex_name_type>);
  simple_trigger(process_group_, msg_add_edge_name_name_with_property, this, 
                 &named_graph::
                   template handle_add_edge_with_property<vertex_name_type, 
                                                          vertex_name_type>);
  simple_trigger(process_group_, 
                 msg_add_edge_name_name_with_reply_and_property, this,
                 &named_graph::template handle_add_edge_with_reply_and_property
                   <vertex_name_type, vertex_name_type>);
  simple_trigger(process_group_, msg_add_edge_name_vertex_with_property, this,
                 &named_graph::
                   template handle_add_edge_with_property<vertex_name_type, 
                                                          vertex_descriptor>);
  simple_trigger(process_group_, 
                 msg_add_edge_name_vertex_with_reply_and_property, this,
                 &named_graph::template handle_add_edge_with_reply_and_property
                   <vertex_name_type, vertex_descriptor>);
  simple_trigger(process_group_, msg_add_edge_vertex_name_with_property, this,
                 &named_graph::
                   template handle_add_edge_with_property<vertex_descriptor, 
                                                          vertex_name_type>);
  simple_trigger(process_group_, 
                 msg_add_edge_vertex_name_with_reply_and_property, this,
                 &named_graph::template handle_add_edge_with_reply_and_property
                   <vertex_descriptor, vertex_name_type>);
}

/// Retrieve the vertex associated with the given name
template<BGL_NAMED_GRAPH_PARAMS>
optional<Vertex> 
find_vertex(typename BGL_NAMED_GRAPH::vertex_name_type const& name,
            const BGL_NAMED_GRAPH& g)
{
  typedef typename Graph::local_vertex_descriptor local_vertex_descriptor;
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

  // Determine the owner of this name
  typename BGL_NAMED_GRAPH::process_id_type owner 
    = g.named_distribution()(name);

  if (owner == process_id(g.process_group())) {
    // The vertex is local, so search for a mapping here
    optional<local_vertex_descriptor> result 
      = find_vertex(name, g.derived().base());
    if (result)
      return Vertex(owner, *result);
    else
      return optional<Vertex>();
  }
  else {
    // Ask the ownering process for the name of this vertex
    boost::parallel::detail::untracked_pair<vertex_descriptor, bool> result;
    send_oob_with_reply(g.process_group(), owner, 
                        BGL_NAMED_GRAPH::msg_find_vertex, name, result);
    if (result.second)
      return result.first;
    else
      return optional<Vertex>();
  }
}

/// meta-function helping in figuring out if the given VertextProerty belongs to 
/// a named graph
template<typename VertexProperty>
struct not_is_named_graph
  : is_same<typename internal_vertex_name<VertexProperty>::type, void>
{};

/// Retrieve the vertex associated with the given name
template<typename Graph>
typename Graph::named_graph_type::lazy_add_vertex
add_vertex(typename Graph::vertex_name_type const& name,
           Graph& g, 
           typename disable_if<
              not_is_named_graph<typename Graph::vertex_property_type>, 
              void*>::type = 0)
{
  return typename Graph::named_graph_type::lazy_add_vertex(g, name);
}

/// Add an edge using vertex names to refer to the vertices
template<BGL_NAMED_GRAPH_PARAMS>
typename BGL_NAMED_GRAPH::lazy_add_edge
add_edge(typename BGL_NAMED_GRAPH::vertex_name_type const& u_name,
         typename BGL_NAMED_GRAPH::vertex_name_type const& v_name,
         BGL_NAMED_GRAPH& g)
{
  typedef typename BGL_NAMED_GRAPH::lazy_add_edge lazy_add_edge;
  typedef typename BGL_NAMED_GRAPH::process_id_type process_id_type;

  process_id_type rank = process_id(g.process_group());
  process_id_type u_owner = g.named_distribution()(u_name);
  process_id_type v_owner = g.named_distribution()(v_name);

  // Resolve local vertex names before building the "lazy" edge
  // addition structure.
  if (u_owner == rank && v_owner == rank)
    return lazy_add_edge(g, add_vertex(u_name, g), add_vertex(v_name, g));
  else if (u_owner == rank && v_owner != rank)
    return lazy_add_edge(g, add_vertex(u_name, g), v_name);
  else if (u_owner != rank && v_owner == rank)
    return lazy_add_edge(g, u_name, add_vertex(v_name, g));
  else
    return lazy_add_edge(g, u_name, v_name);
}

template<BGL_NAMED_GRAPH_PARAMS>
typename BGL_NAMED_GRAPH::lazy_add_edge
add_edge(typename BGL_NAMED_GRAPH::vertex_name_type const& u_name,
         typename BGL_NAMED_GRAPH::vertex_descriptor const& v,
         BGL_NAMED_GRAPH& g)
{
  // Resolve local vertex names before building the "lazy" edge
  // addition structure.
  typedef typename BGL_NAMED_GRAPH::lazy_add_edge lazy_add_edge;
  if (g.named_distribution()(u_name) == process_id(g.process_group()))
    return lazy_add_edge(g, add_vertex(u_name, g), v);
  else
    return lazy_add_edge(g, u_name, v);
}

template<BGL_NAMED_GRAPH_PARAMS>
typename BGL_NAMED_GRAPH::lazy_add_edge
add_edge(typename BGL_NAMED_GRAPH::vertex_descriptor const& u,
         typename BGL_NAMED_GRAPH::vertex_name_type const& v_name,
         BGL_NAMED_GRAPH& g)
{
  // Resolve local vertex names before building the "lazy" edge
  // addition structure.
  typedef typename BGL_NAMED_GRAPH::lazy_add_edge lazy_add_edge;
  if (g.named_distribution()(v_name) == process_id(g.process_group()))
    return lazy_add_edge(g, u, add_vertex(v_name, g));
  else
    return lazy_add_edge(g, u, v_name);
}

/// Add an edge using vertex names to refer to the vertices
template<BGL_NAMED_GRAPH_PARAMS>
typename BGL_NAMED_GRAPH::lazy_add_edge_with_property
add_edge(typename BGL_NAMED_GRAPH::vertex_name_type const& u_name,
         typename BGL_NAMED_GRAPH::vertex_name_type const& v_name,
         typename Graph::edge_property_type const& property,
         BGL_NAMED_GRAPH& g)
{
  typedef typename BGL_NAMED_GRAPH::lazy_add_edge_with_property lazy_add_edge;
  typedef typename BGL_NAMED_GRAPH::process_id_type process_id_type;

  process_id_type rank = process_id(g.process_group());
  process_id_type u_owner = g.named_distribution()(u_name);
  process_id_type v_owner = g.named_distribution()(v_name);

  // Resolve local vertex names before building the "lazy" edge
  // addition structure.
  if (u_owner == rank && v_owner == rank)
    return lazy_add_edge(g, add_vertex(u_name, g), add_vertex(v_name, g), 
                         property);
  else if (u_owner == rank && v_owner != rank)
    return lazy_add_edge(g, add_vertex(u_name, g), v_name, property);
  else if (u_owner != rank && v_owner == rank)
    return lazy_add_edge(g, u_name, add_vertex(v_name, g), property);
  else
    return lazy_add_edge(g, u_name, v_name, property);
}

template<BGL_NAMED_GRAPH_PARAMS>
typename BGL_NAMED_GRAPH::lazy_add_edge_with_property
add_edge(typename BGL_NAMED_GRAPH::vertex_name_type const& u_name,
         typename BGL_NAMED_GRAPH::vertex_descriptor const& v,
         typename Graph::edge_property_type const& property,
         BGL_NAMED_GRAPH& g)
{
  // Resolve local vertex names before building the "lazy" edge
  // addition structure.
  typedef typename BGL_NAMED_GRAPH::lazy_add_edge_with_property lazy_add_edge;
  if (g.named_distribution()(u_name) == process_id(g.process_group()))
    return lazy_add_edge(g, add_vertex(u_name, g), v, property);
  else
    return lazy_add_edge(g, u_name, v, property);
}

template<BGL_NAMED_GRAPH_PARAMS>
typename BGL_NAMED_GRAPH::lazy_add_edge_with_property
add_edge(typename BGL_NAMED_GRAPH::vertex_descriptor const& u,
         typename BGL_NAMED_GRAPH::vertex_name_type const& v_name,
         typename Graph::edge_property_type const& property,
         BGL_NAMED_GRAPH& g)
{
  // Resolve local vertex names before building the "lazy" edge
  // addition structure.
  typedef typename BGL_NAMED_GRAPH::lazy_add_edge_with_property lazy_add_edge;
  if (g.named_distribution()(v_name) == process_id(g.process_group()))
    return lazy_add_edge(g, u, add_vertex(v_name, g), property);
  else
    return lazy_add_edge(g, u, v_name, property);
}

template<BGL_NAMED_GRAPH_PARAMS>
typename BGL_NAMED_GRAPH::process_id_type 
BGL_NAMED_GRAPH::owner_by_property(const vertex_property_type& property)
{
  return distribution_(derived().base().extract_name(property));
}


/*******************************************************************
 * Message handlers                                                *
 *******************************************************************/

template<BGL_NAMED_GRAPH_PARAMS>
void 
BGL_NAMED_GRAPH::
handle_add_vertex_name(int /*source*/, int /*tag*/, 
                       const vertex_name_type& msg, trigger_receive_context)
{
  add_vertex(msg, derived());
}

template<BGL_NAMED_GRAPH_PARAMS>
typename BGL_NAMED_GRAPH::vertex_descriptor
BGL_NAMED_GRAPH::
handle_add_vertex_name_with_reply(int source, int /*tag*/, 
                                  const vertex_name_type& msg, 
                                  trigger_receive_context)
{
  return add_vertex(msg, derived());
}

template<BGL_NAMED_GRAPH_PARAMS>
boost::parallel::detail::untracked_pair<typename BGL_NAMED_GRAPH::vertex_descriptor, bool>
BGL_NAMED_GRAPH::
handle_find_vertex(int source, int /*tag*/, const vertex_name_type& msg, 
                   trigger_receive_context)
{
  using boost::parallel::detail::make_untracked_pair;

  optional<vertex_descriptor> v = find_vertex(msg, derived());
  if (v)
    return make_untracked_pair(*v, true);
  else
    return make_untracked_pair(graph_traits<Graph>::null_vertex(), false);
}

template<BGL_NAMED_GRAPH_PARAMS>
template<typename U, typename V>
void
BGL_NAMED_GRAPH::
handle_add_edge(int source, int /*tag*/, const boost::parallel::detail::untracked_pair<U, V>& msg, 
                trigger_receive_context)
{
  add_edge(msg.first, msg.second, derived());
}

template<BGL_NAMED_GRAPH_PARAMS>
template<typename U, typename V>
boost::parallel::detail::untracked_pair<typename BGL_NAMED_GRAPH::edge_descriptor, bool>
BGL_NAMED_GRAPH::
handle_add_edge_with_reply(int source, int /*tag*/, const boost::parallel::detail::untracked_pair<U, V>& msg,
                           trigger_receive_context)
{
  std::pair<typename BGL_NAMED_GRAPH::edge_descriptor, bool> p =
    add_edge(msg.first, msg.second, derived());
   return p;
}

template<BGL_NAMED_GRAPH_PARAMS>
template<typename U, typename V>
void 
BGL_NAMED_GRAPH::
handle_add_edge_with_property
  (int source, int tag, 
   const pair_with_property<U, V, edge_property_type>& msg,
   trigger_receive_context)
{
  add_edge(msg.first, msg.second, msg.get_property(), derived());
}

template<BGL_NAMED_GRAPH_PARAMS>
template<typename U, typename V>
boost::parallel::detail::untracked_pair<typename BGL_NAMED_GRAPH::edge_descriptor, bool>
BGL_NAMED_GRAPH::
handle_add_edge_with_reply_and_property
  (int source, int tag, 
   const pair_with_property<U, V, edge_property_type>& msg,
   trigger_receive_context)
{
  std:: pair<typename BGL_NAMED_GRAPH::edge_descriptor, bool> p =
    add_edge(msg.first, msg.second, msg.get_property(), derived());
  return p;
}

#undef BGL_NAMED_GRAPH
#undef BGL_NAMED_GRAPH_PARAMS

/*******************************************************************
 * Maybe named graph mixin                                         *
 *******************************************************************/

/**
 * A graph mixin that can provide a mapping from names to vertices,
 * and use that mapping to simplify creation and manipulation of
 * graphs. 
 */
template<typename Graph, typename Vertex, typename Edge, typename Config, 
  typename ExtractName 
    = typename internal_vertex_name<typename Config::vertex_property_type>::type>
struct maybe_named_graph 
  : public named_graph<Graph, Vertex, Edge, Config> 
{
private:
  typedef named_graph<Graph, Vertex, Edge, Config> inherited;
  typedef typename Config::process_group_type process_group_type;
  
public:
  /// The type used to distribute named vertices in the graph
  typedef typename Config::distribution_type distribution_type;
  typedef typename Config::base_distribution_type base_distribution_type;
  
  explicit maybe_named_graph(const process_group_type& pg) : inherited(pg) { }

  maybe_named_graph(const process_group_type& pg, 
                    const base_distribution_type& distribution)
    : inherited(pg, distribution) { }  

  distribution_type&       distribution()       { return this->distribution_; }
  const distribution_type& distribution() const { return this->distribution_; }
};

/**
 * A graph mixin that can provide a mapping from names to vertices,
 * and use that mapping to simplify creation and manipulation of
 * graphs. This partial specialization turns off this functionality
 * when the @c VertexProperty does not have an internal vertex name.
 */
template<typename Graph, typename Vertex, typename Edge, typename Config>
struct maybe_named_graph<Graph, Vertex, Edge, Config, void> 
{ 
private:
  typedef typename Config::process_group_type process_group_type;
  typedef typename Config::vertex_property_type vertex_property_type;
  
public:
  typedef typename process_group_type::process_id_type process_id_type;

  /// The type used to distribute named vertices in the graph
  typedef typename Config::distribution_type distribution_type;
  typedef typename Config::base_distribution_type base_distribution_type;

  explicit maybe_named_graph(const process_group_type&)  { }

  maybe_named_graph(const process_group_type& pg, 
                    const base_distribution_type& distribution) 
    : distribution_(pg, distribution) { }

  /// Notify the named_graph that we have added the given vertex. This
  /// is a no-op.
  void added_vertex(Vertex) { }

  /// Notify the named_graph that we are removing the given
  /// vertex. This is a no-op.
  template <typename VertexIterStability>
  void removing_vertex(Vertex, VertexIterStability) { }

  /// Notify the named_graph that we are clearing the graph
  void clearing_graph() { }

  /// Retrieve the owner of a given vertex based on the properties
  /// associated with that vertex. This operation just returns the
  /// number of the local processor, adding all vertices locally.
  process_id_type owner_by_property(const vertex_property_type&)
  {
    return process_id(pg);
  }

  distribution_type&       distribution()       { return distribution_; }
  const distribution_type& distribution() const { return distribution_; }

protected:
  /// The process group of the graph
  process_group_type pg;
  
  /// The distribution used for the graph
  distribution_type distribution_;
};

} } } // end namespace boost::graph::distributed

#endif // BOOST_GRAPH_DISTRIBUTED_NAMED_GRAPH_HPP
