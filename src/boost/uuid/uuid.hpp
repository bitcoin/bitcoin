// Boost uuid.hpp header file  ----------------------------------------------//

// Copyright 2006 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  06 Feb 2006 - Initial Revision
//  09 Nov 2006 - fixed variant and version bits for v4 guids
//  13 Nov 2006 - added serialization
//  17 Nov 2006 - added name-based guid creation
//  20 Nov 2006 - add fixes for gcc (from Tim Blechmann)
//  07 Mar 2007 - converted to header only
//  10 May 2007 - removed need for Boost.Thread
//              - added better seed - thanks Peter Dimov
//              - removed null()
//              - replaced byte_count() and output_bytes() with size() and begin() and end()
//  11 May 2007 - fixed guid(ByteInputIterator first, ByteInputIterator last)
//              - optimized operator>>
//  14 May 2007 - converted from guid to uuid
//  29 May 2007 - uses new implementation of sha1
//  01 Jun 2007 - removed using namespace directives
//  09 Nov 2007 - moved implementation to uuid.ipp file
//  12 Nov 2007 - moved serialize code to uuid_serialize.hpp file
//  25 Feb 2008 - moved to namespace boost::uuids
//  19 Mar 2009 - changed to a POD, reorganized files
//  28 Nov 2009 - disabled deprecated warnings for MSVC
//  30 Nov 2009 - used BOOST_STATIC_CONSTANT
//  02 Dec 2009 - removed BOOST_STATIC_CONSTANT - not all compilers like it
//  29 Apr 2013 - added support for noexcept and constexpr, added optimizations for SSE/AVX

#ifndef BOOST_UUID_HPP
#define BOOST_UUID_HPP

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/uuid/detail/config.hpp>
#ifndef BOOST_UUID_NO_TYPE_TRAITS
#include <boost/type_traits/is_pod.hpp>
#include <boost/type_traits/integral_constant.hpp>
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(_MSC_VER)
#pragma warning(push) // Save warning settings.
#pragma warning(disable : 4996) // Disable deprecated std::swap_ranges, std::equal
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
    using ::size_t;
    using ::ptrdiff_t;
} //namespace std
#endif //BOOST_NO_STDC_NAMESPACE

namespace boost {
namespace uuids {

struct uuid
{
public:
    typedef uint8_t value_type;
    typedef uint8_t& reference;
    typedef uint8_t const& const_reference;
    typedef uint8_t* iterator;
    typedef uint8_t const* const_iterator;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // This does not work on some compilers
    // They seem to want the variable definec in
    // a cpp file
    //BOOST_STATIC_CONSTANT(size_type, static_size = 16);
    static BOOST_CONSTEXPR size_type static_size() BOOST_NOEXCEPT { return 16; }

public:
    iterator begin() BOOST_NOEXCEPT { return data; }
    const_iterator begin() const BOOST_NOEXCEPT { return data; }
    iterator end() BOOST_NOEXCEPT { return data+size(); }
    const_iterator end() const BOOST_NOEXCEPT { return data+size(); }

    BOOST_CONSTEXPR size_type size() const BOOST_NOEXCEPT { return static_size(); }

    bool is_nil() const BOOST_NOEXCEPT;

    enum variant_type
    {
        variant_ncs, // NCS backward compatibility
        variant_rfc_4122, // defined in RFC 4122 document
        variant_microsoft, // Microsoft Corporation backward compatibility
        variant_future // future definition
    };
    variant_type variant() const BOOST_NOEXCEPT
    {
        // variant is stored in octet 7
        // which is index 8, since indexes count backwards
        unsigned char octet7 = data[8]; // octet 7 is array index 8
        if ( (octet7 & 0x80) == 0x00 ) { // 0b0xxxxxxx
            return variant_ncs;
        } else if ( (octet7 & 0xC0) == 0x80 ) { // 0b10xxxxxx
            return variant_rfc_4122;
        } else if ( (octet7 & 0xE0) == 0xC0 ) { // 0b110xxxxx
            return variant_microsoft;
        } else {
            //assert( (octet7 & 0xE0) == 0xE0 ) // 0b111xxxx
            return variant_future;
        }
    }

    enum version_type
    {
        version_unknown = -1,
        version_time_based = 1,
        version_dce_security = 2,
        version_name_based_md5 = 3,
        version_random_number_based = 4,
        version_name_based_sha1 = 5
    };
    version_type version() const BOOST_NOEXCEPT
    {
        // version is stored in octet 9
        // which is index 6, since indexes count backwards
        uint8_t octet9 = data[6];
        if ( (octet9 & 0xF0) == 0x10 ) {
            return version_time_based;
        } else if ( (octet9 & 0xF0) == 0x20 ) {
            return version_dce_security;
        } else if ( (octet9 & 0xF0) == 0x30 ) {
            return version_name_based_md5;
        } else if ( (octet9 & 0xF0) == 0x40 ) {
            return version_random_number_based;
        } else if ( (octet9 & 0xF0) == 0x50 ) {
            return version_name_based_sha1;
        } else {
            return version_unknown;
        }
    }

    // note: linear complexity
    void swap(uuid& rhs) BOOST_NOEXCEPT;

public:
    // or should it be array<uint8_t, 16>
    uint8_t data[16];
};

bool operator== (uuid const& lhs, uuid const& rhs) BOOST_NOEXCEPT;
bool operator< (uuid const& lhs, uuid const& rhs) BOOST_NOEXCEPT;

inline bool operator!=(uuid const& lhs, uuid const& rhs) BOOST_NOEXCEPT
{
    return !(lhs == rhs);
}

inline bool operator>(uuid const& lhs, uuid const& rhs) BOOST_NOEXCEPT
{
    return rhs < lhs;
}
inline bool operator<=(uuid const& lhs, uuid const& rhs) BOOST_NOEXCEPT
{
    return !(rhs < lhs);
}

inline bool operator>=(uuid const& lhs, uuid const& rhs) BOOST_NOEXCEPT
{
    return !(lhs < rhs);
}

inline void swap(uuid& lhs, uuid& rhs) BOOST_NOEXCEPT
{
    lhs.swap(rhs);
}

// This is equivalent to boost::hash_range(u.begin(), u.end());
inline std::size_t hash_value(uuid const& u) BOOST_NOEXCEPT
{
    std::size_t seed = 0;
    for(uuid::const_iterator i=u.begin(), e=u.end(); i != e; ++i)
    {
        seed ^= static_cast<std::size_t>(*i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
}

}} //namespace boost::uuids

#ifndef BOOST_UUID_NO_TYPE_TRAITS
// type traits specializations
namespace boost {

template <>
struct is_pod<uuids::uuid> : true_type {};

} // namespace boost
#endif

#if defined(BOOST_UUID_USE_SSE2)
#include <boost/uuid/detail/uuid_x86.ipp>
#else
#include <boost/uuid/detail/uuid_generic.ipp>
#endif

#if defined(_MSC_VER)
#pragma warning(pop) // Restore warnings to previous state.
#endif

#endif // BOOST_UUID_HPP
