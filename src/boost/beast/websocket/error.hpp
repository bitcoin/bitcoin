//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_ERROR_HPP
#define BOOST_BEAST_WEBSOCKET_ERROR_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/error.hpp>

namespace boost {
namespace beast {
namespace websocket {

/// Error codes returned from @ref beast::websocket::stream operations.
enum class error
{
    /** The WebSocket stream was gracefully closed at both endpoints
    */
    closed = 1,

/*  The error codes error::failed and error::handshake_failed
    are no longer in use. Please change your code to compare values
    of type error_code against condition::handshake_failed
    and condition::protocol_violation instead.
            
    Apologies for the inconvenience.

    - VFALCO
*/
#if ! BOOST_BEAST_DOXYGEN
    unused1 = 2, // failed
    unused2 = 3, // handshake_failed
#endif

    /** The WebSocket operation caused a dynamic buffer overflow
    */
    buffer_overflow,

    /** The WebSocket stream produced an incomplete deflate block
    */
    partial_deflate_block,

    /** The WebSocket message  exceeded the locally configured limit
    */
    message_too_big,

    //
    // Handshake failure errors
    //
    // These will compare equal to condition::handshake_failed
    //

    /** The WebSocket handshake was not HTTP/1.1

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    bad_http_version,

    /** The WebSocket handshake method was not GET

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    bad_method,

    /** The WebSocket handshake Host field is missing

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    no_host,

    /** The WebSocket handshake Connection field is missing

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    no_connection,

    /** The WebSocket handshake Connection field is missing the upgrade token

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    no_connection_upgrade,

    /** The WebSocket handshake Upgrade field is missing

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    no_upgrade,

    /** The WebSocket handshake Upgrade field is missing the websocket token

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    no_upgrade_websocket,

    /** The WebSocket handshake Sec-WebSocket-Key field is missing

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    no_sec_key,

    /** The WebSocket handshake Sec-WebSocket-Key field is invalid

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    bad_sec_key,

    /** The WebSocket handshake Sec-WebSocket-Version field is missing

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    no_sec_version,

    /** The WebSocket handshake Sec-WebSocket-Version field is invalid

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    bad_sec_version,

    /** The WebSocket handshake Sec-WebSocket-Accept field is missing

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    no_sec_accept,

    /** The WebSocket handshake Sec-WebSocket-Accept field is invalid

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    bad_sec_accept,

    /** The WebSocket handshake was declined by the remote peer

        Error codes with this value will compare equal to @ref condition::handshake_failed
    */
    upgrade_declined,

    //
    // Protocol errors
    //
    // These will compare equal to condition::protocol_violation
    //

    /** The WebSocket frame contained an illegal opcode

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_opcode,

    /** The WebSocket data frame was unexpected

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_data_frame,

    /** The WebSocket continuation frame was unexpected

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_continuation,

    /** The WebSocket frame contained illegal reserved bits

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_reserved_bits,

    /** The WebSocket control frame was fragmented

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_control_fragment,

    /** The WebSocket control frame size was invalid

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_control_size,

    /** The WebSocket frame was unmasked

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_unmasked_frame,

    /** The WebSocket frame was masked

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_masked_frame,

    /** The WebSocket frame size was not canonical

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_size,

    /** The WebSocket frame payload was not valid utf8

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_frame_payload,

    /** The WebSocket close frame reason code was invalid

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_close_code,

    /** The WebSocket close frame payload size was invalid

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_close_size,

    /** The WebSocket close frame payload was not valid utf8

        Error codes with this value will compare equal to @ref condition::protocol_violation
    */
    bad_close_payload
};

/// Error conditions corresponding to sets of error codes.
enum class condition
{
    /** The WebSocket handshake failed

        This condition indicates that the WebSocket handshake failed. If
        the corresponding HTTP response indicates the keep-alive behavior,
        then the handshake may be reattempted.
    */
    handshake_failed = 1,

    /** A WebSocket protocol violation occurred

        This condition indicates that the remote peer on the WebSocket
        connection sent data which violated the protocol.
    */
    protocol_violation
 };

} // websocket
} // beast
} // boost

#include <boost/beast/websocket/impl/error.hpp>
#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/websocket/impl/error.ipp>
#endif

#endif
