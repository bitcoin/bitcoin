//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//
// This is a derivative work based on Zlib, copyright below:
/*
    Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

    Jean-loup Gailly        Mark Adler
    jloup@gzip.org          madler@alumni.caltech.edu

    The data format used by the zlib library is described by RFCs (Request for
    Comments) 1950 to 1952 in the files http://tools.ietf.org/html/rfc1950
    (zlib format), rfc1951 (deflate format) and rfc1952 (gzip format).
*/

#ifndef BOOST_BEAST_ZLIB_DETAIL_BITSTREAM_HPP
#define BOOST_BEAST_ZLIB_DETAIL_BITSTREAM_HPP

#include <boost/assert.hpp>
#include <cstdint>
#include <iterator>

namespace boost {
namespace beast {
namespace zlib {
namespace detail {

class bitstream
{
    using value_type = std::uint32_t;

    value_type v_ = 0;
    unsigned n_ = 0;

public:
    // returns the number of bits in the reservoir
    unsigned
    size() const
    {
        return n_;
    }

    // discard n bits
    void
    drop(std::size_t n)
    {
        BOOST_ASSERT(n <= n_);
        n_ -= static_cast<unsigned>(n);
        v_ >>= n;
    }

    // flush everything
    void
    flush()
    {
        n_ = 0;
        v_ = 0;
    }

    // flush to the next byte boundary
    void
    flush_byte()
    {
        drop(n_ % 8);
    }

    // ensure at least n bits
    template<class FwdIt>
    bool
    fill(std::size_t n, FwdIt& first, FwdIt const& last);

    // fill 8 bits, unchecked
    template<class FwdIt>
    void
    fill_8(FwdIt& it);

    // fill 16 bits, unchecked
    template<class FwdIt>
    void
    fill_16(FwdIt& it);

    // return n bits
    template<class Unsigned>
    void
    peek(Unsigned& value, std::size_t n);

    // return everything in the reservoir
    value_type
    peek_fast() const
    {
        return v_;
    }

    // return n bits, and consume
    template<class Unsigned>
    void
    read(Unsigned& value, std::size_t n);

    // rewind by the number of whole bytes stored (unchecked)
    template<class BidirIt>
    void
    rewind(BidirIt& it);
};

template<class FwdIt>
bool
bitstream::
fill(std::size_t n, FwdIt& first, FwdIt const& last)
{
    while(n_ < n)
    {
        if(first == last)
            return false;
        v_ += static_cast<value_type>(*first++) << n_;
        n_ += 8;
    }
    return true;
}

template<class FwdIt>
void
bitstream::
fill_8(FwdIt& it)
{
    v_ += static_cast<value_type>(*it++) << n_;
    n_ += 8;
}

template<class FwdIt>
void
bitstream::
fill_16(FwdIt& it)
{
    v_ += static_cast<value_type>(*it++) << n_;
    n_ += 8;
    v_ += static_cast<value_type>(*it++) << n_;
    n_ += 8;
}

template<class Unsigned>
void
bitstream::
peek(Unsigned& value, std::size_t n)
{
    BOOST_ASSERT(n <= sizeof(value)*8);
    BOOST_ASSERT(n <= n_);
    value = static_cast<Unsigned>(
        v_ & ((1ULL << n) - 1));
}

template<class Unsigned>
void
bitstream::
read(Unsigned& value, std::size_t n)
{
    BOOST_ASSERT(n < sizeof(v_)*8);
    BOOST_ASSERT(n <= n_);
    value = static_cast<Unsigned>(
        v_ & ((1ULL << n) - 1));
    v_ >>= n;
    n_ -= static_cast<unsigned>(n);
}

template<class BidirIt>
void
bitstream::
rewind(BidirIt& it)
{
    auto len = n_ >> 3;
    it = std::prev(it, len);
    n_ &= 7;
    v_ &= (1U << n_) - 1;
}

} // detail
} // zlib
} // beast
} // boost

#endif
