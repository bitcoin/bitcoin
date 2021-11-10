//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_IMPL_STATUS_IPP
#define BOOST_BEAST_HTTP_IMPL_STATUS_IPP

#include <boost/beast/http/status.hpp>
#include <boost/throw_exception.hpp>

namespace boost {
namespace beast {
namespace http {

status
int_to_status(unsigned v)
{
    switch(static_cast<status>(v))
    {
    // 1xx
    case status::continue_:
    case status::switching_protocols:
    case status::processing:
        BOOST_FALLTHROUGH;

    // 2xx
    case status::ok:
    case status::created:
    case status::accepted:
    case status::non_authoritative_information:
    case status::no_content:
    case status::reset_content:
    case status::partial_content:
    case status::multi_status:
    case status::already_reported:
    case status::im_used:
        BOOST_FALLTHROUGH;

    // 3xx
    case status::multiple_choices:
    case status::moved_permanently:
    case status::found:
    case status::see_other:
    case status::not_modified:
    case status::use_proxy:
    case status::temporary_redirect:
    case status::permanent_redirect:
        BOOST_FALLTHROUGH;

    // 4xx
    case status::bad_request:
    case status::unauthorized:
    case status::payment_required:
    case status::forbidden:
    case status::not_found:
    case status::method_not_allowed:
    case status::not_acceptable:
    case status::proxy_authentication_required:
    case status::request_timeout:
    case status::conflict:
    case status::gone:
    case status::length_required:
    case status::precondition_failed:
    case status::payload_too_large:
    case status::uri_too_long:
    case status::unsupported_media_type:
    case status::range_not_satisfiable:
    case status::expectation_failed:
    case status::misdirected_request:
    case status::unprocessable_entity:
    case status::locked:
    case status::failed_dependency:
    case status::upgrade_required:
    case status::precondition_required:
    case status::too_many_requests:
    case status::request_header_fields_too_large:
    case status::connection_closed_without_response:
    case status::unavailable_for_legal_reasons:
    case status::client_closed_request:
        BOOST_FALLTHROUGH;

    // 5xx
    case status::internal_server_error:
    case status::not_implemented:
    case status::bad_gateway:
    case status::service_unavailable:
    case status::gateway_timeout:
    case status::http_version_not_supported:
    case status::variant_also_negotiates:
    case status::insufficient_storage:
    case status::loop_detected:
    case status::not_extended:
    case status::network_authentication_required:
    case status::network_connect_timeout_error:
        return static_cast<status>(v);

    default:
        break;
    }
    return status::unknown;
}

status_class
to_status_class(unsigned v)
{
    switch(v / 100)
    {
    case 1: return status_class::informational;
    case 2: return status_class::successful;
    case 3: return status_class::redirection;
    case 4: return status_class::client_error;
    case 5: return status_class::server_error;
    default:
        break;
    }
    return status_class::unknown;
}

status_class
to_status_class(status v)
{
    return to_status_class(static_cast<int>(v));
}

string_view
obsolete_reason(status v)
{
    switch(static_cast<status>(v))
    {
    // 1xx
    case status::continue_:                             return "Continue";
    case status::switching_protocols:                   return "Switching Protocols";
    case status::processing:                            return "Processing";

    // 2xx
    case status::ok:                                    return "OK";
    case status::created:                               return "Created";
    case status::accepted:                              return "Accepted";
    case status::non_authoritative_information:         return "Non-Authoritative Information";
    case status::no_content:                            return "No Content";
    case status::reset_content:                         return "Reset Content";
    case status::partial_content:                       return "Partial Content";
    case status::multi_status:                          return "Multi-Status";
    case status::already_reported:                      return "Already Reported";
    case status::im_used:                               return "IM Used";

    // 3xx
    case status::multiple_choices:                      return "Multiple Choices";
    case status::moved_permanently:                     return "Moved Permanently";
    case status::found:                                 return "Found";
    case status::see_other:                             return "See Other";
    case status::not_modified:                          return "Not Modified";
    case status::use_proxy:                             return "Use Proxy";
    case status::temporary_redirect:                    return "Temporary Redirect";
    case status::permanent_redirect:                    return "Permanent Redirect";

    // 4xx
    case status::bad_request:                           return "Bad Request";
    case status::unauthorized:                          return "Unauthorized";
    case status::payment_required:                      return "Payment Required";
    case status::forbidden:                             return "Forbidden";
    case status::not_found:                             return "Not Found";
    case status::method_not_allowed:                    return "Method Not Allowed";
    case status::not_acceptable:                        return "Not Acceptable";
    case status::proxy_authentication_required:         return "Proxy Authentication Required";
    case status::request_timeout:                       return "Request Timeout";
    case status::conflict:                              return "Conflict";
    case status::gone:                                  return "Gone";
    case status::length_required:                       return "Length Required";
    case status::precondition_failed:                   return "Precondition Failed";
    case status::payload_too_large:                     return "Payload Too Large";
    case status::uri_too_long:                          return "URI Too Long";
    case status::unsupported_media_type:                return "Unsupported Media Type";
    case status::range_not_satisfiable:                 return "Range Not Satisfiable";
    case status::expectation_failed:                    return "Expectation Failed";
    case status::misdirected_request:                   return "Misdirected Request";
    case status::unprocessable_entity:                  return "Unprocessable Entity";
    case status::locked:                                return "Locked";
    case status::failed_dependency:                     return "Failed Dependency";
    case status::upgrade_required:                      return "Upgrade Required";
    case status::precondition_required:                 return "Precondition Required";
    case status::too_many_requests:                     return "Too Many Requests";
    case status::request_header_fields_too_large:       return "Request Header Fields Too Large";
    case status::connection_closed_without_response:    return "Connection Closed Without Response";
    case status::unavailable_for_legal_reasons:         return "Unavailable For Legal Reasons";
    case status::client_closed_request:                 return "Client Closed Request";
    // 5xx
    case status::internal_server_error:                 return "Internal Server Error";
    case status::not_implemented:                       return "Not Implemented";
    case status::bad_gateway:                           return "Bad Gateway";
    case status::service_unavailable:                   return "Service Unavailable";
    case status::gateway_timeout:                       return "Gateway Timeout";
    case status::http_version_not_supported:            return "HTTP Version Not Supported";
    case status::variant_also_negotiates:               return "Variant Also Negotiates";
    case status::insufficient_storage:                  return "Insufficient Storage";
    case status::loop_detected:                         return "Loop Detected";
    case status::not_extended:                          return "Not Extended";
    case status::network_authentication_required:       return "Network Authentication Required";
    case status::network_connect_timeout_error:         return "Network Connect Timeout Error";

    default:
        break;
    }
    return "<unknown-status>";
}

std::ostream&
operator<<(std::ostream& os, status v)
{
    return os << obsolete_reason(v);
}

} // http
} // beast
} // boost

#endif
