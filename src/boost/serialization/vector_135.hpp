////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// vector_135.hpp: serialization for stl vector templates for compatibility
//                 with release 1.35, which had a bug

// (C) Copyright 2008 Matthias Troyer
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.


#ifndef  BOOST_SERIALIZATION_VECTOR_135_HPP
#define BOOST_SERIALIZATION_VECTOR_135_HPP

#ifdef BOOST_SERIALIZATION_VECTOR_VERSIONED
#if BOOST_SERIALIZATION_VECTOR_VERSION != 4
#error "Boost.Serialization cannot be compatible with both 1.35 and 1.36-1.40 files"
#endif
#else
#define BOOST_SERIALIZATION_VECTOR_VERSIONED(V) (V>4)
#endif

#include <boost/serialization/vector.hpp>

#endif // BOOST_SERIALIZATION_VECTOR_135_HPP
