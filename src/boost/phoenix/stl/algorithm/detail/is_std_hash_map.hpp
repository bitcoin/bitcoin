// Copyright 2005 Daniel Wallin.
// Copyright 2005 Joel de Guzman.
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Modeled after range_ex, Copyright 2004 Eric Niebler
///////////////////////////////////////////////////////////////////////////////
//
// is_std_hash_map.hpp
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_PHOENIX_IS_STD_HASH_MAP_EN_16_12_2004
#define BOOST_PHOENIX_IS_STD_HASH_MAP_EN_16_12_2004

#include <boost/phoenix/config.hpp>
#include <boost/mpl/bool.hpp>

#ifdef BOOST_PHOENIX_HAS_HASH
#include BOOST_PHOENIX_HASH_MAP_HEADER
#endif

namespace boost
{
    template<class T>
    struct is_std_hash_map
        : boost::mpl::false_
    {};

    template<class T>
    struct is_std_hash_multimap
        : boost::mpl::false_
    {};

#ifdef BOOST_PHOENIX_HAS_HASH

    template<class Kty,class Ty,BOOST_PHOENIX_HASH_template_rest_param>
    struct is_std_hash_map< ::BOOST_PHOENIX_HASH_NAMESPACE::hash_map<Kty,Ty,BOOST_PHOENIX_HASH_type_rest_param> >
        : boost::mpl::true_
    {};

    template<class Kty,class Ty,BOOST_PHOENIX_HASH_template_rest_param>
    struct is_std_hash_multimap< ::BOOST_PHOENIX_HASH_NAMESPACE::hash_multimap<Kty,Ty,BOOST_PHOENIX_HASH_type_rest_param> >
        : boost::mpl::true_
    {};

#endif

}

#endif
