//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_STREAM_PARSER_IPP
#define BOOST_JSON_IMPL_STREAM_PARSER_IPP

#include <boost/json/stream_parser.hpp>
#include <boost/json/basic_parser_impl.hpp>
#include <boost/json/error.hpp>
#include <cstring>
#include <stdexcept>
#include <utility>

BOOST_JSON_NS_BEGIN

stream_parser::
stream_parser(
    storage_ptr sp,
    parse_options const& opt,
    unsigned char* buffer,
    std::size_t size) noexcept
    : p_(
        opt,
        std::move(sp),
        buffer,
        size)
{
    reset();
}

stream_parser::
stream_parser(
    storage_ptr sp,
    parse_options const& opt) noexcept
    : p_(
        opt,
        std::move(sp),
        nullptr,
        0)
{
    reset();
}

void
stream_parser::
reset(storage_ptr sp) noexcept
{
    p_.reset();
    p_.handler().st.reset(sp);
}

std::size_t
stream_parser::
write_some(
    char const* data,
    std::size_t size,
    error_code& ec)
{
    return p_.write_some(
        true, data, size, ec);
}

std::size_t
stream_parser::
write_some(
    char const* data,
    std::size_t size)
{
    error_code ec;
    auto const n = write_some(
        data, size, ec);
    if(ec)
        detail::throw_system_error(ec,
            BOOST_JSON_SOURCE_POS);
    return n;
}

std::size_t
stream_parser::
write(
    char const* data,
    std::size_t size,
    error_code& ec)
{
    auto const n = write_some(
        data, size, ec);
    if(! ec && n < size)
    {
        ec = error::extra_data;
        p_.fail(ec);
    }
    return n;
}

std::size_t
stream_parser::
write(
    char const* data,
    std::size_t size)
{
    error_code ec;
    auto const n = write(
        data, size, ec);
    if(ec)
        detail::throw_system_error(ec,
            BOOST_JSON_SOURCE_POS);
    return n;
}

void
stream_parser::
finish(error_code& ec)
{
    p_.write_some(false, nullptr, 0, ec);
}

void
stream_parser::
finish()
{
    error_code ec;
    finish(ec);
    if(ec)
        detail::throw_system_error(ec,
            BOOST_JSON_SOURCE_POS);
}

value
stream_parser::
release()
{
    if(! p_.done())
    {
        // prevent undefined behavior
        finish();
    }
    return p_.handler().st.release();
}

BOOST_JSON_NS_END

#endif
