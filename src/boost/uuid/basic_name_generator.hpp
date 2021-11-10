// Boost basic_name_generator.hpp header file  -----------------------//

// Copyright 2010 Andy Tompkins.
// Copyright 2017 James E. King III

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UUID_BASIC_NAME_GENERATOR_HPP
#define BOOST_UUID_BASIC_NAME_GENERATOR_HPP

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <boost/uuid/uuid.hpp>
#include <cstring> // for strlen, wcslen
#include <string>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
    using ::size_t;
    using ::strlen;
    using ::wcslen;
} //namespace std
#endif //BOOST_NO_STDC_NAMESPACE

namespace boost {
namespace uuids {

//! \brief Generate a name based UUID using
//!        the provided hashing algorithm that
//!        implements the NameHashProvider concept.
template<class HashAlgo>
class basic_name_generator
{
  public:
    typedef uuid result_type;
    typedef typename HashAlgo::digest_type digest_type;

    explicit basic_name_generator(uuid const& namespace_uuid_)
        : namespace_uuid(namespace_uuid_)
    {}

    uuid operator()(const char* name) const {
        HashAlgo hash;
        hash.process_bytes(namespace_uuid.begin(), namespace_uuid.size());
        process_characters(hash, name, std::strlen(name));
        return hash_to_uuid(hash);
    }

    uuid operator()(const wchar_t* name) const {
        HashAlgo hash;
        hash.process_bytes(namespace_uuid.begin(), namespace_uuid.size());
        process_characters(hash, name, std::wcslen(name));
        return hash_to_uuid(hash);
    }

    template <typename ch, typename char_traits, typename alloc>
    uuid operator()(std::basic_string<ch, char_traits, alloc> const& name) const {
        HashAlgo hash;
        hash.process_bytes(namespace_uuid.begin(), namespace_uuid.size());
        process_characters(hash, name.c_str(), name.length());
        return hash_to_uuid(hash);
    }

    uuid operator()(void const* buffer, std::size_t byte_count) const {
        HashAlgo hash;
        hash.process_bytes(namespace_uuid.begin(), namespace_uuid.size());
        hash.process_bytes(buffer, byte_count);
        return hash_to_uuid(hash);
    }

private:
    // we convert all characters to uint32_t so that each
    // character is 4 bytes regardless of sizeof(char) or
    // sizeof(wchar_t).  We want the name string on any
    // platform / compiler to generate the same uuid
    // except for char
    template <typename char_type>
    void process_characters(HashAlgo& hash, char_type const*const characters, std::size_t count) const {
        BOOST_STATIC_ASSERT(sizeof(uint32_t) >= sizeof(char_type));

        for (std::size_t i=0; i<count; i++) {
            std::size_t c = characters[i];
            hash.process_byte(static_cast<unsigned char>((c >>  0) & 0xFF));
            hash.process_byte(static_cast<unsigned char>((c >>  8) & 0xFF));
            hash.process_byte(static_cast<unsigned char>((c >> 16) & 0xFF));
            hash.process_byte(static_cast<unsigned char>((c >> 24) & 0xFF));
        }
    }

    void process_characters(HashAlgo& hash, char const*const characters, std::size_t count) const {
        hash.process_bytes(characters, count);
    }

    uuid hash_to_uuid(HashAlgo& hash) const
    {
        digest_type digest;
        hash.get_digest(digest);

        BOOST_STATIC_ASSERT(sizeof(digest_type) >= 16);

        uuid u;
        for (int i=0; i<4; ++i) {
            *(u.begin() + i*4+0) = static_cast<uint8_t>((digest[i] >> 24) & 0xFF);
            *(u.begin() + i*4+1) = static_cast<uint8_t>((digest[i] >> 16) & 0xFF);
            *(u.begin() + i*4+2) = static_cast<uint8_t>((digest[i] >> 8) & 0xFF);
            *(u.begin() + i*4+3) = static_cast<uint8_t>((digest[i] >> 0) & 0xFF);
        }

        // set variant: must be 0b10xxxxxx
        *(u.begin()+8) &= 0xBF;
        *(u.begin()+8) |= 0x80;

        // set version
        unsigned char hashver = hash.get_version();
        *(u.begin()+6) &= 0x0F;             // clear out the relevant bits
        *(u.begin()+6) |= (hashver << 4);   // and apply them

        return u;
    }

private:
    uuid namespace_uuid;
};

namespace ns {

BOOST_FORCEINLINE uuid dns() {
    uuid result = {{
        0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1 ,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 }};
    return result;
}

BOOST_FORCEINLINE uuid url() {
    uuid result = {{
        0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1 ,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 }};
    return result;
}

BOOST_FORCEINLINE uuid oid() {
    uuid result = {{
        0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1 ,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 }};
    return result;
}

BOOST_FORCEINLINE uuid x500dn() {
    uuid result = {{
        0x6b, 0xa7, 0xb8, 0x14, 0x9d, 0xad, 0x11, 0xd1 ,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 }};
    return result;
}

} // ns
} // uuids
} // boost

#endif // BOOST_UUID_BASIC_NAME_GENERATOR_HPP
