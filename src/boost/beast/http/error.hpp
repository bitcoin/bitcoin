//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_ERROR_HPP
#define BOOST_BEAST_HTTP_ERROR_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/error.hpp>

namespace boost {
namespace beast {
namespace http {

/// Error codes returned from HTTP algorithms and operations.
enum class error
{
    /** The end of the stream was reached.

        This error is returned when attempting to read HTTP data,
        and the stream returns the error `net::error::eof`
        before any octets corresponding to a new HTTP message have
        been received.
    */
    end_of_stream = 1,

    /** The incoming message is incomplete.

        This happens when the end of stream is reached during
        parsing and some octets have been received, but not the
        entire message.
    */
    partial_message,

    /** Additional buffers are required.

        This error is returned during parsing when additional
        octets are needed. The caller should append more data
        to the existing buffer and retry the parse operaetion.
    */
    need_more,

    /** An unexpected body was encountered during parsing.

        This error is returned when attempting to parse body
        octets into a message container which has the
        @ref empty_body body type.

        @see empty_body
    */
    unexpected_body,

    /** Additional buffers are required.

        This error is returned under the following conditions:

        @li During serialization when using @ref buffer_body.
        The caller should update the body to point to a new
        buffer or indicate that there are no more octets in
        the body.

        @li During parsing when using @ref buffer_body.
        The caller should update the body to point to a new
        storage area to receive additional body octets.
    */
    need_buffer,

    /** The end of a chunk was reached
    */
    end_of_chunk,

    /** Buffer maximum exceeded.

        This error is returned when reading HTTP content
        into a dynamic buffer, and the operation would
        exceed the maximum size of the buffer.
    */
    buffer_overflow,

    /** Header limit exceeded.

        The parser detected an incoming message header which
        exceeded a configured limit.
    */
    header_limit,

    /** Body limit exceeded.

        The parser detected an incoming message body which
        exceeded a configured limit.
    */
    body_limit,

    /** A memory allocation failed.

        When basic_fields throws std::bad_alloc, it is
        converted into this error by @ref parser.
    */
    bad_alloc,

    //
    // (parser errors)
    //

    /// The line ending was malformed
    bad_line_ending,

    /// The method is invalid.
    bad_method,

    /// The request-target is invalid.
    bad_target,

    /// The HTTP-version is invalid.
    bad_version,

    /// The status-code is invalid.
    bad_status,

    /// The reason-phrase is invalid.
    bad_reason,

    /// The field name is invalid.
    bad_field,

    /// The field value is invalid.
    bad_value,

    /// The Content-Length is invalid.
    bad_content_length,

    /// The Transfer-Encoding is invalid.
    bad_transfer_encoding,

    /// The chunk syntax is invalid.
    bad_chunk,

    /// The chunk extension is invalid.
    bad_chunk_extension,

    /// An obs-fold exceeded an internal limit.
    bad_obs_fold,

    /** The parser is stale.

        This happens when attempting to re-use a parser that has
        already completed parsing a message. Programs must construct
        a new parser for each message. This can be easily done by
        storing the parser in an boost or std::optional container.
    */
    stale_parser,

    /** The message body is shorter than expected.

        This error is returned by @ref file_body when an unexpected
        unexpected end-of-file condition is encountered while trying
        to read from the file.
    */
    short_read
};

} // http
} // beast
} // boost

#include <boost/beast/http/impl/error.hpp>
#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/http/impl/error.ipp>
#endif

#endif
