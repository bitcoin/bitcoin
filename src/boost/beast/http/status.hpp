//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_STATUS_HPP
#define BOOST_BEAST_HTTP_STATUS_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/string.hpp>
#include <iosfwd>

namespace boost {
namespace beast {
namespace http {

enum class status : unsigned
{
    /** An unknown status-code.

        This value indicates that the value for the status code
        is not in the list of commonly recognized status codes.
        Callers interested in the exactly value should use the
        interface which provides the raw integer.
    */
    unknown = 0,

    continue_                           = 100,

    /** Switching Protocols

        This status indicates that a request to switch to a new
        protocol was accepted and applied by the server. A successful
        response to a WebSocket Upgrade HTTP request will have this
        code.
    */
    switching_protocols                 = 101,

    processing                          = 102,

    ok                                  = 200,
    created                             = 201,
    accepted                            = 202,
    non_authoritative_information       = 203,
    no_content                          = 204,
    reset_content                       = 205,
    partial_content                     = 206,
    multi_status                        = 207,
    already_reported                    = 208,
    im_used                             = 226,

    multiple_choices                    = 300,
    moved_permanently                   = 301,
    found                               = 302,
    see_other                           = 303,
    not_modified                        = 304,
    use_proxy                           = 305,
    temporary_redirect                  = 307,
    permanent_redirect                  = 308,

    bad_request                         = 400,
    unauthorized                        = 401,
    payment_required                    = 402,
    forbidden                           = 403,
    not_found                           = 404,
    method_not_allowed                  = 405,
    not_acceptable                      = 406,
    proxy_authentication_required       = 407,
    request_timeout                     = 408,
    conflict                            = 409,
    gone                                = 410,
    length_required                     = 411,
    precondition_failed                 = 412,
    payload_too_large                   = 413,
    uri_too_long                        = 414,
    unsupported_media_type              = 415,
    range_not_satisfiable               = 416,
    expectation_failed                  = 417,
    misdirected_request                 = 421,
    unprocessable_entity                = 422,
    locked                              = 423,
    failed_dependency                   = 424,
    upgrade_required                    = 426,
    precondition_required               = 428,
    too_many_requests                   = 429,
    request_header_fields_too_large     = 431,
    connection_closed_without_response  = 444,
    unavailable_for_legal_reasons       = 451,
    client_closed_request               = 499,

    internal_server_error               = 500,
    not_implemented                     = 501,
    bad_gateway                         = 502,
    service_unavailable                 = 503,
    gateway_timeout                     = 504,
    http_version_not_supported          = 505,
    variant_also_negotiates             = 506,
    insufficient_storage                = 507,
    loop_detected                       = 508,
    not_extended                        = 510,
    network_authentication_required     = 511,
    network_connect_timeout_error       = 599
};

/** Represents the class of a status-code.
*/
enum class status_class : unsigned
{
    /// Unknown status-class
    unknown = 0,

    /// The request was received, continuing processing.
    informational = 1,

    /// The request was successfully received, understood, and accepted.
    successful = 2,

    /// Further action needs to be taken in order to complete the request.
    redirection = 3,

    /// The request contains bad syntax or cannot be fulfilled.
    client_error = 4,

    /// The server failed to fulfill an apparently valid request.
    server_error = 5,
};

/** Converts the integer to a known status-code.

    If the integer does not match a known status code,
    @ref status::unknown is returned.
*/
BOOST_BEAST_DECL
status
int_to_status(unsigned v);

/** Convert an integer to a status_class.

    @param v The integer representing a status code.

    @return The status class. If the integer does not match
    a known status class, @ref status_class::unknown is returned.
*/
BOOST_BEAST_DECL
status_class
to_status_class(unsigned v);

/** Convert a status_code to a status_class.

    @param v The status code to convert.

    @return The status class.
*/
BOOST_BEAST_DECL
status_class
to_status_class(status v);

/** Returns the obsolete reason-phrase text for a status code.

    @param v The status code to use.
*/
BOOST_BEAST_DECL
string_view
obsolete_reason(status v);

/// Outputs the standard reason phrase of a status code to a stream.
BOOST_BEAST_DECL
std::ostream&
operator<<(std::ostream&, status);

} // http
} // beast
} // boost

#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/http/impl/status.ipp>
#endif

#endif
