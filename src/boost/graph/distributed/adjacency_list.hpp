// Copyright (C) 2004-2006 The Trustees of Indiana University.
// Copyright (C) 2007 Douglas Gregor 

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

#ifndef BOOST_GRAPH_DISTRIBUTED_ADJACENCY_LIST_HPP
#define BOOST_GRAPH_DISTRIBUTED_ADJACENCY_LIST_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/core/uncaught_exceptions.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/distributed/concepts.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/adjacency_iterator.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/property_map/parallel/local_property_map.hpp>
#include <boost/graph/parallel/detail/property_holders.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/assert.hpp>
#include <list>
#include <iterator>
#include <algorithm>
#include <boost/limits.hpp>
#include <boost/graph/parallel/properties.hpp>
#include <boost/graph/parallel/distribution.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/graph/distributed/selector.hpp>
#include <boost/graph/parallel/process_group.hpp>
#include <boost/pending/container_traits.hpp>

// Callbacks
#include <boost/function/function2.hpp>

// Serialization
#include <boost/serialization/base_object.hpp>
#include <boost/mpi/datatype.hpp>
#include <boost/pending/property_serialize.hpp>
#include <boost/graph/distributed/unsafe_serialize.hpp>

// Named vertices
#include <boost/graph/distributed/named_graph.hpp>

#include <boost/graph/distributed/shuffled_distribution.hpp>

namespace boost {

  /// The type used to store an identifier that uniquely names a processor.
  // NGE: I doubt we'll be running on more than 32768 procs for the time being
  typedef /*int*/ short processor_id_type;

  // Tell which processor the target of an edge resides on (for
  // directed graphs) or which processor the other end point of the
  // edge resides on (for undirected graphs).
  enum edge_target_processor_id_t { edge_target_processor_id };
  BOOST_INSTALL_PROPERTY(edge, target_processor_id);

  // For undirected graphs, tells whether the edge is locally owned.
  enum edge_locally_owned_t { edge_locally_owned };
  BOOST_INSTALL_PROPERTY(edge, locally_owned);

  // For bidirectional graphs, stores the incoming edges.
  enum vertex_in_edges_t { vertex_in_edges };
  BOOST_INSTALL_PROPERTY(vertex, in_edges);

  /// Tag class for directed, distributed adjacency list
  struct directed_distributed_adj_list_tag
    : public virtual distributed_graph_tag,
      public virtual distributed_vertex_list_graph_tag,
      public virtual distributed_edge_list_graph_tag,
      public virtual incidence_graph_tag,
      public virtual adjacency_graph_tag {};

  /// Tag class for bidirectional, distributed adjacency list
  struct bidirectional_distributed_adj_list_tag
    : public virtual distributed_graph_tag,
      public virtual distributed_vertex_list_graph_tag,
      public virtual distributed_edge_list_graph_tag,
      public virtual incidence_graph_tag,
      public virtual adjacency_graph_tag,
      public virtual bidirectional_graph_tag {};

  /// Tag class for undirected, distributed adjacency list
  struct undirected_distributed_adj_list_tag
    : public virtual distributed_graph_tag,
      public virtual distributed_vertex_list_graph_tag,
      public virtual distributed_edge_list_graph_tag,
      public virtual incidence_graph_tag,
      public virtual adjacency_graph_tag,
      public virtual bidirectional_graph_tag {};

  namespace detail {
    template<typename Archiver, typename Directed, typename Vertex>
    void
    serialize(Archiver& ar, edge_base<Directed, Vertex>& e,
              const unsigned int /*version*/)
    {
      ar & unsafe_serialize(e.m_source)
         & unsafe_serialize(e.m_target);
    }

    template<typename Archiver, typename Directed, typename Vertex>
    void
    serialize(Archiver& ar, edge_desc_impl<Directed, Vertex>& e,
              const unsigned int /*version*/)
    {
      ar & boost::serialization::base_object<edge_base<Directed, Vertex> >(e)
         & unsafe_serialize(e.m_eproperty);
    }
  }

  namespace detail { namespace parallel {
  
    /**
     * A distributed vertex descriptor. These descriptors contain both
     * the ID of the processor that owns the vertex and a local vertex
     * descriptor that identifies the particular vertex for that
     * processor.
     */
    template<typename LocalDescriptor>
    struct global_descriptor
    {
      typedef LocalDescriptor local_descriptor_type;

      global_descriptor() : owner(), local() { }

      global_descriptor(processor_id_type owner, LocalDescriptor local)
        : owner(owner), local(local) { }

      processor_id_type owner;
      LocalDescriptor local;

      /**
       * A function object that, given a processor ID, generates
       * distributed vertex descriptors from local vertex
       * descriptors. This function object is used by the
       * vertex_iterator of the distributed adjacency list.
       */
      struct generator
      {
        typedef global_descriptor<LocalDescriptor> result_type;
        typedef LocalDescriptor argument_type;

        generator() {}
        generator(processor_id_type owner) : owner(owner) {}

        result_type operator()(argument_type v) const
        { return result_type(owner, v); }

      private:
        processor_id_type owner;
      };

      template<typename Archiver>
      void serialize(Archiver& ar, const unsigned int /*version*/)
      {
        ar & owner & unsafe_serialize(local);
      }
    };

    /// Determine the process that owns the given descriptor
    template<typename LocalDescriptor>
    inline processor_id_type owner(const global_descriptor<LocalDescriptor>& v)
    { return v.owner; }

    /// Determine the local portion of the given descriptor
    template<typename LocalDescriptor>
    inline LocalDescriptor local(const global_descriptor<LocalDescriptor>& v)
    { return v.local; }

    /// Compare distributed vertex descriptors for equality
    template<typename LocalDescriptor>
    inline bool
    operator==(const global_descriptor<LocalDescriptor>& u,
               const global_descriptor<LocalDescriptor>& v)
    {
      return u.owner == v.owner && u.local == v.local;
    }

    /// Compare distributed vertex descriptors for inequality
    template<typename LocalDescriptor>
    inline bool
    operator!=(const global_descriptor<LocalDescriptor>& u,
               const global_descriptor<LocalDescriptor>& v)
    { return !(u == v); }

    template<typename LocalDescriptor>
    inline bool
    operator<(const global_descriptor<LocalDescriptor>& u,
              const global_descriptor<LocalDescriptor>& v)
    {
      return (u.owner) < v.owner || (u.owner == v.owner && (u.local) < v.local);
    }

    template<typename LocalDescriptor>
    inline bool
    operator<=(const global_descriptor<LocalDescriptor>& u,
               const global_descriptor<LocalDescriptor>& v)
    {
      return u.owner <= v.owner || (u.owner == v.owner && u.local <= v.local);
    }

    template<typename LocalDescriptor>
    inline bool
    operator>(const global_descriptor<LocalDescriptor>& u,
              const global_descriptor<LocalDescriptor>& v)
    {
      return v < u;
    }

    template<typename LocalDescriptor>
    inline bool
    operator>=(const global_descriptor<LocalDescriptor>& u,
               const global_descriptor<LocalDescriptor>& v)
    {
      return v <= u;
    }

    // DPG TBD: Add <, <=, >=, > for global descriptors

    /**
     * A Readable Property Map that extracts a global descriptor pair
     * from a global_descriptor.
     */
    template<typename LocalDescriptor>
    struct global_descriptor_property_map
    {
      typedef std::pair<processor_id_type, LocalDescriptor> value_type;
      typedef value_type reference;
      typedef global_descriptor<LocalDescriptor> key_type;
      typedef readable_property_map_tag category;
    };

    template<typename LocalDescriptor>
    inline std::pair<processor_id_type, LocalDescriptor>
    get(global_descriptor_property_map<LocalDescriptor>,
        global_descriptor<LocalDescriptor> x)
    {
      return std::pair<processor_id_type, LocalDescriptor>(x.owner, x.local);
    }

    /**
     * A Readable Property Map that extracts the owner of a global
     * descriptor.
     */
    template<typename LocalDescriptor>
    struct owner_property_map
    {
      typedef processor_id_type value_type;
      typedef value_type reference;
      typedef global_descriptor<LocalDescriptor> key_type;
      typedef readable_property_map_tag category;
    };

    template<typename LocalDescriptor>
    inline processor_id_type
    get(owner_property_map<LocalDescriptor>,
        global_descriptor<LocalDescriptor> x)
    {
      return x.owner;
    }

    /**
     * A Readable Property Map that extracts the local descriptor from
     * a global descriptor.
     */
    template<typename LocalDescriptor>
    struct local_descriptor_property_map
    {
      typedef LocalDescriptor value_type;
      typedef value_type reference;
      typedef global_descriptor<LocalDescriptor> key_type;
      typedef readable_property_map_tag category;
    };

    template<typename LocalDescriptor>
    inline LocalDescriptor
    get(local_descriptor_property_map<LocalDescriptor>,
        global_descriptor<LocalDescriptor> x)
    {
      return x.local;
    }

    /**
     * Stores an incoming edge for a bidirectional distributed
     * adjacency list. The user does not see this type directly,
     * because it is just an implementation detail.
     */
    template<typename Edge>
    struct stored_in_edge
    {
      stored_in_edge(processor_id_type sp, Edge e)
        : source_processor(sp), e(e) {}

      processor_id_type source_processor;
      Edge e;
    };

    /**
     * A distributed edge descriptor. These descriptors contain the
     * underlying edge descriptor, the processor IDs for both the
     * source and the target of the edge, and a boolean flag that
     * indicates which of the processors actually owns the edge.
     */
    template<typename Edge>
    struct edge_descriptor
    {
      edge_descriptor(processor_id_type sp = processor_id_type(),
                      processor_id_type tp = processor_id_type(),
                      bool owns = false, Edge ld = Edge())
        : source_processor(sp), target_processor(tp),
          source_owns_edge(owns), local(ld) {}

      processor_id_type owner() const
      {
        return source_owns_edge? source_processor : target_processor;
      }

      /// The processor that the source vertex resides on
      processor_id_type source_processor;

      /// The processor that the target vertex resides on
      processor_id_type target_processor;

      /// True when the source processor owns the edge, false when the
      /// target processor owns the edge.
      bool source_owns_edge;

      /// The local edge descriptor.
      Edge local;

      /**
       * Function object that generates edge descriptors for the
       * out_edge_iterator of the given distributed adjacency list
       * from the edge descriptors of the underlying adjacency list.
       */
      template<typename Graph>
      class out_generator
      {
        typedef typename Graph::directed_selector directed_selector;

      public:
        typedef edge_descriptor<Edge> result_type;
        typedef Edge argument_type;

        out_generator() : g(0) {}
        explicit out_generator(const Graph& g) : g(&g) {}

        result_type operator()(argument_type e) const
        { return map(e, directed_selector()); }

      private:
        result_type map(argument_type e, directedS) const
        {
          return result_type(g->processor(),
                             get(edge_target_processor_id, g->base(), e),
                             true, e);
        }

        result_type map(argument_type e, bidirectionalS) const
        {
          return result_type(g->processor(),
                             get(edge_target_processor_id, g->base(), e),
                             true, e);
        }

        result_type map(argument_type e, undirectedS) const
        {
          return result_type(g->processor(),
                             get(edge_target_processor_id, g->base(), e),
                             get(edge_locally_owned, g->base(), e),
                             e);
        }

        const Graph* g;
      };

      /**
       * Function object that generates edge descriptors for the
       * in_edge_iterator of the given distributed adjacency list
       * from the edge descriptors of the underlying adjacency list.
       */
      template<typename Graph>
      class in_generator
      {
        typedef typename Graph::directed_selector DirectedS;

      public:
        typedef typename boost::mpl::if_<is_same<DirectedS, bidirectionalS>,
                                         stored_in_edge<Edge>,
                                         Edge>::type argument_type;
        typedef edge_descriptor<Edge> result_type;

        in_generator() : g(0) {}
        explicit in_generator(const Graph& g) : g(&g) {}

        result_type operator()(argument_type e) const
        { return map(e, DirectedS()); }

      private:
        /**
         * For a bidirectional graph, we just generate the appropriate
         * edge. No tricks.
         */
        result_type map(argument_type e, bidirectionalS) const
        {
          return result_type(e.source_processor,
                             g->processor(),
                             true,
                             e.e);
        }

        /**
         * For an undirected graph, we generate descriptors for the
         * incoming edges by swapping the source/target of the
         * underlying edge descriptor (a hack). The target processor
         * ID on the edge is actually the source processor for this
         * edge, and our processor is the target processor. If the
         * edge is locally owned, then it is owned by the target (us);
         * otherwise it is owned by the source.
         */
        result_type map(argument_type e, undirectedS) const
        {
          typename Graph::local_edge_descriptor local_edge(e);
          // TBD: This is a very, VERY lame hack that takes advantage
          // of our knowledge of the internals of the BGL
          // adjacency_list. There should be a cleaner way to handle
          // this...
          using std::swap;
          swap(local_edge.m_source, local_edge.m_target);
          return result_type(get(edge_target_processor_id, g->base(), e),
                             g->processor(),
                             !get(edge_locally_owned, g->base(), e),
                             local_edge);
        }

        const Graph* g;
      };

    private:
      friend class boost::serialization::access;

      template<typename Archiver>
      void serialize(Archiver& ar, const unsigned int /*version*/)
      {
        ar
          & source_processor
          & target_processor
          & source_owns_edge
          & local;
      }
    };

    /// Determine the process that owns this edge
    template<typename Edge>
    inline processor_id_type
    owner(const edge_descriptor<Edge>& e)
    { return e.source_owns_edge? e.source_processor : e.target_processor; }

    /// Determine the local descriptor for this edge.
    template<typename Edge>
    inline Edge
    local(const edge_descriptor<Edge>& e)
    { return e.local; }

    /**
     * A Readable Property Map that extracts the owner and local
     * descriptor of an edge descriptor.
     */
    template<typename Edge>
    struct edge_global_property_map
    {
      typedef std::pair<processor_id_type, Edge> value_type;
      typedef value_type reference;
      typedef edge_descriptor<Edge> key_type;
      typedef readable_property_map_tag category;
    };

    template<typename Edge>
    inline std::pair<processor_id_type, Edge>
    get(edge_global_property_map<Edge>, const edge_descriptor<Edge>& e)
    {
      typedef std::pair<processor_id_type, Edge> result_type;
      return result_type(e.source_owns_edge? e.source_processor
                         /* target owns edge*/: e.target_processor,
                         e.local);
    }

    /**
     * A Readable Property Map that extracts the owner of an edge
     * descriptor.
     */
    template<typename Edge>
    struct edge_owner_property_map
    {
      typedef processor_id_type value_type;
      typedef value_type reference;
      typedef edge_descriptor<Edge> key_type;
      typedef readable_property_map_tag category;
    };

    template<typename Edge>
    inline processor_id_type
    get(edge_owner_property_map<Edge>, const edge_descriptor<Edge>& e)
    {
      return e.source_owns_edge? e.source_processor : e.target_processor;
    }

    /**
     * A Readable Property Map that extracts the local descriptor from
     * an edge descriptor.
     */
    template<typename Edge>
    struct edge_local_property_map
    {
      typedef Edge value_type;
      typedef value_type reference;
      typedef edge_descriptor<Edge> key_type;
      typedef readable_property_map_tag category;
    };

    template<typename Edge>
    inline Edge
    get(edge_local_property_map<Edge>,
        const edge_descriptor<Edge>& e)
    {
      return e.local;
    }

    /** Compare distributed edge descriptors for equality.
     *
     * \todo need edge_descriptor to know if it is undirected so we
     * can compare both ways.
     */
    template<typename Edge>
    inline bool
    operator==(const edge_descriptor<Edge>& e1,
               const edge_descriptor<Edge>& e2)
    {
      return (e1.source_processor == e2.source_processor
              && e1.target_processor == e2.target_processor
              && e1.local == e2.local);
    }

    /// Compare distributed edge descriptors for inequality.
    template<typename Edge>
    inline bool
    operator!=(const edge_descriptor<Edge>& e1,
               const edge_descriptor<Edge>& e2)
    { return !(e1 == e2); }

    /**
     * Configuration for the distributed adjacency list. We use this
     * parameter to store all of the configuration details for the
     * implementation of the distributed adjacency list, which allows us to
     * get at the distribution type in the maybe_named_graph.
     */
    template<typename OutEdgeListS, typename ProcessGroup,
             typename InVertexListS, typename InDistribution,
             typename DirectedS, typename VertexProperty, 
             typename EdgeProperty, typename GraphProperty, 
             typename EdgeListS>
    struct adjacency_list_config
    {
      typedef typename mpl::if_<is_same<InVertexListS, defaultS>, 
                                vecS, InVertexListS>::type 
        VertexListS;

      /// Introduce the target processor ID property for each edge
      typedef property<edge_target_processor_id_t, processor_id_type,
                       EdgeProperty> edge_property_with_id;

      /// For undirected graphs, introduce the locally-owned property for edges
      typedef typename boost::mpl::if_<is_same<DirectedS, undirectedS>,
                                       property<edge_locally_owned_t, bool,
                                                edge_property_with_id>,
                                       edge_property_with_id>::type
        base_edge_property_type;

      /// The edge descriptor type for the local subgraph
      typedef typename adjacency_list_traits<OutEdgeListS,
                                             VertexListS,
                                             directedS>::edge_descriptor
        local_edge_descriptor;

      /// For bidirectional graphs, the type of an incoming stored edge
      typedef stored_in_edge<local_edge_descriptor> bidir_stored_edge;

      /// The container type that will store incoming edges for a
      /// bidirectional graph.
      typedef typename container_gen<EdgeListS, bidir_stored_edge>::type
        in_edge_list_type;

      // Bidirectional graphs have an extra vertex property to store
      // the incoming edges.
      typedef typename boost::mpl::if_<is_same<DirectedS, bidirectionalS>,
                                       property<vertex_in_edges_t, in_edge_list_type,
                                                VertexProperty>,
                                       VertexProperty>::type 
        base_vertex_property_type;

      // The type of the distributed adjacency list
      typedef adjacency_list<OutEdgeListS,
                             distributedS<ProcessGroup, 
                                          VertexListS, 
                                          InDistribution>,
                             DirectedS, VertexProperty, EdgeProperty,
                             GraphProperty, EdgeListS> 
        graph_type;

      // The type of the underlying adjacency list implementation
      typedef adjacency_list<OutEdgeListS, VertexListS, directedS,
                             base_vertex_property_type,
                             base_edge_property_type,
                             GraphProperty,
                             EdgeListS> 
        inherited;
      
      typedef InDistribution in_distribution_type;
      typedef typename inherited::vertices_size_type vertices_size_type;

          typedef typename ::boost::graph::distributed::select_distribution<
              in_distribution_type, VertexProperty, vertices_size_type, 
              ProcessGroup>::type 
        base_distribution_type;

          typedef ::boost::graph::distributed::shuffled_distribution<
          base_distribution_type> distribution_type;

      typedef VertexProperty vertex_property_type;
      typedef EdgeProperty edge_property_type;
      typedef ProcessGroup process_group_type;

      typedef VertexListS vertex_list_selector;
      typedef OutEdgeListS out_edge_list_selector;
      typedef DirectedS directed_selector;
      typedef GraphProperty graph_property_type;
      typedef EdgeListS edge_list_selector;
    };

    // Maybe initialize the indices of each vertex
    template<typename IteratorPair, typename VertexIndexMap>
    void
    maybe_initialize_vertex_indices(IteratorPair p, VertexIndexMap to_index,
                                    read_write_property_map_tag)
    {
      typedef typename property_traits<VertexIndexMap>::value_type index_t;
      index_t next_index = 0;
      while (p.first != p.second)
        put(to_index, *p.first++, next_index++);
    }

    template<typename IteratorPair, typename VertexIndexMap>
    inline void
    maybe_initialize_vertex_indices(IteratorPair p, VertexIndexMap to_index,
                                    readable_property_map_tag)
    {
      // Do nothing
    }

    template<typename IteratorPair, typename VertexIndexMap>
    inline void
    maybe_initialize_vertex_indices(IteratorPair p, VertexIndexMap to_index)
    {
      typedef typename property_traits<VertexIndexMap>::category category;
      maybe_initialize_vertex_indices(p, to_index, category());
    }

    template<typename IteratorPair>
    inline void
    maybe_initialize_vertex_indices(IteratorPair p,
                                    ::boost::detail::error_property_not_found)
    { }

    /***********************************************************************
     * Message Payloads                                                    *
     ***********************************************************************/

    /**
     * Data stored with a msg_add_edge message, which requests the
     * remote addition of an edge.
     */
    template<typename Vertex, typename LocalVertex>
    struct msg_add_edge_data
    {
      msg_add_edge_data() { }

      msg_add_edge_data(Vertex source, Vertex target)
        : source(source.local), target(target) { }

      /// The source of the edge; the processor will be the
      /// receiving processor.
      LocalVertex source;
        
      /// The target of the edge.
      Vertex target;
        
      template<typename Archiver>
      void serialize(Archiver& ar, const unsigned int /*version*/)
      {
        ar & unsafe_serialize(source) & target;
      }
    };

    /**
     * Like @c msg_add_edge_data, but also includes a user-specified
     * property value to be attached to the edge.
     */
    template<typename Vertex, typename LocalVertex, typename EdgeProperty>
    struct msg_add_edge_with_property_data
      : msg_add_edge_data<Vertex, LocalVertex>, 
        maybe_store_property<EdgeProperty>
    {
    private:
      typedef msg_add_edge_data<Vertex, LocalVertex> inherited_data;
      typedef maybe_store_property<EdgeProperty> inherited_property;

    public:
      msg_add_edge_with_property_data() { }

      msg_add_edge_with_property_data(Vertex source, 
                                      Vertex target,
                                      const EdgeProperty& property)
        : inherited_data(source, target),
          inherited_property(property) { }
      
      template<typename Archiver>
      void serialize(Archiver& ar, const unsigned int /*version*/)
      {
        ar & boost::serialization::base_object<inherited_data>(*this) 
           & boost::serialization::base_object<inherited_property>(*this);
      }
    };

    //------------------------------------------------------------------------
    // Distributed adjacency list property map details
    /**
     * Metafunction that extracts the given property from the given
     * distributed adjacency list type. This could be implemented much
     * more cleanly, but even newer versions of GCC (e.g., 3.2.3)
     * cannot properly handle partial specializations involving
     * enumerator types.
     */
    template<typename Property>
    struct get_adj_list_pmap
    {
      template<typename Graph>
      struct apply
      {
        typedef Graph graph_type;
        typedef typename graph_type::process_group_type process_group_type;
        typedef typename graph_type::inherited base_graph_type;
        typedef typename property_map<base_graph_type, Property>::type
          local_pmap;
        typedef typename property_map<base_graph_type, Property>::const_type
          local_const_pmap;

        typedef graph_traits<graph_type> traits;
        typedef typename graph_type::local_vertex_descriptor local_vertex;
        typedef typename property_traits<local_pmap>::key_type local_key_type;

        typedef typename property_traits<local_pmap>::value_type value_type;

        typedef typename property_map<Graph, vertex_global_t>::const_type
          vertex_global_map;
        typedef typename property_map<Graph, edge_global_t>::const_type
          edge_global_map;

        typedef typename mpl::if_c<(is_same<local_key_type,
                                            local_vertex>::value),
                                   vertex_global_map, edge_global_map>::type
          global_map;

      public:
        typedef ::boost::parallel::distributed_property_map<
                  process_group_type, global_map, local_pmap> type;

        typedef ::boost::parallel::distributed_property_map<
                  process_group_type, global_map, local_const_pmap> const_type;
      };
    };

    /**
     * The local vertex index property map is actually a mapping from
     * the local vertex descriptors to vertex indices.
     */
    template<>
    struct get_adj_list_pmap<vertex_local_index_t>
    {
      template<typename Graph>
      struct apply
        : ::boost::property_map<typename Graph::inherited, vertex_index_t>
      { };
    };

    /**
     * The vertex index property map maps from global descriptors
     * (e.g., the vertex descriptor of a distributed adjacency list)
     * to the underlying local index. It is not valid to use this
     * property map with nonlocal descriptors.
     */
    template<>
    struct get_adj_list_pmap<vertex_index_t>
    {
      template<typename Graph>
      struct apply
      {
      private:
        typedef typename property_map<Graph, vertex_global_t>::const_type
          global_map;

        typedef property_map<typename Graph::inherited, vertex_index_t> local;

      public:
        typedef local_property_map<typename Graph::process_group_type,
                                   global_map,
                                   typename local::type> type;
        typedef local_property_map<typename Graph::process_group_type,
                                   global_map,
                                   typename local::const_type> const_type;
      };
    };

    /**
     * The vertex owner property map maps from vertex descriptors to
     * the processor that owns the vertex.
     */
    template<>
    struct get_adj_list_pmap<vertex_global_t>
    {
      template<typename Graph>
      struct apply
      {
      private:
        typedef typename Graph::local_vertex_descriptor
          local_vertex_descriptor;
      public:
        typedef global_descriptor_property_map<local_vertex_descriptor> type;
        typedef type const_type;
      };
    };

    /**
     * The vertex owner property map maps from vertex descriptors to
     * the processor that owns the vertex.
     */
    template<>
    struct get_adj_list_pmap<vertex_owner_t>
    {
      template<typename Graph>
      struct apply
      {
      private:
        typedef typename Graph::local_vertex_descriptor
          local_vertex_descriptor;
      public:
        typedef owner_property_map<local_vertex_descriptor> type;
        typedef type const_type;
      };
    };

    /**
     * The vertex local property map maps from vertex descriptors to
     * the local descriptor for that vertex.
     */
    template<>
    struct get_adj_list_pmap<vertex_local_t>
    {
      template<typename Graph>
      struct apply
      {
      private:
        typedef typename Graph::local_vertex_descriptor
          local_vertex_descriptor;
      public:
        typedef local_descriptor_property_map<local_vertex_descriptor> type;
        typedef type const_type;
      };
    };

    /**
     * The edge global property map maps from edge descriptors to
     * a pair of the owning processor and local descriptor.
     */
    template<>
    struct get_adj_list_pmap<edge_global_t>
    {
      template<typename Graph>
      struct apply
      {
      private:
        typedef typename Graph::local_edge_descriptor
          local_edge_descriptor;
      public:
        typedef edge_global_property_map<local_edge_descriptor> type;
        typedef type const_type;
      };
    };

    /**
     * The edge owner property map maps from edge descriptors to
     * the processor that owns the edge.
     */
    template<>
    struct get_adj_list_pmap<edge_owner_t>
    {
      template<typename Graph>
      struct apply
      {
      private:
        typedef typename Graph::local_edge_descriptor
          local_edge_descriptor;
      public:
        typedef edge_owner_property_map<local_edge_descriptor> type;
        typedef type const_type;
      };
    };

    /**
     * The edge local property map maps from edge descriptors to
     * the local descriptor for that edge.
     */
    template<>
    struct get_adj_list_pmap<edge_local_t>
    {
      template<typename Graph>
      struct apply
      {
      private:
        typedef typename Graph::local_edge_descriptor
          local_edge_descriptor;
      public:
        typedef edge_local_property_map<local_edge_descriptor> type;
        typedef type const_type;
      };
    };
    //------------------------------------------------------------------------

    // Directed graphs do not have in edges, so this is a no-op
    template<typename Graph>
    inline void
    remove_in_edge(typename Graph::edge_descriptor, Graph&, directedS)
    { }

    // Bidirectional graphs have in edges stored in the
    // vertex_in_edges property.
    template<typename Graph>
    inline void
    remove_in_edge(typename Graph::edge_descriptor e, Graph& g, bidirectionalS)
    {
      typedef typename Graph::in_edge_list_type in_edge_list_type;
      in_edge_list_type& in_edges =
        get(vertex_in_edges, g.base())[target(e, g).local];
      typename in_edge_list_type::iterator i = in_edges.begin();
      while (i != in_edges.end()
             && !(i->source_processor == source(e, g).owner)
             && i->e == e.local)
        ++i;

      BOOST_ASSERT(i != in_edges.end());
      in_edges.erase(i);
    }

    // Undirected graphs have in edges stored as normal edges.
    template<typename Graph>
    inline void
    remove_in_edge(typename Graph::edge_descriptor e, Graph& g, undirectedS)
    {
      typedef typename Graph::inherited base_type;
      typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;

      // TBD: can we make this more efficient?
      // Removing edge (v, u). v is local
      base_type& bg = g.base();
      vertex_descriptor u = source(e, g);
      vertex_descriptor v = target(e, g);
      if (v.owner != process_id(g.process_group())) {
        using std::swap;
        swap(u, v);
      }

      typename graph_traits<base_type>::out_edge_iterator ei, ei_end;
      for (boost::tie(ei, ei_end) = out_edges(v.local, bg); ei != ei_end; ++ei)
      {
        if (target(*ei, g.base()) == u.local
            // TBD: deal with parallel edges properly && *ei == e
            && get(edge_target_processor_id, bg, *ei) == u.owner) {
          remove_edge(ei, bg);
          return;
        }
      }

      if (v.owner == process_id(g.process_group())) {

      }
    }

    //------------------------------------------------------------------------
    // Lazy addition of edges

    // Work around the fact that an adjacency_list with vecS vertex
    // storage automatically adds edges when the descriptor is
    // out-of-range.
    template <class Graph, class Config, class Base>
    inline std::pair<typename Config::edge_descriptor, bool>
    add_local_edge(typename Config::vertex_descriptor u,
                   typename Config::vertex_descriptor v,
                   const typename Config::edge_property_type& p,
                   vec_adj_list_impl<Graph, Config, Base>& g_)
    {
      adj_list_helper<Config, Base>& g = g_;
      return add_edge(u, v, p, g);
    }

    template <class Graph, class Config, class Base>
    inline std::pair<typename Config::edge_descriptor, bool>
    add_local_edge(typename Config::vertex_descriptor u,
                   typename Config::vertex_descriptor v,
                   const typename Config::edge_property_type& p,
                   boost::adj_list_impl<Graph, Config, Base>& g)
    {
      return add_edge(u, v, p, g);
    }

    template <class EdgeProperty,class EdgeDescriptor>
    struct msg_nonlocal_edge_data
      : public detail::parallel::maybe_store_property<EdgeProperty>
    {
      typedef EdgeProperty edge_property_type;
      typedef EdgeDescriptor local_edge_descriptor;
      typedef detail::parallel::maybe_store_property<edge_property_type> 
        inherited;

      msg_nonlocal_edge_data() {}
      msg_nonlocal_edge_data(local_edge_descriptor e,
                             const edge_property_type& p)
        : inherited(p), e(e) { }

      local_edge_descriptor e;

      template<typename Archiver>
      void serialize(Archiver& ar, const unsigned int /*version*/)
      {
        ar & boost::serialization::base_object<inherited>(*this) & e;
      }
    };

    template <class EdgeDescriptor>
    struct msg_remove_edge_data
    {
      typedef EdgeDescriptor edge_descriptor;
      msg_remove_edge_data() {}
      explicit msg_remove_edge_data(edge_descriptor e) : e(e) {}

      edge_descriptor e;

      template<typename Archiver>
      void serialize(Archiver& ar, const unsigned int /*version*/)
      {
        ar & e;
      }
    };

  } } // end namespace detail::parallel

  /**
   * Adjacency list traits for a distributed adjacency list. Contains
   * the vertex and edge descriptors, the directed-ness, and the
   * parallel edges typedefs.
   */
  template<typename OutEdgeListS, typename ProcessGroup,
           typename InVertexListS, typename InDistribution, typename DirectedS>
  struct adjacency_list_traits<OutEdgeListS,
                               distributedS<ProcessGroup, 
                                            InVertexListS,
                                            InDistribution>,
                               DirectedS>
  {
  private:
    typedef typename mpl::if_<is_same<InVertexListS, defaultS>,
                              vecS,
                              InVertexListS>::type VertexListS;

    typedef adjacency_list_traits<OutEdgeListS, VertexListS, directedS>
      base_type;

  public:
    typedef typename base_type::vertex_descriptor local_vertex_descriptor;
    typedef typename base_type::edge_descriptor   local_edge_descriptor;

    typedef typename boost::mpl::if_<typename DirectedS::is_bidir_t,
      bidirectional_tag,
      typename boost::mpl::if_<typename DirectedS::is_directed_t,
        directed_tag, undirected_tag
      >::type
    >::type directed_category;

    typedef typename parallel_edge_traits<OutEdgeListS>::type
      edge_parallel_category;

    typedef detail::parallel::global_descriptor<local_vertex_descriptor>
      vertex_descriptor;

    typedef detail::parallel::edge_descriptor<local_edge_descriptor>
      edge_descriptor;
  };

#define PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS                                    \
  typename OutEdgeListS, typename ProcessGroup, typename InVertexListS,        \
  typename InDistribution, typename DirectedS, typename VertexProperty,        \
  typename EdgeProperty,  typename GraphProperty, typename EdgeListS

#define PBGL_DISTRIB_ADJLIST_TYPE                                              \
  adjacency_list<OutEdgeListS,                                                 \
                 distributedS<ProcessGroup, InVertexListS, InDistribution>,    \
                 DirectedS, VertexProperty, EdgeProperty, GraphProperty,       \
                 EdgeListS>

#define PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG                             \
  typename OutEdgeListS, typename ProcessGroup, typename InVertexListS,        \
  typename InDistribution, typename VertexProperty,                            \
  typename EdgeProperty,  typename GraphProperty, typename EdgeListS
  
#define PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directed)                             \
  adjacency_list<OutEdgeListS,                                                 \
                 distributedS<ProcessGroup, InVertexListS, InDistribution>,    \
                 directed, VertexProperty, EdgeProperty, GraphProperty,        \
                 EdgeListS>
                 
  /** A distributed adjacency list.
   *
   * This class template partial specialization defines a distributed
   * (or "partitioned") adjacency list graph. The distributed
   * adjacency list is similar to the standard Boost Graph Library
   * adjacency list, which stores a list of vertices and for each
   * verted the list of edges outgoing from the vertex (and, in some
   * cases, also the edges incoming to the vertex). The distributed
   * adjacency list differs in that it partitions the graph into
   * several subgraphs that are then divided among different
   * processors (or nodes within a cluster). The distributed adjacency
   * list attempts to maintain a high degree of compatibility with the
   * standard, non-distributed adjacency list.
   *
   * The graph is partitioned by vertex, with each processor storing
   * all of the required information for a particular subset of the
   * vertices, including vertex properties, outgoing edges, and (for
   * bidirectional graphs) incoming edges. This information is
   * accessible only on the processor that owns the vertex: for
   * instance, if processor 0 owns vertex @c v, no other processor can
   * directly access the properties of @c v or enumerate its outgoing
   * edges.
   *
   * Edges in a graph may be entirely local (connecting two local
   * vertices), but more often it is the case that edges are
   * non-local, meaning that the two vertices they connect reside in
   * different processes. Edge properties are stored with the
   * originating vertex for directed and bidirectional graphs, and are
   * therefore only accessible from the processor that owns the
   * originating vertex. Other processors may query the source and
   * target of the edge, but cannot access its properties. This is
   * particularly interesting when accessing the incoming edges of a
   * bidirectional graph, which are not guaranteed to be stored on the
   * processor that is able to perform the iteration. For undirected
   * graphs the situation is more complicated, since no vertex clearly
   * owns the edges: the list of edges incident to a vertex may
   * contain a mix of local and non-local edges.
   *
   * The distributed adjacency list is able to model several of the
   * existing Graph concepts. It models the Graph concept because it
   * exposes vertex and edge descriptors in the normal way; these
   * descriptors model the GlobalDescriptor concept (because they have
   * an owner and a local descriptor), and as such the distributed
   * adjacency list models the DistributedGraph concept. The adjacency
   * list also models the IncidenceGraph and AdjacencyGraph concepts,
   * although this is only true so long as the domain of the valid
   * expression arguments are restricted to vertices and edges stored
   * locally. Likewise, bidirectional and undirected distributed
   * adjacency lists model the BidirectionalGraph concept (vertex and
   * edge domains must be respectived) and the distributed adjacency
   * list models the MutableGraph concept (vertices and edges can only
   * be added or removed locally). T he distributed adjacency list
   * does not, however, model the VertexListGraph or EdgeListGraph
   * concepts, because we can not efficiently enumerate all vertices
   * or edges in the graph. Instead, the local subsets of vertices and
   * edges can be enumerated (with the same syntax): the distributed
   * adjacency list therefore models the DistributedVertexListGraph
   * and DistributedEdgeListGraph concepts, because concurrent
   * iteration over all of the vertices or edges stored on each
   * processor will visit each vertex or edge.
   *
   * The distributed adjacency list is distinguished from the
   * non-distributed version by the vertex list descriptor, which will
   * be @c distributedS<ProcessGroup,VertexListS>. Here,
   * the VertexListS type plays the same role as the VertexListS type
   * in the non-distributed adjacency list: it allows one to select
   * the data structure that will be used to store the local
   * vertices. The ProcessGroup type, on the other hand, is unique to
   * distributed data structures: it is the type that abstracts a
   * group of cooperating processes, and it used for process
   * identification, communication, and synchronization, among other
   * things. Different process group types represent different
   * communication mediums (e.g., MPI, PVM, TCP) or different models
   * of communication (LogP, CGM, BSP, synchronous, etc.). This
   * distributed adjacency list assumes a model based on non-blocking
   * sends.
   *
   * Distribution of vertices across different processors is
   * accomplished in two different ways. When initially constructing
   * the graph, the user may provide a distribution object (that
   * models the Distribution concept), which will determine the
   * distribution of vertices to each process. Additionally, the @c
   * add_vertex and @c add_edge operations add vertices or edges
   * stored on the local processor. For @c add_edge, this is
   * accomplished by requiring that the source vertex of the new edge
   * be local to the process executing @c add_edge.
   *
   * Internal properties of a distributed adjacency list are
   * accessible in the same manner as internal properties for a
   * non-distributed adjacency list for local vertices or
   * edges. Access to properties for remote vertices or edges occurs
   * with the same syntax, but involve communication with the owner of
   * the information: for more information, refer to class template
   * @ref distributed_property_map, which manages distributed
   * property maps. Note that the distributed property maps created
   * for internal properties determine their reduction operation via
   * the metafunction @ref property_reduce, which for the vast
   * majority of uses is correct behavior.
   *
   * Communication among the processes coordinating on a particular
   * distributed graph relies on non-blocking message passing along
   * with synchronization. Local portions of the distributed graph may
   * be modified concurrently, including the introduction of non-local
   * edges, but prior to accessing the graph it is recommended that
   * the @c synchronize free function be invoked on the graph to clear
   * up any pending interprocess communication and modifications. All
   * processes will then be released from the synchronization barrier
   * concurrently.
   *
   * \todo Determine precisely what we should do with nonlocal edges
   * in undirected graphs. Our parallelization of certain algorithms
   * relies on the ability to access edge property maps immediately
   * (e.g., edge_weight_t), so it may be necessary to duplicate the
   * edge properties in both processes (but then we need some form of
   * coherence protocol).
   *
   * \todo What does the user do if @c property_reduce doesn't do the
   * right thing?
   */
  template<typename OutEdgeListS, typename ProcessGroup,
           typename InVertexListS, typename InDistribution, typename DirectedS,
           typename VertexProperty, typename EdgeProperty, 
           typename GraphProperty, typename EdgeListS>
  class adjacency_list<OutEdgeListS,
                       distributedS<ProcessGroup, 
                                    InVertexListS, 
                                    InDistribution>,
                       DirectedS, VertexProperty,
                       EdgeProperty, GraphProperty, EdgeListS>
    : // Support for named vertices
      public graph::distributed::maybe_named_graph<   
        adjacency_list<OutEdgeListS,
                       distributedS<ProcessGroup,
                                    InVertexListS,
                                    InDistribution>,
                       DirectedS, VertexProperty,
                       EdgeProperty, GraphProperty, EdgeListS>,
        typename adjacency_list_traits<OutEdgeListS, 
                                       distributedS<ProcessGroup,
                                                    InVertexListS,
                                                    InDistribution>,
                                       DirectedS>::vertex_descriptor,
        typename adjacency_list_traits<OutEdgeListS, 
                                       distributedS<ProcessGroup,
                                                    InVertexListS,
                                                    InDistribution>,
                                       DirectedS>::edge_descriptor,
        detail::parallel::adjacency_list_config<OutEdgeListS, ProcessGroup, 
                                                InVertexListS, InDistribution,
                                                DirectedS, VertexProperty, 
                                                EdgeProperty, GraphProperty, 
                                                EdgeListS> >
  {
    typedef detail::parallel::adjacency_list_config<OutEdgeListS, ProcessGroup, 
                                                InVertexListS, InDistribution,
                                                DirectedS, VertexProperty, 
                                                EdgeProperty, GraphProperty, 
                                                EdgeListS>
      config_type;
      
    typedef adjacency_list_traits<OutEdgeListS,
                                  distributedS<ProcessGroup, 
                                               InVertexListS, 
                                               InDistribution>,
                                  DirectedS> 
      traits_type;

    typedef typename DirectedS::is_directed_t is_directed;

    typedef EdgeListS edge_list_selector;

  public:
    /// The container type that will store incoming edges for a
    /// bidirectional graph.
    typedef typename config_type::in_edge_list_type in_edge_list_type;
//    typedef typename inherited::edge_descriptor   edge_descriptor;

    /// The type of the underlying adjacency list implementation
    typedef typename config_type::inherited inherited;

    /// The type of properties stored in the local subgraph
    /// Bidirectional graphs have an extra vertex property to store
    /// the incoming edges.
    typedef typename inherited::vertex_property_type
      base_vertex_property_type;

    /// The type of the distributed adjacency list (this type)
    typedef typename config_type::graph_type graph_type;

    /// Expose graph components and graph category
    typedef typename traits_type::local_vertex_descriptor
      local_vertex_descriptor;
    typedef typename traits_type::local_edge_descriptor
      local_edge_descriptor;
    typedef typename traits_type::vertex_descriptor vertex_descriptor;
    typedef typename traits_type::edge_descriptor edge_descriptor;

    typedef typename traits_type::directed_category directed_category;
    typedef typename inherited::edge_parallel_category
      edge_parallel_category;
    typedef typename inherited::graph_tag graph_tag;

    // Current implementation requires the ability to have parallel
    // edges in the underlying adjacency_list. Which processor each
    // edge refers to is attached as an internal property. TBD:
    // remove this restriction, which may require some rewriting.
    BOOST_STATIC_ASSERT((is_same<edge_parallel_category,
                                 allow_parallel_edge_tag>::value));

    /** Determine the graph traversal category.
     *
     * A directed distributed adjacency list models the Distributed
     * Graph, Incidence Graph, and Adjacency Graph
     * concepts. Bidirectional and undirected graphs also model the
     * Bidirectional Graph concept. Note that when modeling these
     * concepts the domains of certain operations (e.g., in_edges)
     * are restricted; see the distributed adjacency_list
     * documentation.
     */
    typedef typename boost::mpl::if_<
              is_same<DirectedS, directedS>,
              directed_distributed_adj_list_tag,
              typename boost::mpl::if_<is_same<DirectedS, bidirectionalS>,
                                       bidirectional_distributed_adj_list_tag,
                                       undirected_distributed_adj_list_tag>::type>
      ::type traversal_category;

    typedef typename inherited::degree_size_type degree_size_type;
    typedef typename inherited::vertices_size_type vertices_size_type;
    typedef typename inherited::edges_size_type edges_size_type;
    typedef VertexProperty vertex_property_type;
    typedef EdgeProperty edge_property_type;
    typedef typename inherited::graph_property_type graph_property_type;
    typedef typename inherited::vertex_bundled vertex_bundled;
    typedef typename inherited::edge_bundled edge_bundled;
    typedef typename inherited::graph_bundled graph_bundled;

    typedef typename container_gen<edge_list_selector, edge_descriptor>::type
      local_edge_list_type;

  private:
    typedef typename boost::mpl::if_<is_same<DirectedS, bidirectionalS>,
                                     typename in_edge_list_type::const_iterator,
                                     typename inherited::out_edge_iterator>::type
      base_in_edge_iterator;

    typedef typename inherited::out_edge_iterator base_out_edge_iterator;
    typedef typename graph_traits<inherited>::edge_iterator
      base_edge_iterator;
    typedef typename inherited::edge_property_type base_edge_property_type;

    typedef typename local_edge_list_type::const_iterator
      undirected_edge_iterator;

    typedef InDistribution in_distribution_type;

    typedef parallel::trigger_receive_context trigger_receive_context;

  public:
    /// Iterator over the (local) vertices of the graph
    typedef transform_iterator<typename vertex_descriptor::generator,
                               typename inherited::vertex_iterator>
      vertex_iterator;

    /// Helper for out_edge_iterator
    typedef typename edge_descriptor::template out_generator<adjacency_list>
      out_edge_generator;

    /// Iterator over the outgoing edges of a vertex
    typedef transform_iterator<out_edge_generator,
                               typename inherited::out_edge_iterator>
      out_edge_iterator;

    /// Helper for in_edge_iterator
    typedef typename edge_descriptor::template in_generator<adjacency_list>
      in_edge_generator;

    /// Iterator over the incoming edges of a vertex
    typedef transform_iterator<in_edge_generator, base_in_edge_iterator>
      in_edge_iterator;

    /// Iterator over the neighbors of a vertex
    typedef boost::adjacency_iterator<
              adjacency_list, vertex_descriptor, out_edge_iterator,
              typename std::iterator_traits<base_out_edge_iterator>
                         ::difference_type>
      adjacency_iterator;

    /// Iterator over the (local) edges in a graph
    typedef typename boost::mpl::if_<is_same<DirectedS, undirectedS>,
                                     undirected_edge_iterator,
                                     transform_iterator<out_edge_generator,
                                                        base_edge_iterator>
                                     >::type 
      edge_iterator;

  public:
    /// The type of the mixin for named vertices
    typedef graph::distributed::maybe_named_graph<graph_type, 
                                                  vertex_descriptor, 
                                                  edge_descriptor, 
                                                  config_type> 
      named_graph_mixin;
        
    /// Process group used for communication
    typedef ProcessGroup process_group_type;

    /// How to refer to a process
    typedef typename process_group_type::process_id_type process_id_type;

    /// Whether this graph is directed, undirected, or bidirectional
    typedef DirectedS directed_selector;

    // Structure used for the lazy addition of vertices
    struct lazy_add_vertex_with_property;
    friend struct lazy_add_vertex_with_property;

    // Structure used for the lazy addition of edges
    struct lazy_add_edge;
    friend struct lazy_add_edge;

    // Structure used for the lazy addition of edges with properties
    struct lazy_add_edge_with_property;
    friend struct lazy_add_edge_with_property;

    /// default_distribution_type is the type of the distribution used if the
    /// user didn't specify an explicit one
    typedef typename graph::distributed::select_distribution<
              InDistribution, VertexProperty, vertices_size_type, 
              ProcessGroup>::default_type 
      default_distribution_type;
    
    /// distribution_type is the type of the distribution instance stored in
    /// the maybe_named_graph base class
    typedef typename graph::distributed::select_distribution<
              InDistribution, VertexProperty, vertices_size_type,
              ProcessGroup>::type 
      base_distribution_type;

      typedef graph::distributed::shuffled_distribution<
          base_distribution_type> distribution_type;

  private:
    // FIXME: the original adjacency_list contained this comment:
    //    Default copy constructor and copy assignment operators OK??? TBD
    // but the adj_list_impl contained these declarations:
    adjacency_list(const adjacency_list& other);
    adjacency_list& operator=(const adjacency_list& other);

  public:
    adjacency_list(const ProcessGroup& pg = ProcessGroup())
      : named_graph_mixin(pg, default_distribution_type(pg, 0)),
        m_local_graph(GraphProperty()), 
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();
    }

    adjacency_list(const ProcessGroup& pg, 
                   const base_distribution_type& distribution)
      : named_graph_mixin(pg, distribution),
        m_local_graph(GraphProperty()), 
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();
    }

    adjacency_list(const GraphProperty& g,
                   const ProcessGroup& pg = ProcessGroup())
      : named_graph_mixin(pg, default_distribution_type(pg, 0)),
        m_local_graph(g), 
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();
    }

    adjacency_list(vertices_size_type n,
                   const GraphProperty& p,
                   const ProcessGroup& pg,
                   const base_distribution_type& distribution)
      : named_graph_mixin(pg, distribution),
        m_local_graph(distribution.block_size(process_id(pg), n), p),
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();

      detail::parallel::maybe_initialize_vertex_indices(vertices(base()),
                                      get(vertex_index, base()));
    }

    adjacency_list(vertices_size_type n,
                   const ProcessGroup& pg,
                   const base_distribution_type& distribution)
      : named_graph_mixin(pg, distribution),
        m_local_graph(distribution.block_size(process_id(pg), n), GraphProperty()),
        process_group_(pg, boost::parallel::attach_distributed_object()) 
    {
      setup_triggers();

      detail::parallel::maybe_initialize_vertex_indices(vertices(base()),
                                      get(vertex_index, base()));
    }

    adjacency_list(vertices_size_type n,
                   const GraphProperty& p,
                   const ProcessGroup& pg = ProcessGroup())
      : named_graph_mixin(pg, default_distribution_type(pg, n)),
        m_local_graph(this->distribution().block_size(process_id(pg), n), p),
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();

      detail::parallel::maybe_initialize_vertex_indices(vertices(base()),
                                      get(vertex_index, base()));
    }

    adjacency_list(vertices_size_type n,
                   const ProcessGroup& pg = ProcessGroup())
      : named_graph_mixin(pg, default_distribution_type(pg, n)),
        m_local_graph(this->distribution().block_size(process_id(pg), n), 
                      GraphProperty()),
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();

      detail::parallel::maybe_initialize_vertex_indices(vertices(base()),
                                      get(vertex_index, base()));
    }

    /*
     * We assume that every processor sees the same list of edges, so
     * they skip over any that don't originate from themselves. This
     * means that programs switching between a local and a distributed
     * graph will keep the same semantics.
     */
    template <class EdgeIterator>
    adjacency_list(EdgeIterator first, EdgeIterator last,
                   vertices_size_type n,
                   const ProcessGroup& pg = ProcessGroup(),
                   const GraphProperty& p = GraphProperty())
      : named_graph_mixin(pg, default_distribution_type(pg, n)),
        m_local_graph(this->distribution().block_size(process_id(pg), n), p),
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();

      typedef typename config_type::VertexListS vertex_list_selector;
      initialize(first, last, n, this->distribution(), vertex_list_selector());
      detail::parallel::maybe_initialize_vertex_indices(vertices(base()),
                                      get(vertex_index, base()));

    }

    template <class EdgeIterator, class EdgePropertyIterator>
    adjacency_list(EdgeIterator first, EdgeIterator last,
                   EdgePropertyIterator ep_iter,
                   vertices_size_type n,
                   const ProcessGroup& pg = ProcessGroup(),
                   const GraphProperty& p = GraphProperty())
      : named_graph_mixin(pg, default_distribution_type(pg, n)),
        m_local_graph(this->distribution().block_size(process_id(pg), n), p),
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();

      typedef typename config_type::VertexListS vertex_list_selector;
      initialize(first, last, ep_iter, n, this->distribution(),
                 vertex_list_selector());
      detail::parallel::maybe_initialize_vertex_indices(vertices(base()),
                                      get(vertex_index, base()));

    }

    template <class EdgeIterator>
    adjacency_list(EdgeIterator first, EdgeIterator last,
                   vertices_size_type n,
                   const ProcessGroup& pg,
                   const base_distribution_type& distribution,
                   const GraphProperty& p = GraphProperty())
      : named_graph_mixin(pg, distribution),
        m_local_graph(distribution.block_size(process_id(pg), n), p),
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();

      typedef typename config_type::VertexListS vertex_list_selector;
      initialize(first, last, n, this->distribution(), vertex_list_selector());
      detail::parallel::maybe_initialize_vertex_indices(vertices(base()),
                                      get(vertex_index, base()));

    }

    template <class EdgeIterator, class EdgePropertyIterator>
    adjacency_list(EdgeIterator first, EdgeIterator last,
                   EdgePropertyIterator ep_iter,
                   vertices_size_type n,
                   const ProcessGroup& pg,
                   const base_distribution_type& distribution,
                   const GraphProperty& p = GraphProperty())
      : named_graph_mixin(pg, distribution),
        m_local_graph(this->distribution().block_size(process_id(pg), n), p),
        process_group_(pg, boost::parallel::attach_distributed_object())
    {
      setup_triggers();

      typedef typename config_type::VertexListS vertex_list_selector;
      initialize(first, last, ep_iter, n, distribution,
                 vertex_list_selector());
      detail::parallel::maybe_initialize_vertex_indices(vertices(base()),
                                      get(vertex_index, base()));

    }

    ~adjacency_list()
    {
      synchronize(process_group_);
    }

    void clear()
    {
      base().clear();
      local_edges_.clear();
      named_graph_mixin::clearing_graph();
    }

    void swap(adjacency_list& other)
    {
      using std::swap;

      base().swap(other);
      swap(process_group_, other.process_group_);
    }

    static vertex_descriptor null_vertex()
    {
      return vertex_descriptor(processor_id_type(0),
                               inherited::null_vertex());
    }

    inherited&       base()       { return m_local_graph; }
    const inherited& base() const { return m_local_graph; }

    processor_id_type processor() const { return process_id(process_group_); }
    process_group_type process_group() const { return process_group_.base(); }

    local_edge_list_type&       local_edges()       { return local_edges_; }
    const local_edge_list_type& local_edges() const { return local_edges_; }

    // Redistribute the vertices of the graph by placing each vertex
    // v on the processor get(vertex_to_processor, v).
    template<typename VertexProcessorMap>
    void redistribute(VertexProcessorMap vertex_to_processor);

    // Directly access a vertex or edge bundle
    vertex_bundled& operator[](vertex_descriptor v)
    {
      BOOST_ASSERT(v.owner == processor());
      return base()[v.local];
    }
    
    const vertex_bundled& operator[](vertex_descriptor v) const
    {
      BOOST_ASSERT(v.owner == processor());
      return base()[v.local];
    }
    
    edge_bundled& operator[](edge_descriptor e)
    {
      BOOST_ASSERT(e.owner() == processor());
      return base()[e.local];
    }
    
    const edge_bundled& operator[](edge_descriptor e) const
    {
      BOOST_ASSERT(e.owner() == processor());
      return base()[e.local];
    }

    graph_bundled& operator[](graph_bundle_t)
    { return get_property(*this); }

    graph_bundled const& operator[](graph_bundle_t) const
    { return get_property(*this); }

    template<typename OStreamConstructibleArchive>
    void save(std::string const& filename) const;

    template<typename IStreamConstructibleArchive>
    void load(std::string const& filename);

    // Callback that will be invoked whenever a new vertex is added locally
    boost::function<void(vertex_descriptor, adjacency_list&)> on_add_vertex;

    // Callback that will be invoked whenever a new edge is added locally
    boost::function<void(edge_descriptor, adjacency_list&)> on_add_edge;

  private:
    // Request vertex->processor mapping for neighbors <does nothing>
    template<typename VertexProcessorMap>
    void
    request_in_neighbors(vertex_descriptor,
                         VertexProcessorMap,
                         directedS) { }

    // Request vertex->processor mapping for neighbors <does nothing>
    template<typename VertexProcessorMap>
    void
    request_in_neighbors(vertex_descriptor,
                         VertexProcessorMap,
                         undirectedS) { }

    // Request vertex->processor mapping for neighbors
    template<typename VertexProcessorMap>
    void
    request_in_neighbors(vertex_descriptor v,
                         VertexProcessorMap vertex_to_processor,
                         bidirectionalS);

    // Clear the list of in-edges, but don't tell the remote processor
    void clear_in_edges_local(vertex_descriptor v, directedS) {}
    void clear_in_edges_local(vertex_descriptor v, undirectedS) {}

    void clear_in_edges_local(vertex_descriptor v, bidirectionalS)
    { get(vertex_in_edges, base())[v.local].clear(); }

    // Remove in-edges that have migrated <does nothing>
    template<typename VertexProcessorMap>
    void
    remove_migrated_in_edges(vertex_descriptor,
                             VertexProcessorMap,
                             directedS) { }

    // Remove in-edges that have migrated <does nothing>
    template<typename VertexProcessorMap>
    void
    remove_migrated_in_edges(vertex_descriptor,
                             VertexProcessorMap,
                             undirectedS) { }

    // Remove in-edges that have migrated
    template<typename VertexProcessorMap>
    void
    remove_migrated_in_edges(vertex_descriptor v,
                             VertexProcessorMap vertex_to_processor,
                             bidirectionalS);

    // Initialize the graph with the given edge list and vertex
    // distribution. This variation works only when
    // VertexListS=vecS, and we know how to create remote vertex
    // descriptors based solely on the distribution.
    template<typename EdgeIterator>
    void
    initialize(EdgeIterator first, EdgeIterator last,
               vertices_size_type, const base_distribution_type& distribution, 
               vecS);

    // Initialize the graph with the given edge list, edge
    // properties, and vertex distribution. This variation works
    // only when VertexListS=vecS, and we know how to create remote
    // vertex descriptors based solely on the distribution.
    template<typename EdgeIterator, typename EdgePropertyIterator>
    void
    initialize(EdgeIterator first, EdgeIterator last,
               EdgePropertyIterator ep_iter,
               vertices_size_type, const base_distribution_type& distribution, 
               vecS);

    // Initialize the graph with the given edge list, edge
    // properties, and vertex distribution.
    template<typename EdgeIterator, typename EdgePropertyIterator,
             typename VertexListS>
    void
    initialize(EdgeIterator first, EdgeIterator last,
               EdgePropertyIterator ep_iter,
               vertices_size_type n, 
               const base_distribution_type& distribution,
               VertexListS);

    // Initialize the graph with the given edge list and vertex
    // distribution. This is nearly identical to the one below it,
    // for which I should be flogged. However, this version does use
    // slightly less memory than the version that accepts an edge
    // property iterator.
    template<typename EdgeIterator, typename VertexListS>
    void
    initialize(EdgeIterator first, EdgeIterator last,
               vertices_size_type n, 
               const base_distribution_type& distribution,
               VertexListS);

  public:
    //---------------------------------------------------------------------
    // Build a vertex property instance for the underlying adjacency
    // list from the given property instance of the type exposed to
    // the user.
    base_vertex_property_type 
    build_vertex_property(const vertex_property_type& p)
    { return build_vertex_property(p, directed_selector()); }

    base_vertex_property_type
    build_vertex_property(const vertex_property_type& p, directedS)
    {
      return base_vertex_property_type(p);
    }

    base_vertex_property_type
    build_vertex_property(const vertex_property_type& p, bidirectionalS)
    {
      return base_vertex_property_type(in_edge_list_type(), p);
    }

    base_vertex_property_type
    build_vertex_property(const vertex_property_type& p, undirectedS)
    {
      return base_vertex_property_type(p);
    }
    //---------------------------------------------------------------------

    //---------------------------------------------------------------------
    // Build an edge property instance for the underlying adjacency
    // list from the given property instance of the type exposed to
    // the user.
    base_edge_property_type build_edge_property(const edge_property_type& p)
    { return build_edge_property(p, directed_selector()); }

    base_edge_property_type
    build_edge_property(const edge_property_type& p, directedS)
    {
      return base_edge_property_type(0, p);
    }

    base_edge_property_type
    build_edge_property(const edge_property_type& p, bidirectionalS)
    {
      return base_edge_property_type(0, p);
    }

    base_edge_property_type
    build_edge_property(const edge_property_type& p, undirectedS)
    {
      typedef typename base_edge_property_type::next_type
        edge_property_with_id;
      return base_edge_property_type(true, edge_property_with_id(0, p));
    }
    //---------------------------------------------------------------------

    //---------------------------------------------------------------------
    // Opposite of above.
    edge_property_type split_edge_property(const base_edge_property_type& p)
    { return split_edge_property(p, directed_selector()); }

    edge_property_type
    split_edge_property(const base_edge_property_type& p, directedS)
    {
      return p.m_base;
    }

    edge_property_type
    split_edge_property(const base_edge_property_type& p, bidirectionalS)
    {
      return p.m_base;
    }

    edge_property_type
    split_edge_property(const base_edge_property_type& p, undirectedS)
    {
      return p.m_base.m_base;
    }
    //---------------------------------------------------------------------

    /** The set of messages that can be transmitted and received by
     *  a distributed adjacency list. This list will eventually be
     *  exhaustive, but is currently quite limited.
     */
    enum {
      /**
       * Request to add or find a vertex with the given vertex
       * property. The data will be a vertex_property_type
       * structure.
       */
      msg_add_vertex_with_property = 0,

      /**
       * Request to add or find a vertex with the given vertex
       * property, and request that the remote processor return the
       * descriptor for the added/found edge. The data will be a
       * vertex_property_type structure.
       */
      msg_add_vertex_with_property_and_reply,

      /**
       * Reply to a msg_add_vertex_* message, containing the local
       * vertex descriptor that was added or found.
       */
      msg_add_vertex_reply,

      /**
       * Request to add an edge remotely. The data will be a
       * msg_add_edge_data structure. 
       */
      msg_add_edge,

      /**
       * Request to add an edge remotely. The data will be a
       * msg_add_edge_with_property_data structure. 
       */
      msg_add_edge_with_property,

      /**
       * Request to add an edge remotely and reply back with the
       * edge descriptor. The data will be a
       * msg_add_edge_data structure. 
       */
      msg_add_edge_with_reply,

      /**
       * Request to add an edge remotely and reply back with the
       * edge descriptor. The data will be a
       * msg_add_edge_with_property_data structure.
       */
      msg_add_edge_with_property_and_reply,

      /**
       * Reply message responding to an @c msg_add_edge_with_reply
       * or @c msg_add_edge_with_property_and_reply messages. The
       * data will be a std::pair<edge_descriptor, bool>.
       */
      msg_add_edge_reply,

      /**
       * Indicates that a nonlocal edge has been created that should
       * be added locally. Only valid for bidirectional and
       * undirected graphs. The message carries a
       * msg_nonlocal_edge_data structure.
       */
      msg_nonlocal_edge,

      /**
       * Indicates that a remote edge should be removed. This
       * message does not exist for directedS graphs but may refer
       * to either in-edges or out-edges for undirectedS graphs.
       */
      msg_remove_edge,

      /**
       * Indicates the number of vertices and edges that will be
       * relocated from the source processor to the target
       * processor. The data will be a pair<vertices_size_type,
       * edges_size_type>.
       */
      msg_num_relocated
    };

    typedef detail::parallel::msg_add_edge_data<vertex_descriptor,
                                                local_vertex_descriptor>
      msg_add_edge_data;

    typedef detail::parallel::msg_add_edge_with_property_data
              <vertex_descriptor, local_vertex_descriptor, 
               edge_property_type> msg_add_edge_with_property_data;

    typedef  boost::detail::parallel::msg_nonlocal_edge_data<
      edge_property_type,local_edge_descriptor> msg_nonlocal_edge_data;

    typedef boost::detail::parallel::msg_remove_edge_data<edge_descriptor>
      msg_remove_edge_data;

    void send_remove_edge_request(edge_descriptor e)
    {
      process_id_type dest = e.target_processor;
      if (e.target_processor == process_id(process_group_))
        dest = e.source_processor;
      send(process_group_, dest, msg_remove_edge, msg_remove_edge_data(e));
    }

    /// Process incoming messages.
    void setup_triggers();

    void 
    handle_add_vertex_with_property(int source, int tag,
                                    const vertex_property_type&,
                                    trigger_receive_context);

    local_vertex_descriptor
    handle_add_vertex_with_property_and_reply(int source, int tag,
                                        const vertex_property_type&,
                                        trigger_receive_context);

    void 
    handle_add_edge(int source, int tag, const msg_add_edge_data& data,
                    trigger_receive_context);

    boost::parallel::detail::untracked_pair<edge_descriptor, bool>
    handle_add_edge_with_reply(int source, int tag, 
                         const msg_add_edge_data& data,
                         trigger_receive_context);

    void 
    handle_add_edge_with_property(int source, int tag,
                                  const msg_add_edge_with_property_data&,
                                  trigger_receive_context);
              
    boost::parallel::detail::untracked_pair<edge_descriptor, bool>
    handle_add_edge_with_property_and_reply
      (int source, int tag, const msg_add_edge_with_property_data&,
       trigger_receive_context);

    void 
    handle_nonlocal_edge(int source, int tag, 
                         const msg_nonlocal_edge_data& data,
                         trigger_receive_context);

    void 
    handle_remove_edge(int source, int tag, 
                       const msg_remove_edge_data& data,
                       trigger_receive_context);
         
  protected:
    /** Add an edge (locally) that was received from another
     * processor. This operation is a no-op for directed graphs,
     * because all edges reside on the local processor. For
     * bidirectional graphs, this routine places the edge onto the
     * list of incoming edges for the target vertex. For undirected
     * graphs, the edge is placed along with all of the other edges
     * for the target vertex, but it is marked as a non-local edge
     * descriptor.
     *
     * \todo There is a potential problem here, where we could
     * unintentionally allow duplicate edges in undirected graphs
     * because the same edge is added on two different processors
     * simultaneously. It's not an issue now, because we require
     * that the graph allow parallel edges. Once we do support
     * containers such as setS or hash_setS that disallow parallel
     * edges we will need to deal with this.
     */
    void
    add_remote_edge(const msg_nonlocal_edge_data&,
                    processor_id_type, directedS)
    { }


    /**
     * \overload
     */
    void
    add_remote_edge(const msg_nonlocal_edge_data& data,
                    processor_id_type other_proc, bidirectionalS)
    {
      typedef detail::parallel::stored_in_edge<local_edge_descriptor> stored_edge;

      stored_edge edge(other_proc, data.e);
      local_vertex_descriptor v = target(data.e, base());
      boost::graph_detail::push(get(vertex_in_edges, base())[v], edge);
    }

    /**
     * \overload
     */
    void
    add_remote_edge(const msg_nonlocal_edge_data& data,
                    processor_id_type other_proc, undirectedS)
    {
      std::pair<local_edge_descriptor, bool> edge =
        detail::parallel::add_local_edge(target(data.e, base()), 
                       source(data.e, base()),
                       build_edge_property(data.get_property()), base());
      BOOST_ASSERT(edge.second);
      put(edge_target_processor_id, base(), edge.first, other_proc);

      if (edge.second && on_add_edge)
        on_add_edge(edge_descriptor(processor(), other_proc, false, 
                                    edge.first),
                    *this);
    }

    void
    remove_local_edge(const msg_remove_edge_data&, processor_id_type,
                      directedS)
    { }

    void
    remove_local_edge(const msg_remove_edge_data& data,
                      processor_id_type other_proc, bidirectionalS)
    {
      /* When the source is local, we first check if the edge still
       * exists (it may have been deleted locally) and, if so,
       * remove it locally.
       */
      vertex_descriptor src = source(data.e, *this);
      vertex_descriptor tgt = target(data.e, *this);

      if (src.owner == process_id(process_group_)) {
        base_out_edge_iterator ei, ei_end;
        for (boost::tie(ei, ei_end) = out_edges(src.local, base());
             ei != ei_end; ++ei) {
          // TBD: can't check the descriptor here, because it could
          // have changed if we're allowing the removal of
          // edges. Egads!
          if (tgt.local == target(*ei, base())
              && get(edge_target_processor_id, base(), *ei) == other_proc)
            break;
        }

        if (ei != ei_end) boost::remove_edge(ei, base());

        remove_local_edge_from_list(src, tgt, undirectedS());
      } else {
        BOOST_ASSERT(tgt.owner == process_id(process_group_));
        in_edge_list_type& in_edges =
          get(vertex_in_edges, base())[tgt.local];
        typename in_edge_list_type::iterator ei;
        for (ei = in_edges.begin(); ei != in_edges.end(); ++ei) {
          if (src.local == source(ei->e, base())
              && src.owner == ei->source_processor)
            break;
        }

        if (ei != in_edges.end()) in_edges.erase(ei);
      }
    }

    void
    remove_local_edge(const msg_remove_edge_data& data,
                      processor_id_type other_proc, undirectedS)
    {
      vertex_descriptor local_vertex = source(data.e, *this);
      vertex_descriptor remote_vertex = target(data.e, *this);
      if (remote_vertex.owner == process_id(process_group_)) {
        using std::swap;
        swap(local_vertex, remote_vertex);
      }

      // Remove the edge from the out-edge list, if it is there
      {
        base_out_edge_iterator ei, ei_end;
        for (boost::tie(ei, ei_end) = out_edges(local_vertex.local, base());
             ei != ei_end; ++ei) {
          // TBD: can't check the descriptor here, because it could
          // have changed if we're allowing the removal of
          // edges. Egads!
          if (remote_vertex.local == target(*ei, base())
              && get(edge_target_processor_id, base(), *ei) == other_proc)
            break;
        }

        if (ei != ei_end) boost::remove_edge(ei, base());
      }

      remove_local_edge_from_list(local_vertex, remote_vertex, undirectedS());
    }

  public:
    void
    remove_local_edge_from_list(vertex_descriptor, vertex_descriptor,
                                directedS)
    {
    }

    void
    remove_local_edge_from_list(vertex_descriptor, vertex_descriptor,
                                bidirectionalS)
    {
    }

    void
    remove_local_edge_from_list(vertex_descriptor src, vertex_descriptor tgt,
                                undirectedS)
    {
      // TBD: At some point we'll be able to improve the speed here
      // because we'll know when the edge can't be in the local
      // list.
      {
        typename local_edge_list_type::iterator ei;
        for (ei = local_edges_.begin(); ei != local_edges_.end(); ++ei) {
          if ((source(*ei, *this) == src
               && target(*ei, *this) == tgt)
              || (source(*ei, *this) == tgt
                  && target(*ei, *this) == src))
            break;
        }

        if (ei != local_edges_.end()) local_edges_.erase(ei);
      }

    }

  private:
    /// The local subgraph
    inherited m_local_graph;

    /// The process group through which this distributed graph
    /// communicates.
    process_group_type process_group_;

    // TBD: should only be available for undirected graphs, but for
    // now it'll just be empty for directed and bidirectional
    // graphs.
    local_edge_list_type local_edges_;
  };

  //------------------------------------------------------------------------
  // Lazy addition of vertices
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  struct PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_vertex_with_property
  {
    /// Construct a lazy request to add a vertex
    lazy_add_vertex_with_property(adjacency_list& self, 
                                  const vertex_property_type& property)
      : self(self), property(property), committed(false) { }

    /// Copying a lazy_add_vertex_with_property transfers the
    /// responsibility for adding the vertex to the newly-constructed
    /// object.
    lazy_add_vertex_with_property(const lazy_add_vertex_with_property& other)
      : self(other.self), property(other.property),
        committed(other.committed)
    {
      other.committed = true;
    }

    /// If the vertex has not yet been added, add the vertex but don't
    /// wait for a reply.
    ~lazy_add_vertex_with_property();

    /// Returns commit().
    operator vertex_descriptor() const { return commit(); }

    // Add the vertex. This operation will block if the vertex is
    // being added remotely.
    vertex_descriptor commit() const;

  protected:
    adjacency_list& self;
    vertex_property_type property;
    mutable bool committed;

  private:
    // No copy-assignment semantics
    void operator=(lazy_add_vertex_with_property&);
  };

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_vertex_with_property::
  ~lazy_add_vertex_with_property()
  {
    /// If this vertex has already been created or will be created by
    /// someone else, or if someone threw an exception, we will not
    /// create the vertex now.
    if (committed || boost::core::uncaught_exceptions() > 0)
      return;

    committed = true;

    process_id_type owner 
      = static_cast<graph_type&>(self).owner_by_property(property);
    if (owner == self.processor()) {
      /// Add the vertex locally.
      vertex_descriptor v(owner, 
                          add_vertex(self.build_vertex_property(property), 
                                     self.base()));
      if (self.on_add_vertex)
        self.on_add_vertex(v, self);
    }
    else
      /// Ask the owner of this new vertex to add the vertex. We
      /// don't need a reply.
      send(self.process_group_, owner, msg_add_vertex_with_property,
           property);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor 
  PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_vertex_with_property::
  commit() const
  {
    BOOST_ASSERT(!this->committed);
    this->committed = true;

    process_id_type owner 
      = static_cast<graph_type&>(self).owner_by_property(property);
    local_vertex_descriptor local_v;
    if (owner == self.processor())
      /// Add the vertex locally.
      local_v = add_vertex(self.build_vertex_property(property), 
                           self.base());
    else {
      // Request that the remote process add the vertex immediately
      send_oob_with_reply(self.process_group_, owner,
               msg_add_vertex_with_property_and_reply, property,
               local_v);
    }

    vertex_descriptor v(owner, local_v);
    if (self.on_add_vertex)
      self.on_add_vertex(v, self);

    // Build the full vertex descriptor to return
    return v;
  }
  

  /** 
   * Data structure returned from add_edge that will "lazily" add
   * the edge, either when it is converted to a
   * @c pair<edge_descriptor, bool> or when the most recent copy has
   * been destroyed.
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  struct PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge
  {
    /// Construct a lazy request to add an edge
    lazy_add_edge(adjacency_list& self, 
                  vertex_descriptor source, vertex_descriptor target)
      : self(self), source(source), target(target), committed(false) { }

    /// Copying a lazy_add_edge transfers the responsibility for
    /// adding the edge to the newly-constructed object.
    lazy_add_edge(const lazy_add_edge& other)
      : self(other.self), source(other.source), target(other.target), 
        committed(other.committed)
    {
      other.committed = true;
    }

    /// If the edge has not yet been added, add the edge but don't
    /// wait for a reply.
    ~lazy_add_edge();

    /// Returns commit().
    operator std::pair<edge_descriptor, bool>() const { return commit(); }

    // Add the edge. This operation will block if a remote edge is
    // being added.
    std::pair<edge_descriptor, bool> commit() const;

  protected:
    std::pair<edge_descriptor, bool> 
    add_local_edge(const edge_property_type& property, directedS) const;

    std::pair<edge_descriptor, bool> 
    add_local_edge(const edge_property_type& property, bidirectionalS) const;

    std::pair<edge_descriptor, bool> 
    add_local_edge(const edge_property_type& property, undirectedS) const;

    adjacency_list& self;
    vertex_descriptor source;
    vertex_descriptor target;
    mutable bool committed;

  private:
    // No copy-assignment semantics
    void operator=(lazy_add_edge&);
  };

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge::~lazy_add_edge()
  {
    /// If this edge has already been created or will be created by
    /// someone else, or if someone threw an exception, we will not
    /// create the edge now.
    if (committed || boost::core::uncaught_exceptions() > 0)
      return;

    committed = true;

    if (source.owner == self.processor())
      this->add_local_edge(edge_property_type(), DirectedS());
    else
      // Request that the remote processor add an edge and, but
      // don't wait for a reply.
      send(self.process_group_, source.owner, msg_add_edge,
           msg_add_edge_data(source, target));
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor, bool>
  PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge::commit() const
  {
    BOOST_ASSERT(!committed);
    committed = true;

    if (source.owner == self.processor())
      return this->add_local_edge(edge_property_type(), DirectedS());
    else {
      // Request that the remote processor add an edge
      boost::parallel::detail::untracked_pair<edge_descriptor, bool> result;
      send_oob_with_reply(self.process_group_, source.owner, 
                          msg_add_edge_with_reply,
                          msg_add_edge_data(source, target), result);
      return result;
    }
  }

  // Add a local edge into a directed graph
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor, bool>
  PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge::
  add_local_edge(const edge_property_type& property, directedS) const
  {
    // Add the edge to the local part of the graph
    std::pair<local_edge_descriptor, bool> inserted =
      detail::parallel::add_local_edge(source.local, target.local,
                                       self.build_edge_property(property), 
                                       self.base());

    if (inserted.second)
      // Keep track of the owner of the target
      put(edge_target_processor_id, self.base(), inserted.first, 
          target.owner);

    // Compose the edge descriptor and return the result
    edge_descriptor e(source.owner, target.owner, true, inserted.first);

    // Trigger the on_add_edge event
    if (inserted.second && self.on_add_edge)
      self.on_add_edge(e, self);

    return std::pair<edge_descriptor, bool>(e, inserted.second);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor, bool>
  PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge::
  add_local_edge(const edge_property_type& property, bidirectionalS) const
  {
    // Add the directed edge.
    std::pair<edge_descriptor, bool> result 
      = this->add_local_edge(property, directedS());

    if (result.second) {
      if (target.owner == self.processor()) {
        // Edge is local, so add the stored edge to the in_edges list
        typedef detail::parallel::stored_in_edge<local_edge_descriptor>
          stored_edge;

        stored_edge e(self.processor(), result.first.local);
        boost::graph_detail::push(get(vertex_in_edges, 
                                      self.base())[target.local], e);
      } 
      else {
        // Edge is remote, so notify the target's owner that an edge
        // has been added.
        if (self.process_group_.trigger_context() == boost::parallel::trc_out_of_band)
          send_oob(self.process_group_, target.owner, msg_nonlocal_edge,
                   msg_nonlocal_edge_data(result.first.local, property));
        else
          send(self.process_group_, target.owner, msg_nonlocal_edge,
               msg_nonlocal_edge_data(result.first.local, property));
      }
    }

    return result;
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor, bool>
  PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge::
  add_local_edge(const edge_property_type& property, undirectedS) const
  {
    // Add the directed edge
    std::pair<edge_descriptor, bool> result
      = this->add_local_edge(property, directedS());

    if (result.second) {
      if (target.owner == self.processor()) {
        // Edge is local, so add the new edge to the list

        // TODO: This is not what we want to do for an undirected
        // edge, because we haven't linked the source and target's
        // representations of those edges.
        local_edge_descriptor return_edge =
          detail::parallel::add_local_edge(target.local, source.local,
                                           self.build_edge_property(property),
                                           self.base()).first;

        put(edge_target_processor_id, self.base(), return_edge, 
            source.owner);
      }
      else {
        // Edge is remote, so notify the target's owner that an edge
        // has been added.
        if (self.process_group_.trigger_context() == boost::parallel::trc_out_of_band)
          send_oob(self.process_group_, target.owner, msg_nonlocal_edge,
                   msg_nonlocal_edge_data(result.first.local, property));
        else
          send(self.process_group_, target.owner, msg_nonlocal_edge,
               msg_nonlocal_edge_data(result.first.local, property));
          
      }

      // Add this edge to the list of local edges
      graph_detail::push(self.local_edges(), result.first);
    }

    return result;
  }


  /** 
   * Data structure returned from add_edge that will "lazily" add
   * the edge with its property, either when it is converted to a
   * pair<edge_descriptor, bool> or when the most recent copy has
   * been destroyed.
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  struct PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge_with_property
    : lazy_add_edge
  {
    /// Construct a lazy request to add an edge
    lazy_add_edge_with_property(adjacency_list& self, 
                                vertex_descriptor source, 
                                vertex_descriptor target,
                                const edge_property_type& property)
      : lazy_add_edge(self, source, target), property(property) { }

    /// Copying a lazy_add_edge transfers the responsibility for
    /// adding the edge to the newly-constructed object.
    lazy_add_edge_with_property(const lazy_add_edge& other)
      : lazy_add_edge(other), property(other.property) { }

    /// If the edge has not yet been added, add the edge but don't
    /// wait for a reply.
    ~lazy_add_edge_with_property();

    /// Returns commit().
    operator std::pair<edge_descriptor, bool>() const { return commit(); }

    // Add the edge. This operation will block if a remote edge is
    // being added.
    std::pair<edge_descriptor, bool> commit() const;

  private:
    // No copy-assignment semantics
    void operator=(lazy_add_edge_with_property&);

    edge_property_type property;
  };

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge_with_property::
  ~lazy_add_edge_with_property()
  {
    /// If this edge has already been created or will be created by
    /// someone else, or if someone threw an exception, we will not
    /// create the edge now.
    if (this->committed || boost::core::uncaught_exceptions() > 0)
      return;

    this->committed = true;

    if (this->source.owner == this->self.processor())
      // Add a local edge
      this->add_local_edge(property, DirectedS());
    else
      // Request that the remote processor add an edge and, but
      // don't wait for a reply.
      send(this->self.process_group_, this->source.owner, 
           msg_add_edge_with_property,
           msg_add_edge_with_property_data(this->source, this->target, 
                                           property));
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor, bool>
  PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge_with_property::
  commit() const
  {
    BOOST_ASSERT(!this->committed);
    this->committed = true;

    if (this->source.owner == this->self.processor())
      // Add a local edge
      return this->add_local_edge(property, DirectedS());
    else {
      // Request that the remote processor add an edge
      boost::parallel::detail::untracked_pair<edge_descriptor, bool> result;
      send_oob_with_reply(this->self.process_group_, this->source.owner, 
                          msg_add_edge_with_property_and_reply,
                          msg_add_edge_with_property_data(this->source, 
                                                          this->target, 
                                                          property),
                          result);
      return result;
    }
  }


  /**
   * Returns the set of vertices local to this processor. Note that
   * although this routine matches a valid expression of a
   * VertexListGraph, it does not meet the semantic requirements of
   * VertexListGraph because it returns only local vertices (not all
   * vertices).
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE
                       ::vertex_iterator,
            typename PBGL_DISTRIB_ADJLIST_TYPE
                       ::vertex_iterator>
  vertices(const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE
      ::vertex_descriptor Vertex;

    typedef typename Vertex::generator generator;

    return std::make_pair(make_transform_iterator(vertices(g.base()).first,
                                                  generator(g.processor())),
                          make_transform_iterator(vertices(g.base()).second,
                                                  generator(g.processor())));
  }

  /**
   * Returns the number of vertices local to this processor. Note that
   * although this routine matches a valid expression of a
   * VertexListGraph, it does not meet the semantic requirements of
   * VertexListGraph because it returns only a count of local vertices
   * (not all vertices).
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename PBGL_DISTRIB_ADJLIST_TYPE
             ::vertices_size_type
  num_vertices(const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    return num_vertices(g.base());
  }

  /***************************************************************************
   * Implementation of Incidence Graph concept
   ***************************************************************************/
  /**
   * Returns the source of edge @param e in @param g.
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS, typename Edge>
  typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor
  source(const detail::parallel::edge_descriptor<Edge>& e,
         const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE
      ::vertex_descriptor Vertex;
    return Vertex(e.source_processor, source(e.local, g.base()));
  }

  /**
   * Returns the target of edge @param e in @param g.
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS, typename Edge>
  typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor
  target(const detail::parallel::edge_descriptor<Edge>& e,
         const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE
      ::vertex_descriptor Vertex;
    return Vertex(e.target_processor, target(e.local, g.base()));
  }

  /**
   * Return the set of edges outgoing from a particular vertex. The
   * vertex @param v must be local to the processor executing this
   * routine.
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE::out_edge_iterator,
            typename PBGL_DISTRIB_ADJLIST_TYPE::out_edge_iterator>
  out_edges(typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v,
            const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    BOOST_ASSERT(v.owner == g.processor());

    typedef PBGL_DISTRIB_ADJLIST_TYPE impl;
    typedef typename impl::out_edge_generator generator;

    return std::make_pair(
             make_transform_iterator(out_edges(v.local, g.base()).first,
                                     generator(g)),
             make_transform_iterator(out_edges(v.local, g.base()).second,
                                     generator(g)));
  }

  /**
   * Return the number of edges outgoing from a particular vertex. The
   * vertex @param v must be local to the processor executing this
   * routine.
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename PBGL_DISTRIB_ADJLIST_TYPE::degree_size_type
  out_degree(typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v,
             const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    BOOST_ASSERT(v.owner == g.processor());

    return out_degree(v.local, g.base());
  }

  /***************************************************************************
   * Implementation of Bidirectional Graph concept
   ***************************************************************************/
  /**
   * Returns the set of edges incoming to a particular vertex. The
   * vertex @param v must be local to the processor executing this
   * routine.
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
                       ::in_edge_iterator,
            typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
                       ::in_edge_iterator>
  in_edges(typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
                         ::vertex_descriptor v,
           const PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)& g)
  {
    BOOST_ASSERT(v.owner == g.processor());

    typedef PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS) impl;
    typedef typename impl::inherited base_graph_type;
    typedef typename impl::in_edge_generator generator;


    typename property_map<base_graph_type, vertex_in_edges_t>::const_type
      in_edges = get(vertex_in_edges, g.base());

    return std::make_pair(make_transform_iterator(in_edges[v.local].begin(),
                                                  generator(g)),
                          make_transform_iterator(in_edges[v.local].end(),
                                                  generator(g)));
  }

  /**
   * \overload
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)
                       ::in_edge_iterator,
            typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)
                       ::in_edge_iterator>
  in_edges(typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)
                         ::vertex_descriptor v,
           const PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)& g)
  {
    BOOST_ASSERT(v.owner == g.processor());

    typedef PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS) impl;
    typedef typename impl::in_edge_generator generator;

    return std::make_pair(
              make_transform_iterator(out_edges(v.local, g.base()).first,
                                     generator(g)),
             make_transform_iterator(out_edges(v.local, g.base()).second,
                                     generator(g)));
  }

  /**
   * Returns the number of edges incoming to a particular vertex. The
   * vertex @param v must be local to the processor executing this
   * routine.
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)::degree_size_type
  in_degree(typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
                           ::vertex_descriptor v,
            const PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)& g)
  {
    BOOST_ASSERT(v.owner == g.processor());

    return get(vertex_in_edges, g.base())[v.local].size();
  }

  /**
   * \overload
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)::degree_size_type
  in_degree(typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)
                           ::vertex_descriptor v,
            const PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)& g)
  {
    BOOST_ASSERT(v.owner == g.processor());

    return out_degree(v.local, g.base());
  }

  /**
   * Returns the number of edges incident on the given vertex. The
   * vertex @param v must be local to the processor executing this
   * routine.
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)
                       ::degree_size_type
  degree(typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)
                         ::vertex_descriptor v,
         const PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)& g)
  {
    BOOST_ASSERT(v.owner == g.processor());
    return out_degree(v.local, g.base());
  }

  /**
   * \overload
   */
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
                       ::degree_size_type
  degree(typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
                         ::vertex_descriptor v,
         const PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)& g)
  {
    BOOST_ASSERT(v.owner == g.processor());
    return out_degree(v, g) + in_degree(v, g);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename PBGL_DISTRIB_ADJLIST_TYPE::edges_size_type
  num_edges(const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    return num_edges(g.base());
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)::edges_size_type
  num_edges(const PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)& g)
  {
    return g.local_edges().size();
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<
    typename PBGL_DISTRIB_ADJLIST_TYPE::edge_iterator,
    typename PBGL_DISTRIB_ADJLIST_TYPE::edge_iterator>
  edges(const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE impl;
    typedef typename impl::out_edge_generator generator;

    return std::make_pair(make_transform_iterator(edges(g.base()).first,
                                                  generator(g)),
                          make_transform_iterator(edges(g.base()).second,
                                                  generator(g)));
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  std::pair<
    typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)::edge_iterator,
    typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)::edge_iterator>
  edges(const PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)& g)
  {
    return std::make_pair(g.local_edges().begin(), g.local_edges().end());
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  inline
  typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor
  vertex(typename PBGL_DISTRIB_ADJLIST_TYPE::vertices_size_type n,
         const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor 
      vertex_descriptor;

    return vertex_descriptor(g.distribution()(n), g.distribution().local(n));
  }

  /***************************************************************************
   * Access to particular edges
   ***************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  std::pair<
    typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)::edge_descriptor,
    bool
  >
  edge(typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)::vertex_descriptor u,
       typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)::vertex_descriptor v,
       const PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)
                       ::edge_descriptor edge_descriptor;

    // For directed graphs, u must be local
    BOOST_ASSERT(u.owner == process_id(g.process_group()));

    typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)
        ::out_edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei) {
      if (target(*ei, g) == v) return std::make_pair(*ei, true);
    }
    return std::make_pair(edge_descriptor(), false);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<
    typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor,
    bool
  >
  edge(typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor u,
       typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v,
       const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE
                       ::edge_descriptor edge_descriptor;

    // For bidirectional and undirected graphs, u must be local or v
    // must be local
    if (u.owner == process_id(g.process_group())) {
      typename PBGL_DISTRIB_ADJLIST_TYPE::out_edge_iterator ei, ei_end;
      for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei) {
        if (target(*ei, g) == v) return std::make_pair(*ei, true);
      }
      return std::make_pair(edge_descriptor(), false);
    } else if (v.owner == process_id(g.process_group())) {
      typename PBGL_DISTRIB_ADJLIST_TYPE::in_edge_iterator ei, ei_end;
      for (boost::tie(ei, ei_end) = in_edges(v, g); ei != ei_end; ++ei) {
        if (source(*ei, g) == u) return std::make_pair(*ei, true);
      }
      return std::make_pair(edge_descriptor(), false);
    } else {
      BOOST_ASSERT(false);
      abort();
    }
  }

#if 0
  // TBD: not yet supported
  std::pair<out_edge_iterator, out_edge_iterator>
  edge_range(vertex_descriptor u, vertex_descriptor v,
             const adjacency_list& g);
#endif

  /***************************************************************************
   * Implementation of Adjacency Graph concept
   ***************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  std::pair<typename PBGL_DISTRIB_ADJLIST_TYPE::adjacency_iterator,
            typename PBGL_DISTRIB_ADJLIST_TYPE::adjacency_iterator>
  adjacent_vertices(typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v,
                    const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE::adjacency_iterator iter;
    return std::make_pair(iter(out_edges(v, g).first, &g),
                          iter(out_edges(v, g).second, &g));
  }

  /***************************************************************************
   * Implementation of Mutable Graph concept
   ***************************************************************************/

  /************************************************************************
   * add_edge
   ************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge
  add_edge(typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor u,
           typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v,
           PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_edge lazy_add_edge;

    return lazy_add_edge(g, u, v);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename PBGL_DISTRIB_ADJLIST_TYPE
    ::lazy_add_edge_with_property
  add_edge(typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor u,
           typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v,
           typename PBGL_DISTRIB_ADJLIST_TYPE::edge_property_type const& p,
           PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE
      ::lazy_add_edge_with_property lazy_add_edge_with_property;
    return lazy_add_edge_with_property(g, u, v, p);
  }

  /************************************************************************
   *
   * remove_edge
   *
   ************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  void
  remove_edge(typename PBGL_DISTRIB_ADJLIST_TYPE::edge_descriptor e,
              PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    BOOST_ASSERT(source(e, g).owner == g.processor()
                 || target(e, g).owner == g.processor());

    if (target(e, g).owner == g.processor())
      detail::parallel::remove_in_edge(e, g, DirectedS());
    if (source(e, g).owner == g.processor())
      remove_edge(e.local, g.base());

    g.remove_local_edge_from_list(source(e, g), target(e, g), DirectedS());

    if (source(e, g).owner != g.processor()
        || (target(e, g).owner != g.processor()
            && !(is_same<DirectedS, directedS>::value))) {
      g.send_remove_edge_request(e);
    }
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  void
  remove_edge(typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor u,
              typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v,
              PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE
                       ::edge_descriptor edge_descriptor;
    std::pair<edge_descriptor, bool> the_edge = edge(u, v, g);
    if (the_edge.second) remove_edge(the_edge.first, g);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  inline void
  remove_edge(typename PBGL_DISTRIB_ADJLIST_TYPE::out_edge_iterator ei,
              PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    remove_edge(*ei, g);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  inline void
  remove_edge(typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)
                ::out_edge_iterator ei,
              PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)& g)
  {
    BOOST_ASSERT(source(*ei, g).owner == g.processor());
    remove_edge(ei->local, g.base());
  }

  /************************************************************************
   *
   * remove_out_edge_if
   *
   ************************************************************************/
  namespace parallel { namespace detail {
    /**
     * Function object that applies the underlying predicate to
     * determine if an out-edge should be removed. If so, either
     * removes the incoming edge (if it is stored locally) or sends a
     * message to the owner of the target requesting that it remove
     * the edge.
     */
    template<typename Graph, typename Predicate>
    struct remove_out_edge_predicate
    {
      typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
      typedef typename Graph::local_edge_descriptor         argument_type;
      typedef typename Graph::directed_selector             directed_selector;
      typedef bool result_type;

      remove_out_edge_predicate(Graph& g, Predicate& predicate)
        : g(g), predicate(predicate) { }

      bool operator()(const argument_type& le)
      {
        typedef typename edge_descriptor::template out_generator<Graph>
          generator;

        edge_descriptor e = generator(g)(le);

        if (predicate(e)) {
          if (source(e, g).owner != target(e, g).owner
              && !(is_same<directed_selector, directedS>::value))
            g.send_remove_edge_request(e);
          else
            ::boost::detail::parallel::remove_in_edge(e, g,
                                                      directed_selector());

           g.remove_local_edge_from_list(source(e, g), target(e, g),
                                         directed_selector());
          return true;
        } else return false;
      }

    private:
      Graph& g;
      Predicate predicate;
    };
  } } // end namespace parallel::detail

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS, typename Predicate>
  inline void
  remove_out_edge_if
     (typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor u,
      Predicate predicate,
      PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE Graph;
    typedef parallel::detail::remove_out_edge_predicate<Graph, Predicate>
      Pred;

    BOOST_ASSERT(u.owner == g.processor());
    remove_out_edge_if(u.local, Pred(g, predicate), g.base());
  }

  /************************************************************************
   *
   * remove_in_edge_if
   *
   ************************************************************************/
  namespace parallel { namespace detail {
    /**
     * Function object that applies the underlying predicate to
     * determine if an in-edge should be removed. If so, either
     * removes the outgoing edge (if it is stored locally) or sends a
     * message to the owner of the target requesting that it remove
     * the edge. Only required for bidirectional graphs.
     */
    template<typename Graph, typename Predicate>
    struct remove_in_edge_predicate
    {
      typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
      typedef bool result_type;

      remove_in_edge_predicate(Graph& g, const Predicate& predicate)
        : g(g), predicate(predicate) { }

      template<typename StoredEdge>
      bool operator()(const StoredEdge& le)
      {
        typedef typename edge_descriptor::template in_generator<Graph>
          generator;

        edge_descriptor e = generator(g)(le);

        if (predicate(e)) {
          if (source(e, g).owner != target(e, g).owner)
            g.send_remove_edge_request(e);
          else
            remove_edge(source(e, g).local, target(e, g).local, g.base());
          return true;
        } else return false;
      }

    private:
      Graph& g;
      Predicate predicate;
    };
  } } // end namespace parallel::detail

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG, typename Predicate>
  inline void
  remove_in_edge_if
     (typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
                 ::vertex_descriptor u,
      Predicate predicate,
      PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS) Graph;
    typedef parallel::detail::remove_in_edge_predicate<Graph, Predicate>
      Pred;

    BOOST_ASSERT(u.owner == g.processor());
    graph_detail::erase_if(get(vertex_in_edges, g.base())[u.local],
                           Pred(g, predicate));
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG, typename Predicate>
  inline void
  remove_in_edge_if
     (typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)
                 ::vertex_descriptor u,
      Predicate predicate,
      PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)& g)
  {
    remove_out_edge_if(u, predicate, g);
  }

  /************************************************************************
   *
   * remove_edge_if
   *
   ************************************************************************/
  namespace parallel { namespace detail {
    /**
     * Function object that applies the underlying predicate to
     * determine if a directed edge can be removed. This only applies
     * to directed graphs.
     */
    template<typename Graph, typename Predicate>
    struct remove_directed_edge_predicate
    {
      typedef typename Graph::local_edge_descriptor argument_type;
      typedef typename graph_traits<Graph>::edge_descriptor edge_descriptor;
      typedef bool result_type;

      remove_directed_edge_predicate(Graph& g, const Predicate& predicate)
        : g(g), predicate(predicate) { }

      bool operator()(const argument_type& le)
      {
        typedef typename edge_descriptor::template out_generator<Graph>
          generator;

        edge_descriptor e = generator(g)(le);
        return predicate(e);
      }

    private:
      Graph& g;
      Predicate predicate;
    };
  } } // end namespace parallel::detail

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG, typename Predicate>
  inline void
  remove_edge_if(Predicate predicate, 
                 PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS) Graph;
    typedef parallel::detail::remove_directed_edge_predicate<Graph,
                                                             Predicate> Pred;
    remove_edge_if(Pred(g, predicate), g.base());
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG, typename Predicate>
  inline void
  remove_edge_if(Predicate predicate,
                 PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS) Graph;
    typedef parallel::detail::remove_out_edge_predicate<Graph,
                                                        Predicate> Pred;
    remove_edge_if(Pred(g, predicate), g.base());
  }

  namespace parallel { namespace detail {
    /**
     * Function object that applies the underlying predicate to
     * determine if an undirected edge should be removed. If so,
     * removes the local edges associated with the edge and
     * (potentially) sends a message to the remote processor that also
     * is removing this edge.
     */
    template<typename Graph, typename Predicate>
    struct remove_undirected_edge_predicate
    {
      typedef typename graph_traits<Graph>::edge_descriptor argument_type;
      typedef bool result_type;

      remove_undirected_edge_predicate(Graph& g, Predicate& predicate)
        : g(g), predicate(predicate) { }

      bool operator()(const argument_type& e)
      {
        if (predicate(e)) {
          if (source(e, g).owner != target(e, g).owner)
            g.send_remove_edge_request(e);
          if (target(e, g).owner == g.processor())
            ::boost::detail::parallel::remove_in_edge(e, g, undirectedS());
          if (source(e, g).owner == g.processor())
            remove_edge(e.local, g.base());
          return true;
        } else return false;
      }

    private:
      Graph& g;
      Predicate predicate;
    };
  } } // end namespace parallel::detail

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG, typename Predicate>
  inline void
  remove_edge_if(Predicate predicate,
                 PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS) Graph;
    typedef parallel::detail::remove_undirected_edge_predicate<Graph,
                                                               Predicate> Pred;
    graph_detail::erase_if(g.local_edges(), Pred(g, predicate));
  }

  /************************************************************************
   *
   * clear_vertex
   *
   ************************************************************************/
  namespace parallel { namespace detail {
    struct always_true
    {
      typedef bool result_type;

      template<typename T> bool operator()(const T&) const { return true; }
    };
  } } // end namespace parallel::detail

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  void
  clear_vertex
    (typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
          ::vertex_descriptor u,
      PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)& g)
  {
    clear_out_edges(u, g);
    clear_in_edges(u, g);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  void
  clear_vertex
    (typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)
                ::vertex_descriptor u,
      PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(undirectedS)& g)
  {
    remove_out_edge_if(u, parallel::detail::always_true(), g);
  }

  /************************************************************************
   *
   * clear_out_edges
   *
   ************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  void
  clear_out_edges
    (typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)::vertex_descriptor u,
      PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(directedS)& g)
  {
    BOOST_ASSERT(u.owner == g.processor());
    clear_out_edges(u.local, g.base());
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  void
  clear_out_edges
    (typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
                ::vertex_descriptor u,
      PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)& g)
  {
    remove_out_edge_if(u, parallel::detail::always_true(), g);
  }

  /************************************************************************
   *
   * clear_in_edges
   *
   ************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS_CONFIG>
  void
  clear_in_edges
    (typename PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)
                ::vertex_descriptor u,
      PBGL_DISTRIB_ADJLIST_TYPE_CONFIG(bidirectionalS)& g)
  {
    remove_in_edge_if(u, parallel::detail::always_true(), g);
  }

  /************************************************************************
   *
   * add_vertex
   *
   ************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor
  add_vertex(PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE graph_type;
    typename graph_type::vertex_property_type p;
    return add_vertex(p, g);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename PBGL_DISTRIB_ADJLIST_TYPE::lazy_add_vertex_with_property
  add_vertex(typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_property_type const& p,
             PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE
                       ::lazy_add_vertex_with_property lazy_add_vertex;
    return lazy_add_vertex(g, p);
  }

  /************************************************************************
   *
   * remove_vertex
   *
   ************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  void
  remove_vertex(typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor u,
                PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename PBGL_DISTRIB_ADJLIST_TYPE::graph_type graph_type;
    typedef typename graph_type::named_graph_mixin named_graph_mixin;
    BOOST_ASSERT(u.owner == g.processor());
    static_cast<named_graph_mixin&>(static_cast<graph_type&>(g))
      .removing_vertex(u, boost::graph_detail::iterator_stability(g.base().m_vertices));
    g.distribution().clear();
    remove_vertex(u.local, g.base());
  }

  /***************************************************************************
   * Implementation of Property Graph concept
   ***************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS, typename Property>
  struct property_map<PBGL_DISTRIB_ADJLIST_TYPE, Property>
  : detail::parallel::get_adj_list_pmap<Property>
      ::template apply<PBGL_DISTRIB_ADJLIST_TYPE>
  { };

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS, typename Property>
  struct property_map<PBGL_DISTRIB_ADJLIST_TYPE const, Property>
          : boost::detail::parallel::get_adj_list_pmap<Property>
// FIXME: in the original code the following was not const
      ::template apply<PBGL_DISTRIB_ADJLIST_TYPE const>
  { };

  template<typename Property, PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, Property>::type
  get(Property p, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE Graph;
    typedef typename property_map<Graph, Property>::type result_type;
    typedef typename property_traits<result_type>::value_type value_type;
    typedef typename property_reduce<Property>::template apply<value_type>
      reduce;

    typedef typename property_traits<result_type>::key_type descriptor;
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename mpl::if_<is_same<descriptor, vertex_descriptor>,
                              vertex_global_t, edge_global_t>::type
      global_map_t;

    return result_type(g.process_group(), get(global_map_t(), g),
                       get(p, g.base()), reduce());
  }

  template<typename Property, PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, Property>::const_type
  get(Property p, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE Graph;
    typedef typename property_map<Graph, Property>::const_type result_type;
    typedef typename property_traits<result_type>::value_type value_type;
    typedef typename property_reduce<Property>::template apply<value_type>
      reduce;

    typedef typename property_traits<result_type>::key_type descriptor;
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename mpl::if_<is_same<descriptor, vertex_descriptor>,
                              vertex_global_t, edge_global_t>::type
      global_map_t;

    return result_type(g.process_group(), get(global_map_t(), g),
                       get(p, g.base()), reduce());
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, vertex_local_index_t>::type
  get(vertex_local_index_t, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    return get(vertex_local_index, g.base());
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE,
                        vertex_local_index_t>::const_type
  get(vertex_local_index_t, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    return get(vertex_local_index, g.base());
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, vertex_global_t>::const_type
  get(vertex_global_t, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       vertex_global_t>::const_type result_type;
    return result_type();
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, vertex_global_t>::const_type
  get(vertex_global_t, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       vertex_global_t>::const_type result_type;
    return result_type();
  }

  /// Retrieve a property map mapping from a vertex descriptor to its
  /// owner.
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, vertex_owner_t>::type
  get(vertex_owner_t, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       vertex_owner_t>::type result_type;
    return result_type();
  }

  /// Retrieve a property map mapping from a vertex descriptor to its
  /// owner.
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, vertex_owner_t>::const_type
  get(vertex_owner_t, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       vertex_owner_t>::const_type result_type;
    return result_type();
  }

  /// Retrieve the owner of a vertex
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  inline processor_id_type
  get(vertex_owner_t, PBGL_DISTRIB_ADJLIST_TYPE&,
      typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v)
  {
    return v.owner;
  }

  /// Retrieve the owner of a vertex
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  inline processor_id_type
  get(vertex_owner_t, const PBGL_DISTRIB_ADJLIST_TYPE&,
      typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v)
  {
    return v.owner;
  }

  /// Retrieve a property map that maps from a vertex descriptor to
  /// its local descriptor.
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, vertex_local_t>::type
  get(vertex_local_t, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       vertex_local_t>::type result_type;
    return result_type();
  }

  /// Retrieve a property map that maps from a vertex descriptor to
  /// its local descriptor.
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, vertex_local_t>::const_type
  get(vertex_local_t, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       vertex_local_t>::const_type result_type;
    return result_type();
  }

  /// Retrieve the local descriptor of a vertex
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  inline typename PBGL_DISTRIB_ADJLIST_TYPE::local_vertex_descriptor
  get(vertex_local_t, PBGL_DISTRIB_ADJLIST_TYPE&,
      typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v)
  {
    return v.local;
  }

  /// Retrieve the local descriptor of a vertex
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  inline typename PBGL_DISTRIB_ADJLIST_TYPE::local_vertex_descriptor
  get(vertex_local_t, const PBGL_DISTRIB_ADJLIST_TYPE&,
      typename PBGL_DISTRIB_ADJLIST_TYPE::vertex_descriptor v)
  {
    return v.local;
  }


  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, edge_global_t>::const_type
  get(edge_global_t, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       edge_global_t>::const_type result_type;
    return result_type();
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, edge_global_t>::const_type
  get(edge_global_t, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       edge_global_t>::const_type result_type;
    return result_type();
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, edge_owner_t>::type
  get(edge_owner_t, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       edge_owner_t>::type result_type;
    return result_type();
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, edge_owner_t>::const_type
  get(edge_owner_t, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       edge_owner_t>::const_type result_type;
    return result_type();
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, edge_local_t>::type
  get(edge_local_t, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       edge_local_t>::type result_type;
    return result_type();
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, edge_local_t>::const_type
  get(edge_local_t, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef typename property_map<
                       PBGL_DISTRIB_ADJLIST_TYPE,
                       edge_local_t>::const_type result_type;
    return result_type();
  }

  template<typename Property, PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS,
           typename Key>
  inline
  typename property_traits<typename property_map<
                PBGL_DISTRIB_ADJLIST_TYPE, Property>::const_type
           >::value_type
  get(Property p, const PBGL_DISTRIB_ADJLIST_TYPE& g, const Key& key)
  {
    if (owner(key) == process_id(g.process_group()))
      return get(p, g.base(), local(key));
    else
      BOOST_ASSERT(false);
  }

  template<typename Property, PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS,
           typename Key, typename Value>
  void
  put(Property p, PBGL_DISTRIB_ADJLIST_TYPE& g, const Key& key, const Value& v)
  {
    if (owner(key) == process_id(g.process_group()))
      put(p, g.base(), local(key), v);
    else
      BOOST_ASSERT(false);
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, vertex_index_t>::type
  get(vertex_index_t vi, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE graph_type;
    typedef typename property_map<graph_type, vertex_index_t>::type
      result_type;
    return result_type(g.process_group(), get(vertex_global, g),
                       get(vi, g.base()));
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, vertex_index_t>::const_type
  get(vertex_index_t vi, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE graph_type;
    typedef typename property_map<graph_type, vertex_index_t>::const_type
      result_type;
    return result_type(g.process_group(), get(vertex_global, g),
                       get(vi, g.base()));
  }

  /***************************************************************************
   * Implementation of bundled properties
   ***************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS, typename T, typename Bundle>
  struct property_map<PBGL_DISTRIB_ADJLIST_TYPE, T Bundle::*>
    : detail::parallel::get_adj_list_pmap<T Bundle::*>
      ::template apply<PBGL_DISTRIB_ADJLIST_TYPE>
  { };

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS, typename T, typename Bundle>
  struct property_map<PBGL_DISTRIB_ADJLIST_TYPE const, T Bundle::*>
    : detail::parallel::get_adj_list_pmap<T Bundle::*>
      ::template apply<PBGL_DISTRIB_ADJLIST_TYPE const>
  { };

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS, typename T, typename Bundle>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, T Bundle::*>::type
  get(T Bundle::* p, PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE Graph;
    typedef typename property_map<Graph, T Bundle::*>::type result_type;
    typedef typename property_traits<result_type>::value_type value_type;
    typedef typename property_reduce<T Bundle::*>::template apply<value_type>
      reduce;

    typedef typename property_traits<result_type>::key_type descriptor;
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename mpl::if_<is_same<descriptor, vertex_descriptor>,
                              vertex_global_t, edge_global_t>::type
      global_map_t;

    return result_type(g.process_group(), get(global_map_t(), g),
                       get(p, g.base()), reduce());
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS, typename T, typename Bundle>
  typename property_map<PBGL_DISTRIB_ADJLIST_TYPE, T Bundle::*>::const_type
  get(T Bundle::* p, const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    typedef PBGL_DISTRIB_ADJLIST_TYPE Graph;
    typedef typename property_map<Graph, T Bundle::*>::const_type result_type;
    typedef typename property_traits<result_type>::value_type value_type;
    typedef typename property_reduce<T Bundle::*>::template apply<value_type>
      reduce;

    typedef typename property_traits<result_type>::key_type descriptor;
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_descriptor;
    typedef typename mpl::if_<is_same<descriptor, vertex_descriptor>,
                              vertex_global_t, edge_global_t>::type
      global_map_t;

    return result_type(g.process_group(), get(global_map_t(), g),
                       get(p, g.base()), reduce());
  }

  /***************************************************************************
   * Implementation of DistributedGraph concept
   ***************************************************************************/
  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  void synchronize(const PBGL_DISTRIB_ADJLIST_TYPE& g)
  {
    synchronize(g.process_group());
  }

  template<PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
  ProcessGroup
  process_group(const PBGL_DISTRIB_ADJLIST_TYPE& g)
  { return g.process_group(); }

  /***************************************************************************
   * Specializations of is_mpi_datatype for Serializable entities
   ***************************************************************************/
  namespace mpi {
    template<typename Directed, typename Vertex>
    struct is_mpi_datatype<boost::detail::edge_base<Directed, Vertex> >
      : is_mpi_datatype<Vertex> { };

    template<typename Directed, typename Vertex>
    struct is_mpi_datatype<boost::detail::edge_desc_impl<Directed, Vertex> >
      : is_mpi_datatype<boost::detail::edge_base<Directed, Vertex> > { };

    template<typename LocalDescriptor>
    struct is_mpi_datatype<boost::detail::parallel::global_descriptor<LocalDescriptor> >
      : is_mpi_datatype<LocalDescriptor> { };

    template<typename Edge>
    struct is_mpi_datatype<boost::detail::parallel::edge_descriptor<Edge> >
      : is_mpi_datatype<Edge> { };

    template<typename Vertex, typename LocalVertex>
    struct is_mpi_datatype<boost::detail::parallel::
                             msg_add_edge_data<Vertex, LocalVertex> >
      : is_mpi_datatype<Vertex> { };

    template<typename Vertex, typename LocalVertex, typename EdgeProperty>
    struct is_mpi_datatype<boost::detail::parallel::
                             msg_add_edge_with_property_data<Vertex, 
                                                             LocalVertex,
                                                             EdgeProperty> >
      : mpl::and_<is_mpi_datatype<Vertex>, is_mpi_datatype<EdgeProperty> > { };


   template<typename EdgeProperty, typename EdgeDescriptor>
   struct is_mpi_datatype<boost::detail::parallel::msg_nonlocal_edge_data<
                          EdgeProperty,EdgeDescriptor> >
           : mpl::and_<
               is_mpi_datatype<boost::detail::parallel::maybe_store_property<
                           EdgeProperty> >,
           is_mpi_datatype<EdgeDescriptor> >
  {};
   
   template<typename EdgeDescriptor>
   struct is_mpi_datatype<
            boost::detail::parallel::msg_remove_edge_data<EdgeDescriptor> >
           : is_mpi_datatype<EdgeDescriptor> {};
  }

  /***************************************************************************
   * Specializations of is_bitwise_serializable for Serializable entities
   ***************************************************************************/
  namespace serialization {
    template<typename Directed, typename Vertex>
    struct is_bitwise_serializable<boost::detail::edge_base<Directed, Vertex> >
      : is_bitwise_serializable<Vertex> { };

    template<typename Directed, typename Vertex>
    struct is_bitwise_serializable<boost::detail::edge_desc_impl<Directed, Vertex> >
      : is_bitwise_serializable<boost::detail::edge_base<Directed, Vertex> > { };

    template<typename LocalDescriptor>
    struct is_bitwise_serializable<boost::detail::parallel::global_descriptor<LocalDescriptor> >
      : is_bitwise_serializable<LocalDescriptor> { };

    template<typename Edge>
    struct is_bitwise_serializable<boost::detail::parallel::edge_descriptor<Edge> >
      : is_bitwise_serializable<Edge> { };

    template<typename Vertex, typename LocalVertex>
    struct is_bitwise_serializable<boost::detail::parallel::
                             msg_add_edge_data<Vertex, LocalVertex> >
      : is_bitwise_serializable<Vertex> { };

    template<typename Vertex, typename LocalVertex, typename EdgeProperty>
    struct is_bitwise_serializable<boost::detail::parallel::
                             msg_add_edge_with_property_data<Vertex, 
                                                             LocalVertex,
                                                             EdgeProperty> >
      : mpl::and_<is_bitwise_serializable<Vertex>, 
                  is_bitwise_serializable<EdgeProperty> > { };

   template<typename EdgeProperty, typename EdgeDescriptor>
   struct is_bitwise_serializable<boost::detail::parallel::msg_nonlocal_edge_data<
                                  EdgeProperty,EdgeDescriptor> >
           : mpl::and_<
               is_bitwise_serializable<
                boost::detail::parallel::maybe_store_property<EdgeProperty> >,
           is_bitwise_serializable<EdgeDescriptor> >
  {};
   
   template<typename EdgeDescriptor>
   struct is_bitwise_serializable<
            boost::detail::parallel::msg_remove_edge_data<EdgeDescriptor> >
           : is_bitwise_serializable<EdgeDescriptor> {};
   
    template<typename Directed, typename Vertex>
    struct implementation_level<boost::detail::edge_base<Directed, Vertex> >
      : mpl::int_<object_serializable> {};

    template<typename Directed, typename Vertex>
    struct implementation_level<boost::detail::edge_desc_impl<Directed, Vertex> >
      : mpl::int_<object_serializable> {};

    template<typename LocalDescriptor>
    struct implementation_level<boost::detail::parallel::global_descriptor<LocalDescriptor> >
      : mpl::int_<object_serializable> {};

    template<typename Edge>
    struct implementation_level<boost::detail::parallel::edge_descriptor<Edge> >
      : mpl::int_<object_serializable> {};

    template<typename Vertex, typename LocalVertex>
    struct implementation_level<boost::detail::parallel::
                             msg_add_edge_data<Vertex, LocalVertex> >
      : mpl::int_<object_serializable> {};

    template<typename Vertex, typename LocalVertex, typename EdgeProperty>
    struct implementation_level<boost::detail::parallel::
                             msg_add_edge_with_property_data<Vertex, 
                                                             LocalVertex,
                                                             EdgeProperty> >
      : mpl::int_<object_serializable> {};

   template<typename EdgeProperty, typename EdgeDescriptor>
   struct implementation_level<boost::detail::parallel::msg_nonlocal_edge_data<
                               EdgeProperty,EdgeDescriptor> >
           : mpl::int_<object_serializable> {};
   
   template<typename EdgeDescriptor>
   struct implementation_level<
            boost::detail::parallel::msg_remove_edge_data<EdgeDescriptor> >
          : mpl::int_<object_serializable> {};
   
    template<typename Directed, typename Vertex>
    struct tracking_level<boost::detail::edge_base<Directed, Vertex> >
      : mpl::int_<track_never> {};

    template<typename Directed, typename Vertex>
    struct tracking_level<boost::detail::edge_desc_impl<Directed, Vertex> >
      : mpl::int_<track_never> {};

    template<typename LocalDescriptor>
    struct tracking_level<boost::detail::parallel::global_descriptor<LocalDescriptor> >
      : mpl::int_<track_never> {};

    template<typename Edge>
    struct tracking_level<boost::detail::parallel::edge_descriptor<Edge> >
      : mpl::int_<track_never> {};

    template<typename Vertex, typename LocalVertex>
    struct tracking_level<boost::detail::parallel::
                             msg_add_edge_data<Vertex, LocalVertex> >
      : mpl::int_<track_never> {};

    template<typename Vertex, typename LocalVertex, typename EdgeProperty>
    struct tracking_level<boost::detail::parallel::
                             msg_add_edge_with_property_data<Vertex, 
                                                             LocalVertex,
                                                             EdgeProperty> >
      : mpl::int_<track_never> {};

   template<typename EdgeProperty, typename EdgeDescriptor>
   struct tracking_level<boost::detail::parallel::msg_nonlocal_edge_data<
                         EdgeProperty,EdgeDescriptor> >
           : mpl::int_<track_never> {};
   
   template<typename EdgeDescriptor>
   struct tracking_level<
            boost::detail::parallel::msg_remove_edge_data<EdgeDescriptor> >
          : mpl::int_<track_never> {};
  }

  // Hash function for global descriptors
  template<typename LocalDescriptor>
  struct hash<detail::parallel::global_descriptor<LocalDescriptor> >
  {
    typedef detail::parallel::global_descriptor<LocalDescriptor> argument_type;
    std::size_t operator()(argument_type const& x) const
    {
      std::size_t hash = hash_value(x.owner);
      hash_combine(hash, x.local);
      return hash;
    }
  };

  // Hash function for parallel edge descriptors
  template<typename Edge>
  struct hash<detail::parallel::edge_descriptor<Edge> >
  {
    typedef detail::parallel::edge_descriptor<Edge> argument_type;

    std::size_t operator()(argument_type const& x) const
    {
      std::size_t hash = hash_value(x.owner());
      hash_combine(hash, x.local);
      return hash;
    }
  };

} // end namespace boost

#include <boost/graph/distributed/adjlist/handlers.hpp>
#include <boost/graph/distributed/adjlist/initialize.hpp>
#include <boost/graph/distributed/adjlist/redistribute.hpp>
#include <boost/graph/distributed/adjlist/serialization.hpp>

#endif // BOOST_GRAPH_DISTRIBUTED_ADJACENCY_LIST_HPP
