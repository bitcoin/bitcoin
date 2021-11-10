//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_SSL_STREAM_HPP
#define BOOST_BEAST_CORE_SSL_STREAM_HPP

#include <boost/beast/core/detail/config.hpp>

// This include is necessary to work with `ssl::stream` and `boost::beast::websocket::stream`
#include <boost/beast/websocket/ssl.hpp>

#include <boost/beast/core/flat_stream.hpp>

// VFALCO We include this because anyone who uses ssl will
//        very likely need to check for ssl::error::stream_truncated
#include <boost/asio/ssl/error.hpp>

#include <boost/asio/ssl/stream.hpp>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {

/** Provides stream-oriented functionality using OpenSSL

    The stream class template provides asynchronous and blocking
    stream-oriented functionality using SSL.

    @par Thread Safety
    @e Distinct @e objects: Safe.@n
    @e Shared @e objects: Unsafe. The application must also ensure that all
    asynchronous operations are performed within the same implicit or explicit
    strand.

    @par Example
    To use this template with a @ref tcp_stream, you would write:
    @code
        net::io_context ioc;
        net::ssl::context ctx{net::ssl::context::tlsv12};
        beast::ssl_stream<beast::tcp_stream> sock{ioc, ctx};
    @endcode

    In addition to providing an interface identical to `net::ssl::stream`,
    the wrapper has the following additional properties:

    @li Satisfies @b MoveConstructible

    @li Satisfies @b MoveAssignable

    @li Constructible from a moved socket.

    @li Uses @ref flat_stream internally, as a performance work-around for a
        limitation of `net::ssl::stream` when writing buffer sequences
        having length greater than one.

    @par Concepts:
        @li AsyncReadStream
        @li AsyncWriteStream
        @li Stream
        @li SyncReadStream
        @li SyncWriteStream
*/
template<class NextLayer>
class ssl_stream
    : public net::ssl::stream_base
{
    using ssl_stream_type = net::ssl::stream<NextLayer>;
    using stream_type = boost::beast::flat_stream<ssl_stream_type>;

    std::unique_ptr<stream_type> p_;

public:
    /// The native handle type of the SSL stream.
    using native_handle_type =
        typename ssl_stream_type::native_handle_type;

    /// Structure for use with deprecated impl_type.
    using impl_struct = typename ssl_stream_type::impl_struct;

    /// The type of the next layer.
    using next_layer_type = typename ssl_stream_type::next_layer_type;

    /// The type of the executor associated with the object.
    using executor_type = typename stream_type::executor_type;

    /** Construct a stream.

        This constructor creates a stream and initialises the underlying stream
        object.

        @param arg The argument to be passed to initialise the underlying stream.

        @param ctx The SSL context to be used for the stream.
    */
    template<class Arg>
    ssl_stream(
        Arg&& arg,
        net::ssl::context& ctx)
        : p_(new stream_type{
            std::forward<Arg>(arg), ctx})
    {
    }

    /** Get the executor associated with the object.

        This function may be used to obtain the executor object that the stream
        uses to dispatch handlers for asynchronous operations.

        @return A copy of the executor that stream will use to dispatch handlers.
    */
    executor_type
    get_executor() noexcept
    {
        return p_->get_executor();
    }

    /** Get the underlying implementation in the native type.

        This function may be used to obtain the underlying implementation of the
        context. This is intended to allow access to context functionality that is
        not otherwise provided.

        @par Example
        The native_handle() function returns a pointer of type @c SSL* that is
        suitable for passing to functions such as @c SSL_get_verify_result and
        @c SSL_get_peer_certificate:
        @code
        boost::beast::ssl_stream<net::ip::tcp::socket> ss{ioc, ctx};

        // ... establish connection and perform handshake ...

        if (X509* cert = SSL_get_peer_certificate(ss.native_handle()))
        {
          if (SSL_get_verify_result(ss.native_handle()) == X509_V_OK)
          {
            // ...
          }
        }
        @endcode
    */
    native_handle_type
    native_handle() noexcept
    {
        return p_->next_layer().native_handle();
    }

    /** Get a reference to the next layer.

        This function returns a reference to the next layer in a stack of stream
        layers.

        @note The next layer is the wrapped stream and not the @ref flat_stream
        used in the implementation.

        @return A reference to the next layer in the stack of stream layers.
        Ownership is not transferred to the caller.
    */
    next_layer_type const&
    next_layer() const noexcept
    {
        return p_->next_layer().next_layer();
    }

    /** Get a reference to the next layer.

        This function returns a reference to the next layer in a stack of stream
        layers.

        @note The next layer is the wrapped stream and not the @ref flat_stream
        used in the implementation.

        @return A reference to the next layer in the stack of stream layers.
        Ownership is not transferred to the caller.
    */
    next_layer_type&
    next_layer() noexcept
    {
        return p_->next_layer().next_layer();
    }

    /** Set the peer verification mode.

        This function may be used to configure the peer verification mode used by
        the stream. The new mode will override the mode inherited from the context.

        @param v A bitmask of peer verification modes.

        @throws boost::system::system_error Thrown on failure.

        @note Calls @c SSL_set_verify.
    */
    void
    set_verify_mode(net::ssl::verify_mode v)
    {
        p_->next_layer().set_verify_mode(v);
    }

    /** Set the peer verification mode.

        This function may be used to configure the peer verification mode used by
        the stream. The new mode will override the mode inherited from the context.

        @param v A bitmask of peer verification modes. See `verify_mode` for
        available values.

        @param ec Set to indicate what error occurred, if any.

        @note Calls @c SSL_set_verify.
    */
    void
    set_verify_mode(net::ssl::verify_mode v,
        boost::system::error_code& ec)
    {
        p_->next_layer().set_verify_mode(v, ec);
    }

    /** Set the peer verification depth.

        This function may be used to configure the maximum verification depth
        allowed by the stream.

        @param depth Maximum depth for the certificate chain verification that
        shall be allowed.

        @throws boost::system::system_error Thrown on failure.

        @note Calls @c SSL_set_verify_depth.
    */
    void
    set_verify_depth(int depth)
    {
        p_->next_layer().set_verify_depth(depth);
    }

    /** Set the peer verification depth.

        This function may be used to configure the maximum verification depth
        allowed by the stream.

        @param depth Maximum depth for the certificate chain verification that
        shall be allowed.

        @param ec Set to indicate what error occurred, if any.

        @note Calls @c SSL_set_verify_depth.
    */
    void
    set_verify_depth(
        int depth, boost::system::error_code& ec)
    {
        p_->next_layer().set_verify_depth(depth, ec);
    }

    /** Set the callback used to verify peer certificates.

        This function is used to specify a callback function that will be called
        by the implementation when it needs to verify a peer certificate.

        @param callback The function object to be used for verifying a certificate.
        The function signature of the handler must be:
        @code bool verify_callback(
          bool preverified, // True if the certificate passed pre-verification.
          verify_context& ctx // The peer certificate and other context.
        ); @endcode
        The return value of the callback is true if the certificate has passed
        verification, false otherwise.

        @throws boost::system::system_error Thrown on failure.

        @note Calls @c SSL_set_verify.
    */
    template<class VerifyCallback>
    void
    set_verify_callback(VerifyCallback callback)
    {
        p_->next_layer().set_verify_callback(callback);
    }

    /** Set the callback used to verify peer certificates.

        This function is used to specify a callback function that will be called
        by the implementation when it needs to verify a peer certificate.

        @param callback The function object to be used for verifying a certificate.
        The function signature of the handler must be:
        @code bool verify_callback(
          bool preverified, // True if the certificate passed pre-verification.
          net::verify_context& ctx // The peer certificate and other context.
        ); @endcode
        The return value of the callback is true if the certificate has passed
        verification, false otherwise.

        @param ec Set to indicate what error occurred, if any.

        @note Calls @c SSL_set_verify.
    */
    template<class VerifyCallback>
    void
    set_verify_callback(VerifyCallback callback,
        boost::system::error_code& ec)
    {
        p_->next_layer().set_verify_callback(callback, ec);
    }

    /** Perform SSL handshaking.

        This function is used to perform SSL handshaking on the stream. The
        function call will block until handshaking is complete or an error occurs.

        @param type The type of handshaking to be performed, i.e. as a client or as
        a server.

        @throws boost::system::system_error Thrown on failure.
    */
    void
    handshake(handshake_type type)
    {
        p_->next_layer().handshake(type);
    }

    /** Perform SSL handshaking.

        This function is used to perform SSL handshaking on the stream. The
        function call will block until handshaking is complete or an error occurs.

        @param type The type of handshaking to be performed, i.e. as a client or as
        a server.

        @param ec Set to indicate what error occurred, if any.
    */
    void
    handshake(handshake_type type,
        boost::system::error_code& ec)
    {
        p_->next_layer().handshake(type, ec);
    }

    /** Perform SSL handshaking.

        This function is used to perform SSL handshaking on the stream. The
        function call will block until handshaking is complete or an error occurs.

        @param type The type of handshaking to be performed, i.e. as a client or as
        a server.

        @param buffers The buffered data to be reused for the handshake.

        @throws boost::system::system_error Thrown on failure.
    */
    template<class ConstBufferSequence>
    void
    handshake(
        handshake_type type, ConstBufferSequence const& buffers)
    {
        p_->next_layer().handshake(type, buffers);
    }

    /** Perform SSL handshaking.

        This function is used to perform SSL handshaking on the stream. The
        function call will block until handshaking is complete or an error occurs.

        @param type The type of handshaking to be performed, i.e. as a client or as
        a server.

        @param buffers The buffered data to be reused for the handshake.

        @param ec Set to indicate what error occurred, if any.
    */
    template<class ConstBufferSequence>
    void
    handshake(handshake_type type,
        ConstBufferSequence const& buffers,
            boost::system::error_code& ec)
    {
        p_->next_layer().handshake(type, buffers, ec);
    }

    /** Start an asynchronous SSL handshake.

        This function is used to asynchronously perform an SSL handshake on the
        stream. This function call always returns immediately.

        @param type The type of handshaking to be performed, i.e. as a client or as
        a server.

        @param handler The handler to be called when the handshake operation
        completes. Copies will be made of the handler as required. The equivalent
        function signature of the handler must be:
        @code void handler(
          const boost::system::error_code& error // Result of operation.
        ); @endcode
    */
    template<class HandshakeHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(HandshakeHandler, void(boost::system::error_code))
    async_handshake(handshake_type type,
        BOOST_ASIO_MOVE_ARG(HandshakeHandler) handler)
    {
        return p_->next_layer().async_handshake(type,
            BOOST_ASIO_MOVE_CAST(HandshakeHandler)(handler));
    }

    /** Start an asynchronous SSL handshake.

        This function is used to asynchronously perform an SSL handshake on the
        stream. This function call always returns immediately.

        @param type The type of handshaking to be performed, i.e. as a client or as
        a server.

        @param buffers The buffered data to be reused for the handshake. Although
        the buffers object may be copied as necessary, ownership of the underlying
        buffers is retained by the caller, which must guarantee that they remain
        valid until the handler is called.

        @param handler The handler to be called when the handshake operation
        completes. Copies will be made of the handler as required. The equivalent
        function signature of the handler must be:
        @code void handler(
          const boost::system::error_code& error, // Result of operation.
          std::size_t bytes_transferred // Amount of buffers used in handshake.
        ); @endcode
    */
    template<class ConstBufferSequence, class BufferedHandshakeHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(BufferedHandshakeHandler, void(boost::system::error_code, std::size_t))
    async_handshake(handshake_type type, ConstBufferSequence const& buffers,
        BOOST_ASIO_MOVE_ARG(BufferedHandshakeHandler) handler)
    {
        return p_->next_layer().async_handshake(type, buffers,
            BOOST_ASIO_MOVE_CAST(BufferedHandshakeHandler)(handler));
    }

    /** Shut down SSL on the stream.

        This function is used to shut down SSL on the stream. The function call
        will block until SSL has been shut down or an error occurs.

        @throws boost::system::system_error Thrown on failure.
    */
    void
    shutdown()
    {
        p_->next_layer().shutdown();
    }

    /** Shut down SSL on the stream.

        This function is used to shut down SSL on the stream. The function call
        will block until SSL has been shut down or an error occurs.

        @param ec Set to indicate what error occurred, if any.
    */
    void
    shutdown(boost::system::error_code& ec)
    {
        p_->next_layer().shutdown(ec);
    }

    /** Asynchronously shut down SSL on the stream.

        This function is used to asynchronously shut down SSL on the stream. This
        function call always returns immediately.

        @param handler The handler to be called when the handshake operation
        completes. Copies will be made of the handler as required. The equivalent
        function signature of the handler must be:
        @code void handler(
          const boost::system::error_code& error // Result of operation.
        ); @endcode
    */
    template<class ShutdownHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ShutdownHandler, void(boost::system::error_code))
    async_shutdown(BOOST_ASIO_MOVE_ARG(ShutdownHandler) handler)
    {
        return p_->next_layer().async_shutdown(
            BOOST_ASIO_MOVE_CAST(ShutdownHandler)(handler));
    }

    /** Write some data to the stream.

        This function is used to write data on the stream. The function call will
        block until one or more bytes of data has been written successfully, or
        until an error occurs.

        @param buffers The data to be written.

        @returns The number of bytes written.

        @throws boost::system::system_error Thrown on failure.

        @note The `write_some` operation may not transmit all of the data to the
        peer. Consider using the `net::write` function if you need to
        ensure that all data is written before the blocking operation completes.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers)
    {
        return p_->write_some(buffers);
    }

    /** Write some data to the stream.

        This function is used to write data on the stream. The function call will
        block until one or more bytes of data has been written successfully, or
        until an error occurs.

        @param buffers The data to be written to the stream.

        @param ec Set to indicate what error occurred, if any.

        @returns The number of bytes written. Returns 0 if an error occurred.

        @note The `write_some` operation may not transmit all of the data to the
        peer. Consider using the `net::write` function if you need to
        ensure that all data is written before the blocking operation completes.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers,
        boost::system::error_code& ec)
    {
        return p_->write_some(buffers, ec);
    }

    /** Start an asynchronous write.

        This function is used to asynchronously write one or more bytes of data to
        the stream. The function call always returns immediately.

        @param buffers The data to be written to the stream. Although the buffers
        object may be copied as necessary, ownership of the underlying buffers is
        retained by the caller, which must guarantee that they remain valid until
        the handler is called.

        @param handler The handler to be called when the write operation completes.
        Copies will be made of the handler as required. The equivalent function
        signature of the handler must be:
        @code void handler(
          const boost::system::error_code& error, // Result of operation.
          std::size_t bytes_transferred           // Number of bytes written.
        ); @endcode

        @note The `async_write_some` operation may not transmit all of the data to
        the peer. Consider using the `net::async_write` function if you
        need to ensure that all data is written before the asynchronous operation
        completes.
    */
    template<class ConstBufferSequence, BOOST_BEAST_ASYNC_TPARAM2 WriteHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void(boost::system::error_code, std::size_t))
    async_write_some(ConstBufferSequence const& buffers,
        BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
    {
        return p_->async_write_some(buffers,
            BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
    }

    /** Read some data from the stream.

        This function is used to read data from the stream. The function call will
        block until one or more bytes of data has been read successfully, or until
        an error occurs.

        @param buffers The buffers into which the data will be read.

        @returns The number of bytes read.

        @throws boost::system::system_error Thrown on failure.

        @note The `read_some` operation may not read all of the requested number of
        bytes. Consider using the `net::read` function if you need to ensure
        that the requested amount of data is read before the blocking operation
        completes.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers)
    {
        return p_->read_some(buffers);
    }

    /** Read some data from the stream.

        This function is used to read data from the stream. The function call will
        block until one or more bytes of data has been read successfully, or until
        an error occurs.

        @param buffers The buffers into which the data will be read.

        @param ec Set to indicate what error occurred, if any.

        @returns The number of bytes read. Returns 0 if an error occurred.

        @note The `read_some` operation may not read all of the requested number of
        bytes. Consider using the `net::read` function if you need to ensure
        that the requested amount of data is read before the blocking operation
        completes.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers,
        boost::system::error_code& ec)
    {
        return p_->read_some(buffers, ec);
    }

    /** Start an asynchronous read.

        This function is used to asynchronously read one or more bytes of data from
        the stream. The function call always returns immediately.

        @param buffers The buffers into which the data will be read. Although the
        buffers object may be copied as necessary, ownership of the underlying
        buffers is retained by the caller, which must guarantee that they remain
        valid until the handler is called.

        @param handler The handler to be called when the read operation completes.
        Copies will be made of the handler as required. The equivalent function
        signature of the handler must be:
        @code void handler(
          const boost::system::error_code& error, // Result of operation.
          std::size_t bytes_transferred           // Number of bytes read.
        ); @endcode

        @note The `async_read_some` operation may not read all of the requested
        number of bytes. Consider using the `net::async_read` function
        if you need to ensure that the requested amount of data is read before
        the asynchronous operation completes.
    */
    template<class MutableBufferSequence, BOOST_BEAST_ASYNC_TPARAM2 ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(boost::system::error_code, std::size_t))
    async_read_some(MutableBufferSequence const& buffers,
        BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
    {
        return p_->async_read_some(buffers,
            BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));
    }

    // These hooks are used to inform boost::beast::websocket::stream on
    // how to tear down the connection as part of the WebSocket
    // protocol specifications
    #if ! BOOST_BEAST_DOXYGEN
    template<class SyncStream>
    friend
    void
    teardown(
        boost::beast::role_type role,
        ssl_stream<SyncStream>& stream,
        boost::system::error_code& ec);

    template<class AsyncStream, class TeardownHandler>
    friend
    void
    async_teardown(
        boost::beast::role_type role,
        ssl_stream<AsyncStream>& stream,
        TeardownHandler&& handler);
    #endif
};

#if ! BOOST_BEAST_DOXYGEN
template<class SyncStream>
void
teardown(
    boost::beast::role_type role,
    ssl_stream<SyncStream>& stream,
    boost::system::error_code& ec)
{
    // Just forward it to the underlying ssl::stream
    using boost::beast::websocket::teardown;
    teardown(role, *stream.p_, ec);
}

template<class AsyncStream, class TeardownHandler>
void
async_teardown(
    boost::beast::role_type role,
    ssl_stream<AsyncStream>& stream,
    TeardownHandler&& handler)
{
    // Just forward it to the underlying ssl::stream
    using boost::beast::websocket::async_teardown;
    async_teardown(role, *stream.p_,
        std::forward<TeardownHandler>(handler));
}
#endif

} // beast
} // boost

#endif
