//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_IMPL_ERROR_IPP
#define BOOST_BEAST_HTTP_IMPL_ERROR_IPP

#include <boost/beast/http/error.hpp>
#include <type_traits>

namespace boost {
namespace beast {
namespace http {
namespace detail {

class http_error_category : public error_category
{
public:
    const char*
    name() const noexcept override
    {
        return "beast.http";
    }

    std::string
    message(int ev) const override
    {
        switch(static_cast<error>(ev))
        {
        case error::end_of_stream: return "end of stream";
        case error::partial_message: return "partial message";
        case error::need_more: return "need more";
        case error::unexpected_body: return "unexpected body";
        case error::need_buffer: return "need buffer";
        case error::end_of_chunk: return "end of chunk";
        case error::buffer_overflow: return "buffer overflow";
        case error::header_limit: return "header limit exceeded";
        case error::body_limit: return "body limit exceeded";
        case error::bad_alloc: return "bad alloc";
        case error::bad_line_ending: return "bad line ending";
        case error::bad_method: return "bad method";
        case error::bad_target: return "bad target";
        case error::bad_version: return "bad version";
        case error::bad_status: return "bad status";
        case error::bad_reason: return "bad reason";
        case error::bad_field: return "bad field";
        case error::bad_value: return "bad value";
        case error::bad_content_length: return "bad Content-Length";
        case error::bad_transfer_encoding: return "bad Transfer-Encoding";
        case error::bad_chunk: return "bad chunk";
        case error::bad_chunk_extension: return "bad chunk extension";
        case error::bad_obs_fold: return "bad obs-fold";
        case error::stale_parser: return "stale parser";
        case error::short_read: return "unexpected eof in body";

        default:
            return "beast.http error";
        }
    }

    error_condition
    default_error_condition(
        int ev) const noexcept override
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
    equivalent(error_code const& error,
        int ev) const noexcept override
    {
        return error.value() == ev &&
            &error.category() == this;
    }
};

} // detail

error_code
make_error_code(error ev)
{
    static detail::http_error_category const cat{};
    return error_code{static_cast<
        std::underlying_type<error>::type>(ev), cat};
}

} // http
} // beast
} // boost

#endif
