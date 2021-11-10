#ifndef BOOST_ARCHIVE_POLYMORPHIC_BINARY_IARCHIVE_HPP
#define BOOST_ARCHIVE_POLYMORPHIC_BINARY_IARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// polymorphic_binary_iarchive.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/detail/polymorphic_iarchive_route.hpp>

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

namespace boost {
namespace archive {

class BOOST_SYMBOL_VISIBLE polymorphic_binary_iarchive :
    public detail::polymorphic_iarchive_route<binary_iarchive>
{
public:
    polymorphic_binary_iarchive(std::istream & is, unsigned int flags = 0) :
        detail::polymorphic_iarchive_route<binary_iarchive>(is, flags)
    {}
    ~polymorphic_binary_iarchive() BOOST_OVERRIDE {}
};

} // namespace archive
} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

// required by export
BOOST_SERIALIZATION_REGISTER_ARCHIVE(
    boost::archive::polymorphic_binary_iarchive
)

#endif // BOOST_ARCHIVE_POLYMORPHIC_BINARY_IARCHIVE_HPP
