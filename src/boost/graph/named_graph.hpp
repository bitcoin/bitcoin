// Copyright (C) 2007 Douglas Gregor

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Provides support for named vertices in graphs, allowing one to more
// easily associate unique external names (URLs, city names, employee
// ID numbers, etc.) with the vertices of a graph.
#ifndef BOOST_GRAPH_NAMED_GRAPH_HPP
#define BOOST_GRAPH_NAMED_GRAPH_HPP

#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/functional/hash.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/optional.hpp>
#include <boost/pending/property.hpp> // for boost::lookup_one_property
#include <boost/pending/container_traits.hpp>
#include <boost/throw_exception.hpp>
#include <boost/tuple/tuple.hpp> // for boost::make_tuple
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <functional> // for std::equal_to
#include <stdexcept> // for std::runtime_error
#include <utility> // for std::pair

namespace boost
{
namespace graph
{

    /*******************************************************************
     * User-customized traits                                          *
     *******************************************************************/

    /**
     * @brief Trait used to extract the internal vertex name from a vertex
     * property.
     *
     * To enable the use of internal vertex names in a graph type,
     * specialize the @c internal_vertex_name trait for your graph
     * property (e.g., @c a City class, which stores information about the
     * vertices in a road map).
     */
    template < typename VertexProperty > struct internal_vertex_name
    {
        /**
         *  The @c type field provides a function object that extracts a key
         *  from the @c VertexProperty. The function object type must have a
         *  nested @c result_type that provides the type of the key. For
         *  more information, see the @c KeyExtractor concept in the
         *  Boost.MultiIndex documentation: @c type must either be @c void
         *  (if @c VertexProperty does not have an internal vertex name) or
         *  a model of @c KeyExtractor.
         */
        typedef void type;
    };

    /**
     * Extract the internal vertex name from a @c property structure by
     * looking at its base.
     */
    template < typename Tag, typename T, typename Base >
    struct internal_vertex_name< property< Tag, T, Base > >
    : internal_vertex_name< Base >
    {
    };

    /**
     * Construct an instance of @c VertexProperty directly from its
     * name. This function object should be used within the @c
     * internal_vertex_constructor trait.
     */
    template < typename VertexProperty > struct vertex_from_name
    {
    private:
        typedef typename internal_vertex_name< VertexProperty >::type
            extract_name_type;

        typedef typename remove_cv< typename remove_reference<
            typename extract_name_type::result_type >::type >::type
            vertex_name_type;

    public:
        typedef vertex_name_type argument_type;
        typedef VertexProperty result_type;

        VertexProperty operator()(const vertex_name_type& name)
        {
            return VertexProperty(name);
        }
    };

    /**
     * Throw an exception whenever one tries to construct a @c
     * VertexProperty instance from its name.
     */
    template < typename VertexProperty > struct cannot_add_vertex
    {
    private:
        typedef typename internal_vertex_name< VertexProperty >::type
            extract_name_type;

        typedef typename remove_cv< typename remove_reference<
            typename extract_name_type::result_type >::type >::type
            vertex_name_type;

    public:
        typedef vertex_name_type argument_type;
        typedef VertexProperty result_type;

        VertexProperty operator()(const vertex_name_type&)
        {
            boost::throw_exception(
                std::runtime_error("add_vertex: "
                                   "unable to create a vertex from its name"));
        }
    };

    /**
     * @brief Trait used to construct an instance of a @c VertexProperty,
     * which is a class type that stores the properties associated with a
     * vertex in a graph, from just the name of that vertex property. This
     * operation is used when an operation is required to map from a
     * vertex name to a vertex descriptor (e.g., to add an edge outgoing
     * from that vertex), but no vertex by the name exists. The function
     * object provided by this trait will be used to add new vertices
     * based only on their names. Since this cannot be done for arbitrary
     * types, the default behavior is to throw an exception when this
     * routine is called, which requires that all named vertices be added
     * before adding any edges by name.
     */
    template < typename VertexProperty > struct internal_vertex_constructor
    {
        /**
         * The @c type field provides a function object that constructs a
         * new instance of @c VertexProperty from the name of the vertex (as
         * determined by @c internal_vertex_name). The function object shall
         * accept a vertex name and return a @c VertexProperty. Predefined
         * options include:
         *
         *   @c vertex_from_name<VertexProperty>: construct an instance of
         *   @c VertexProperty directly from the name.
         *
         *   @c cannot_add_vertex<VertexProperty>: the default value, which
         *   throws an @c std::runtime_error if one attempts to add a vertex
         *   given just the name.
         */
        typedef cannot_add_vertex< VertexProperty > type;
    };

    /**
     * Extract the internal vertex constructor from a @c property structure by
     * looking at its base.
     */
    template < typename Tag, typename T, typename Base >
    struct internal_vertex_constructor< property< Tag, T, Base > >
    : internal_vertex_constructor< Base >
    {
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
     *   VertexProperty: the type stored with each vertex in the Graph.
     */
    template < typename Graph, typename Vertex, typename VertexProperty >
    class named_graph
    {
    public:
        /// The type of the function object that extracts names from vertex
        /// properties.
        typedef typename internal_vertex_name< VertexProperty >::type
            extract_name_type;
        /// The type of the "bundled" property, from which the name can be
        /// extracted.
        typedef typename lookup_one_property< VertexProperty,
            vertex_bundle_t >::type bundled_vertex_property_type;

        /// The type of the function object that generates vertex properties
        /// from names, for the implicit addition of vertices.
        typedef typename internal_vertex_constructor< VertexProperty >::type
            vertex_constructor_type;

        /// The type used to name vertices in the graph
        typedef typename remove_cv< typename remove_reference<
            typename extract_name_type::result_type >::type >::type
            vertex_name_type;

        /// The type of vertex descriptors in the graph
        typedef Vertex vertex_descriptor;

    private:
        /// Key extractor for use with the multi_index_container
        struct extract_name_from_vertex
        {
            typedef vertex_name_type result_type;

            extract_name_from_vertex(
                Graph& graph, const extract_name_type& extract)
            : graph(graph), extract(extract)
            {
            }

            const result_type& operator()(Vertex vertex) const
            {
                return extract(graph[vertex]);
            }

            Graph& graph;
            extract_name_type extract;
        };

    public:
        /// The type that maps names to vertices
        typedef multi_index::multi_index_container< Vertex,
            multi_index::indexed_by<
                multi_index::hashed_unique< multi_index::tag< vertex_name_t >,
                    extract_name_from_vertex > > >
            named_vertices_type;

        /// The set of vertices, indexed by name
        typedef
            typename named_vertices_type::template index< vertex_name_t >::type
                vertices_by_name_type;

        /// Construct an instance of the named graph mixin, using the given
        /// function object to extract a name from the bundled property
        /// associated with a vertex.
        named_graph(const extract_name_type& extract = extract_name_type(),
            const vertex_constructor_type& vertex_constructor
            = vertex_constructor_type());

        /// Notify the named_graph that we have added the given vertex. The
        /// name of the vertex will be added to the mapping.
        void added_vertex(Vertex vertex);

        /// Notify the named_graph that we are removing the given
        /// vertex. The name of the vertex will be removed from the mapping.
        template < typename VertexIterStability >
        void removing_vertex(Vertex vertex, VertexIterStability);

        /// Notify the named_graph that we are clearing the graph.
        /// This will clear out all of the name->vertex mappings
        void clearing_graph();

        /// Retrieve the derived instance
        Graph& derived() { return static_cast< Graph& >(*this); }
        const Graph& derived() const
        {
            return static_cast< const Graph& >(*this);
        }

        /// Extract the name from a vertex property instance
        typename extract_name_type::result_type extract_name(
            const bundled_vertex_property_type& property);

        /// Search for a vertex that has the given property (based on its
        /// name)
        optional< vertex_descriptor > vertex_by_property(
            const bundled_vertex_property_type& property);

        /// Mapping from names to vertices
        named_vertices_type named_vertices;

        /// Constructs a vertex from the name of that vertex
        vertex_constructor_type vertex_constructor;
    };

/// Helper macro containing the template parameters of named_graph
#define BGL_NAMED_GRAPH_PARAMS \
    typename Graph, typename Vertex, typename VertexProperty
/// Helper macro containing the named_graph<...> instantiation
#define BGL_NAMED_GRAPH named_graph< Graph, Vertex, VertexProperty >

    template < BGL_NAMED_GRAPH_PARAMS >
    BGL_NAMED_GRAPH::named_graph(const extract_name_type& extract,
        const vertex_constructor_type& vertex_constructor)
    : named_vertices(typename named_vertices_type::ctor_args_list(
        boost::make_tuple(boost::make_tuple(0, // initial number of buckets
            extract_name_from_vertex(derived(), extract),
            boost::hash< vertex_name_type >(),
            std::equal_to< vertex_name_type >()))))
    , vertex_constructor(vertex_constructor)
    {
    }

    template < BGL_NAMED_GRAPH_PARAMS >
    inline void BGL_NAMED_GRAPH::added_vertex(Vertex vertex)
    {
        named_vertices.insert(vertex);
    }

    template < BGL_NAMED_GRAPH_PARAMS >
    template < typename VertexIterStability >
    inline void BGL_NAMED_GRAPH::removing_vertex(
        Vertex vertex, VertexIterStability)
    {
        BOOST_STATIC_ASSERT_MSG(
            (boost::is_base_of< boost::graph_detail::stable_tag,
                VertexIterStability >::value),
            "Named graphs cannot use vecS as vertex container and remove "
            "vertices; the lack of vertex descriptor stability (which iterator "
            "stability is a proxy for) means that the name -> vertex mapping "
            "would need to be completely rebuilt after each deletion.  See "
            "https://svn.boost.org/trac/boost/ticket/7863 for more information "
            "and a test case.");
        typedef typename BGL_NAMED_GRAPH::vertex_name_type vertex_name_type;
        const vertex_name_type& vertex_name = extract_name(derived()[vertex]);
        named_vertices.erase(vertex_name);
    }

    template < BGL_NAMED_GRAPH_PARAMS >
    inline void BGL_NAMED_GRAPH::clearing_graph()
    {
        named_vertices.clear();
    }

    template < BGL_NAMED_GRAPH_PARAMS >
    typename BGL_NAMED_GRAPH::extract_name_type::result_type
    BGL_NAMED_GRAPH::extract_name(const bundled_vertex_property_type& property)
    {
        return named_vertices.key_extractor().extract(property);
    }

    template < BGL_NAMED_GRAPH_PARAMS >
    optional< typename BGL_NAMED_GRAPH::vertex_descriptor >
    BGL_NAMED_GRAPH::vertex_by_property(
        const bundled_vertex_property_type& property)
    {
        return find_vertex(extract_name(property), *this);
    }

    /// Retrieve the vertex associated with the given name
    template < BGL_NAMED_GRAPH_PARAMS >
    optional< Vertex > find_vertex(
        typename BGL_NAMED_GRAPH::vertex_name_type const& name,
        const BGL_NAMED_GRAPH& g)
    {
        typedef typename BGL_NAMED_GRAPH::vertices_by_name_type
            vertices_by_name_type;

        // Retrieve the set of vertices indexed by name
        vertices_by_name_type const& vertices_by_name
            = g.named_vertices.template get< vertex_name_t >();

        /// Look for a vertex with the given name
        typename vertices_by_name_type::const_iterator iter
            = vertices_by_name.find(name);

        if (iter == vertices_by_name.end())
            return optional< Vertex >(); // vertex not found
        else
            return *iter;
    }

    /// Retrieve the vertex associated with the given name, or add a new
    /// vertex with that name if no such vertex is available.
    /// Note: This is enabled only when the vertex property type is different
    ///       from the vertex name to avoid ambiguous overload problems with
    ///       the add_vertex() function that takes a vertex property.
    template < BGL_NAMED_GRAPH_PARAMS >
    typename disable_if<
        is_same< typename BGL_NAMED_GRAPH::vertex_name_type, VertexProperty >,
        Vertex >::type
    add_vertex(typename BGL_NAMED_GRAPH::vertex_name_type const& name,
        BGL_NAMED_GRAPH& g)
    {
        if (optional< Vertex > vertex = find_vertex(name, g))
            /// We found the vertex, so return it
            return *vertex;
        else
            /// There is no vertex with the given name, so create one
            return add_vertex(g.vertex_constructor(name), g.derived());
    }

    /// Add an edge using vertex names to refer to the vertices
    template < BGL_NAMED_GRAPH_PARAMS >
    std::pair< typename graph_traits< Graph >::edge_descriptor, bool > add_edge(
        typename BGL_NAMED_GRAPH::vertex_name_type const& u_name,
        typename BGL_NAMED_GRAPH::vertex_name_type const& v_name,
        BGL_NAMED_GRAPH& g)
    {
        return add_edge(add_vertex(u_name, g.derived()),
            add_vertex(v_name, g.derived()), g.derived());
    }

    /// Add an edge using vertex descriptors or names to refer to the vertices
    template < BGL_NAMED_GRAPH_PARAMS >
    std::pair< typename graph_traits< Graph >::edge_descriptor, bool > add_edge(
        typename BGL_NAMED_GRAPH::vertex_descriptor const& u,
        typename BGL_NAMED_GRAPH::vertex_name_type const& v_name,
        BGL_NAMED_GRAPH& g)
    {
        return add_edge(u, add_vertex(v_name, g.derived()), g.derived());
    }

    /// Add an edge using vertex descriptors or names to refer to the vertices
    template < BGL_NAMED_GRAPH_PARAMS >
    std::pair< typename graph_traits< Graph >::edge_descriptor, bool > add_edge(
        typename BGL_NAMED_GRAPH::vertex_name_type const& u_name,
        typename BGL_NAMED_GRAPH::vertex_descriptor const& v,
        BGL_NAMED_GRAPH& g)
    {
        return add_edge(add_vertex(u_name, g.derived()), v, g.derived());
    }

    // Overloads to support EdgeMutablePropertyGraph graphs
    template < BGL_NAMED_GRAPH_PARAMS >
    std::pair< typename graph_traits< Graph >::edge_descriptor, bool > add_edge(
        typename BGL_NAMED_GRAPH::vertex_descriptor const& u,
        typename BGL_NAMED_GRAPH::vertex_name_type const& v_name,
        typename edge_property_type< Graph >::type const& p, BGL_NAMED_GRAPH& g)
    {
        return add_edge(u, add_vertex(v_name, g.derived()), p, g.derived());
    }

    template < BGL_NAMED_GRAPH_PARAMS >
    std::pair< typename graph_traits< Graph >::edge_descriptor, bool > add_edge(
        typename BGL_NAMED_GRAPH::vertex_name_type const& u_name,
        typename BGL_NAMED_GRAPH::vertex_descriptor const& v,
        typename edge_property_type< Graph >::type const& p, BGL_NAMED_GRAPH& g)
    {
        return add_edge(add_vertex(u_name, g.derived()), v, p, g.derived());
    }

    template < BGL_NAMED_GRAPH_PARAMS >
    std::pair< typename graph_traits< Graph >::edge_descriptor, bool > add_edge(
        typename BGL_NAMED_GRAPH::vertex_name_type const& u_name,
        typename BGL_NAMED_GRAPH::vertex_name_type const& v_name,
        typename edge_property_type< Graph >::type const& p, BGL_NAMED_GRAPH& g)
    {
        return add_edge(add_vertex(u_name, g.derived()),
            add_vertex(v_name, g.derived()), p, g.derived());
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
    template < typename Graph, typename Vertex, typename VertexProperty,
        typename ExtractName =
            typename internal_vertex_name< VertexProperty >::type >
    struct maybe_named_graph
    : public named_graph< Graph, Vertex, VertexProperty >
    {
    };

    /**
     * A graph mixin that can provide a mapping from names to vertices,
     * and use that mapping to simplify creation and manipulation of
     * graphs. This partial specialization turns off this functionality
     * when the @c VertexProperty does not have an internal vertex name.
     */
    template < typename Graph, typename Vertex, typename VertexProperty >
    struct maybe_named_graph< Graph, Vertex, VertexProperty, void >
    {
        /// The type of the "bundled" property, from which the name can be
        /// extracted.
        typedef typename lookup_one_property< VertexProperty,
            vertex_bundle_t >::type bundled_vertex_property_type;

        /// Notify the named_graph that we have added the given vertex. This
        /// is a no-op.
        void added_vertex(Vertex) {}

        /// Notify the named_graph that we are removing the given
        /// vertex. This is a no-op.
        template < typename VertexIterStability >
        void removing_vertex(Vertex, VertexIterStability)
        {
        }

        /// Notify the named_graph that we are clearing the graph. This is a
        /// no-op.
        void clearing_graph() {}

        /// Search for a vertex that has the given property (based on its
        /// name). This always returns an empty optional<>
        optional< Vertex > vertex_by_property(
            const bundled_vertex_property_type&)
        {
            return optional< Vertex >();
        }
    };

}
} // end namespace boost::graph

#endif // BOOST_GRAPH_NAMED_GRAPH_HPP
