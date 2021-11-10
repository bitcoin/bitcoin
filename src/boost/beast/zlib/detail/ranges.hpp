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

#ifndef BOOST_BEAST_ZLIB_DETAIL_RANGES_HPP
#define BOOST_BEAST_ZLIB_DETAIL_RANGES_HPP

#include <cstdint>
#include <type_traits>

namespace boost {
namespace beast {
namespace zlib {
namespace detail {

struct ranges
{
    template<bool isConst>
    struct range
    {
        using iter_t =
            typename std::conditional<isConst,
                std::uint8_t const*,
                std::uint8_t*>::type;

        iter_t first;
        iter_t last;
        iter_t next;

        // total bytes in range
        std::size_t
        size() const
        {
            return last - first;
        }

        // bytes consumed
        std::size_t
        used() const
        {
            return next - first;
        }

        // bytes remaining
        std::size_t
        avail() const
        {
            return last - next;
        }
    };

    range<true> in;
    range<false> out;
};

// Clamp u to v where u and v are different types
template<class U, class V>
U clamp(U u, V v)
{
    if(u > v)
        u = static_cast<U>(v);
    return u;
}

} // detail
} // zlib
} // beast
} // boost

#endif
