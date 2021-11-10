// Copyright 2005 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Jeremiah Willcock
//           Douglas Gregor
//           Andrew Lumsdaine

// Indexed properties -- used for CSR and CSR-like graphs

#ifndef BOOST_GRAPH_INDEXED_PROPERTIES_HPP
#define BOOST_GRAPH_INDEXED_PROPERTIES_HPP

#include <vector>
#include <utility>
#include <algorithm>
#include <climits>
#include <iterator>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/integer.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/mpl/if.hpp>

namespace boost
{
namespace detail
{

    template < typename Derived, typename Property, typename Descriptor,
        typename IndexMap >
    class indexed_vertex_properties
    {
    public:
        typedef no_property vertex_property_type;
        typedef Property vertex_bundled;
        typedef iterator_property_map<
            typename std::vector< Property >::iterator, IndexMap >
            vertex_map_type;
        typedef iterator_property_map<
            typename std::vector< Property >::const_iterator, IndexMap >
            const_vertex_map_type;

        // Directly access a vertex or edge bundle
        Property& operator[](Descriptor v)
        {
            return m_vertex_properties[get(vertex_index, derived(), v)];
        }

        const Property& operator[](Descriptor v) const
        {
            return m_vertex_properties[get(vertex_index, derived(), v)];
        }

        vertex_map_type get_vertex_bundle(
            const IndexMap& index_map = IndexMap())
        {
            return vertex_map_type(m_vertex_properties.begin(), index_map);
        }

        const_vertex_map_type get_vertex_bundle(
            const IndexMap& index_map = IndexMap()) const
        {
            return const_vertex_map_type(
                m_vertex_properties.begin(), index_map);
        }

    protected:
        // Default-construct with no property values
        indexed_vertex_properties() {}

        // Initialize with n default-constructed property values
        indexed_vertex_properties(std::size_t n) : m_vertex_properties(n) {}

    public:
        // Clear the properties vector
        void clear() { m_vertex_properties.clear(); }

        // Resize the properties vector
        void resize(std::size_t n) { m_vertex_properties.resize(n); }

        // Reserve space in the vector of properties
        void reserve(std::size_t n) { m_vertex_properties.reserve(n); }

        // Add a new property value to the back
        void push_back(const Property& prop)
        {
            m_vertex_properties.push_back(prop);
        }

        // Write an element by raw index
        void write_by_index(std::size_t idx, const Property& prop)
        {
            m_vertex_properties[idx] = prop;
        }

        // Access to the derived object
        Derived& derived() { return *static_cast< Derived* >(this); }

        const Derived& derived() const
        {
            return *static_cast< const Derived* >(this);
        }

    public: // should be private, but friend templates not portable
        std::vector< Property > m_vertex_properties;
    };

    template < typename Derived, typename Descriptor, typename IndexMap >
    class indexed_vertex_properties< Derived, void, Descriptor, IndexMap >
    {
        struct secret
        {
        };

    public:
        typedef no_property vertex_property_type;
        typedef void vertex_bundled;
        typedef secret vertex_map_type;
        typedef secret const_vertex_map_type;

        secret operator[](secret) { return secret(); }

        vertex_map_type get_vertex_bundle() const { return vertex_map_type(); }

    protected:
        // All operations do nothing.
        indexed_vertex_properties() {}
        indexed_vertex_properties(std::size_t) {}

    public:
        void clear() {}
        void resize(std::size_t) {}
        void reserve(std::size_t) {}
    };

    template < typename Derived, typename Property, typename Descriptor,
        typename IndexMap >
    class indexed_edge_properties
    {
    public:
        typedef no_property edge_property_type;
        typedef Property edge_bundled;
        typedef Property edge_push_back_type;
        typedef iterator_property_map<
            typename std::vector< Property >::iterator, IndexMap >
            edge_map_type;
        typedef iterator_property_map<
            typename std::vector< Property >::const_iterator, IndexMap >
            const_edge_map_type;

        // Directly access a edge or edge bundle
        Property& operator[](Descriptor v)
        {
            return m_edge_properties[get(edge_index, derived(), v)];
        }

        const Property& operator[](Descriptor v) const
        {
            return m_edge_properties[get(edge_index, derived(), v)];
        }

        edge_map_type get_edge_bundle(const IndexMap& index_map = IndexMap())
        {
            return edge_map_type(m_edge_properties.begin(), index_map);
        }

        const_edge_map_type get_edge_bundle(
            const IndexMap& index_map = IndexMap()) const
        {
            return const_edge_map_type(m_edge_properties.begin(), index_map);
        }

    protected:
        // Default-construct with no property values
        indexed_edge_properties() {}

        // Initialize with n default-constructed property values
        indexed_edge_properties(std::size_t n) : m_edge_properties(n) {}

        // Get the size of the properties vector
        std::size_t size() const { return m_edge_properties.size(); }

        // Clear the properties vector
        void clear() { m_edge_properties.clear(); }

        // Resize the properties vector
        void resize(std::size_t n) { m_edge_properties.resize(n); }

        // Reserve space in the vector of properties
        void reserve(std::size_t n) { m_edge_properties.reserve(n); }

        // Write an element by raw index
        void write_by_index(std::size_t idx, const Property& prop)
        {
            m_edge_properties[idx] = prop;
        }

    public:
        // Add a new property value to the back
        void push_back(const Property& prop)
        {
            m_edge_properties.push_back(prop);
        }

        // Move range of properties backwards
        void move_range(
            std::size_t src_begin, std::size_t src_end, std::size_t dest_begin)
        {
            std::copy_backward(m_edge_properties.begin() + src_begin,
                m_edge_properties.begin() + src_end,
                m_edge_properties.begin() + dest_begin + (src_end - src_begin));
        }

        typedef typename std::vector< Property >::iterator iterator;
        iterator begin() { return m_edge_properties.begin(); }
        iterator end() { return m_edge_properties.end(); }

    private:
        // Access to the derived object
        Derived& derived() { return *static_cast< Derived* >(this); }

        const Derived& derived() const
        {
            return *static_cast< const Derived* >(this);
        }

    public: // should be private, but friend templates not portable
        std::vector< Property > m_edge_properties;
    };

    struct dummy_no_property_iterator
    : public boost::iterator_facade< dummy_no_property_iterator, no_property,
          std::random_access_iterator_tag >
    {
        mutable no_property prop;
        no_property& dereference() const { return prop; }
        bool equal(const dummy_no_property_iterator&) const { return true; }
        void increment() {}
        void decrement() {}
        void advance(std::ptrdiff_t) {}
        std::ptrdiff_t distance_to(const dummy_no_property_iterator) const
        {
            return 0;
        }
    };

    template < typename Derived, typename Descriptor, typename IndexMap >
    class indexed_edge_properties< Derived, void, Descriptor, IndexMap >
    {
        struct secret
        {
        };

    public:
        typedef no_property edge_property_type;
        typedef void edge_bundled;
        typedef void* edge_push_back_type;
        typedef secret edge_map_type;
        typedef secret const_edge_map_type;

        secret operator[](secret) { return secret(); }
        void write_by_index(std::size_t /*idx*/, const no_property& /*prop*/) {}

        edge_map_type get_edge_bundle(const IndexMap& = IndexMap()) const
        {
            return edge_map_type();
        }

    protected:
        // All operations do nothing.
        indexed_edge_properties() {}
        indexed_edge_properties(std::size_t) {}
        std::size_t size() const { return 0; }
        void clear() {}
        void resize(std::size_t) {}
        void reserve(std::size_t) {}

    public:
        void push_back(const edge_push_back_type&) {}
        void move_range(std::size_t /*src_begin*/, std::size_t /*src_end*/,
            std::size_t /*dest_begin*/)
        {
        }

        typedef dummy_no_property_iterator iterator;
        iterator begin() { return dummy_no_property_iterator(); }
        iterator end() { return dummy_no_property_iterator(); }
    };

}
}

#endif // BOOST_GRAPH_INDEXED_PROPERTIES_HPP
