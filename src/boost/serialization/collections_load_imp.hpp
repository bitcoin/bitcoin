#ifndef  BOOST_SERIALIZATION_COLLECTIONS_LOAD_IMP_HPP
#define BOOST_SERIALIZATION_COLLECTIONS_LOAD_IMP_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#if defined(_MSC_VER) && (_MSC_VER <= 1020)
#  pragma warning (disable : 4786) // too long name, harmless warning
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// collections_load_imp.hpp: serialization for loading stl collections

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// helper function templates for serialization of collections

#include <boost/assert.hpp>
#include <cstddef> // size_t
#include <boost/config.hpp> // msvc 6.0 needs this for warning suppression
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
} // namespace std
#endif
#include <boost/detail/workaround.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/detail/stack_constructor.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/item_version_type.hpp>
#include <boost/serialization/detail/is_default_constructible.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/move/utility_core.hpp>

namespace boost{
namespace serialization {
namespace stl {

//////////////////////////////////////////////////////////////////////
// implementation of serialization for STL containers
//

template<
    class Archive,
    class T
>
typename boost::enable_if<
    typename detail::is_default_constructible<
        typename T::value_type
    >,
    void
>::type
collection_load_impl(
    Archive & ar,
    T & t,
    collection_size_type count,
    item_version_type /*item_version*/
){
    t.resize(count);
    typename T::iterator hint;
    hint = t.begin();
    while(count-- > 0){
        ar >> boost::serialization::make_nvp("item", *hint++);
    }
}

template<
    class Archive,
    class T
>
typename boost::disable_if<
    typename detail::is_default_constructible<
        typename T::value_type
    >,
    void
>::type
collection_load_impl(
    Archive & ar,
    T & t,
    collection_size_type count,
    item_version_type item_version
){
    t.clear();
    while(count-- > 0){
        detail::stack_construct<Archive, typename T::value_type> u(ar, item_version);
        ar >> boost::serialization::make_nvp("item", u.reference());
        t.push_back(boost::move(u.reference()));
        ar.reset_object_address(& t.back() , u.address());
     }
}

} // namespace stl
} // namespace serialization
} // namespace boost

#endif //BOOST_SERIALIZATION_COLLECTIONS_LOAD_IMP_HPP
