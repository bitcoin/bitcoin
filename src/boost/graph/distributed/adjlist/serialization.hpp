// Copyright Daniel Wallin 2007. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_DISTRIBUTED_ADJLIST_SERIALIZATION_070925_HPP
#define BOOST_GRAPH_DISTRIBUTED_ADJLIST_SERIALIZATION_070925_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

# include <boost/assert.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/foreach.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/operations.hpp>
# include <cctype>
# include <fstream>

namespace boost {

namespace detail { namespace parallel
{

  // Wraps a local descriptor, making it serializable.
  template <class Local>
  struct serializable_local_descriptor
  {
      serializable_local_descriptor()
      {}

      serializable_local_descriptor(Local local)
        : local(local)
      {}

      operator Local const&() const
      {
          return local;
      }

      bool operator==(serializable_local_descriptor const& other) const
      {
          return local == other.local;
      }

      bool operator<(serializable_local_descriptor const& other) const
      {
          return local < other.local;
      }

      template <class Archive>
      void serialize(Archive& ar, const unsigned int /*version*/)
      {
          ar & unsafe_serialize(local);
      }

      Local local;
  };

  template <class Vertex, class Properties>
  struct pending_edge
  {
      pending_edge(
          Vertex source, Vertex target
        , Properties properties, void* property_ptr
      )
        : source(source)
        , target(target)
        , properties(properties)
        , property_ptr(property_ptr)
      {}

      Vertex source;
      Vertex target;
      Properties properties;
      void* property_ptr;
  };

  inline bool is_digit(char c)
  {
      return std::isdigit(c) != 0;
  }

  inline std::vector<int> 
      available_process_files(std::string const& filename)
      {
          if (!filesystem::exists(filename))
              return std::vector<int>();

          std::vector<int> result;

          for (filesystem::directory_iterator i(filename), end; i != end; ++i)
          {
              if (!filesystem::is_regular(*i))
                  boost::throw_exception(std::runtime_error("directory contains non-regular entries"));

              std::string process_name = i->path().filename().string();
              for (std::string::size_type i = 0; i < process_name.size(); ++i)
                if (!is_digit(process_name[i]))
                  boost::throw_exception(std::runtime_error("directory contains files with invalid names"));

              result.push_back(boost::lexical_cast<int>(process_name));
          }

          return result;
      }

  template <class Archive, class Tag, class T, class Base>
  void maybe_load_properties(
      Archive& ar, char const* name, property<Tag, T, Base>& properties)
  {
      ar >> serialization::make_nvp(name, get_property_value(properties, Tag()));
      maybe_load_properties(ar, name, static_cast<Base&>(properties));
  }

  template <class Archive>
  void maybe_load_properties(
      Archive&, char const*, no_property&)
  {}

  template <class Archive, typename Bundle>
  void maybe_load_properties(
      Archive& ar, char const* name, Bundle& bundle)
  {
    ar >> serialization::make_nvp(name, bundle);
    no_property prop;
    maybe_load_properties(ar, name, prop);
  }






  template <class Graph, class Archive, class VertexListS>
  struct graph_loader
  {
      typedef typename Graph::vertex_descriptor vertex_descriptor;
      typedef typename Graph::local_vertex_descriptor local_vertex_descriptor;
      typedef typename Graph::vertex_property_type vertex_property_type;
      typedef typename Graph::edge_descriptor edge_descriptor;
      typedef typename Graph::local_edge_descriptor local_edge_descriptor;
      typedef typename Graph::edge_property_type edge_property_type;
      typedef typename Graph::process_group_type process_group_type;
      typedef typename process_group_type::process_id_type process_id_type;
      typedef typename Graph::directed_selector directed_selector;
      typedef typename mpl::if_<
          is_same<VertexListS, defaultS>, vecS, VertexListS
      >::type vertex_list_selector;
      typedef pending_edge<vertex_descriptor, edge_property_type> 
          pending_edge_type; 
      typedef serializable_local_descriptor<local_vertex_descriptor>
          serializable_vertex_descriptor;

      graph_loader(Graph& g, Archive& ar)
        : m_g(g)
        , m_ar(ar)
        , m_pg(g.process_group())
        , m_requested_vertices(num_processes(m_pg))
        , m_remote_vertices(num_processes(m_pg))
        , m_property_ptrs(num_processes(m_pg))
      {
          g.clear();
          load_prefix();
          load_vertices();
          load_edges();
          ar >> make_nvp("distribution", m_g.distribution());
      }

  private:
      struct pending_in_edge
      {
          pending_in_edge(
              vertex_descriptor u, vertex_descriptor v, void* property_ptr
          )
            : u(u)
            , v(v)
            , property_ptr(property_ptr)
          {}

          vertex_descriptor u;
          vertex_descriptor v;
          void* property_ptr;
      };

      bool is_root() const
      {
          return process_id(m_pg) == 0;
      }

      template <class T>
      serialization::nvp<T> const make_nvp(char const* name, T& value) const
      {
          return serialization::nvp<T>(name, value);
      }

      void load_prefix();
      void load_vertices();

      template <class Anything>
      void maybe_load_and_store_local_vertex(Anything);
      void maybe_load_and_store_local_vertex(vecS);

      void load_edges();
      void load_in_edges(bidirectionalS);
      void load_in_edges(directedS);
      void add_pending_in_edge(
          vertex_descriptor u, vertex_descriptor v, void* property_ptr, vecS);
      template <class Anything>
      void add_pending_in_edge(
          vertex_descriptor u, vertex_descriptor v, void* property_ptr, Anything);
      template <class Anything>
      void add_edge(
          vertex_descriptor u, vertex_descriptor v
        , edge_property_type const& property, void* property_ptr, Anything);
      void add_edge(
          vertex_descriptor u, vertex_descriptor v
        , edge_property_type const& property, void* property_ptr, vecS);
      void add_remote_vertex_request(
          vertex_descriptor u, vertex_descriptor v, directedS);
      void add_remote_vertex_request(
          vertex_descriptor u, vertex_descriptor v, bidirectionalS);
      void add_in_edge(
          edge_descriptor const&, void*, directedS);
      void add_in_edge(
          edge_descriptor const& edge, void* old_property_ptr, bidirectionalS);

      void resolve_remote_vertices(directedS);
      void resolve_remote_vertices(bidirectionalS);
      vertex_descriptor resolve_remote_vertex(vertex_descriptor u) const;
      vertex_descriptor resolve_remote_vertex(vertex_descriptor u, vecS) const;
      template <class Anything>
      vertex_descriptor resolve_remote_vertex(vertex_descriptor u, Anything) const;

      void resolve_property_ptrs();

      void commit_pending_edges(vecS);
      template <class Anything>
      void commit_pending_edges(Anything);
      void commit_pending_in_edges(directedS);
      void commit_pending_in_edges(bidirectionalS);

      void* maybe_load_property_ptr(directedS) { return 0; }
      void* maybe_load_property_ptr(bidirectionalS);

      Graph& m_g;
      Archive& m_ar;
      process_group_type m_pg;

      std::vector<process_id_type> m_id_mapping;

      // Maps local vertices as loaded from the archive to
      // the ones actually added to the graph. Only used 
      // when !vecS.
      std::map<local_vertex_descriptor, local_vertex_descriptor> m_local_vertices;

      // This is the list of remote vertex descriptors that we
      // are going to receive from other processes. This is
      // kept sorted so that we can determine the position of
      // the matching vertex descriptor in m_remote_vertices.
      std::vector<std::vector<serializable_vertex_descriptor> > m_requested_vertices;

      // This is the list of remote vertex descriptors that
      // we send and receive from other processes.
      std::vector<std::vector<serializable_vertex_descriptor> > m_remote_vertices;

      // ...
      std::vector<pending_edge_type> m_pending_edges;

      // The pending in-edges that will be added in the commit step, after
      // the remote vertex descriptors has been resolved. Only used
      // when bidirectionalS and !vecS.
      std::vector<pending_in_edge> m_pending_in_edges;

      std::vector<std::vector<unsafe_pair<void*,void*> > > m_property_ptrs;
  };

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::load_prefix()
  {
      typename process_group_type::process_size_type num_processes_;
      m_ar >> make_nvp("num_processes", num_processes_);

      if (num_processes_ != num_processes(m_pg))
          boost::throw_exception(std::runtime_error("number of processes mismatch"));

      process_id_type old_id;
      m_ar >> make_nvp("id", old_id);

      std::vector<typename Graph::distribution_type::size_type> mapping;
      m_ar >> make_nvp("mapping", mapping);

      // Fetch all the old id's from the other processes.
      std::vector<process_id_type> old_ids;
      all_gather(m_pg, &old_id, &old_id+1, old_ids);

      m_id_mapping.resize(num_processes(m_pg), -1);

      for (process_id_type i = 0; i < num_processes(m_pg); ++i)
      {
# ifdef PBGL_SERIALIZE_DEBUG
          if (is_root())
              std::cout << i << " used to be " << old_ids[i] << "\n"; 
# endif
          BOOST_ASSERT(m_id_mapping[old_ids[i]] == -1);
          m_id_mapping[old_ids[i]] = i;
      }

      std::vector<typename Graph::distribution_type::size_type> new_mapping(
          mapping.size());

      for (int i = 0; i < num_processes(m_pg); ++i)
      {
          new_mapping[mapping[old_ids[i]]] = i;
      }

      m_g.distribution().assign_mapping(
          new_mapping.begin(), new_mapping.end());
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::load_vertices()
  {
      int V;
      m_ar >> BOOST_SERIALIZATION_NVP(V); 

# ifdef PBGL_SERIALIZE_DEBUG
      if (is_root())
          std::cout << "Loading vertices\n";
# endif

      for (int i = 0; i < V; ++i)
      {
          maybe_load_and_store_local_vertex(vertex_list_selector());
      }
  }

  template <class Graph, class Archive, class VertexListS>
  template <class Anything>
  void graph_loader<Graph, Archive, VertexListS>::maybe_load_and_store_local_vertex(Anything)
  {
      // Load the original vertex descriptor
      local_vertex_descriptor local;
      m_ar >> make_nvp("local", unsafe_serialize(local)); 

      // Load the properties
      vertex_property_type property;
      detail::parallel::maybe_load_properties(m_ar, "vertex_property",
                          property);

      // Add the vertex
      vertex_descriptor v(process_id(m_pg), add_vertex(property, m_g.base()));

      if (m_g.on_add_vertex)
        m_g.on_add_vertex(v, m_g);

      // Create the mapping from the "old" local descriptor to the new
      // local descriptor.
      m_local_vertices[local] = v.local;
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::maybe_load_and_store_local_vertex(vecS)
  {
      // Load the properties
      vertex_property_type property;
      detail::parallel::maybe_load_properties(m_ar, "vertex_property",
                          property);

      // Add the vertex
      vertex_descriptor v(process_id(m_pg), 
                          add_vertex(m_g.build_vertex_property(property), 
                                     m_g.base()));

      if (m_g.on_add_vertex)
        m_g.on_add_vertex(v, m_g);
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::load_edges()
  {
      int E;
      m_ar >> BOOST_SERIALIZATION_NVP(E);

# ifdef PBGL_SERIALIZE_DEBUG
      if (is_root())
          std::cout << "Loading edges\n";
# endif

      for (int i = 0; i < E; ++i)
      {
          local_vertex_descriptor local_src;
          process_id_type target_owner;
          local_vertex_descriptor local_tgt;

          m_ar >> make_nvp("source", unsafe_serialize(local_src)); 
          m_ar >> make_nvp("target_owner", target_owner); 
          m_ar >> make_nvp("target", unsafe_serialize(local_tgt)); 

          process_id_type new_src_owner = process_id(m_pg);
          process_id_type new_tgt_owner = m_id_mapping[target_owner];

          vertex_descriptor source(new_src_owner, local_src);
          vertex_descriptor target(new_tgt_owner, local_tgt);

          edge_property_type properties;
          detail::parallel::maybe_load_properties(m_ar, "edge_property", properties);

          void* property_ptr = maybe_load_property_ptr(directed_selector());
          add_edge(source, target, properties, property_ptr, vertex_list_selector());
      }

      load_in_edges(directed_selector());
      commit_pending_edges(vertex_list_selector());
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::load_in_edges(bidirectionalS)
  {
      std::size_t I;
      m_ar >> BOOST_SERIALIZATION_NVP(I);

# ifdef PBGL_SERIALIZE_DEBUG
      if (is_root())
          std::cout << "Loading in-edges\n";
# endif

      for (int i = 0; i < I; ++i)
      {
          process_id_type src_owner;
          local_vertex_descriptor local_src;
          local_vertex_descriptor local_target;
          void* property_ptr;

          m_ar >> make_nvp("src_owner", src_owner);
          m_ar >> make_nvp("source", unsafe_serialize(local_src));
          m_ar >> make_nvp("target", unsafe_serialize(local_target));
          m_ar >> make_nvp("property_ptr", unsafe_serialize(property_ptr));

          src_owner = m_id_mapping[src_owner];

          vertex_descriptor u(src_owner, local_src);
          vertex_descriptor v(process_id(m_pg), local_target);

          add_pending_in_edge(u, v, property_ptr, vertex_list_selector());
      }
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::load_in_edges(directedS)
  {}

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::add_pending_in_edge(
      vertex_descriptor u, vertex_descriptor v, void* property_ptr, vecS)
  {
      m_pending_in_edges.push_back(pending_in_edge(u,v,property_ptr));
  }

  template <class Graph, class Archive, class VertexListS>
  template <class Anything>
  void graph_loader<Graph, Archive, VertexListS>::add_pending_in_edge(
      vertex_descriptor u, vertex_descriptor v, void* property_ptr, Anything)
  {
      // u and v represent the out-edge here, meaning v is local
      // to us, and u is always remote.
      m_pending_in_edges.push_back(pending_in_edge(u,v,property_ptr));
      add_remote_vertex_request(v, u, bidirectionalS());
  }

  template <class Graph, class Archive, class VertexListS> 
  template <class Anything>
  void graph_loader<Graph, Archive, VertexListS>::add_edge(
      vertex_descriptor u, vertex_descriptor v
    , edge_property_type const& property, void* property_ptr, Anything)
  {
      m_pending_edges.push_back(pending_edge_type(u, v, property, property_ptr));
      add_remote_vertex_request(u, v, directed_selector());
  }

  template <class Graph, class Archive, class VertexListS> 
  void graph_loader<Graph, Archive, VertexListS>::add_remote_vertex_request(
      vertex_descriptor u, vertex_descriptor v, directedS)
  {
      // We have to request the remote vertex.
      m_requested_vertices[owner(v)].push_back(local(v));
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::add_remote_vertex_request(
      vertex_descriptor u, vertex_descriptor v, bidirectionalS)
  {
      // If the edge spans to another process, we know
      // that that process has a matching in-edge, so
      // we can just send our vertex. No requests
      // necessary.
      if (owner(v) != m_g.processor())
      {
          m_remote_vertices[owner(v)].push_back(local(u));
          m_requested_vertices[owner(v)].push_back(local(v));
      }
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::add_edge(
      vertex_descriptor u, vertex_descriptor v
    , edge_property_type const& property, void* property_ptr, vecS)
  {
      std::pair<local_edge_descriptor, bool> inserted = 
          detail::parallel::add_local_edge(
              local(u), local(v)
            , m_g.build_edge_property(property), m_g.base());
      BOOST_ASSERT(inserted.second);
      put(edge_target_processor_id, m_g.base(), inserted.first, owner(v));

      edge_descriptor e(owner(u), owner(v), true, inserted.first);

      if (inserted.second && m_g.on_add_edge)
        m_g.on_add_edge(e, m_g);

      add_in_edge(e, property_ptr, directed_selector());
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::add_in_edge(
      edge_descriptor const&, void*, directedS)
  {}

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::add_in_edge(
      edge_descriptor const& edge, void* old_property_ptr, bidirectionalS)
  {
      if (owner(target(edge, m_g)) == m_g.processor())
      {
          detail::parallel::stored_in_edge<local_edge_descriptor>
              e(m_g.processor(), local(edge));
          boost::graph_detail::push(get(
              vertex_in_edges, m_g.base())[local(target(edge, m_g))], e);
      }
      else
      {
          // We send the (old,new) property pointer pair to
          // the remote process. This could be optimized to
          // only send the new one -- the ordering can be
          // made implicit because the old pointer value is
          // stored on the remote process.
          //
          // Doing that is a little bit more complicated, but
          // in case it turns out it's important we can do it.
          void* property_ptr = local(edge).get_property();
          m_property_ptrs[owner(target(edge, m_g))].push_back(
              unsafe_pair<void*,void*>(old_property_ptr, property_ptr));
      }
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::resolve_property_ptrs()
  {
# ifdef PBGL_SERIALIZE_DEBUG
      if (is_root())
          std::cout << "Resolving property pointers\n";
# endif

      for (int i = 0; i < num_processes(m_pg); ++i)
      {
          std::sort(
              m_property_ptrs[i].begin(), m_property_ptrs[i].end());
      }

      boost::parallel::inplace_all_to_all(m_pg, m_property_ptrs);
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::resolve_remote_vertices(directedS)
  {
      for (int i = 0; i < num_processes(m_pg); ++i)
      {
          std::sort(m_requested_vertices[i].begin(), m_requested_vertices[i].end());
      }

      boost::parallel::inplace_all_to_all(
          m_pg, m_requested_vertices, m_remote_vertices);

      for (int i = 0; i < num_processes(m_pg); ++i)
      {
          BOOST_FOREACH(serializable_vertex_descriptor& u, m_remote_vertices[i])
          {
              u = m_local_vertices[u];
          }
      }

      boost::parallel::inplace_all_to_all(m_pg, m_remote_vertices);
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::resolve_remote_vertices(bidirectionalS)
  {
# ifdef PBGL_SERIALIZE_DEBUG
      if (is_root())
          std::cout << "Resolving remote vertices\n";
# endif

      for (int i = 0; i < num_processes(m_pg); ++i)
      {
          std::sort(m_requested_vertices[i].begin(), m_requested_vertices[i].end());
          std::sort(m_remote_vertices[i].begin(), m_remote_vertices[i].end());

          BOOST_FOREACH(serializable_vertex_descriptor& u, m_remote_vertices[i])
          {
              u = m_local_vertices[u];
          }
      }

      boost::parallel::inplace_all_to_all(m_pg, m_remote_vertices);

      for (int i = 0; i < num_processes(m_pg); ++i)
          BOOST_ASSERT(m_remote_vertices[i].size() == m_requested_vertices[i].size());
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::commit_pending_edges(vecS)
  {
      commit_pending_in_edges(directed_selector());
  }

  template <class Graph, class Archive, class VertexListS>
  template <class Anything>
  void graph_loader<Graph, Archive, VertexListS>::commit_pending_edges(Anything)
  {
      resolve_remote_vertices(directed_selector());

      BOOST_FOREACH(pending_edge_type const& e, m_pending_edges)
      {
          vertex_descriptor u = resolve_remote_vertex(e.source);
          vertex_descriptor v = resolve_remote_vertex(e.target);
          add_edge(u, v, e.properties, e.property_ptr, vecS());
      }

      commit_pending_in_edges(directed_selector());
  }

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::commit_pending_in_edges(directedS)
  {}

  template <class Graph, class Archive, class VertexListS>
  void graph_loader<Graph, Archive, VertexListS>::commit_pending_in_edges(bidirectionalS)
  {
      resolve_property_ptrs();

      BOOST_FOREACH(pending_in_edge const& e, m_pending_in_edges)
      {
          vertex_descriptor u = resolve_remote_vertex(e.u, vertex_list_selector());
          vertex_descriptor v = resolve_remote_vertex(e.v, vertex_list_selector());

          typedef detail::parallel::stored_in_edge<local_edge_descriptor> stored_edge;

          std::vector<unsafe_pair<void*,void*> >::iterator i = std::lower_bound(
              m_property_ptrs[owner(u)].begin()
            , m_property_ptrs[owner(u)].end()
            , unsafe_pair<void*,void*>(e.property_ptr, 0)
          );

          if (i == m_property_ptrs[owner(u)].end()
              || i->first != e.property_ptr)
          {
              BOOST_ASSERT(false);
          }

          local_edge_descriptor local_edge(local(u), local(v), i->second);
          stored_edge edge(owner(u), local_edge);
          boost::graph_detail::push(
              get(vertex_in_edges, m_g.base())[local(v)], edge);
      }
  }

  template <class Graph, class Archive, class VertexListS>
  typename graph_loader<Graph, Archive, VertexListS>::vertex_descriptor 
  graph_loader<Graph, Archive, VertexListS>::resolve_remote_vertex(
      vertex_descriptor u) const
  {
      if (owner(u) == process_id(m_pg))
      { 
          return vertex_descriptor(
              process_id(m_pg), m_local_vertices.find(local(u))->second);
      }

      typename std::vector<serializable_vertex_descriptor>::const_iterator 
          i = std::lower_bound(
              m_requested_vertices[owner(u)].begin()
            , m_requested_vertices[owner(u)].end()
            , serializable_vertex_descriptor(local(u))
          );

      if (i == m_requested_vertices[owner(u)].end()
          || *i != local(u))
      {
          BOOST_ASSERT(false);
      }

      local_vertex_descriptor local =
          m_remote_vertices[owner(u)][m_requested_vertices[owner(u)].end() - i];
      return vertex_descriptor(owner(u), local);
  }

  template <class Graph, class Archive, class VertexListS>
  typename graph_loader<Graph, Archive, VertexListS>::vertex_descriptor 
  graph_loader<Graph, Archive, VertexListS>::resolve_remote_vertex(
      vertex_descriptor u, vecS) const
  {
      return u;
  }

  template <class Graph, class Archive, class VertexListS>
  template <class Anything>
  typename graph_loader<Graph, Archive, VertexListS>::vertex_descriptor 
  graph_loader<Graph, Archive, VertexListS>::resolve_remote_vertex(
      vertex_descriptor u, Anything) const
  {
      return resolve_remote_vertex(u);
  }

  template <class Graph, class Archive, class VertexListS>
  void* 
  graph_loader<Graph, Archive, VertexListS>::maybe_load_property_ptr(bidirectionalS)
  {
      void* ptr;
      m_ar >> make_nvp("property_ptr", unsafe_serialize(ptr));
      return ptr;
  }

template <class Archive, class D>
void maybe_save_local_descriptor(Archive& ar, D const&, vecS)
{}

template <class Archive, class D, class NotVecS>
void maybe_save_local_descriptor(Archive& ar, D const& d, NotVecS)
{
    ar << serialization::make_nvp(
        "local", unsafe_serialize(const_cast<D&>(d)));
}

template <class Archive>
void maybe_save_properties(
    Archive&, char const*, no_property const&)
{}

template <class Archive, class Tag, class T, class Base>
void maybe_save_properties(
    Archive& ar, char const* name, property<Tag, T, Base> const& properties)
{
    ar & serialization::make_nvp(name, get_property_value(properties, Tag()));
    maybe_save_properties(ar, name, static_cast<Base const&>(properties));
}

template <class Archive, class Graph>
void save_in_edges(Archive& ar, Graph const& g, directedS)
{}

// We need to save the edges in the base edge
// list, and the in_edges that are stored in the
// vertex_in_edges vertex property.
template <class Archive, class Graph>
void save_in_edges(Archive& ar, Graph const& g, bidirectionalS)
{
    typedef typename Graph::process_group_type
        process_group_type;
    typedef typename process_group_type::process_id_type
        process_id_type;
    typedef typename graph_traits<
        Graph>::vertex_descriptor vertex_descriptor;
    typedef typename vertex_descriptor::local_descriptor_type 
        local_vertex_descriptor;
    typedef typename graph_traits<
        Graph>::edge_descriptor edge_descriptor;

    process_id_type id = g.processor();

    std::vector<edge_descriptor> saved_in_edges;

    BGL_FORALL_VERTICES_T(v, g, Graph) 
    {
        BOOST_FOREACH(edge_descriptor const& e, in_edges(v, g))
        {
            // Only save the in_edges that isn't owned by this process.
            if (owner(e) == id)
                continue;

            saved_in_edges.push_back(e);
        }
    }

    std::size_t I = saved_in_edges.size();
    ar << BOOST_SERIALIZATION_NVP(I);

    BOOST_FOREACH(edge_descriptor const& e, saved_in_edges)
    {
        process_id_type src_owner = owner(source(e,g));
        local_vertex_descriptor local_src = local(source(e,g));
        local_vertex_descriptor local_target = local(target(e,g));
        void* property_ptr = local(e).get_property();

        using serialization::make_nvp;

        ar << make_nvp("src_owner", src_owner);
        ar << make_nvp("source", unsafe_serialize(local_src));
        ar << make_nvp("target", unsafe_serialize(local_target));
        ar << make_nvp("property_ptr", unsafe_serialize(property_ptr));
    }
}

template <class Archive, class Edge>
void maybe_save_property_ptr(Archive&, Edge const&, directedS)
{}

template <class Archive, class Edge>
void maybe_save_property_ptr(Archive& ar, Edge const& e, bidirectionalS)
{
    void* ptr = local(e).get_property();
    ar << serialization::make_nvp("property_ptr", unsafe_serialize(ptr));
}

template <class Archive, class Graph, class DirectedS>
void save_edges(Archive& ar, Graph const& g, DirectedS)
{
    typedef typename Graph::process_group_type
        process_group_type;
    typedef typename process_group_type::process_id_type
        process_id_type;
    typedef typename graph_traits<
        Graph>::vertex_descriptor vertex_descriptor;

    typedef typename Graph::edge_property_type edge_property_type;

    int E = num_edges(g);
    ar << BOOST_SERIALIZATION_NVP(E);

    // For *directed* graphs, we can just save
    // the edge list and be done.
    //
    // For *bidirectional* graphs, we need to also
    // save the "vertex_in_edges" property map,
    // because it might contain in-edges that
    // are not locally owned.
    BGL_FORALL_EDGES_T(e, g, Graph) 
    {
        vertex_descriptor src(source(e, g));
        vertex_descriptor tgt(target(e, g));

        typename vertex_descriptor::local_descriptor_type
            local_u(local(src));
        typename vertex_descriptor::local_descriptor_type
            local_v(local(tgt));

        process_id_type target_owner = owner(tgt);

        using serialization::make_nvp;

        ar << make_nvp("source", unsafe_serialize(local_u)); 
        ar << make_nvp("target_owner", target_owner); 
        ar << make_nvp("target", unsafe_serialize(local_v)); 

        maybe_save_properties(
            ar, "edge_property"
          , static_cast<edge_property_type const&>(get(edge_all_t(), g, e))
        );

        maybe_save_property_ptr(ar, e, DirectedS());
    }

    save_in_edges(ar, g, DirectedS());
}

}} // namespace detail::parallel

template <PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
template <class IStreamConstructibleArchive>
void PBGL_DISTRIB_ADJLIST_TYPE::load(std::string const& filename)
{
    process_group_type pg = process_group();
    process_id_type id = process_id(pg);

    synchronize(pg);

    std::vector<int> disk_files = detail::parallel::available_process_files(filename);
    std::sort(disk_files.begin(), disk_files.end());

    // Negotiate which process gets which file. Serialized.
    std::vector<int> consumed_files;
    int picked_file = -1;

    if (id > 0)
        receive_oob(pg, id-1, 0, consumed_files);

    std::sort(consumed_files.begin(), consumed_files.end());
    std::vector<int> available_files;
    std::set_difference(
        disk_files.begin(), disk_files.end()
      , consumed_files.begin(), consumed_files.end()
      , std::back_inserter(available_files)
    );

    if (available_files.empty())
        boost::throw_exception(std::runtime_error("no file available"));

    // back() used for debug purposes. Making sure the
    // ranks are shuffled.
    picked_file = available_files.back();

# ifdef PBGL_SERIALIZE_DEBUG
    std::cout << id << " picked " << picked_file << "\n";
# endif

    consumed_files.push_back(picked_file);

    if (id < num_processes(pg) - 1)
        send_oob(pg, id+1, 0, consumed_files);

    std::string local_filename = filename + "/" + 
        lexical_cast<std::string>(picked_file);

    std::ifstream in(local_filename.c_str(), std::ios_base::binary);
    IStreamConstructibleArchive ar(in);

    detail::parallel::graph_loader<
        graph_type, IStreamConstructibleArchive, InVertexListS
    > loader(*this, ar);

# ifdef PBGL_SERIALIZE_DEBUG
    std::cout << "Process " << id << " done loading.\n";
# endif

    synchronize(pg);
}

template <PBGL_DISTRIB_ADJLIST_TEMPLATE_PARMS>
template <class OStreamConstructibleArchive>
void PBGL_DISTRIB_ADJLIST_TYPE::save(std::string const& filename) const
{
    typedef typename config_type::VertexListS vertex_list_selector;

    process_group_type pg = process_group();
    process_id_type id = process_id(pg);

    if (filesystem::exists(filename) && !filesystem::is_directory(filename))
        boost::throw_exception(std::runtime_error("entry exists, but is not a directory"));

    filesystem::remove_all(filename);
    filesystem::create_directory(filename);

    synchronize(pg);

    std::string local_filename = filename + "/" + 
        lexical_cast<std::string>(id);

    std::ofstream out(local_filename.c_str(), std::ios_base::binary);
    OStreamConstructibleArchive ar(out);

    using serialization::make_nvp;

    typename process_group_type::process_size_type num_processes_ = num_processes(pg);
    ar << make_nvp("num_processes", num_processes_);
    ar << BOOST_SERIALIZATION_NVP(id);
    ar << make_nvp("mapping", this->distribution().mapping());

    int V = num_vertices(*this);
    ar << BOOST_SERIALIZATION_NVP(V);

    BGL_FORALL_VERTICES_T(v, *this, graph_type)
    {
        local_vertex_descriptor local_descriptor(local(v));
        detail::parallel::maybe_save_local_descriptor(
            ar, local_descriptor, vertex_list_selector());
        detail::parallel::maybe_save_properties(
            ar, "vertex_property"
          , static_cast<vertex_property_type const&>(get(vertex_all_t(), *this, v))
        );
    }

    detail::parallel::save_edges(ar, *this, directed_selector());

    ar << make_nvp("distribution", this->distribution());
}

} // namespace boost

#endif // BOOST_GRAPH_DISTRIBUTED_ADJLIST_SERIALIZATION_070925_HPP

