#ifndef  BOOST_SERIALIZATION_DETAIL_IS_DEFAULT_CONSTRUCTIBLE_HPP
#define BOOST_SERIALIZATION_DETAIL_IS_DEFAULT_CONSTRUCTIBLE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// is_default_constructible.hpp: serialization for loading stl collections
//
// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>

#if ! defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
    #include <type_traits>
    namespace boost{
    namespace serialization {
    namespace detail {

    template<typename T>
    struct is_default_constructible : public std::is_default_constructible<T> {};

    } // detail
    } // serializaition
    } // boost
#else
    // we don't have standard library support for is_default_constructible
    // so we fake it by using boost::has_trivial_construtor.  But this is not
    // actually correct because it's possible that a default constructor
    // to be non trivial. So when using this, make sure you're not using your
    // own definition of of T() but are using the actual default one!
    #include <boost/type_traits/has_trivial_constructor.hpp>
    namespace boost{
    namespace serialization {
    namespace detail {

    template<typename T>
    struct is_default_constructible : public boost::has_trivial_constructor<T> {};

    } // detail
    } // serializaition
    } // boost

#endif


#endif //  BOOST_SERIALIZATION_DETAIL_IS_DEFAULT_CONSTRUCTIBLE_HPP
