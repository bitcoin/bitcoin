#ifndef BOOST_ARCHIVE_BINARY_WOARCHIVE_HPP
#define BOOST_ARCHIVE_BINARY_WOARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// binary_woarchive.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>
#ifdef BOOST_NO_STD_WSTREAMBUF
#error "wide char i/o not supported on this platform"
#else

#include <ostream>
#include <boost/archive/binary_oarchive_impl.hpp>
#include <boost/archive/detail/register_archive.hpp>

namespace boost {
namespace archive {

// do not derive from this class.  If you want to extend this functionality
// via inhertance, derived from binary_oarchive_impl instead.  This will
// preserve correct static polymorphism.
class binary_woarchive :
    public binary_oarchive_impl<
            binary_woarchive, std::wostream::char_type, std::wostream::traits_type
        >
{
public:
    binary_woarchive(std::wostream & os, unsigned int flags = 0) :
        binary_oarchive_impl<
            binary_woarchive, std::wostream::char_type, std::wostream::traits_type
        >(os, flags)
    {}
    binary_woarchive(std::wstreambuf & bsb, unsigned int flags = 0) :
        binary_oarchive_impl<
            binary_woarchive, std::wostream::char_type, std::wostream::traits_type
        >(bsb, flags)
    {}
};

} // namespace archive
} // namespace boost

// required by export
BOOST_SERIALIZATION_REGISTER_ARCHIVE(boost::archive::binary_woarchive)

#endif // BOOST_NO_STD_WSTREAMBUF
#endif // BOOST_ARCHIVE_BINARY_WOARCHIVE_HPP
