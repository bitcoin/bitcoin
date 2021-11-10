// Copyright (C) Vladimir Prus 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/graph/vector_property_map.html for
// documentation.
//

#ifndef BOOST_PROPERTY_MAP_VECTOR_PROPERTY_MAP_HPP
#define BOOST_PROPERTY_MAP_VECTOR_PROPERTY_MAP_HPP

#include <boost/property_map/property_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <iterator>
#include <vector>

namespace boost {
    template<typename T, typename IndexMap = identity_property_map>
    class vector_property_map
        : public boost::put_get_helper< 
              typename std::iterator_traits< 
                  typename std::vector<T>::iterator >::reference,
              vector_property_map<T, IndexMap> >
    {
    public:
        typedef typename property_traits<IndexMap>::key_type  key_type;
        typedef T value_type;
        typedef typename std::iterator_traits< 
            typename std::vector<T>::iterator >::reference reference;
        typedef boost::lvalue_property_map_tag category;

        vector_property_map(const IndexMap& index = IndexMap())
        : store(new std::vector<T>()), index(index)
        {}

        vector_property_map(unsigned initial_size, 
                            const IndexMap& index = IndexMap())
        : store(new std::vector<T>(initial_size)), index(index)
        {}

        typename std::vector<T>::iterator storage_begin()
        {
            return store->begin();
        }

        typename std::vector<T>::iterator storage_end()
        {
            return store->end();
        }

        typename std::vector<T>::const_iterator storage_begin() const
        {
            return store->begin();
        }

        typename std::vector<T>::const_iterator storage_end() const
        {
            return store->end();
        }

        IndexMap&       get_index_map()       { return index; }
        const IndexMap& get_index_map() const { return index; }

    public:
        // Copy ctor absent, default semantics is OK.
        // Assignment operator absent, default semantics is OK.
        // CONSIDER: not sure that assignment to 'index' is correct.

        reference operator[](const key_type& v) const {
            typename property_traits<IndexMap>::value_type i = get(index, v);
            if (static_cast<unsigned>(i) >= store->size()) {
                store->resize(i + 1, T());
            }
            return (*store)[i];
        }
    private:
        // Conceptually, we have a vector of infinite size. For practical 
        // purposes, we start with an empty vector and grow it as needed.
        // Note that we cannot store pointer to vector here -- we cannot
        // store pointer to data, because if copy of property map resizes
        // the vector, the pointer to data will be invalidated. 
        // I wonder if class 'pmap_ref' is simply needed.
        shared_ptr< std::vector<T> > store;
        IndexMap index;
    };

    template<typename T, typename IndexMap>
    vector_property_map<T, IndexMap>
    make_vector_property_map(IndexMap index)
    {
        return vector_property_map<T, IndexMap>(index);
    }
}

#ifdef BOOST_GRAPH_USE_MPI
// Hide include from dependency trackers; the right fix
// is not to have it at all, but who knows what'll break
#define BOOST_VPMAP_HEADER_NAME <boost/property_map/parallel/vector_property_map.hpp>
#include BOOST_VPMAP_HEADER_NAME
#undef BOOST_VPMAP_HEADER_NAME
#endif

#endif
