#ifndef BOOST_SERIALIZATION_HASH_COLLECTIONS_LOAD_IMP_HPP
#define BOOST_SERIALIZATION_HASH_COLLECTIONS_LOAD_IMP_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
# pragma warning (disable : 4786) // too long name, harmless warning
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// hash_collections_load_imp.hpp: serialization for loading stl collections

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// helper function templates for serialization of hashed collections
#include <boost/config.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/item_version_type.hpp>
#include <boost/serialization/library_version_type.hpp>

namespace boost{
namespace serialization {
namespace stl {

//////////////////////////////////////////////////////////////////////
// implementation of serialization for STL containers
//
template<class Archive, class Container, class InputFunction>
inline void load_hash_collection(Archive & ar, Container &s)
{
    collection_size_type count;
    collection_size_type bucket_count;
    boost::serialization::item_version_type item_version(0);
    boost::serialization::library_version_type library_version(
        ar.get_library_version()
    );
    // retrieve number of elements
    if(boost::serialization::library_version_type(6) != library_version){
        ar >> BOOST_SERIALIZATION_NVP(count);
        ar >> BOOST_SERIALIZATION_NVP(bucket_count);
    }
    else{
        // note: fixup for error in version 6.  collection size was
        // changed to size_t BUT for hashed collections it was implemented
        // as an unsigned int.  This should be a problem only on win64 machines
        // but I'll leave it for everyone just in case.
        unsigned int c;
        unsigned int bc;
        ar >> BOOST_SERIALIZATION_NVP(c);
        count = c;
        ar >> BOOST_SERIALIZATION_NVP(bc);
        bucket_count = bc;
    }
    if(boost::serialization::library_version_type(3) < library_version){
        ar >> BOOST_SERIALIZATION_NVP(item_version);
    }
    s.clear();
    #if ! defined(__MWERKS__)
    s.resize(bucket_count);
    #endif
    InputFunction ifunc;
    while(count-- > 0){
        ifunc(ar, s, item_version);
    }
}

} // namespace stl
} // namespace serialization
} // namespace boost

#endif //BOOST_SERIALIZATION_HASH_COLLECTIONS_LOAD_IMP_HPP
