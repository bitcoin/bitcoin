//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_READ_HPP
#define BOOST_BEAST_HTTP_READ_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/http/basic_parser.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/asio/async_result.hpp>

namespace boost {
namespace beast {
namespace http {

//------------------------------------------------------------------------------

/** Read part of a message from a stream using a parser.

    This function is used to read part of a message from a stream into an
    instance of @ref basic_parser. The call will block until one of the
    following conditions is true:

    @li A call to @ref basic_parser::put with a non-empty buffer sequence
        is successful.

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `read_some` function. The implementation may read additional bytes from
    the stream that lie past the end of the message being read. These additional
    bytes are stored in the dynamic buffer, which must be preserved for
    subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type must
    meet the <em>SyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements.

    @param parser The parser to use.

    @return The number of bytes transferred from the stream.

    @throws system_error Thrown on failure.

    @note The function returns the total number of bytes transferred from the
    stream. This may be zero for the case where there is sufficient pre-existing
    message data in the dynamic buffer.
*/
template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read_some(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser);

/** Read part of a message from a stream using a parser.

    This function is used to read part of a message from a stream into an
    instance of @ref basic_parser. The call will block until one of the
    following conditions is true:

    @li A call to @ref basic_parser::put with a non-empty buffer sequence
        is successful.

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `read_some` function. The implementation may read additional bytes from
    the stream that lie past the end of the message being read. These additional
    bytes are stored in the dynamic buffer, which must be preserved for
    subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type must
    support the <em>SyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements.

    @param parser The parser to use.

    @param ec Set to the error, if any occurred.

    @return The number of bytes transferred from the stream.

    @note The function returns the total number of bytes transferred from the
    stream. This may be zero for the case where there is sufficient pre-existing
    message data in the dynamic buffer.
*/
template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read_some(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    error_code& ec);

/** Read part of a message asynchronously from a stream using a parser.

    This function is used to asynchronously read part of a message from
    a stream into an instance of @ref basic_parser. The function call
    always returns immediately. The asynchronous operation will continue
    until one of the following conditions is true:

    @li A call to @ref basic_parser::put with a non-empty buffer sequence
        is successful.

    @li An error occurs.

    This operation is implemented in terms of zero or more calls to the
    next layer's `async_read_some` function, and is known as a <em>composed
    operation</em>. The program must ensure that the stream performs no other
    reads until this operation completes. The implementation may read additional
    bytes from the stream that lie past the end of the message being read.
    These additional bytes are stored in the dynamic buffer, which must be
    preserved for subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type
    must meet the <em>AsyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements. The object must remain valid at least until the handler
    is called; ownership is not transferred.

    @param parser The parser to use. The object must remain valid at least until
    the handler is called; ownership is not transferred.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void handler(
        error_code const& error,        // result of operation
        std::size_t bytes_transferred   // the total number of bytes transferred from the stream
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.

    @note The completion handler will receive as a parameter the total number
    of bytes transferred from the stream. This may be zero for the case where
    there is sufficient pre-existing message data in the dynamic buffer.
*/
template<
    class AsyncReadStream,
    class DynamicBuffer,
    bool isRequest,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler =
        net::default_completion_token_t<
            executor_type<AsyncReadStream>>>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read_some(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    ReadHandler&& handler =
        net::default_completion_token_t<
            executor_type<AsyncReadStream>>{});

//------------------------------------------------------------------------------

/** Read a complete message header from a stream using a parser.

    This function is used to read a complete message header from a stream
    into an instance of @ref basic_parser. The call will block until one of the
    following conditions is true:

    @li @ref basic_parser::is_header_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `read_some` function. The implementation may read additional bytes from
    the stream that lie past the end of the message being read. These additional
    bytes are stored in the dynamic buffer, which must be preserved for
    subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type must
    meet the <em>SyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements.

    @param parser The parser to use.

    @return The number of bytes transferred from the stream.

    @throws system_error Thrown on failure.

    @note The function returns the total number of bytes transferred from the
    stream. This may be zero for the case where there is sufficient pre-existing
    message data in the dynamic buffer. The implementation will call
    @ref basic_parser::eager with the value `false` on the parser passed in.
*/
template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read_header(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser);

/** Read a complete message header from a stream using a parser.

    This function is used to read a complete message header from a stream
    into an instance of @ref basic_parser. The call will block until one of the
    following conditions is true:

    @li @ref basic_parser::is_header_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `read_some` function. The implementation may read additional bytes from
    the stream that lie past the end of the message being read. These additional
    bytes are stored in the dynamic buffer, which must be preserved for
    subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type must
    meet the <em>SyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements.

    @param parser The parser to use.

    @param ec Set to the error, if any occurred.

    @return The number of bytes transferred from the stream.

    @note The function returns the total number of bytes transferred from the
    stream. This may be zero for the case where there is sufficient pre-existing
    message data in the dynamic buffer. The implementation will call
    @ref basic_parser::eager with the value `false` on the parser passed in.
*/
template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read_header(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    error_code& ec);

/** Read a complete message header asynchronously from a stream using a parser.

    This function is used to asynchronously read a complete message header from
    a stream into an instance of @ref basic_parser. The function call always
    returns immediately. The asynchronous operation will continue until one of
    the following conditions is true:

    @li @ref basic_parser::is_header_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of zero or more calls to the
    next layer's `async_read_some` function, and is known as a <em>composed
    operation</em>. The program must ensure that the stream performs no other
    reads until this operation completes. The implementation may read additional
    bytes from the stream that lie past the end of the message being read.
    These additional bytes are stored in the dynamic buffer, which must be
    preserved for subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type
    must meet the <em>AsyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements. The object must remain valid at least until the handler
    is called; ownership is not transferred.

    @param parser The parser to use. The object must remain valid at least until
    the handler is called; ownership is not transferred.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void handler(
        error_code const& error,        // result of operation
        std::size_t bytes_transferred   // the total number of bytes transferred from the stream
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.

    @note The completion handler will receive as a parameter the total number
    of bytes transferred from the stream. This may be zero for the case where
    there is sufficient pre-existing message data in the dynamic buffer. The
    implementation will call @ref basic_parser::eager with the value `false`
    on the parser passed in.
*/
template<
    class AsyncReadStream,
    class DynamicBuffer,
    bool isRequest,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler =
        net::default_completion_token_t<
            executor_type<AsyncReadStream>>>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read_header(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    ReadHandler&& handler =
        net::default_completion_token_t<
            executor_type<AsyncReadStream>>{});

//------------------------------------------------------------------------------

/** Read a complete message from a stream using a parser.

    This function is used to read a complete message from a stream into an
    instance of @ref basic_parser. The call will block until one of the
    following conditions is true:

    @li @ref basic_parser::is_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `read_some` function. The implementation may read additional bytes from
    the stream that lie past the end of the message being read. These additional
    bytes are stored in the dynamic buffer, which must be preserved for
    subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type must
    meet the <em>SyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements.

    @param parser The parser to use.

    @return The number of bytes transferred from the stream.

    @throws system_error Thrown on failure.

    @note The function returns the total number of bytes transferred from the
    stream. This may be zero for the case where there is sufficient pre-existing
    message data in the dynamic buffer. The implementation will call
    @ref basic_parser::eager with the value `true` on the parser passed in.
*/
template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser);

/** Read a complete message from a stream using a parser.

    This function is used to read a complete message from a stream into an
    instance of @ref basic_parser. The call will block until one of the
    following conditions is true:

    @li @ref basic_parser::is_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `read_some` function. The implementation may read additional bytes from
    the stream that lie past the end of the message being read. These additional
    bytes are stored in the dynamic buffer, which must be preserved for
    subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type must
    meet the <em>SyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements.

    @param parser The parser to use.

    @param ec Set to the error, if any occurred.

    @return The number of bytes transferred from the stream.

    @note The function returns the total number of bytes transferred from the
    stream. This may be zero for the case where there is sufficient pre-existing
    message data in the dynamic buffer. The implementation will call
    @ref basic_parser::eager with the value `true` on the parser passed in.
*/
template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    error_code& ec);

/** Read a complete message asynchronously from a stream using a parser.

    This function is used to asynchronously read a complete message from a
    stream into an instance of @ref basic_parser. The function call always
    returns immediately. The asynchronous operation will continue until one
    of the following conditions is true:

    @li @ref basic_parser::is_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of zero or more calls to the
    next layer's `async_read_some` function, and is known as a <em>composed
    operation</em>. The program must ensure that the stream performs no other
    reads until this operation completes. The implementation may read additional
    bytes from the stream that lie past the end of the message being read.
    These additional bytes are stored in the dynamic buffer, which must be
    preserved for subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type
    must meet the <em>AsyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements. The object must remain valid at least until the handler
    is called; ownership is not transferred.

    @param parser The parser to use. The object must remain valid at least until
    the handler is called; ownership is not transferred.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void handler(
        error_code const& error,        // result of operation
        std::size_t bytes_transferred   // the total number of bytes transferred from the stream
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.

    @note The completion handler will receive as a parameter the total number
    of bytes transferred from the stream. This may be zero for the case where
    there is sufficient pre-existing message data in the dynamic buffer. The
    implementation will call @ref basic_parser::eager with the value `true`
    on the parser passed in.
*/
template<
    class AsyncReadStream,
    class DynamicBuffer,
    bool isRequest,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler =
        net::default_completion_token_t<
            executor_type<AsyncReadStream>>>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    ReadHandler&& handler =
        net::default_completion_token_t<
            executor_type<AsyncReadStream>>{});

//------------------------------------------------------------------------------

/** Read a complete message from a stream.

    This function is used to read a complete message from a stream into an
    instance of @ref message. The call will block until one of the following
    conditions is true:

    @li The entire message is read in.

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `read_some` function. The implementation may read additional bytes from
    the stream that lie past the end of the message being read. These additional
    bytes are stored in the dynamic buffer, which must be preserved for
    subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type must
    meet the <em>SyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements.

    @param msg The container in which to store the message contents. This
    message container should not have previous contents, otherwise the behavior
    is undefined. The type must be meet the <em>MoveAssignable</em> and
    <em>MoveConstructible</em> requirements.

    @return The number of bytes transferred from the stream.

    @throws system_error Thrown on failure.

    @note The function returns the total number of bytes transferred from the
    stream. This may be zero for the case where there is sufficient pre-existing
    message data in the dynamic buffer. The implementation will call
    @ref basic_parser::eager with the value `true` on the parser passed in.
*/
template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest, class Body, class Allocator>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    message<isRequest, Body, basic_fields<Allocator>>& msg);

/** Read a complete message from a stream.

    This function is used to read a complete message from a stream into an
    instance of @ref message. The call will block until one of the following
    conditions is true:

    @li The entire message is read in.

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `read_some` function. The implementation may read additional bytes from
    the stream that lie past the end of the message being read. These additional
    bytes are stored in the dynamic buffer, which must be preserved for
    subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type must
    meet the <em>SyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements.

    @param msg The container in which to store the message contents. This
    message container should not have previous contents, otherwise the behavior
    is undefined. The type must be meet the <em>MoveAssignable</em> and
    <em>MoveConstructible</em> requirements.

    @param ec Set to the error, if any occurred.

    @return The number of bytes transferred from the stream.

    @note The function returns the total number of bytes transferred from the
    stream. This may be zero for the case where there is sufficient pre-existing
    message data in the dynamic buffer. The implementation will call
    @ref basic_parser::eager with the value `true` on the parser passed in.
*/
template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest, class Body, class Allocator>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    message<isRequest, Body, basic_fields<Allocator>>& msg,
    error_code& ec);

/** Read a complete message asynchronously from a stream.

    This function is used to asynchronously read a complete message from a
    stream into an instance of @ref message. The function call always returns
    immediately. The asynchronous operation will continue until one of the
    following conditions is true:

    @li The entire message is read in.

    @li An error occurs.

    This operation is implemented in terms of zero or more calls to the
    next layer's `async_read_some` function, and is known as a <em>composed
    operation</em>. The program must ensure that the stream performs no other
    reads until this operation completes. The implementation may read additional
    bytes from the stream that lie past the end of the message being read.
    These additional bytes are stored in the dynamic buffer, which must be
    preserved for subsequent reads.

    If the end of file error is received while reading from the stream, then
    the error returned from this function will be:

    @li @ref error::end_of_stream if no bytes were parsed, or

    @li @ref error::partial_message if any bytes were parsed but the
        message was incomplete, otherwise:

    @li A successful result. The next attempt to read will return
        @ref error::end_of_stream

    @param stream The stream from which the data is to be read. The type
    must meet the <em>AsyncReadStream</em> requirements.

    @param buffer Storage for additional bytes read by the implementation from
    the stream. This is both an input and an output parameter; on entry, the
    parser will be presented with any remaining data in the dynamic buffer's
    readable bytes sequence first. The type must meet the <em>DynamicBuffer</em>
    requirements. The object must remain valid at least until the handler
    is called; ownership is not transferred.

    @param msg The container in which to store the message contents. This
    message container should not have previous contents, otherwise the behavior
    is undefined. The type must be meet the <em>MoveAssignable</em> and
    <em>MoveConstructible</em> requirements. The object must remain valid
    at least until the handler is called; ownership is not transferred.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void handler(
        error_code const& error,        // result of operation
        std::size_t bytes_transferred   // the total number of bytes transferred from the stream
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.

    @note The completion handler will receive as a parameter the total number
    of bytes transferred from the stream. This may be zero for the case where
    there is sufficient pre-existing message data in the dynamic buffer. The
    implementation will call @ref basic_parser::eager with the value `true`
    on the parser passed in.
*/
template<
    class AsyncReadStream,
    class DynamicBuffer,
    bool isRequest, class Body, class Allocator,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler =
        net::default_completion_token_t<
            executor_type<AsyncReadStream>>>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    message<isRequest, Body, basic_fields<Allocator>>& msg,
    ReadHandler&& handler =
        net::default_completion_token_t<
            executor_type<AsyncReadStream>>{});

} // http
} // beast
} // boost

#include <boost/beast/http/impl/read.hpp>

#endif
