//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_IMPL_MESSAGE_HPP
#define BOOST_BEAST_HTTP_IMPL_MESSAGE_HPP

#include <boost/beast/core/error.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace boost {
namespace beast {
namespace http {

template<class Fields>
template<class Arg1, class... ArgN, class>
header<true, Fields>::
header(Arg1&& arg1, ArgN&&... argn)
    : Fields(std::forward<Arg1>(arg1),
        std::forward<ArgN>(argn)...)
{
}

template<class Fields>
verb
header<true, Fields>::
method() const
{
    return method_;
}

template<class Fields>
void
header<true, Fields>::
method(verb v)
{
    if(v == verb::unknown)
        BOOST_THROW_EXCEPTION(
            std::invalid_argument{"unknown method"});
    method_ = v;
    this->set_method_impl({});
}

template<class Fields>
string_view
header<true, Fields>::
method_string() const
{
    if(method_ != verb::unknown)
        return to_string(method_);
    return this->get_method_impl();
}

template<class Fields>
void
header<true, Fields>::
method_string(string_view s)
{
    method_ = string_to_verb(s);
    if(method_ != verb::unknown)
        this->set_method_impl({});
    else
        this->set_method_impl(s);
}

template<class Fields>
string_view
header<true, Fields>::
target() const
{
    return this->get_target_impl();
}

template<class Fields>
void
header<true, Fields>::
target(string_view s)
{
    this->set_target_impl(s);
}

template<class Fields>
void
swap(
    header<true, Fields>& h1,
    header<true, Fields>& h2)
{
    using std::swap;
    swap(
        static_cast<Fields&>(h1),
        static_cast<Fields&>(h2));
    swap(h1.version_, h2.version_);
    swap(h1.method_, h2.method_);
}

//------------------------------------------------------------------------------

template<class Fields>
template<class Arg1, class... ArgN, class>
header<false, Fields>::
header(Arg1&& arg1, ArgN&&... argn)
    : Fields(std::forward<Arg1>(arg1),
        std::forward<ArgN>(argn)...)
{
}

template<class Fields>
status
header<false, Fields>::
result() const
{
    return int_to_status(
        static_cast<int>(result_));
}

template<class Fields>
void
header<false, Fields>::
result(status v)
{
    result_ = v;
}

template<class Fields>
void
header<false, Fields>::
result(unsigned v)
{
    if(v > 999)
        BOOST_THROW_EXCEPTION(
            std::invalid_argument{
                "invalid status-code"});
    result_ = static_cast<status>(v);
}

template<class Fields>
unsigned
header<false, Fields>::
result_int() const
{
    return static_cast<unsigned>(result_);
}

template<class Fields>
string_view
header<false, Fields>::
reason() const
{
    auto const s = this->get_reason_impl();
    if(! s.empty())
        return s;
    return obsolete_reason(result_);
}

template<class Fields>
void
header<false, Fields>::
reason(string_view s)
{
    this->set_reason_impl(s);
}

template<class Fields>
void
swap(
    header<false, Fields>& h1,
    header<false, Fields>& h2)
{
    using std::swap;
    swap(
        static_cast<Fields&>(h1),
        static_cast<Fields&>(h2));
    swap(h1.version_, h2.version_);
    swap(h1.result_, h2.result_);
}

//------------------------------------------------------------------------------

template<bool isRequest, class Body, class Fields>
template<class... BodyArgs>
message<isRequest, Body, Fields>::
message(header_type&& h, BodyArgs&&... body_args)
    : header_type(std::move(h))
    , boost::empty_value<
        typename Body::value_type>(boost::empty_init_t(),
            std::forward<BodyArgs>(body_args)...)
{
}

template<bool isRequest, class Body, class Fields>
template<class... BodyArgs>
message<isRequest, Body, Fields>::
message(header_type const& h, BodyArgs&&... body_args)
    : header_type(h)
    , boost::empty_value<
        typename Body::value_type>(boost::empty_init_t(),
            std::forward<BodyArgs>(body_args)...)
{
}

template<bool isRequest, class Body, class Fields>
template<class Version, class>
message<isRequest, Body, Fields>::
message(verb method, string_view target, Version version)
    : header_type(method, target, version)
{
}

template<bool isRequest, class Body, class Fields>
template<class Version, class BodyArg, class>
message<isRequest, Body, Fields>::
message(verb method, string_view target,
        Version version, BodyArg&& body_arg)
    : header_type(method, target, version)
    , boost::empty_value<
        typename Body::value_type>(boost::empty_init_t(),
            std::forward<BodyArg>(body_arg))
{
}

template<bool isRequest, class Body, class Fields>
template<class Version, class BodyArg, class FieldsArg, class>
message<isRequest, Body, Fields>::
message(
    verb method, string_view target, Version version,
    BodyArg&& body_arg,
    FieldsArg&& fields_arg)
    : header_type(method, target, version,
        std::forward<FieldsArg>(fields_arg))
    , boost::empty_value<
        typename Body::value_type>(boost::empty_init_t(),
            std::forward<BodyArg>(body_arg))
{
}

template<bool isRequest, class Body, class Fields>
template<class Version, class>
message<isRequest, Body, Fields>::
message(status result, Version version)
    : header_type(result, version)
{
}

template<bool isRequest, class Body, class Fields>
template<class Version, class BodyArg, class>
message<isRequest, Body, Fields>::
message(status result, Version version,
    BodyArg&& body_arg)
    : header_type(result, version)
    , boost::empty_value<
        typename Body::value_type>(boost::empty_init_t(),
            std::forward<BodyArg>(body_arg))
{
}

template<bool isRequest, class Body, class Fields>
template<class Version, class BodyArg, class FieldsArg, class>
message<isRequest, Body, Fields>::
message(status result, Version version,
    BodyArg&& body_arg, FieldsArg&& fields_arg)
    : header_type(result, version,
        std::forward<FieldsArg>(fields_arg))
    , boost::empty_value<
        typename Body::value_type>(boost::empty_init_t(),
            std::forward<BodyArg>(body_arg))
{
}

template<bool isRequest, class Body, class Fields>
message<isRequest, Body, Fields>::
message(std::piecewise_construct_t)
{
}

template<bool isRequest, class Body, class Fields>
template<class... BodyArgs>
message<isRequest, Body, Fields>::
message(std::piecewise_construct_t,
        std::tuple<BodyArgs...> body_args)
    : message(std::piecewise_construct,
        body_args,
        mp11::make_index_sequence<
            sizeof...(BodyArgs)>{})
{
}

template<bool isRequest, class Body, class Fields>
template<class... BodyArgs, class... FieldsArgs>
message<isRequest, Body, Fields>::
message(std::piecewise_construct_t,
    std::tuple<BodyArgs...> body_args,
    std::tuple<FieldsArgs...> fields_args)
    : message(std::piecewise_construct,
        body_args,
        fields_args,
        mp11::make_index_sequence<
            sizeof...(BodyArgs)>{},
        mp11::make_index_sequence<
            sizeof...(FieldsArgs)>{})
{
}

template<bool isRequest, class Body, class Fields>
void
message<isRequest, Body, Fields>::
chunked(bool value)
{
    this->set_chunked_impl(value);
    this->set_content_length_impl(boost::none);
}

template<bool isRequest, class Body, class Fields>
void
message<isRequest, Body, Fields>::
content_length(
    boost::optional<std::uint64_t> const& value)
{
    this->set_content_length_impl(value);
    this->set_chunked_impl(false);
}

template<bool isRequest, class Body, class Fields>
boost::optional<std::uint64_t>
message<isRequest, Body, Fields>::
payload_size() const
{
    return payload_size(detail::is_body_sized<Body>{});
}

template<bool isRequest, class Body, class Fields>
bool
message<isRequest, Body, Fields>::
need_eof(std::false_type) const
{
    // VFALCO Do we need a way to let the caller say "the body is intentionally skipped"?
    if( this->result() == status::no_content ||
        this->result() == status::not_modified ||
        to_status_class(this->result()) ==
            status_class::informational ||
        has_content_length() ||
        chunked())
        return ! keep_alive();
    return true;
}

template<bool isRequest, class Body, class Fields>
void
message<isRequest, Body, Fields>::
prepare_payload(std::true_type)
{
    auto const n = payload_size();
    if(this->method() == verb::trace && (! n || *n > 0))
        BOOST_THROW_EXCEPTION(std::invalid_argument{
            "invalid request body"});
    if(n)
    {
        if(*n > 0 ||
            this->method() == verb::options ||
            this->method() == verb::put ||
            this->method() == verb::post)
        {
            this->content_length(n);
        }
        else
        {
            this->chunked(false);
        }
    }
    else if(this->version() == 11)
    {
        this->chunked(true);
    }
    else
    {
        this->chunked(false);
    }
}

template<bool isRequest, class Body, class Fields>
void
message<isRequest, Body, Fields>::
prepare_payload(std::false_type)
{
    auto const n = payload_size();
    if( (! n || *n > 0) && (
        (status_class(this->result()) == status_class::informational ||
        this->result() == status::no_content ||
        this->result() == status::not_modified)))
    {
        // The response body MUST be empty for this case
        BOOST_THROW_EXCEPTION(std::invalid_argument{
            "invalid response body"});
    }
    if(n)
        this->content_length(n);
    else if(this->version() == 11)
        this->chunked(true);
    else
        this->chunked(false);
}

//------------------------------------------------------------------------------

template<bool isRequest, class Body, class Fields>
void
swap(
    message<isRequest, Body, Fields>& m1,
    message<isRequest, Body, Fields>& m2)
{
    using std::swap;
    swap(
        static_cast<header<isRequest, Fields>&>(m1),
        static_cast<header<isRequest, Fields>&>(m2));
    swap(m1.body(), m2.body());
}

} // http
} // beast
} // boost

#endif
