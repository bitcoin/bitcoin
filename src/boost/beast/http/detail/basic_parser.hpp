//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_DETAIL_BASIC_PARSER_HPP
#define BOOST_BEAST_HTTP_DETAIL_BASIC_PARSER_HPP

#include <boost/beast/core/string.hpp>
#include <boost/beast/core/detail/char_buffer.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/detail/rfc7230.hpp>
#include <boost/config.hpp>
#include <boost/version.hpp>
#include <cstddef>
#include <utility>

namespace boost {
namespace beast {
namespace http {
namespace detail {

struct basic_parser_base
{
    // limit on the size of the obs-fold buffer
    //
    // https://stackoverflow.com/questions/686217/maximum-on-http-header-values
    //
    static std::size_t constexpr max_obs_fold = 4096;

    enum class state
    {
        nothing_yet = 0,
        start_line,
        fields,
        body0,
        body,
        body_to_eof0,
        body_to_eof,
        chunk_header0,
        chunk_header,
        chunk_body,
        complete
    };

    static
    bool
    is_digit(char c)
    {
        return static_cast<unsigned char>(c-'0') < 10;
    }

    static
    bool
    is_print(char c)
    {
        return static_cast<unsigned char>(c-32) < 95;
    }

    BOOST_BEAST_DECL
    static
    char const*
    trim_front(char const* it, char const* end);

    BOOST_BEAST_DECL
    static
    char const*
    trim_back(
        char const* it, char const* first);

    static
    string_view
    make_string(char const* first, char const* last)
    {
        return {first, static_cast<
            std::size_t>(last - first)};
    }

    //--------------------------------------------------------------------------

    BOOST_BEAST_DECL
    static
    bool
    is_pathchar(char c);

    BOOST_BEAST_DECL
    static
    bool
    unhex(unsigned char& d, char c);

    BOOST_BEAST_DECL
    static
    std::pair<char const*, bool>
    find_fast(
        char const* buf,
        char const* buf_end,
        char const* ranges,
        size_t ranges_size);

    BOOST_BEAST_DECL
    static
    char const*
    find_eol(
        char const* it, char const* last,
            error_code& ec);

    BOOST_BEAST_DECL
    static
    char const*
    find_eom(char const* p, char const* last);

    //--------------------------------------------------------------------------

    BOOST_BEAST_DECL
    static
    char const*
    parse_token_to_eol(
        char const* p,
        char const* last,
        char const*& token_last,
        error_code& ec);

    BOOST_BEAST_DECL
    static
    bool
    parse_dec(string_view s, std::uint64_t& v);

    BOOST_BEAST_DECL
    static
    bool
    parse_hex(char const*& it, std::uint64_t& v);

    BOOST_BEAST_DECL
    static
    bool
    parse_crlf(char const*& it);

    BOOST_BEAST_DECL
    static
    void
    parse_method(
        char const*& it, char const* last,
        string_view& result, error_code& ec);

    BOOST_BEAST_DECL
    static
    void
    parse_target(
        char const*& it, char const* last,
        string_view& result, error_code& ec);

    BOOST_BEAST_DECL
    static
    void
    parse_version(
        char const*& it, char const* last,
        int& result, error_code& ec);

    BOOST_BEAST_DECL
    static
    void
    parse_status(
        char const*& it, char const* last,
        unsigned short& result, error_code& ec);

    BOOST_BEAST_DECL
    static
    void
    parse_reason(
        char const*& it, char const* last,
        string_view& result, error_code& ec);

    BOOST_BEAST_DECL
    static
    void
    parse_field(
        char const*& p,
        char const* last,
        string_view& name,
        string_view& value,
        beast::detail::char_buffer<max_obs_fold>& buf,
        error_code& ec);

    BOOST_BEAST_DECL
    static
    void
    parse_chunk_extensions(
        char const*& it,
        char const* last,
        error_code& ec);
};

} // detail
} // http
} // beast
} // boost

#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/http/detail/basic_parser.ipp>
#endif

#endif
