//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_WRITE_HPP
#define BOOST_BEAST_HTTP_WRITE_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/type_traits.hpp>
#include <boost/beast/http/detail/chunk_encode.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/asio/async_result.hpp>
#include <iosfwd>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {
namespace http {

/** Write part of a message to a stream using a serializer.

    This function is used to write part of a message to a stream using
    a caller-provided HTTP/1 serializer. The call will block until one
    of the following conditions is true:
        
    @li One or more bytes have been transferred.

    @li The function @ref serializer::is_done returns `true`

    @li An error occurs on the stream.

    This operation is implemented in terms of one or more calls
    to the stream's `write_some` function.

    The amount of data actually transferred is controlled by the behavior
    of the underlying stream, subject to the buffer size limit of the
    serializer obtained or set through a call to @ref serializer::limit.
    Setting a limit and performing bounded work helps applications set
    reasonable timeouts. It also allows application-level flow control
    to function correctly. For example when using a TCP/IP based
    stream.

    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param sr The serializer to use.

    @return The number of bytes written to the stream.

    @throws system_error Thrown on failure.

    @see serializer
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
std::size_t
write_some(
    SyncWriteStream& stream,
    serializer<isRequest, Body, Fields>& sr);

/** Write part of a message to a stream using a serializer.

    This function is used to write part of a message to a stream using
    a caller-provided HTTP/1 serializer. The call will block until one
    of the following conditions is true:
        
    @li One or more bytes have been transferred.

    @li The function @ref serializer::is_done returns `true`

    @li An error occurs on the stream.

    This operation is implemented in terms of one or more calls
    to the stream's `write_some` function.

    The amount of data actually transferred is controlled by the behavior
    of the underlying stream, subject to the buffer size limit of the
    serializer obtained or set through a call to @ref serializer::limit.
    Setting a limit and performing bounded work helps applications set
    reasonable timeouts. It also allows application-level flow control
    to function correctly. For example when using a TCP/IP based
    stream.
    
    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param sr The serializer to use.

    @param ec Set to indicate what error occurred, if any.

    @return The number of bytes written to the stream.

    @see async_write_some, serializer
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
std::size_t
write_some(
    SyncWriteStream& stream,
    serializer<isRequest, Body, Fields>& sr,
    error_code& ec);

/** Write part of a message to a stream asynchronously using a serializer.

    This function is used to write part of a message to a stream
    asynchronously using a caller-provided HTTP/1 serializer. The function
    call always returns immediately. The asynchronous operation will continue
    until one of the following conditions is true:

    @li One or more bytes have been transferred.

    @li The function @ref serializer::is_done returns `true`

    @li An error occurs on the stream.

    This operation is implemented in terms of zero or more calls to the stream's
    `async_write_some` function, and is known as a <em>composed operation</em>.
    The program must ensure that the stream performs no other writes
    until this operation completes.

    The amount of data actually transferred is controlled by the behavior
    of the underlying stream, subject to the buffer size limit of the
    serializer obtained or set through a call to @ref serializer::limit.
    Setting a limit and performing bounded work helps applications set
    reasonable timeouts. It also allows application-level flow control
    to function correctly. For example when using a TCP/IP based
    stream.
    
    @param stream The stream to which the data is to be written.
    The type must support the <em>AsyncWriteStream</em> concept.

    @param sr The serializer to use.
    The object must remain valid at least until the
    handler is called; ownership is not transferred.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void handler(
        error_code const& error,        // result of operation
        std::size_t bytes_transferred   // the number of bytes written to the stream
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.

    @see serializer
*/
template<
    class AsyncWriteStream,
    bool isRequest, class Body, class Fields,
    BOOST_BEAST_ASYNC_TPARAM2 WriteHandler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
async_write_some(
    AsyncWriteStream& stream,
    serializer<isRequest, Body, Fields>& sr,
    WriteHandler&& handler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>{});

//------------------------------------------------------------------------------

/** Write a header to a stream using a serializer.

    This function is used to write a header to a stream using a
    caller-provided HTTP/1 serializer. The call will block until one
    of the following conditions is true:

    @li The function @ref serializer::is_header_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of one or more calls
    to the stream's `write_some` function.

    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param sr The serializer to use.

    @return The number of bytes written to the stream.

    @throws system_error Thrown on failure.

    @note The implementation will call @ref serializer::split with
    the value `true` on the serializer passed in.

    @see serializer
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
std::size_t
write_header(
    SyncWriteStream& stream,
    serializer<isRequest, Body, Fields>& sr);

/** Write a header to a stream using a serializer.

    This function is used to write a header to a stream using a
    caller-provided HTTP/1 serializer. The call will block until one
    of the following conditions is true:

    @li The function @ref serializer::is_header_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of one or more calls
    to the stream's `write_some` function.

    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param sr The serializer to use.

    @param ec Set to indicate what error occurred, if any.

    @return The number of bytes written to the stream.

    @note The implementation will call @ref serializer::split with
    the value `true` on the serializer passed in.

    @see serializer
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
std::size_t
write_header(
    SyncWriteStream& stream,
    serializer<isRequest, Body, Fields>& sr,
    error_code& ec);

/** Write a header to a stream asynchronously using a serializer.

    This function is used to write a header to a stream asynchronously
    using a caller-provided HTTP/1 serializer. The function call always
    returns immediately. The asynchronous operation will continue until
    one of the following conditions is true:

    @li The function @ref serializer::is_header_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of zero or more calls to the stream's
    `async_write_some` function, and is known as a <em>composed operation</em>.
    The program must ensure that the stream performs no other writes
    until this operation completes.

    @param stream The stream to which the data is to be written.
    The type must support the <em>AsyncWriteStream</em> concept.

    @param sr The serializer to use.
    The object must remain valid at least until the
    handler is called; ownership is not transferred.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void handler(
        error_code const& error,        // result of operation
        std::size_t bytes_transferred   // the number of bytes written to the stream
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.

    @note The implementation will call @ref serializer::split with
    the value `true` on the serializer passed in.

    @see serializer
*/
template<
    class AsyncWriteStream,
    bool isRequest, class Body, class Fields,
    BOOST_BEAST_ASYNC_TPARAM2 WriteHandler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
async_write_header(
    AsyncWriteStream& stream,
    serializer<isRequest, Body, Fields>& sr,
    WriteHandler&& handler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>{});

//------------------------------------------------------------------------------

/** Write a complete message to a stream using a serializer.

    This function is used to write a complete message to a stream using
    a caller-provided HTTP/1 serializer. The call will block until one
    of the following conditions is true:

    @li The function @ref serializer::is_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of one or more calls
    to the stream's `write_some` function.

    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param sr The serializer to use.

    @return The number of bytes written to the stream.

    @throws system_error Thrown on failure.

    @see serializer
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
std::size_t
write(
    SyncWriteStream& stream,
    serializer<isRequest, Body, Fields>& sr);

/** Write a complete message to a stream using a serializer.

    This function is used to write a complete message to a stream using
    a caller-provided HTTP/1 serializer. The call will block until one
    of the following conditions is true:

    @li The function @ref serializer::is_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of one or more calls
    to the stream's `write_some` function.

    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param sr The serializer to use.

    @param ec Set to the error, if any occurred.

    @return The number of bytes written to the stream.

    @see serializer
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
std::size_t
write(
    SyncWriteStream& stream,
    serializer<isRequest, Body, Fields>& sr,
   error_code& ec);

/** Write a complete message to a stream asynchronously using a serializer.

    This function is used to write a complete message to a stream
    asynchronously using a caller-provided HTTP/1 serializer. The
    function call always returns immediately. The asynchronous
    operation will continue until one of the following conditions is true:

    @li The function @ref serializer::is_done returns `true`

    @li An error occurs.

    This operation is implemented in terms of zero or more calls to the stream's
    `async_write_some` function, and is known as a <em>composed operation</em>.
    The program must ensure that the stream performs no other writes
    until this operation completes.

    @param stream The stream to which the data is to be written.
    The type must support the <em>AsyncWriteStream</em> concept.

    @param sr The serializer to use.
    The object must remain valid at least until the
    handler is called; ownership is not transferred.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void handler(
        error_code const& error,        // result of operation
        std::size_t bytes_transferred   // the number of bytes written to the stream
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.

    @see serializer
*/
template<
    class AsyncWriteStream,
    bool isRequest, class Body, class Fields,
    BOOST_BEAST_ASYNC_TPARAM2 WriteHandler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
async_write(
    AsyncWriteStream& stream,
    serializer<isRequest, Body, Fields>& sr,
    WriteHandler&& handler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>{});

//------------------------------------------------------------------------------

/** Write a complete message to a stream.

    This function is used to write a complete message to a stream using
    HTTP/1. The call will block until one of the following conditions is true:

    @li The entire message is written.

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `write_some` function. The algorithm will use a temporary @ref serializer
    with an empty chunk decorator to produce buffers.

    @note This function only participates in overload resolution
    if @ref is_mutable_body_writer for <em>Body</em> returns `true`.

    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param msg The message to write.

    @return The number of bytes written to the stream.

    @throws system_error Thrown on failure.

    @see message
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
#if BOOST_BEAST_DOXYGEN
std::size_t
#else
typename std::enable_if<
    is_mutable_body_writer<Body>::value,
    std::size_t>::type
#endif
write(
    SyncWriteStream& stream,
    message<isRequest, Body, Fields>& msg);

/** Write a complete message to a stream.

    This function is used to write a complete message to a stream using
    HTTP/1. The call will block until one of the following conditions is true:

    @li The entire message is written.

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `write_some` function. The algorithm will use a temporary @ref serializer
    with an empty chunk decorator to produce buffers.

    @note This function only participates in overload resolution
    if @ref is_mutable_body_writer for <em>Body</em> returns `false`.

    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param msg The message to write.

    @return The number of bytes written to the stream.

    @throws system_error Thrown on failure.

    @see message
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
#if BOOST_BEAST_DOXYGEN
std::size_t
#else
typename std::enable_if<
    ! is_mutable_body_writer<Body>::value,
    std::size_t>::type
#endif
write(
    SyncWriteStream& stream,
    message<isRequest, Body, Fields> const& msg);

/** Write a complete message to a stream.

    This function is used to write a complete message to a stream using
    HTTP/1. The call will block until one of the following conditions is true:

    @li The entire message is written.

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `write_some` function. The algorithm will use a temporary @ref serializer
    with an empty chunk decorator to produce buffers.

    @note This function only participates in overload resolution
    if @ref is_mutable_body_writer for <em>Body</em> returns `true`.

    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param msg The message to write.

    @param ec Set to the error, if any occurred.

    @return The number of bytes written to the stream.

    @see message
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
#if BOOST_BEAST_DOXYGEN
std::size_t
#else
typename std::enable_if<
    is_mutable_body_writer<Body>::value,
    std::size_t>::type
#endif
write(
    SyncWriteStream& stream,
    message<isRequest, Body, Fields>& msg,
    error_code& ec);

/** Write a complete message to a stream.

    This function is used to write a complete message to a stream using
    HTTP/1. The call will block until one of the following conditions is true:

    @li The entire message is written.

    @li An error occurs.

    This operation is implemented in terms of one or more calls to the stream's
    `write_some` function. The algorithm will use a temporary @ref serializer
    with an empty chunk decorator to produce buffers.

    @note This function only participates in overload resolution
    if @ref is_mutable_body_writer for <em>Body</em> returns `false`.

    @param stream The stream to which the data is to be written.
    The type must support the <em>SyncWriteStream</em> concept.

    @param msg The message to write.

    @param ec Set to the error, if any occurred.

    @return The number of bytes written to the stream.

    @see message
*/
template<
    class SyncWriteStream,
    bool isRequest, class Body, class Fields>
#if BOOST_BEAST_DOXYGEN
std::size_t
#else
typename std::enable_if<
    ! is_mutable_body_writer<Body>::value,
    std::size_t>::type
#endif
write(
    SyncWriteStream& stream,
    message<isRequest, Body, Fields> const& msg,
    error_code& ec);

/** Write a complete message to a stream asynchronously.

    This function is used to write a complete message to a stream asynchronously
    using HTTP/1. The function call always returns immediately. The asynchronous
    operation will continue until one of the following conditions is true:

    @li The entire message is written.

    @li An error occurs.

    This operation is implemented in terms of zero or more calls to the stream's
    `async_write_some` function, and is known as a <em>composed operation</em>.
    The program must ensure that the stream performs no other writes
    until this operation completes. The algorithm will use a temporary
    @ref serializer with an empty chunk decorator to produce buffers.

    @note This function only participates in overload resolution
    if @ref is_mutable_body_writer for <em>Body</em> returns `true`.

    @param stream The stream to which the data is to be written.
    The type must support the <em>AsyncWriteStream</em> concept.

    @param msg The message to write.
    The object must remain valid at least until the
    handler is called; ownership is not transferred.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void handler(
        error_code const& error,        // result of operation
        std::size_t bytes_transferred   // the number of bytes written to the stream
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.

    @see message
*/
template<
    class AsyncWriteStream,
    bool isRequest, class Body, class Fields,
    BOOST_BEAST_ASYNC_TPARAM2 WriteHandler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
async_write(
    AsyncWriteStream& stream,
    message<isRequest, Body, Fields>& msg,
    WriteHandler&& handler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>{}
#ifndef BOOST_BEAST_DOXYGEN
    , typename std::enable_if<
        is_mutable_body_writer<Body>::value>::type* = 0
#endif
    );

/** Write a complete message to a stream asynchronously.

    This function is used to write a complete message to a stream asynchronously
    using HTTP/1. The function call always returns immediately. The asynchronous
    operation will continue until one of the following conditions is true:

    @li The entire message is written.

    @li An error occurs.

    This operation is implemented in terms of zero or more calls to the stream's
    `async_write_some` function, and is known as a <em>composed operation</em>.
    The program must ensure that the stream performs no other writes
    until this operation completes. The algorithm will use a temporary
    @ref serializer with an empty chunk decorator to produce buffers.

    @note This function only participates in overload resolution
    if @ref is_mutable_body_writer for <em>Body</em> returns `false`.

    @param stream The stream to which the data is to be written.
    The type must support the <em>AsyncWriteStream</em> concept.

    @param msg The message to write.
    The object must remain valid at least until the
    handler is called; ownership is not transferred.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void handler(
        error_code const& error,        // result of operation
        std::size_t bytes_transferred   // the number of bytes written to the stream
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.

    @see message
*/
template<
    class AsyncWriteStream,
    bool isRequest, class Body, class Fields,
    BOOST_BEAST_ASYNC_TPARAM2 WriteHandler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
async_write(
    AsyncWriteStream& stream,
    message<isRequest, Body, Fields> const& msg,
    WriteHandler&& handler =
        net::default_completion_token_t<
            executor_type<AsyncWriteStream>>{}
#ifndef BOOST_BEAST_DOXYGEN
    , typename std::enable_if<
        ! is_mutable_body_writer<Body>::value>::type* = 0
#endif
    );


//------------------------------------------------------------------------------

/** Serialize an HTTP/1 header to a `std::ostream`.

    The function converts the header to its HTTP/1 serialized
    representation and stores the result in the output stream.

    @param os The output stream to write to.

    @param msg The message fields to write.
*/
template<bool isRequest, class Fields>
std::ostream&
operator<<(std::ostream& os,
    header<isRequest, Fields> const& msg);

/** Serialize an HTTP/1 message to a `std::ostream`.

    The function converts the message to its HTTP/1 serialized
    representation and stores the result in the output stream.

    The implementation will automatically perform chunk encoding if
    the contents of the message indicate that chunk encoding is required.

    @param os The output stream to write to.

    @param msg The message to write.
*/
template<bool isRequest, class Body, class Fields>
std::ostream&
operator<<(std::ostream& os,
    message<isRequest, Body, Fields> const& msg);

} // http
} // beast
} // boost

#include <boost/beast/http/impl/write.hpp>

#endif
