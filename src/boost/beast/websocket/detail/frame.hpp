//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_DETAIL_FRAME_HPP
#define BOOST_BEAST_WEBSOCKET_DETAIL_FRAME_HPP

#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/detail/utf8_checker.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/assert.hpp>
#include <boost/endian/conversion.hpp>
#include <cstdint>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

// frame header opcodes
enum class opcode : std::uint8_t
{
    cont    = 0,
    text    = 1,
    binary  = 2,
    rsv3    = 3,
    rsv4    = 4,
    rsv5    = 5,
    rsv6    = 6,
    rsv7    = 7,
    close   = 8,
    ping    = 9,
    pong    = 10,
    crsvb   = 11,
    crsvc   = 12,
    crsvd   = 13,
    crsve   = 14,
    crsvf   = 15
};

// Contents of a WebSocket frame header
struct frame_header
{
    std::uint64_t len;
    std::uint32_t key;
    opcode op;
    bool fin  : 1;
    bool mask : 1;
    bool rsv1 : 1;
    bool rsv2 : 1;
    bool rsv3 : 1;
};

// holds the largest possible frame header
using fh_buffer = flat_static_buffer<14>;

// holds the largest possible control frame
using frame_buffer =
    flat_static_buffer< 2 + 8 + 4 + 125 >;

inline
bool constexpr
is_reserved(opcode op)
{
    return
        (op >= opcode::rsv3  && op <= opcode::rsv7) ||
        (op >= opcode::crsvb && op <= opcode::crsvf);
}

inline
bool constexpr
is_valid(opcode op)
{
    return op <= opcode::crsvf;
}

inline
bool constexpr
is_control(opcode op)
{
    return op >= opcode::close;
}

inline
bool
is_valid_close_code(std::uint16_t v)
{
    switch(v)
    {
    case close_code::normal:            // 1000
    case close_code::going_away:        // 1001
    case close_code::protocol_error:    // 1002
    case close_code::unknown_data:      // 1003
    case close_code::bad_payload:       // 1007
    case close_code::policy_error:      // 1008
    case close_code::too_big:           // 1009
    case close_code::needs_extension:   // 1010
    case close_code::internal_error:    // 1011
    case close_code::service_restart:   // 1012
    case close_code::try_again_later:   // 1013
        return true;

    // explicitly reserved
    case close_code::reserved1:         // 1004
    case close_code::no_status:         // 1005
    case close_code::abnormal:          // 1006
    case close_code::reserved2:         // 1014
    case close_code::reserved3:         // 1015
        return false;
    }
    // reserved
    if(v >= 1016 && v <= 2999)
        return false;
    // not used
    if(v <= 999)
        return false;
    return true;
}

//------------------------------------------------------------------------------

// Write frame header to dynamic buffer
//
template<class DynamicBuffer>
void
write(DynamicBuffer& db, frame_header const& fh)
{
    std::size_t n;
    std::uint8_t b[14];
    b[0] = (fh.fin ? 0x80 : 0x00) | static_cast<std::uint8_t>(fh.op);
    if(fh.rsv1)
        b[0] |= 0x40;
    if(fh.rsv2)
        b[0] |= 0x20;
    if(fh.rsv3)
        b[0] |= 0x10;
    b[1] = fh.mask ? 0x80 : 0x00;
    if(fh.len <= 125)
    {
        b[1] |= fh.len;
        n = 2;
    }
    else if(fh.len <= 65535)
    {
        b[1] |= 126;
        auto len_be = endian::native_to_big(
            static_cast<std::uint16_t>(fh.len));
        std::memcpy(&b[2], &len_be, sizeof(len_be));
        n = 4;
    }
    else
    {
        b[1] |= 127;
        auto len_be = endian::native_to_big(
            static_cast<std::uint64_t>(fh.len));
        std::memcpy(&b[2], &len_be, sizeof(len_be));
        n = 10;
    }
    if(fh.mask)
    {
        auto key_le = endian::native_to_little(
            static_cast<std::uint32_t>(fh.key));
        std::memcpy(&b[n], &key_le, sizeof(key_le));
        n += 4;
    }
    db.commit(net::buffer_copy(
        db.prepare(n), net::buffer(b)));
}

// Read data from buffers
// This is for ping and pong payloads
//
template<class Buffers>
void
read_ping(ping_data& data, Buffers const& bs)
{
    BOOST_ASSERT(buffer_bytes(bs) <= data.max_size());
    data.resize(buffer_bytes(bs));
    net::buffer_copy(net::mutable_buffer{
        data.data(), data.size()}, bs);
}

// Read close_reason, return true on success
// This is for the close payload
//
template<class Buffers>
void
read_close(
    close_reason& cr,
    Buffers const& bs,
    error_code& ec)
{
    auto const n = buffer_bytes(bs);
    BOOST_ASSERT(n <= 125);
    if(n == 0)
    {
        cr = close_reason{};
        ec = {};
        return;
    }
    if(n == 1)
    {
        // invalid payload size == 1
        ec = error::bad_close_size;
        return;
    }

    std::uint16_t code_be;
    cr.reason.resize(n - 2);
    std::array<net::mutable_buffer, 2> out_bufs{{
        net::mutable_buffer(&code_be, sizeof(code_be)),
        net::mutable_buffer(&cr.reason[0], n - 2)}};

    net::buffer_copy(out_bufs, bs);

    cr.code = endian::big_to_native(code_be);
    if(! is_valid_close_code(cr.code))
    {
        // invalid close code
        ec = error::bad_close_code;
        return;
    }

    if(n > 2 && !check_utf8(
        cr.reason.data(), cr.reason.size()))
    {
        // not valid utf-8
        ec = error::bad_close_payload;
        return;
    }
    ec = {};
}

} // detail
} // websocket
} // beast
} // boost

#endif
