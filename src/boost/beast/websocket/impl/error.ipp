//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_IMPL_ERROR_IPP
#define BOOST_BEAST_WEBSOCKET_IMPL_ERROR_IPP

#include <boost/beast/websocket/error.hpp>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

class error_codes : public error_category
{
public:
    const char*
    name() const noexcept override
    {
        return "boost.beast.websocket";
    }

    std::string
    message(int ev) const override
    {
        switch(static_cast<error>(ev))
        {
        default:
        case error::closed:                 return "The WebSocket stream was gracefully closed at both endpoints";
        case error::buffer_overflow:        return "The WebSocket operation caused a dynamic buffer overflow";
        case error::partial_deflate_block:  return "The WebSocket stream produced an incomplete deflate block";
        case error::message_too_big:        return "The WebSocket message exceeded the locally configured limit";

        case error::bad_http_version:       return "The WebSocket handshake was not HTTP/1.1";
        case error::bad_method:             return "The WebSocket handshake method was not GET";
        case error::no_host:                return "The WebSocket handshake Host field is missing";
        case error::no_connection:          return "The WebSocket handshake Connection field is missing";
        case error::no_connection_upgrade:  return "The WebSocket handshake Connection field is missing the upgrade token";
        case error::no_upgrade:             return "The WebSocket handshake Upgrade field is missing";
        case error::no_upgrade_websocket:   return "The WebSocket handshake Upgrade field is missing the websocket token";
        case error::no_sec_key:             return "The WebSocket handshake Sec-WebSocket-Key field is missing";
        case error::bad_sec_key:            return "The WebSocket handshake Sec-WebSocket-Key field is invalid";
        case error::no_sec_version:         return "The WebSocket handshake Sec-WebSocket-Version field is missing";
        case error::bad_sec_version:        return "The WebSocket handshake Sec-WebSocket-Version field is invalid";
        case error::no_sec_accept:          return "The WebSocket handshake Sec-WebSocket-Accept field is missing";
        case error::bad_sec_accept:         return "The WebSocket handshake Sec-WebSocket-Accept field is invalid";
        case error::upgrade_declined:       return "The WebSocket handshake was declined by the remote peer";

        case error::bad_opcode:             return "The WebSocket frame contained an illegal opcode";
        case error::bad_data_frame:         return "The WebSocket data frame was unexpected";
        case error::bad_continuation:       return "The WebSocket continuation frame was unexpected";
        case error::bad_reserved_bits:      return "The WebSocket frame contained illegal reserved bits";
        case error::bad_control_fragment:   return "The WebSocket control frame was fragmented";
        case error::bad_control_size:       return "The WebSocket control frame size was invalid";
        case error::bad_unmasked_frame:     return "The WebSocket frame was unmasked";
        case error::bad_masked_frame:       return "The WebSocket frame was masked";
        case error::bad_size:               return "The WebSocket frame size was not canonical";
        case error::bad_frame_payload:      return "The WebSocket frame payload was not valid utf8";
        case error::bad_close_code:         return "The WebSocket close frame reason code was invalid";
        case error::bad_close_size:         return "The WebSocket close frame payload size was invalid";
        case error::bad_close_payload:      return "The WebSocket close frame payload was not valid utf8";
        }
    }

    error_condition
    default_error_condition(int ev) const noexcept override
    {
        switch(static_cast<error>(ev))
        {
        default:
        case error::closed:
        case error::buffer_overflow:
        case error::partial_deflate_block:
        case error::message_too_big:
            return {ev, *this};

        case error::bad_http_version:
        case error::bad_method:
        case error::no_host:
        case error::no_connection:
        case error::no_connection_upgrade:
        case error::no_upgrade:
        case error::no_upgrade_websocket:
        case error::no_sec_key:
        case error::bad_sec_key:
        case error::no_sec_version:
        case error::bad_sec_version:
        case error::no_sec_accept:
        case error::bad_sec_accept:
        case error::upgrade_declined:
            return condition::handshake_failed;

        case error::bad_opcode:
        case error::bad_data_frame:
        case error::bad_continuation:
        case error::bad_reserved_bits:
        case error::bad_control_fragment:
        case error::bad_control_size:
        case error::bad_unmasked_frame:
        case error::bad_masked_frame:
        case error::bad_size:
        case error::bad_frame_payload:
        case error::bad_close_code:
        case error::bad_close_size:
        case error::bad_close_payload:
            return condition::protocol_violation;
        }
    }
};

class error_conditions : public error_category
{
public:
    const char*
    name() const noexcept override
    {
        return "boost.beast.websocket";
    }

    std::string
    message(int cv) const override
    {
        switch(static_cast<condition>(cv))
        {
        default:
        case condition::handshake_failed: return "The WebSocket handshake failed";
        case condition::protocol_violation: return "A WebSocket protocol violation occurred";
        }
    }
};

} // detail

error_code
make_error_code(error e)
{
    static detail::error_codes const cat{};
    return error_code{static_cast<
        std::underlying_type<error>::type>(e), cat};
}

error_condition
make_error_condition(condition c)
{
    static detail::error_conditions const cat{};
    return error_condition{static_cast<
        std::underlying_type<condition>::type>(c), cat};
}

} // websocket
} // beast
} // boost

#endif
