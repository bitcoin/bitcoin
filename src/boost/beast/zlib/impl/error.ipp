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

#ifndef BOOST_BEAST_ZLIB_IMPL_ERROR_IPP
#define BOOST_BEAST_ZLIB_IMPL_ERROR_IPP

#include <boost/beast/zlib/error.hpp>
#include <type_traits>

namespace boost {
namespace beast {
namespace zlib {
namespace detail {

class error_codes : public error_category
{
public:
    const char*
    name() const noexcept override
    {
        return "boost.beast.zlib";
    }

    std::string
    message(int ev) const override
    {
        switch(static_cast<error>(ev))
        {
        case error::need_buffers: return "need buffers";
        case error::end_of_stream: return "unexpected end of deflate stream";
        case error::need_dict: return "need dict";
        case error::stream_error: return "stream error";

        case error::invalid_block_type: return "invalid block type";
        case error::invalid_stored_length: return "invalid stored block length";
        case error::too_many_symbols: return "too many symbols";
        case error::invalid_code_lengths: return "invalid code lengths";
        case error::invalid_bit_length_repeat: return "invalid bit length repeat";
        case error::missing_eob: return "missing end of block code";
        case error::invalid_literal_length: return "invalid literal/length code";
        case error::invalid_distance_code: return "invalid distance code";
        case error::invalid_distance: return "invalid distance";

        case error::over_subscribed_length: return "over-subscribed length";
        case error::incomplete_length_set: return "incomplete length set";

        case error::general:
        default:
            return "beast.zlib error";
        }
    }

    error_condition
    default_error_condition(int ev) const noexcept override
    {
        return error_condition{ev, *this};
    }

    bool
    equivalent(int ev,
        error_condition const& condition
            ) const noexcept override
    {
        return condition.value() == ev &&
            &condition.category() == this;
    }

    bool
    equivalent(error_code const& error, int ev) const noexcept override
    {
        return error.value() == ev &&
            &error.category() == this;
    }
};

} // detail

error_code
make_error_code(error ev)
{
    static detail::error_codes const cat{};
    return error_code{static_cast<
        std::underlying_type<error>::type>(ev), cat};
}

} // zlib
} // beast
} // boost

#endif
