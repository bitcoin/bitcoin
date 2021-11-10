// Boost.TypeErasure library
//
// Copyright 2015 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DYNAMIC_BINDING_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DYNAMIC_BINDING_HPP_INCLUDED

#include <boost/type_erasure/detail/dynamic_vtable.hpp>
#include <boost/type_erasure/static_binding.hpp>

namespace boost {
namespace type_erasure {

/**
 * Maps a set of placeholders to actual types.
 */
template<class PlaceholderList>
class dynamic_binding
{
public:
    template<class Map>
    dynamic_binding(const static_binding<Map>&)
    {
        impl.template init<Map>();
    }
    template<class Concept, class Map>
    dynamic_binding(const binding<Concept>& other, const static_binding<Map>&)
    {
        impl.template convert_from<Map>(*other.impl.table);
    }
private:
    template<class Concept>
    friend class binding;
    typename ::boost::type_erasure::detail::make_dynamic_vtable<PlaceholderList>::type impl;
};

}
}

#endif
