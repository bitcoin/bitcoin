//-----------------------------------------------------------------------------
// boost variant/detail/std_hash.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2018-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_VARIANT_DETAIL_STD_HASH_HPP
#define BOOST_VARIANT_DETAIL_STD_HASH_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

#include <boost/variant/variant_fwd.hpp>
#include <boost/variant/detail/hash_variant.hpp>

///////////////////////////////////////////////////////////////////////////////
// macro BOOST_VARIANT_DO_NOT_SPECIALIZE_STD_HASH
//
// Define this macro if you do not wish to have a std::hash specialization for
// boost::variant.

#if !defined(BOOST_VARIANT_DO_NOT_SPECIALIZE_STD_HASH) && !defined(BOOST_NO_CXX11_HDR_FUNCTIONAL)

#include <functional> // for std::hash

namespace std {
    template < BOOST_VARIANT_ENUM_PARAMS(typename T) >
    struct hash<boost::variant< BOOST_VARIANT_ENUM_PARAMS(T) > > {
        std::size_t operator()(const boost::variant< BOOST_VARIANT_ENUM_PARAMS(T) >& val) const {
            return ::boost::hash_value(val);
        }
    };
}

#endif // #if !defined(BOOST_VARIANT_DO_NOT_SPECIALIZE_STD_HASH) && !defined(BOOST_NO_CXX11_HDR_FUNCTIONAL)

#endif // BOOST_VARIANT_DETAIL_STD_HASH_HPP

