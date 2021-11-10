// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  29 May 2007 - Initial Revision
//  25 Feb 2008 - moved to namespace boost::uuids::detail
//  10 Jan 2012 - can now handle the full size of messages (2^64 - 1 bits)

// This is a byte oriented implementation

#ifndef BOOST_UUID_SHA1_H
#define BOOST_UUID_SHA1_H

#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/uuid/uuid.hpp> // for version
#include <cstddef>
#include <stdexcept>
#include <string>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
    using ::size_t;
} // namespace std
#endif

namespace boost {
namespace uuids {
namespace detail {

BOOST_STATIC_ASSERT(sizeof(unsigned char)*8 == 8);
BOOST_STATIC_ASSERT(sizeof(unsigned int)*8 == 32);

inline unsigned int left_rotate(unsigned int x, std::size_t n)
{
    return (x<<n) ^ (x>> (32-n));
}

class sha1
{
public:
    typedef unsigned int(digest_type)[5];
public:
    sha1();

    void reset();

    void process_byte(unsigned char byte);
    void process_block(void const* bytes_begin, void const* bytes_end);
    void process_bytes(void const* buffer, std::size_t byte_count);

    void get_digest(digest_type& digest);
    unsigned char get_version() const;

private:
    void process_block();
    void process_byte_impl(unsigned char byte);

private:
    unsigned int h_[5];

    unsigned char block_[64];

    std::size_t block_byte_index_;
    std::size_t bit_count_low;
    std::size_t bit_count_high;
};

inline sha1::sha1()
{
    reset();
}

inline void sha1::reset()
{
    h_[0] = 0x67452301;
    h_[1] = 0xEFCDAB89;
    h_[2] = 0x98BADCFE;
    h_[3] = 0x10325476;
    h_[4] = 0xC3D2E1F0;

    block_byte_index_ = 0;
    bit_count_low = 0;
    bit_count_high = 0;
}

inline void sha1::process_byte(unsigned char byte)
{
    process_byte_impl(byte);

    // size_t max value = 0xFFFFFFFF
    //if (bit_count_low + 8 >= 0x100000000) { // would overflow
    //if (bit_count_low >= 0x100000000-8) {
    if (bit_count_low < 0xFFFFFFF8) {
        bit_count_low += 8;
    } else {
        bit_count_low = 0;

        if (bit_count_high <= 0xFFFFFFFE) {
            ++bit_count_high;
        } else {
            BOOST_THROW_EXCEPTION(std::runtime_error("sha1 too many bytes"));
        }
    }
}

inline void sha1::process_byte_impl(unsigned char byte)
{
    block_[block_byte_index_++] = byte;

    if (block_byte_index_ == 64) {
        block_byte_index_ = 0;
        process_block();
    }
}

inline void sha1::process_block(void const* bytes_begin, void const* bytes_end)
{
    unsigned char const* begin = static_cast<unsigned char const*>(bytes_begin);
    unsigned char const* end = static_cast<unsigned char const*>(bytes_end);
    for(; begin != end; ++begin) {
        process_byte(*begin);
    }
}

inline void sha1::process_bytes(void const* buffer, std::size_t byte_count)
{
    unsigned char const* b = static_cast<unsigned char const*>(buffer);
    process_block(b, b+byte_count);
}

inline void sha1::process_block()
{
    unsigned int w[80];
    for (std::size_t i=0; i<16; ++i) {
        w[i]  = (block_[i*4 + 0] << 24);
        w[i] |= (block_[i*4 + 1] << 16);
        w[i] |= (block_[i*4 + 2] << 8);
        w[i] |= (block_[i*4 + 3]);
    }
    for (std::size_t i=16; i<80; ++i) {
        w[i] = left_rotate((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]), 1);
    }

    unsigned int a = h_[0];
    unsigned int b = h_[1];
    unsigned int c = h_[2];
    unsigned int d = h_[3];
    unsigned int e = h_[4];

    for (std::size_t i=0; i<80; ++i) {
        unsigned int f;
        unsigned int k;

        if (i<20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999;
        } else if (i<40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i<60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        unsigned temp = left_rotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = left_rotate(b, 30);
        b = a;
        a = temp;
    }

    h_[0] += a;
    h_[1] += b;
    h_[2] += c;
    h_[3] += d;
    h_[4] += e;
}

inline unsigned char sha1::get_version() const
{
    // RFC 4122 Section 4.1.3
    return uuid::version_name_based_sha1;
}

inline void sha1::get_digest(digest_type& digest)
{
    // append the bit '1' to the message
    process_byte_impl(0x80);

    // append k bits '0', where k is the minimum number >= 0
    // such that the resulting message length is congruent to 56 (mod 64)
    // check if there is enough space for padding and bit_count
    if (block_byte_index_ > 56) {
        // finish this block
        while (block_byte_index_ != 0) {
            process_byte_impl(0);
        }

        // one more block
        while (block_byte_index_ < 56) {
            process_byte_impl(0);
        }
    } else {
        while (block_byte_index_ < 56) {
            process_byte_impl(0);
        }
    }

    // append length of message (before pre-processing)
    // as a 64-bit big-endian integer
    process_byte_impl( static_cast<unsigned char>((bit_count_high>>24) & 0xFF) );
    process_byte_impl( static_cast<unsigned char>((bit_count_high>>16) & 0xFF) );
    process_byte_impl( static_cast<unsigned char>((bit_count_high>>8 ) & 0xFF) );
    process_byte_impl( static_cast<unsigned char>((bit_count_high)     & 0xFF) );
    process_byte_impl( static_cast<unsigned char>((bit_count_low>>24) & 0xFF) );
    process_byte_impl( static_cast<unsigned char>((bit_count_low>>16) & 0xFF) );
    process_byte_impl( static_cast<unsigned char>((bit_count_low>>8 ) & 0xFF) );
    process_byte_impl( static_cast<unsigned char>((bit_count_low)     & 0xFF) );

    // get final digest
    digest[0] = h_[0];
    digest[1] = h_[1];
    digest[2] = h_[2];
    digest[3] = h_[3];
    digest[4] = h_[4];
}

}}} // namespace boost::uuids::detail

#endif
