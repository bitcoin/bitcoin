#ifndef BOOST_ARCHIVE_SERIALIZER_MAP_HPP
#define BOOST_ARCHIVE_SERIALIZER_MAP_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// archive_serializer_map.hpp: extenstion of type_info required for
// serialization.

// (C) Copyright 2009 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// note: this is nothing more than the thinest of wrappers around
// basic_serializer_map so we can have a one map / archive type.

#include <boost/config.hpp>
#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

namespace boost {

namespace serialization {
    class extended_type_info;
} // namespace serialization

namespace archive {
namespace detail {

class basic_serializer;

template<class Archive>
class BOOST_SYMBOL_VISIBLE archive_serializer_map {
public:
    static BOOST_ARCHIVE_OR_WARCHIVE_DECL bool insert(const basic_serializer * bs);
    static BOOST_ARCHIVE_OR_WARCHIVE_DECL void erase(const basic_serializer * bs);
    static BOOST_ARCHIVE_OR_WARCHIVE_DECL const basic_serializer * find(
        const boost::serialization::extended_type_info & type_
    );
};

} // namespace detail
} // namespace archive
} // namespace boost

#include <boost/archive/detail/abi_suffix.hpp> // must be the last header

#endif //BOOST_ARCHIVE_SERIALIZER_MAP_HPP
