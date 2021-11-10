// Copyright (C) 2006  Tiago de Paula Peixoto <tiago@forked.de>
// Copyright (C) 2004  The Trustees of Indiana University.
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  Authors: Douglas Gregor
//           Andrew Lumsdaine
//           Tiago de Paula Peixoto

#ifndef BOOST_GRAPH_GRAPHML_HPP
#define BOOST_GRAPH_GRAPHML_HPP

#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/any.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/graph/dll_import_export.hpp>
#include <boost/graph/graphviz.hpp> // for exceptions
#include <boost/mpl/bool.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/property_tree/detail/xml_parser_utils.hpp>
#include <boost/throw_exception.hpp>
#include <exception>
#include <sstream>
#include <typeinfo>

namespace boost
{

/////////////////////////////////////////////////////////////////////////////
// Graph reader exceptions
/////////////////////////////////////////////////////////////////////////////
struct BOOST_SYMBOL_VISIBLE parse_error : public graph_exception
{
    parse_error(const std::string& err)
    {
        error = err;
        statement = "parse error: " + error;
    }
    ~parse_error() throw() BOOST_OVERRIDE {}
    const char* what() const throw() BOOST_OVERRIDE { return statement.c_str(); }
    std::string statement;
    std::string error;
};

class mutate_graph
{
public:
    virtual ~mutate_graph() {}
    virtual bool is_directed() const = 0;

    virtual boost::any do_add_vertex() = 0;
    virtual std::pair< boost::any, bool > do_add_edge(
        boost::any source, boost::any target)
        = 0;

    virtual void set_graph_property(const std::string& name,
        const std::string& value, const std::string& value_type)
        = 0;

    virtual void set_vertex_property(const std::string& name, boost::any vertex,
        const std::string& value, const std::string& value_type)
        = 0;

    virtual void set_edge_property(const std::string& name, boost::any edge,
        const std::string& value, const std::string& value_type)
        = 0;
};

template < typename MutableGraph > class mutate_graph_impl : public mutate_graph
{
    typedef typename graph_traits< MutableGraph >::vertex_descriptor
        vertex_descriptor;
    typedef
        typename graph_traits< MutableGraph >::edge_descriptor edge_descriptor;

public:
    mutate_graph_impl(MutableGraph& g, dynamic_properties& dp)
    : m_g(g), m_dp(dp)
    {
    }

    bool is_directed() const BOOST_OVERRIDE
    {
        return is_convertible<
            typename graph_traits< MutableGraph >::directed_category,
            directed_tag >::value;
    }

    any do_add_vertex() BOOST_OVERRIDE { return any(add_vertex(m_g)); }

    std::pair< any, bool > do_add_edge(any source, any target) BOOST_OVERRIDE
    {
        std::pair< edge_descriptor, bool > retval
            = add_edge(any_cast< vertex_descriptor >(source),
                any_cast< vertex_descriptor >(target), m_g);
        return std::make_pair(any(retval.first), retval.second);
    }

    void set_graph_property(const std::string& name,
        const std::string& value, const std::string& value_type) BOOST_OVERRIDE
    {
        bool type_found = false;
        try
        {
            mpl::for_each< value_types >(
                put_property< MutableGraph*, value_types >(name, m_dp, &m_g,
                    value, value_type, m_type_names, type_found));
        }
        catch (const bad_lexical_cast&)
        {
            BOOST_THROW_EXCEPTION(parse_error("invalid value \"" + value
                + "\" for key " + name + " of type " + value_type));
        }
        if (!type_found)
        {
            BOOST_THROW_EXCEPTION(parse_error(
                "unrecognized type \"" + value_type + "\" for key " + name));
        }
    }

    void set_vertex_property(const std::string& name, any vertex,
        const std::string& value, const std::string& value_type) BOOST_OVERRIDE
    {
        bool type_found = false;
        try
        {
            mpl::for_each< value_types >(
                put_property< vertex_descriptor, value_types >(name, m_dp,
                    any_cast< vertex_descriptor >(vertex), value, value_type,
                    m_type_names, type_found));
        }
        catch (const bad_lexical_cast&)
        {
            BOOST_THROW_EXCEPTION(parse_error("invalid value \"" + value
                + "\" for key " + name + " of type " + value_type));
        }
        if (!type_found)
        {
            BOOST_THROW_EXCEPTION(parse_error(
                "unrecognized type \"" + value_type + "\" for key " + name));
        }
    }

    void set_edge_property(const std::string& name, any edge,
        const std::string& value, const std::string& value_type) BOOST_OVERRIDE
    {
        bool type_found = false;
        try
        {
            mpl::for_each< value_types >(
                put_property< edge_descriptor, value_types >(name, m_dp,
                    any_cast< edge_descriptor >(edge), value, value_type,
                    m_type_names, type_found));
        }
        catch (const bad_lexical_cast&)
        {
            BOOST_THROW_EXCEPTION(parse_error("invalid value \"" + value
                + "\" for key " + name + " of type " + value_type));
        }
        if (!type_found)
        {
            BOOST_THROW_EXCEPTION(parse_error(
                "unrecognized type \"" + value_type + "\" for key " + name));
        }
    }

    template < typename Key, typename ValueVector > class put_property
    {
    public:
        put_property(const std::string& name, dynamic_properties& dp,
            const Key& key, const std::string& value,
            const std::string& value_type, const char** type_names,
            bool& type_found)
        : m_name(name)
        , m_dp(dp)
        , m_key(key)
        , m_value(value)
        , m_value_type(value_type)
        , m_type_names(type_names)
        , m_type_found(type_found)
        {
        }
        template < class Value > void operator()(Value)
        {
            if (m_value_type
                == m_type_names[mpl::find< ValueVector,
                    Value >::type::pos::value])
            {
                put(m_name, m_dp, m_key, lexical_cast< Value >(m_value));
                m_type_found = true;
            }
        }

    private:
        const std::string& m_name;
        dynamic_properties& m_dp;
        const Key& m_key;
        const std::string& m_value;
        const std::string& m_value_type;
        const char** m_type_names;
        bool& m_type_found;
    };

protected:
    MutableGraph& m_g;
    dynamic_properties& m_dp;
    typedef mpl::vector< bool, int, long, float, double, std::string >
        value_types;
    static const char* m_type_names[];
};

template < typename MutableGraph >
const char* mutate_graph_impl< MutableGraph >::m_type_names[]
    = { "boolean", "int", "long", "float", "double", "string" };

void BOOST_GRAPH_DECL read_graphml(
    std::istream& in, mutate_graph& g, size_t desired_idx);

template < typename MutableGraph >
void read_graphml(std::istream& in, MutableGraph& g, dynamic_properties& dp,
    size_t desired_idx = 0)
{
    mutate_graph_impl< MutableGraph > mg(g, dp);
    read_graphml(in, mg, desired_idx);
}

template < typename Types > class get_type_name
{
public:
    get_type_name(const std::type_info& type, const char** type_names,
        std::string& type_name)
    : m_type(type), m_type_names(type_names), m_type_name(type_name)
    {
    }
    template < typename Type > void operator()(Type)
    {
        if (typeid(Type) == m_type)
            m_type_name
                = m_type_names[mpl::find< Types, Type >::type::pos::value];
    }

private:
    const std::type_info& m_type;
    const char** m_type_names;
    std::string& m_type_name;
};

template < typename Graph, typename VertexIndexMap >
void write_graphml(std::ostream& out, const Graph& g,
    VertexIndexMap vertex_index, const dynamic_properties& dp,
    bool ordered_vertices = false)
{
    typedef typename graph_traits< Graph >::directed_category directed_category;
    typedef typename graph_traits< Graph >::edge_descriptor edge_descriptor;
    typedef typename graph_traits< Graph >::vertex_descriptor vertex_descriptor;

    using boost::property_tree::xml_parser::encode_char_entities;

    BOOST_STATIC_CONSTANT(bool,
        graph_is_directed
        = (is_convertible< directed_category*, directed_tag* >::value));

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
           "xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns "
           "http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">\n";

    typedef mpl::vector< bool, short, unsigned short, int, unsigned int, long,
        unsigned long, long long, unsigned long long, float, double,
        long double, std::string >
        value_types;
    const char* type_names[] = { "boolean", "int", "int", "int", "int", "long",
        "long", "long", "long", "float", "double", "double", "string" };
    std::map< std::string, std::string > graph_key_ids;
    std::map< std::string, std::string > vertex_key_ids;
    std::map< std::string, std::string > edge_key_ids;
    int key_count = 0;

    // Output keys
    for (dynamic_properties::const_iterator i = dp.begin(); i != dp.end(); ++i)
    {
        std::string key_id = "key" + lexical_cast< std::string >(key_count++);
        if (i->second->key() == typeid(Graph*))
            graph_key_ids[i->first] = key_id;
        else if (i->second->key() == typeid(vertex_descriptor))
            vertex_key_ids[i->first] = key_id;
        else if (i->second->key() == typeid(edge_descriptor))
            edge_key_ids[i->first] = key_id;
        else
            continue;
        std::string type_name = "string";
        mpl::for_each< value_types >(get_type_name< value_types >(
            i->second->value(), type_names, type_name));
        out << "  <key id=\"" << encode_char_entities(key_id) << "\" for=\""
            << (i->second->key() == typeid(Graph*)
                       ? "graph"
                       : (i->second->key() == typeid(vertex_descriptor)
                               ? "node"
                               : "edge"))
            << "\""
            << " attr.name=\"" << i->first << "\""
            << " attr.type=\"" << type_name << "\""
            << " />\n";
    }

    out << "  <graph id=\"G\" edgedefault=\""
        << (graph_is_directed ? "directed" : "undirected") << "\""
        << " parse.nodeids=\"" << (ordered_vertices ? "canonical" : "free")
        << "\""
        << " parse.edgeids=\"canonical\" parse.order=\"nodesfirst\">\n";

    // Output graph data
    for (dynamic_properties::const_iterator i = dp.begin(); i != dp.end(); ++i)
    {
        if (i->second->key() == typeid(Graph*))
        {
            // The const_cast here is just to get typeid correct for property
            // map key; the graph should not be mutated using it.
            out << "   <data key=\"" << graph_key_ids[i->first] << "\">"
                << encode_char_entities(
                       i->second->get_string(const_cast< Graph* >(&g)))
                << "</data>\n";
        }
    }

    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator;
    vertex_iterator v, v_end;
    for (boost::tie(v, v_end) = vertices(g); v != v_end; ++v)
    {
        out << "    <node id=\"n" << get(vertex_index, *v) << "\">\n";
        // Output data
        for (dynamic_properties::const_iterator i = dp.begin(); i != dp.end();
             ++i)
        {
            if (i->second->key() == typeid(vertex_descriptor))
            {
                out << "      <data key=\"" << vertex_key_ids[i->first] << "\">"
                    << encode_char_entities(i->second->get_string(*v))
                    << "</data>\n";
            }
        }
        out << "    </node>\n";
    }

    typedef typename graph_traits< Graph >::edge_iterator edge_iterator;
    edge_iterator e, e_end;
    typename graph_traits< Graph >::edges_size_type edge_count = 0;
    for (boost::tie(e, e_end) = edges(g); e != e_end; ++e)
    {
        out << "    <edge id=\"e" << edge_count++ << "\" source=\"n"
            << get(vertex_index, source(*e, g)) << "\" target=\"n"
            << get(vertex_index, target(*e, g)) << "\">\n";

        // Output data
        for (dynamic_properties::const_iterator i = dp.begin(); i != dp.end();
             ++i)
        {
            if (i->second->key() == typeid(edge_descriptor))
            {
                out << "      <data key=\"" << edge_key_ids[i->first] << "\">"
                    << encode_char_entities(i->second->get_string(*e))
                    << "</data>\n";
            }
        }
        out << "    </edge>\n";
    }

    out << "  </graph>\n"
        << "</graphml>\n";
}

template < typename Graph >
void write_graphml(std::ostream& out, const Graph& g,
    const dynamic_properties& dp, bool ordered_vertices = false)
{
    write_graphml(out, g, get(vertex_index, g), dp, ordered_vertices);
}

} // boost namespace

#endif // BOOST_GRAPH_GRAPHML_HPP
