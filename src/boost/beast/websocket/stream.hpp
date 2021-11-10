//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_STREAM_HPP
#define BOOST_BEAST_WEBSOCKET_STREAM_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/beast/websocket/option.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream_base.hpp>
#include <boost/beast/websocket/stream_fwd.hpp>
#include <boost/beast/websocket/detail/hybi13.hpp>
#include <boost/beast/websocket/detail/impl_base.hpp>
#include <boost/beast/websocket/detail/pmd_extension.hpp>
#include <boost/beast/websocket/detail/prng.hpp>
#include <boost/beast/core/role.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/http/detail/type_traits.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/error.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <type_traits>
#include <random>

namespace boost {
namespace beast {
namespace websocket {

/** The type of received control frame.

    Values of this type are passed to the control frame
    callback set using @ref stream::control_callback.
*/
enum class frame_type
{
    /// A close frame was received
    close,

    /// A ping frame was received
    ping,

    /// A pong frame was received
    pong
};

namespace detail {
class frame_test;
} // detail

//--------------------------------------------------------------------

/** Provides message-oriented functionality using WebSocket.

    The @ref stream class template provides asynchronous and blocking
    message-oriented functionality necessary for clients and servers
    to utilize the WebSocket protocol.

    For asynchronous operations, the application must ensure
    that they are are all performed within the same implicit
    or explicit strand.

    @par Thread Safety
    @e Distinct @e objects: Safe.@n
    @e Shared @e objects: Unsafe.
    The application must also ensure that all asynchronous
    operations are performed within the same implicit or explicit strand.

    @par Example
    To declare the @ref stream object with a @ref tcp_stream in a
    multi-threaded asynchronous program using a strand, you may write:
    @code
    websocket::stream<tcp_stream> ws{net::make_strand(ioc)};
    @endcode
    Alternatively, for a single-threaded or synchronous application
    you may write:
    @code
    websocket::stream<tcp_stream> ws(ioc);
    @endcode

    @tparam NextLayer The type representing the next layer, to which
    data will be read and written during operations. For synchronous
    operations, the type must support the <em>SyncStream</em> concept.
    For asynchronous operations, the type must support the
    <em>AsyncStream</em> concept.

    @tparam deflateSupported A `bool` indicating whether or not the
    stream will be capable of negotiating the permessage-deflate websocket
    extension. Note that even if this is set to `true`, the permessage
    deflate options (set by the caller at runtime) must still have the
    feature enabled for a successful negotiation to occur.

    @note A stream object must not be moved or destroyed while there
    are pending asynchronous operations associated with it.

    @par Concepts
        @li <em>AsyncStream</em>
        @li <em>DynamicBuffer</em>
        @li <em>SyncStream</em>

    @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.1">Websocket Opening Handshake Client Requirements (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc6455#section-7.1.2">Websocket Closing Handshake (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc6455#section-5.5.1">Websocket Close (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc6455#section-5.5.2">WebSocket Ping (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc6455#section-5.5.3">WebSocket Pong (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.4">Host field (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-3.1.1">request-target (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.3.1">origin-form (RFC7230)</a>
*/
template<
    class NextLayer,
    bool deflateSupported>
class stream
#if ! BOOST_BEAST_DOXYGEN
    : private stream_base
#endif
{
    struct impl_type;

    boost::shared_ptr<impl_type> impl_;

    using time_point = typename
        std::chrono::steady_clock::time_point;

    using control_cb_type =
        std::function<void(frame_type, string_view)>;

    friend class close_test;
    friend class frame_test;
    friend class ping_test;
    friend class read2_test;
    friend class read3_test;
    friend class stream_test;
    friend class write_test;

    /*  The read buffer has to be at least as large
        as the largest possible control frame including
        the frame header.
    */
    static std::size_t constexpr max_control_frame_size = 2 + 8 + 4 + 125;
    static std::size_t constexpr tcp_frame_size = 1536;

    static time_point never() noexcept
    {
        return (time_point::max)();
    }

public:
    /// Indicates if the permessage-deflate extension is supported
    using is_deflate_supported =
        std::integral_constant<bool, deflateSupported>;

    /// The type of the next layer.
    using next_layer_type =
        typename std::remove_reference<NextLayer>::type;

    /// The type of the executor associated with the object.
    using executor_type =
        beast::executor_type<next_layer_type>;

    /** Destructor

        Destroys the stream and all associated resources.

        @note A stream object must not be destroyed while there
        are pending asynchronous operations associated with it.
    */
    ~stream();

    /** Constructor

        If `NextLayer` is move constructible, this function
        will move-construct a new stream from the existing stream.

        After the move, the only valid operation on the moved-from
        object is destruction.
    */
    stream(stream&&) = default;

    /// Move assignment (deleted)
    stream& operator=(stream&&) = delete;

    /** Constructor

        This constructor creates a websocket stream and initializes
        the next layer object.

        @throws Any exceptions thrown by the NextLayer constructor.

        @param args The arguments to be passed to initialize the
        next layer object. The arguments are forwarded to the next
        layer's constructor.
    */
    template<class... Args>
    explicit
    stream(Args&&... args);

    //--------------------------------------------------------------------------

    /** Get the executor associated with the object.

        This function may be used to obtain the executor object that the
        stream uses to dispatch handlers for asynchronous operations.

        @return A copy of the executor that stream will use to dispatch handlers.
    */
    executor_type
    get_executor() noexcept;

    /** Get a reference to the next layer

        This function returns a reference to the next layer
        in a stack of stream layers.

        @return A reference to the next layer in the stack of
        stream layers.
    */
    next_layer_type&
    next_layer() noexcept;

    /** Get a reference to the next layer

        This function returns a reference to the next layer in a
        stack of stream layers.

        @return A reference to the next layer in the stack of
        stream layers.
    */
    next_layer_type const&
    next_layer() const noexcept;

    //--------------------------------------------------------------------------
    //
    // Observers
    //
    //--------------------------------------------------------------------------

    /** Returns `true` if the stream is open.

        The stream is open after a successful handshake, and when
        no error has occurred.
    */
    bool
    is_open() const noexcept;

    /** Returns `true` if the latest message data indicates binary.

        This function informs the caller of whether the last
        received message frame represents a message with the
        binary opcode.

        If there is no last message frame, the return value is
        undefined.
    */
    bool
    got_binary() const noexcept;

    /** Returns `true` if the latest message data indicates text.

        This function informs the caller of whether the last
        received message frame represents a message with the
        text opcode.

        If there is no last message frame, the return value is
        undefined.
    */
    bool
    got_text() const
    {
        return ! got_binary();
    }

    /// Returns `true` if the last completed read finished the current message.
    bool
    is_message_done() const noexcept;

    /** Returns the close reason received from the remote peer.

        This is only valid after a read completes with error::closed.
    */
    close_reason const&
    reason() const noexcept;

    /** Returns a suggested maximum buffer size for the next call to read.

        This function returns a reasonable upper limit on the number
        of bytes for the size of the buffer passed in the next call
        to read. The number is determined by the state of the current
        frame and whether or not the permessage-deflate extension is
        enabled.

        @param initial_size A non-zero size representing the caller's
        desired buffer size for when there is no information which may
        be used to calculate a more specific value. For example, when
        reading the first frame header of a message.
    */
    std::size_t
    read_size_hint(
        std::size_t initial_size = +tcp_frame_size) const;

    /** Returns a suggested maximum buffer size for the next call to read.

        This function returns a reasonable upper limit on the number
        of bytes for the size of the buffer passed in the next call
        to read. The number is determined by the state of the current
        frame and whether or not the permessage-deflate extension is
        enabled.

        @param buffer The buffer which will be used for reading. The
        implementation will query the buffer to obtain the optimum
        size of a subsequent call to `buffer.prepare` based on the
        state of the current frame, if any.
    */
    template<class DynamicBuffer
#if ! BOOST_BEAST_DOXYGEN
        , class = typename std::enable_if<
            ! std::is_integral<DynamicBuffer>::value>::type
#endif
    >
    std::size_t
    read_size_hint(
        DynamicBuffer& buffer) const;

    //--------------------------------------------------------------------------
    //
    // Settings
    //
    //--------------------------------------------------------------------------

#if BOOST_BEAST_DOXYGEN
    template<class Option>
    void
    get_option(Option& opt);

    template<class Option>
    void
    set_option(Option opt);
#else

    void set_option(decorator opt);

    void get_option(timeout& opt);
    void set_option(timeout const& opt);
#endif

    /** Set the permessage-deflate extension options

        @throws invalid_argument if `deflateSupported == false`, and either
        `client_enable` or `server_enable` is `true`.
    */
    void
    set_option(permessage_deflate const& o);

    /// Get the permessage-deflate extension options
    void
    get_option(permessage_deflate& o);

    /** Set the automatic fragmentation option.

        Determines if outgoing message payloads are broken up into
        multiple pieces.

        When the automatic fragmentation size is turned on, outgoing
        message payloads are broken up into multiple frames no larger
        than the write buffer size.

        The default setting is to fragment messages.

        @param value A `bool` indicating if auto fragmentation should be on.

        @par Example
        Setting the automatic fragmentation option:
        @code
            ws.auto_fragment(true);
        @endcode
    */
    void
    auto_fragment(bool value);

    /// Returns `true` if the automatic fragmentation option is set.
    bool
    auto_fragment() const;

    /** Set the binary message write option.

        This controls whether or not outgoing message opcodes
        are set to binary or text. The setting is only applied
        at the start when a caller begins a new message. Changing
        the opcode after a message is started will only take effect
        after the current message being sent is complete.

        The default setting is to send text messages.

        @param value `true` if outgoing messages should indicate
        binary, or `false` if they should indicate text.

        @par Example
        Setting the message type to binary.
        @code
            ws.binary(true);
        @endcode
        */
    void
    binary(bool value);

    /// Returns `true` if the binary message write option is set.
    bool
    binary() const;

    /** Set a callback to be invoked on each incoming control frame.

        Sets the callback to be invoked whenever a ping, pong,
        or close control frame is received during a call to one
        of the following functions:

        @li @ref beast::websocket::stream::read
        @li @ref beast::websocket::stream::read_some
        @li @ref beast::websocket::stream::async_read
        @li @ref beast::websocket::stream::async_read_some

        Unlike completion handlers, the callback will be invoked
        for each control frame during a call to any synchronous
        or asynchronous read function. The operation is passive,
        with no associated error code, and triggered by reads.

        For close frames, the close reason code may be obtained by
        calling the function @ref reason.

        @param cb The function object to call, which must be
        invocable with this equivalent signature:
        @code
        void
        callback(
            frame_type kind,       // The type of frame
            string_view payload    // The payload in the frame
        );
        @endcode
        The implementation type-erases the callback which may require
        a dynamic allocation. To prevent the possibility of a dynamic
        allocation, use `std::ref` to wrap the callback.
        If the read operation which receives the control frame is
        an asynchronous operation, the callback will be invoked using
        the same method as that used to invoke the final handler.

        @note Incoming ping and close frames are automatically
        handled. Pings are responded to with pongs, and a close frame
        is responded to with a close frame leading to the closure of
        the stream. It is not necessary to manually send pings, pongs,
        or close frames from inside the control callback.
        Attempting to manually send a close frame from inside the
        control callback after receiving a close frame will result
        in undefined behavior.
    */
    void
    control_callback(std::function<void(frame_type, string_view)> cb);

    /** Reset the control frame callback.

        This function removes any previously set control frame callback.
    */
    void
    control_callback();

    /** Set the maximum incoming message size option.

        Sets the largest permissible incoming message size. Message
        frame fields indicating a size that would bring the total
        message size over this limit will cause a protocol failure.

        The default setting is 16 megabytes. A value of zero indicates
        a limit of the maximum value of a `std::uint64_t`.

        @par Example
        Setting the maximum read message size.
        @code
            ws.read_message_max(65536);
        @endcode

        @param amount The limit on the size of incoming messages.
    */
    void
    read_message_max(std::size_t amount);

    /// Returns the maximum incoming message size setting.
    std::size_t
    read_message_max() const;

    /** Set whether the PRNG is cryptographically secure

        This controls whether or not the source of pseudo-random
        numbers used to produce the masks required by the WebSocket
        protocol are of cryptographic quality. When the setting is
        `true`, a strong algorithm is used which cannot be guessed
        by observing outputs. When the setting is `false`, a much
        faster algorithm is used.
        Masking is only performed by streams operating in the client
        mode. For streams operating in the server mode, this setting
        has no effect.
        By default, newly constructed streams use a secure PRNG.

        If the WebSocket stream is used with an encrypted SSL or TLS
        next layer, if it is known to the application that intermediate
        proxies are not vulnerable to cache poisoning, or if the
        application is designed such that an attacker cannot send
        arbitrary inputs to the stream interface, then the faster
        algorithm may be used.

        For more information please consult the WebSocket protocol RFC.

        @param value `true` if the PRNG algorithm should be
        cryptographically secure.
    */
    void
    secure_prng(bool value);

    /** Set the write buffer size option.

        Sets the size of the write buffer used by the implementation to
        send frames. The write buffer is needed when masking payload data
        in the client role, compressing frames, or auto-fragmenting message
        data.

        Lowering the size of the buffer can decrease the memory requirements
        for each connection, while increasing the size of the buffer can reduce
        the number of calls made to the next layer to write data.

        The default setting is 4096. The minimum value is 8.

        The write buffer size can only be changed when the stream is not
        open. Undefined behavior results if the option is modified after a
        successful WebSocket handshake.

        @par Example
        Setting the write buffer size.
        @code
            ws.write_buffer_bytes(8192);
        @endcode

        @param amount The size of the write buffer in bytes.
    */
    void
    write_buffer_bytes(std::size_t amount);

    /// Returns the size of the write buffer.
    std::size_t
    write_buffer_bytes() const;

    /** Set the text message write option.

        This controls whether or not outgoing message opcodes
        are set to binary or text. The setting is only applied
        at the start when a caller begins a new message. Changing
        the opcode after a message is started will only take effect
        after the current message being sent is complete.

        The default setting is to send text messages.

        @param value `true` if outgoing messages should indicate
        text, or `false` if they should indicate binary.

        @par Example
        Setting the message type to text.
        @code
            ws.text(true);
        @endcode
    */
    void
    text(bool value);

    /// Returns `true` if the text message write option is set.
    bool
    text() const;

    /*
        timer settings

        * Timer is disabled
        * Close on timeout
            - no complete frame received, OR
            - no complete frame sent
        * Ping on timeout
            - ping on no complete frame received
                * if can't ping?
    */

    //--------------------------------------------------------------------------
    //
    // Handshaking (Client)
    //
    //--------------------------------------------------------------------------

    /** Perform the WebSocket handshake in the client role.

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The request is sent and the response is received.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        The handshake is successful if the received HTTP response
        indicates the upgrade was accepted by the server, represented by a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols.

        @param host The name of the remote host. This is required by
        the HTTP protocol to set the "Host" header field.

        @param target The request-target, in origin-form. The server may use the
        target to distinguish different services on the same listening port.

        @throws system_error Thrown on failure.

        @par Example
        @code
        ws.handshake("localhost", "/");
        @endcode

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.1">Websocket Opening Handshake Client Requirements (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.4">Host field (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-3.1.1">request-target (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.3.1">origin-form (RFC7230)</a>
    */
    void
    handshake(
        string_view host,
        string_view target);

    /** Perform the WebSocket handshake in the client role.

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The request is sent and the response is received.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        The handshake is successful if the received HTTP response
        indicates the upgrade was accepted by the server, represented by a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols.

        @param res The HTTP Upgrade response returned by the remote
        endpoint. The caller may use the response to access any
        additional information sent by the server.

        @param host The name of the remote host. This is required by
        the HTTP protocol to set the "Host" header field.

        @param target The request-target, in origin-form. The server may use the
        target to distinguish different services on the same listening port.

        @throws system_error Thrown on failure.

        @par Example
        @code
        response_type res;
        ws.handshake(res, "localhost", "/");
        std::cout << res;
        @endcode

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.1">Websocket Opening Handshake Client Requirements (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.4">Host field (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-3.1.1">request-target (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.3.1">origin-form (RFC7230)</a>
    */
    void
    handshake(
        response_type& res,
        string_view host,
        string_view target);

    /** Perform the WebSocket handshake in the client role.

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The request is sent and the response is received.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        The handshake is successful if the received HTTP response
        indicates the upgrade was accepted by the server, represented by a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols.

        @param host The name of the remote host. This is required by
        the HTTP protocol to set the "Host" header field.

        @param target The request-target, in origin-form. The server may use the
        target to distinguish different services on the same listening port.

        @param ec Set to indicate what error occurred, if any.

        @par Example
        @code
        error_code ec;
        ws.handshake("localhost", "/", ec);
        @endcode

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.1">Websocket Opening Handshake Client Requirements (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.4">Host field (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-3.1.1">request-target (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.3.1">origin-form (RFC7230)</a>
    */
    void
    handshake(
        string_view host,
        string_view target,
        error_code& ec);

    /** Perform the WebSocket handshake in the client role.

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The request is sent and the response is received.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        The handshake is successful if the received HTTP response
        indicates the upgrade was accepted by the server, represented by a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols.

        @param res The HTTP Upgrade response returned by the remote
        endpoint. The caller may use the response to access any
        additional information sent by the server.

        @param host The name of the remote host. This is required by
        the HTTP protocol to set the "Host" header field.

        @param target The request-target, in origin-form. The server may use the
        target to distinguish different services on the same listening port.

        @param ec Set to indicate what error occurred, if any.

        @par Example
        @code
        error_code ec;
        response_type res;
        ws.handshake(res, "localhost", "/", ec);
        if(! ec)
            std::cout << res;
        @endcode

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.1">Websocket Opening Handshake Client Requirements (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.4">Host field (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-3.1.1">request-target (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.3.1">origin-form (RFC7230)</a>
    */
    void
    handshake(
        response_type& res,
        string_view host,
        string_view target,
        error_code& ec);

    /** Perform the WebSocket handshake asynchronously in the client role.

        This initiating function is used to asynchronously begin performing the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li The request is sent and the response is received.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_read_some`
        and `async_write_some` functions. No other operation may be performed
        on the stream until this operation completes.

        The handshake is successful if the received HTTP response
        indicates the upgrade was accepted by the server, represented by a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols.

        @param host The name of the remote host. This is required by
        the HTTP protocol to set the "Host" header field.
        The implementation will not access the string data after the
        initiating function returns.

        @param target The request-target, in origin-form. The server may use the
        target to distinguish different services on the same listening port.
        The implementation will not access the string data after the
        initiating function returns.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec    // Result of operation
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @par Example
        @code
        ws.async_handshake("localhost", "/",
            [](error_code ec)
            {
                if(ec)
                    std::cerr << "Error: " << ec.message() << "\n";
            });
        @endcode

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.1">Websocket Opening Handshake Client Requirements (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.4">Host field (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-3.1.1">request-target (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.3.1">origin-form (RFC7230)</a>
    */
    template<
        BOOST_BEAST_ASYNC_TPARAM1 HandshakeHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT1(HandshakeHandler)
    async_handshake(
        string_view host,
        string_view target,
        HandshakeHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    /** Perform the WebSocket handshake asynchronously in the client role.

        This initiating function is used to asynchronously begin performing the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li The request is sent and the response is received.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_read_some`
        and `async_write_some` functions. No other operation may be performed
        on the stream until this operation completes.

        The handshake is successful if the received HTTP response
        indicates the upgrade was accepted by the server, represented by a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols.

        @param res The HTTP Upgrade response returned by the remote
        endpoint. The caller may use the response to access any
        additional information sent by the server. This object will
        be assigned before the completion handler is invoked.

        @param host The name of the remote host. This is required by
        the HTTP protocol to set the "Host" header field.
        The implementation will not access the string data after the
        initiating function returns.

        @param target The request-target, in origin-form. The server may use the
        target to distinguish different services on the same listening port.
        The implementation will not access the string data after the
        initiating function returns.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec    // Result of operation
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @par Example
        @code
        response_type res;
        ws.async_handshake(res, "localhost", "/",
            [&res](error_code ec)
            {
                if(ec)
                    std::cerr << "Error: " << ec.message() << "\n";
                else
                    std::cout << res;

            });
        @endcode

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.1">Websocket Opening Handshake Client Requirements (RFC6455)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.4">Host field (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-3.1.1">request-target (RFC7230)</a>
        @li <a href="https://tools.ietf.org/html/rfc7230#section-5.3.1">origin-form (RFC7230)</a>
    */
    template<
        BOOST_BEAST_ASYNC_TPARAM1 HandshakeHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT1(HandshakeHandler)
    async_handshake(
        response_type& res,
        string_view host,
        string_view target,
        HandshakeHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    //--------------------------------------------------------------------------
    //
    // Handshaking (Server)
    //
    //--------------------------------------------------------------------------

    /** Perform the WebSocket handshake in the server role.

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The request is received and the response is sent.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        If a valid upgrade request is received, an HTTP response with a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols is sent to
        the peer, otherwise a non-successful error is associated with
        the operation.

        If the request size exceeds the capacity of the stream's
        internal buffer, the error @ref error::buffer_overflow will be
        indicated. To handle larger requests, an application should
        read the HTTP request directly using @ref http::read and then
        pass the request to the appropriate overload of @ref accept or
        @ref async_accept

        @throws system_error Thrown on failure.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
    */
    void
    accept();

    /** Read and respond to a WebSocket HTTP Upgrade request.

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The request is received and the response is sent.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        If a valid upgrade request is received, an HTTP response with a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols is sent to
        the peer, otherwise a non-successful error is associated with
        the operation.

        If the request size exceeds the capacity of the stream's
        internal buffer, the error @ref error::buffer_overflow will be
        indicated. To handle larger requests, an application should
        read the HTTP request directly using @ref http::read and then
        pass the request to the appropriate overload of @ref accept or
        @ref async_accept

        @param ec Set to indicate what error occurred, if any.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
    */
    void
    accept(error_code& ec);

    /** Read and respond to a WebSocket HTTP Upgrade request.

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The request is received and the response is sent.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        If a valid upgrade request is received, an HTTP response with a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols is sent to
        the peer, otherwise a non-successful error is associated with
        the operation.

        If the request size exceeds the capacity of the stream's
        internal buffer, the error @ref error::buffer_overflow will be
        indicated. To handle larger requests, an application should
        read the HTTP request directly using @ref http::read and then
        pass the request to the appropriate overload of @ref accept or
        @ref async_accept

        @param buffers Caller provided data that has already been
        received on the stream. The implementation will copy the
        caller provided data before the function returns.

        @throws system_error Thrown on failure.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
    */
    template<class ConstBufferSequence>
#if BOOST_BEAST_DOXYGEN
    void
#else
    typename std::enable_if<! http::detail::is_header<
        ConstBufferSequence>::value>::type
#endif
    accept(ConstBufferSequence const& buffers);

    /** Read and respond to a WebSocket HTTP Upgrade request.

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The request is received and the response is sent.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        If a valid upgrade request is received, an HTTP response with a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols is sent to
        the peer, otherwise a non-successful error is associated with
        the operation.

        If the request size exceeds the capacity of the stream's
        internal buffer, the error @ref error::buffer_overflow will be
        indicated. To handle larger requests, an application should
        read the HTTP request directly using @ref http::read and then
        pass the request to the appropriate overload of @ref accept or
        @ref async_accept

        @param buffers Caller provided data that has already been
        received on the stream. The implementation will copy the
        caller provided data before the function returns.

        @param ec Set to indicate what error occurred, if any.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
    */
    template<class ConstBufferSequence>
#if BOOST_BEAST_DOXYGEN
    void
#else
    typename std::enable_if<! http::detail::is_header<
        ConstBufferSequence>::value>::type
#endif
    accept(
        ConstBufferSequence const& buffers,
        error_code& ec);

    /** Respond to a WebSocket HTTP Upgrade request

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The response is sent.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        If a valid upgrade request is received, an HTTP response with a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols is sent to
        the peer, otherwise a non-successful error is associated with
        the operation.

        @param req An object containing the HTTP Upgrade request.
        Ownership is not transferred, the implementation will not
        access this object from other threads.

        @throws system_error Thrown on failure.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
    */
    template<class Body, class Allocator>
    void
    accept(http::request<Body,
        http::basic_fields<Allocator>> const& req);

    /** Respond to a WebSocket HTTP Upgrade request

        This function is used to perform the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        The call blocks until one of the following conditions is true:

        @li The response is sent.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        If a valid upgrade request is received, an HTTP response with a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols is sent to
        the peer, otherwise a non-successful error is associated with
        the operation.

        @param req An object containing the HTTP Upgrade request.
        Ownership is not transferred, the implementation will not
        access this object from other threads.

        @param ec Set to indicate what error occurred, if any.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
    */
    template<class Body, class Allocator>
    void
    accept(http::request<Body,
        http::basic_fields<Allocator>> const& req,
            error_code& ec);

    /** Perform the WebSocket handshake asynchronously in the server role.

        This initiating function is used to asynchronously begin performing the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li The request is received and the response is sent.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_read_some`
        and `async_write_some` functions. No other operation may be performed
        on the stream until this operation completes.

        If a valid upgrade request is received, an HTTP response with a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols is sent to
        the peer, otherwise a non-successful error is associated with
        the operation.

        If the request size exceeds the capacity of the stream's
        internal buffer, the error @ref error::buffer_overflow will be
        indicated. To handle larger requests, an application should
        read the HTTP request directly using @ref http::async_read and then
        pass the request to the appropriate overload of @ref accept or
        @ref async_accept

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec    // Result of operation
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
    */
    template<
        BOOST_BEAST_ASYNC_TPARAM1 AcceptHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT1(AcceptHandler)
    async_accept(
        AcceptHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    /** Perform the WebSocket handshake asynchronously in the server role.

        This initiating function is used to asynchronously begin performing the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li The request is received and the response is sent.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_read_some`
        and `async_write_some` functions. No other operation may be performed
        on the stream until this operation completes.

        If a valid upgrade request is received, an HTTP response with a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols is sent to
        the peer, otherwise a non-successful error is associated with
        the operation.

        If the request size exceeds the capacity of the stream's
        internal buffer, the error @ref error::buffer_overflow will be
        indicated. To handle larger requests, an application should
        read the HTTP request directly using @ref http::async_read and then
        pass the request to the appropriate overload of @ref accept or
        @ref async_accept

        @param buffers Caller provided data that has already been
        received on the stream. This may be used for implementations
        allowing multiple protocols on the same stream. The
        buffered data will first be applied to the handshake, and
        then to received WebSocket frames. The implementation will
        copy the caller provided data before the function returns.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec    // Result of operation
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
    */
    template<
        class ConstBufferSequence,
        BOOST_BEAST_ASYNC_TPARAM1 AcceptHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT1(AcceptHandler)
    async_accept(
        ConstBufferSequence const& buffers,
        AcceptHandler&& handler =
            net::default_completion_token_t<
                executor_type>{}
#ifndef BOOST_BEAST_DOXYGEN
        , typename std::enable_if<
            ! http::detail::is_header<
            ConstBufferSequence>::value>::type* = 0
#endif
    );

    /** Perform the WebSocket handshake asynchronously in the server role.

        This initiating function is used to asynchronously begin performing the
        <a href="https://en.wikipedia.org/wiki/WebSocket#Protocol_handshake">WebSocket handshake</a>,
        required before messages can be sent and received. During the handshake,
        the client sends the Websocket Upgrade HTTP request, and the server
        replies with an HTTP response indicating the result of the handshake.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li The request is received and the response is sent.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_read_some`
        and `async_write_some` functions. No other operation may be performed
        on the stream until this operation completes.

        If a valid upgrade request is received, an HTTP response with a
        <a href="https://tools.ietf.org/html/rfc7230#section-3.1.2">status-code</a>
        of @ref beast::http::status::switching_protocols is sent to
        the peer, otherwise a non-successful error is associated with
        the operation.

        @param req An object containing the HTTP Upgrade request.
        Ownership is not transferred, the implementation will not access
        this object from other threads.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec    // Result of operation
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-4.2">Websocket Opening Handshake Server Requirements (RFC6455)</a>
    */
    template<
        class Body, class Allocator,
        BOOST_BEAST_ASYNC_TPARAM1 AcceptHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT1(AcceptHandler)
    async_accept(
        http::request<Body,
            http::basic_fields<Allocator>> const& req,
        AcceptHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    //--------------------------------------------------------------------------
    //
    // Close Frames
    //
    //--------------------------------------------------------------------------

    /** Send a websocket close control frame.

        This function is used to send a
        <a href="https://tools.ietf.org/html/rfc6455#section-5.5.1">close frame</a>,
        which begins the websocket closing handshake. The session ends when
        both ends of the connection have sent and received a close frame.

        The call blocks until one of the following conditions is true:

        @li The close frame is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        After beginning the closing handshake, the program should not write
        further message data, pings, or pongs. Instead, the program should
        continue reading message data until an error occurs. A read returning
        @ref error::closed indicates a successful connection closure.

        @param cr The reason for the close.
        If the close reason specifies a close code other than
        @ref beast::websocket::close_code::none, the close frame is
        sent with the close code and optional reason string. Otherwise,
        the close frame is sent with no payload.

        @throws system_error Thrown on failure.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-7.1.2">Websocket Closing Handshake (RFC6455)</a>
    */
    void
    close(close_reason const& cr);

    /** Send a websocket close control frame.

        This function is used to send a
        <a href="https://tools.ietf.org/html/rfc6455#section-5.5.1">close frame</a>,
        which begins the websocket closing handshake. The session ends when
        both ends of the connection have sent and received a close frame.

        The call blocks until one of the following conditions is true:

        @li The close frame is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        After beginning the closing handshake, the program should not write
        further message data, pings, or pongs. Instead, the program should
        continue reading message data until an error occurs. A read returning
        @ref error::closed indicates a successful connection closure.

        @param cr The reason for the close.
        If the close reason specifies a close code other than
        @ref beast::websocket::close_code::none, the close frame is
        sent with the close code and optional reason string. Otherwise,
        the close frame is sent with no payload.

        @param ec Set to indicate what error occurred, if any.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-7.1.2">Websocket Closing Handshake (RFC6455)</a>
    */
    void
    close(close_reason const& cr, error_code& ec);

    /** Send a websocket close control frame asynchronously.

        This function is used to asynchronously send a
        <a href="https://tools.ietf.org/html/rfc6455#section-5.5.1">close frame</a>,
        which begins the websocket closing handshake. The session ends when
        both ends of the connection have sent and received a close frame.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li The close frame finishes sending.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_write_some`
        function. No other operations except for message reading operations
        should be initiated on the stream after a close operation is started.

        After beginning the closing handshake, the program should not write
        further message data, pings, or pongs. Instead, the program should
        continue reading message data until an error occurs. A read returning
        @ref error::closed indicates a successful connection closure.

        @param cr The reason for the close.
        If the close reason specifies a close code other than
        @ref beast::websocket::close_code::none, the close frame is
        sent with the close code and optional reason string. Otherwise,
        the close frame is sent with no payload.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec     // Result of operation
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @see
        @li <a href="https://tools.ietf.org/html/rfc6455#section-7.1.2">Websocket Closing Handshake (RFC6455)</a>
    */
    template<
        BOOST_BEAST_ASYNC_TPARAM1 CloseHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT1(CloseHandler)
    async_close(
        close_reason const& cr,
        CloseHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    //--------------------------------------------------------------------------
    //
    // Ping/Pong Frames
    //
    //--------------------------------------------------------------------------

    /** Send a websocket ping control frame.

        This function is used to send a
        <a href="https://tools.ietf.org/html/rfc6455#section-5.5.2">ping frame</a>,
        which usually elicits an automatic pong control frame response from
        the peer.

        The call blocks until one of the following conditions is true:

        @li The ping frame is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        @param payload The payload of the ping message, which may be empty.

        @throws system_error Thrown on failure.
    */
    void
    ping(ping_data const& payload);

    /** Send a websocket ping control frame.

        This function is used to send a
        <a href="https://tools.ietf.org/html/rfc6455#section-5.5.2">ping frame</a>,
        which usually elicits an automatic pong control frame response from
        the peer.

        The call blocks until one of the following conditions is true:

        @li The ping frame is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        @param payload The payload of the ping message, which may be empty.

        @param ec Set to indicate what error occurred, if any.
    */
    void
    ping(ping_data const& payload, error_code& ec);

    /** Send a websocket ping control frame asynchronously.

        This function is used to asynchronously send a
        <a href="https://tools.ietf.org/html/rfc6455#section-5.5.2">ping frame</a>,
        which usually elicits an automatic pong control frame response from
        the peer.

        @li The ping frame is written.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_write_some`
        function. The program must ensure that no other calls to @ref ping,
        @ref pong, @ref async_ping, or @ref async_pong are performed until
        this operation completes.

        If a close frame is sent or received before the ping frame is
        sent, the error received by this completion handler will be
        `net::error::operation_aborted`.

        @param payload The payload of the ping message, which may be empty.
        The implementation will not access the contents of this object after
        the initiating function returns.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec     // Result of operation
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        BOOST_BEAST_ASYNC_TPARAM1 WriteHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT1(WriteHandler)
    async_ping(
        ping_data const& payload,
        WriteHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    /** Send a websocket pong control frame.

        This function is used to send a
        <a href="https://tools.ietf.org/html/rfc6455#section-5.5.3">pong frame</a>,
        which is usually sent automatically in response to a ping frame
        from the remote peer.

        The call blocks until one of the following conditions is true:

        @li The pong frame is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        WebSocket allows pong frames to be sent at any time, without first
        receiving a ping. An unsolicited pong sent in this fashion may
        indicate to the remote peer that the connection is still active.

        @param payload The payload of the pong message, which may be empty.

        @throws system_error Thrown on failure.
    */
    void
    pong(ping_data const& payload);

    /** Send a websocket pong control frame.

        This function is used to send a
        <a href="https://tools.ietf.org/html/rfc6455#section-5.5.3">pong frame</a>,
        which is usually sent automatically in response to a ping frame
        from the remote peer.

        The call blocks until one of the following conditions is true:

        @li The pong frame is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        WebSocket allows pong frames to be sent at any time, without first
        receiving a ping. An unsolicited pong sent in this fashion may
        indicate to the remote peer that the connection is still active.

        @param payload The payload of the pong message, which may be empty.

        @param ec Set to indicate what error occurred, if any.
    */
    void
    pong(ping_data const& payload, error_code& ec);

    /** Send a websocket pong control frame asynchronously.

        This function is used to asynchronously send a
        <a href="https://tools.ietf.org/html/rfc6455#section-5.5.3">pong frame</a>,
        which is usually sent automatically in response to a ping frame
        from the remote peer.

        @li The pong frame is written.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_write_some`
        function. The program must ensure that no other calls to @ref ping,
        @ref pong, @ref async_ping, or @ref async_pong are performed until
        this operation completes.

        If a close frame is sent or received before the pong frame is
        sent, the error received by this completion handler will be
        `net::error::operation_aborted`.

        WebSocket allows pong frames to be sent at any time, without first
        receiving a ping. An unsolicited pong sent in this fashion may
        indicate to the remote peer that the connection is still active.

        @param payload The payload of the pong message, which may be empty.
        The implementation will not access the contents of this object after
        the initiating function returns.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec     // Result of operation
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        BOOST_BEAST_ASYNC_TPARAM1 WriteHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT1(WriteHandler)
    async_pong(
        ping_data const& payload,
        WriteHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    //--------------------------------------------------------------------------
    //
    // Reading
    //
    //--------------------------------------------------------------------------

    /** Read a complete message.

        This function is used to read a complete message.

        The call blocks until one of the following is true:

        @li A complete message is received.

        @li A close frame is received. In this case the error indicated by
            the function will be @ref error::closed.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        Received message data is appended to the buffer.
        The functions @ref got_binary and @ref got_text may be used
        to query the stream and determine the type of the last received message.

        Until the call returns, the implementation will read incoming control
        frames and handle them automatically as follows:

        @li The @ref control_callback will be invoked for each control frame.

        @li For each received ping frame, a pong frame will be
            automatically sent.

        @li If a close frame is received, the WebSocket closing handshake is
            performed. In this case, when the function returns, the error
            @ref error::closed will be indicated.

        @return The number of message payload bytes appended to the buffer.

        @param buffer A dynamic buffer to append message data to.

        @throws system_error Thrown on failure.
    */
    template<class DynamicBuffer>
    std::size_t
    read(DynamicBuffer& buffer);

    /** Read a complete message.

        This function is used to read a complete message.

        The call blocks until one of the following is true:

        @li A complete message is received.

        @li A close frame is received. In this case the error indicated by
            the function will be @ref error::closed.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        Received message data is appended to the buffer.
        The functions @ref got_binary and @ref got_text may be used
        to query the stream and determine the type of the last received message.

        Until the call returns, the implementation will read incoming control
        frames and handle them automatically as follows:

        @li The @ref control_callback will be invoked for each control frame.

        @li For each received ping frame, a pong frame will be
            automatically sent.

        @li If a close frame is received, the WebSocket closing handshake is
            performed. In this case, when the function returns, the error
            @ref error::closed will be indicated.

        @return The number of message payload bytes appended to the buffer.

        @param buffer A dynamic buffer to append message data to.

        @param ec Set to indicate what error occurred, if any.
    */
    template<class DynamicBuffer>
    std::size_t
    read(DynamicBuffer& buffer, error_code& ec);

    /** Read a complete message asynchronously.

        This function is used to asynchronously read a complete message.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li A complete message is received.

        @li A close frame is received. In this case the error indicated by
            the function will be @ref error::closed.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_read_some`
        and `async_write_some` functions. The program must ensure that no other
        calls to @ref read, @ref read_some, @ref async_read, or @ref async_read_some
        are performed until this operation completes.

        Received message data is appended to the buffer.
        The functions @ref got_binary and @ref got_text may be used
        to query the stream and determine the type of the last received message.

        Until the operation completes, the implementation will read incoming
        control frames and handle them automatically as follows:

        @li The @ref control_callback will be invoked for each control frame.

        @li For each received ping frame, a pong frame will be
            automatically sent.

        @li If a close frame is received, the WebSocket close procedure is
            performed. In this case, when the function returns, the error
            @ref error::closed will be indicated.

        Pong frames and close frames sent by the implementation while the
        read operation is outstanding do not prevent the application from
        also writing message data, sending pings, sending pongs, or sending
        close frames.

        @param buffer A dynamic buffer to append message data to.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec,       // Result of operation
            std::size_t bytes_written   // Number of bytes appended to buffer
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        class DynamicBuffer,
        BOOST_BEAST_ASYNC_TPARAM2 ReadHandler =
            net::default_completion_token_t<
                executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
    async_read(
        DynamicBuffer& buffer,
        ReadHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    //--------------------------------------------------------------------------

    /** Read some message data.

        This function is used to read some message data.

        The call blocks until one of the following is true:

        @li Some message data is received.

        @li A close frame is received. In this case the error indicated by
            the function will be @ref error::closed.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        Received message data is appended to the buffer.
        The functions @ref got_binary and @ref got_text may be used
        to query the stream and determine the type of the last received message.
        The function @ref is_message_done may be called to determine if the
        message received by the last read operation is complete.

        Until the call returns, the implementation will read incoming control
        frames and handle them automatically as follows:

        @li The @ref control_callback will be invoked for each control frame.

        @li For each received ping frame, a pong frame will be
            automatically sent.

        @li If a close frame is received, the WebSocket closing handshake is
            performed. In this case, when the function returns, the error
            @ref error::closed will be indicated.

        @return The number of message payload bytes appended to the buffer.

        @param buffer A dynamic buffer to append message data to.

        @param limit An upper limit on the number of bytes this function
        will append into the buffer. If this value is zero, then a reasonable
        size will be chosen automatically.

        @throws system_error Thrown on failure.
    */
    template<class DynamicBuffer>
    std::size_t
    read_some(
        DynamicBuffer& buffer,
        std::size_t limit);

    /** Read some message data.

        This function is used to read some message data.

        The call blocks until one of the following is true:

        @li Some message data is received.

        @li A close frame is received. In this case the error indicated by
            the function will be @ref error::closed.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        Received message data is appended to the buffer.
        The functions @ref got_binary and @ref got_text may be used
        to query the stream and determine the type of the last received message.
        The function @ref is_message_done may be called to determine if the
        message received by the last read operation is complete.

        Until the call returns, the implementation will read incoming control
        frames and handle them automatically as follows:

        @li The @ref control_callback will be invoked for each control frame.

        @li For each received ping frame, a pong frame will be
            automatically sent.

        @li If a close frame is received, the WebSocket closing handshake is
            performed. In this case, when the function returns, the error
            @ref error::closed will be indicated.

        @return The number of message payload bytes appended to the buffer.

        @param buffer A dynamic buffer to append message data to.

        @param limit An upper limit on the number of bytes this function
        will append into the buffer. If this value is zero, then a reasonable
        size will be chosen automatically.

        @param ec Set to indicate what error occurred, if any.
    */
    template<class DynamicBuffer>
    std::size_t
    read_some(
        DynamicBuffer& buffer,
        std::size_t limit,
        error_code& ec);

    /** Read some message data asynchronously.

        This function is used to asynchronously read some message data.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li Some message data is received.

        @li A close frame is received. In this case the error indicated by
            the function will be @ref error::closed.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_read_some`
        and `async_write_some` functions. The program must ensure that no other
        calls to @ref read, @ref read_some, @ref async_read, or @ref async_read_some
        are performed until this operation completes.

        Received message data is appended to the buffer.
        The functions @ref got_binary and @ref got_text may be used
        to query the stream and determine the type of the last received message.

        Until the operation completes, the implementation will read incoming
        control frames and handle them automatically as follows:

        @li The @ref control_callback will be invoked for each control frame.

        @li For each received ping frame, a pong frame will be
            automatically sent.

        @li If a close frame is received, the WebSocket close procedure is
            performed. In this case, when the function returns, the error
            @ref error::closed will be indicated.

        Pong frames and close frames sent by the implementation while the
        read operation is outstanding do not prevent the application from
        also writing message data, sending pings, sending pongs, or sending
        close frames.

        @param buffer A dynamic buffer to append message data to.

        @param limit An upper limit on the number of bytes this function
        will append into the buffer. If this value is zero, then a reasonable
        size will be chosen automatically.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec,       // Result of operation
            std::size_t bytes_written   // Number of bytes appended to buffer
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        class DynamicBuffer,
        BOOST_BEAST_ASYNC_TPARAM2 ReadHandler =
            net::default_completion_token_t<
                executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
    async_read_some(
        DynamicBuffer& buffer,
        std::size_t limit,
        ReadHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    //--------------------------------------------------------------------------

    /** Read some message data.

        This function is used to read some message data.

        The call blocks until one of the following is true:

        @li Some message data is received.

        @li A close frame is received. In this case the error indicated by
            the function will be @ref error::closed.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        The functions @ref got_binary and @ref got_text may be used
        to query the stream and determine the type of the last received message.
        The function @ref is_message_done may be called to determine if the
        message received by the last read operation is complete.

        Until the call returns, the implementation will read incoming control
        frames and handle them automatically as follows:

        @li The @ref control_callback will be invoked for each control frame.

        @li For each received ping frame, a pong frame will be
            automatically sent.

        @li If a close frame is received, the WebSocket closing handshake is
            performed. In this case, when the function returns, the error
            @ref error::closed will be indicated.

        @return The number of message payload bytes appended to the buffer.

        @param buffers A buffer sequence to write message data into.
        The previous contents of the buffers will be overwritten, starting
        from the beginning.

        @throws system_error Thrown on failure.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(
        MutableBufferSequence const& buffers);

    /** Read some message data.

        This function is used to read some message data.

        The call blocks until one of the following is true:

        @li Some message data is received.

        @li A close frame is received. In this case the error indicated by
            the function will be @ref error::closed.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `read_some` and `write_some`
        functions.

        The functions @ref got_binary and @ref got_text may be used
        to query the stream and determine the type of the last received message.
        The function @ref is_message_done may be called to determine if the
        message received by the last read operation is complete.

        Until the call returns, the implementation will read incoming control
        frames and handle them automatically as follows:

        @li The @ref control_callback will be invoked for each control frame.

        @li For each received ping frame, a pong frame will be
            automatically sent.

        @li If a close frame is received, the WebSocket closing handshake is
            performed. In this case, when the function returns, the error
            @ref error::closed will be indicated.

        @return The number of message payload bytes appended to the buffer.

        @param buffers A buffer sequence to write message data into.
        The previous contents of the buffers will be overwritten, starting
        from the beginning.

        @param ec Set to indicate what error occurred, if any.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(
        MutableBufferSequence const& buffers,
        error_code& ec);

    /** Read some message data asynchronously.

        This function is used to asynchronously read some message data.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li Some message data is received.

        @li A close frame is received. In this case the error indicated by
            the function will be @ref error::closed.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_read_some`
        and `async_write_some` functions. The program must ensure that no other
        calls to @ref read, @ref read_some, @ref async_read, or @ref async_read_some
        are performed until this operation completes.

        Received message data is appended to the buffer.
        The functions @ref got_binary and @ref got_text may be used
        to query the stream and determine the type of the last received message.

        Until the operation completes, the implementation will read incoming
        control frames and handle them automatically as follows:

        @li The @ref control_callback will be invoked for each control frame.

        @li For each received ping frame, a pong frame will be
            automatically sent.

        @li If a close frame is received, the WebSocket close procedure is
            performed. In this case, when the function returns, the error
            @ref error::closed will be indicated.

        Pong frames and close frames sent by the implementation while the
        read operation is outstanding do not prevent the application from
        also writing message data, sending pings, sending pongs, or sending
        close frames.

        @param buffers A buffer sequence to write message data into.
        The previous contents of the buffers will be overwritten, starting
        from the beginning.
        The implementation will make copies of this object as needed, but
        but ownership of the underlying memory is not transferred. The
        caller is responsible for ensuring that the memory locations
        pointed to by the buffer sequence remain valid until the
        completion handler is called.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec,       // Result of operation
            std::size_t bytes_written   // Number of bytes written to the buffers
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        class MutableBufferSequence,
        BOOST_BEAST_ASYNC_TPARAM2 ReadHandler =
            net::default_completion_token_t<
                executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
    async_read_some(
        MutableBufferSequence const& buffers,
        ReadHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    //--------------------------------------------------------------------------
    //
    // Writing
    //
    //--------------------------------------------------------------------------

    /** Write a complete message.

        This function is used to write a complete message.

        The call blocks until one of the following is true:

        @li The message is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        The current setting of the @ref binary option controls
        whether the message opcode is set to text or binary. If the
        @ref auto_fragment option is set, the message will be split
        into one or more frames as necessary. The actual payload contents
        sent may be transformed as per the WebSocket protocol settings.

        @param buffers The buffers containing the message to send.

        @return The number of bytes sent from the buffers.

        @throws system_error Thrown on failure.
    */
    template<class ConstBufferSequence>
    std::size_t
    write(ConstBufferSequence const& buffers);

    /** Write a complete message.

        This function is used to write a complete message.

        The call blocks until one of the following is true:

        @li The complete message is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        The current setting of the @ref binary option controls
        whether the message opcode is set to text or binary. If the
        @ref auto_fragment option is set, the message will be split
        into one or more frames as necessary. The actual payload contents
        sent may be transformed as per the WebSocket protocol settings.

        @param buffers The buffers containing the message to send.

        @param ec Set to indicate what error occurred, if any.

        @return The number of bytes sent from the buffers.
    */
    template<class ConstBufferSequence>
    std::size_t
    write(ConstBufferSequence const& buffers, error_code& ec);

    /** Write a complete message asynchronously.

        This function is used to asynchronously write a complete message.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li The complete message is written.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's
        `async_write_some` function. The program must ensure that no other
        calls to @ref write, @ref write_some, @ref async_write, or
        @ref async_write_some are performed until this operation completes.

        The current setting of the @ref binary option controls
        whether the message opcode is set to text or binary. If the
        @ref auto_fragment option is set, the message will be split
        into one or more frames as necessary. The actual payload contents
        sent may be transformed as per the WebSocket protocol settings.

        @param buffers A buffer sequence containing the entire message
        payload. The implementation will make copies of this object
        as needed, but ownership of the underlying memory is not
        transferred. The caller is responsible for ensuring that
        the memory locations pointed to by buffers remains valid
        until the completion handler is called.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec,           // Result of operation
            std::size_t bytes_transferred   // Number of bytes sent from the
                                            // buffers. If an error occurred,
                                            // this will be less than the buffer_size.
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        class ConstBufferSequence,
        BOOST_BEAST_ASYNC_TPARAM2 WriteHandler =
            net::default_completion_token_t<
                executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
    async_write(
        ConstBufferSequence const& buffers,
        WriteHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    /** Write some message data.

        This function is used to send part of a message.

        The call blocks until one of the following is true:

        @li The message data is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        If this is the beginning of a new message, the message opcode
        will be set to text or binary based on the current setting of
        the @ref binary (or @ref text) option. The actual payload sent
        may be transformed as per the WebSocket protocol settings.

        @param fin `true` if this is the last part of the message.

        @param buffers The buffers containing the message part to send.

        @return The number of bytes sent from the buffers.

        @throws system_error Thrown on failure.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(bool fin, ConstBufferSequence const& buffers);

    /** Write some message data.

        This function is used to send part of a message.

        The call blocks until one of the following is true:

        @li The message data is written.

        @li An error occurs.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the next layer's `write_some` function.

        If this is the beginning of a new message, the message opcode
        will be set to text or binary based on the current setting of
        the @ref binary (or @ref text) option. The actual payload sent
        may be transformed as per the WebSocket protocol settings.

        @param fin `true` if this is the last part of the message.

        @param buffers The buffers containing the message part to send.

        @param ec Set to indicate what error occurred, if any.

        @return The number of bytes sent from the buffers.

        @return The number of bytes consumed in the input buffers.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(bool fin,
        ConstBufferSequence const& buffers, error_code& ec);

    /** Write some message data asynchronously.

        This function is used to asynchronously write part of a message.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li The message data is written.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's
        `async_write_some` function. The program must ensure that no other
        calls to @ref write, @ref write_some, @ref async_write, or
        @ref async_write_some are performed until this operation completes.

        If this is the beginning of a new message, the message opcode
        will be set to text or binary based on the current setting of
        the @ref binary (or @ref text) option. The actual payload sent
        may be transformed as per the WebSocket protocol settings.

        @param fin `true` if this is the last part of the message.

        @param buffers The buffers containing the message part to send.
        The implementation will make copies of this object
        as needed, but ownership of the underlying memory is not
        transferred. The caller is responsible for ensuring that
        the memory locations pointed to by buffers remains valid
        until the completion handler is called.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec,           // Result of operation
            std::size_t bytes_transferred   // Number of bytes sent from the
                                            // buffers. If an error occurred,
                                            // this will be less than the buffer_size.
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        class ConstBufferSequence,
        BOOST_BEAST_ASYNC_TPARAM2 WriteHandler =
            net::default_completion_token_t<
                executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
    async_write_some(
        bool fin,
        ConstBufferSequence const& buffers,
        WriteHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

private:
    template<class, class>  class accept_op;
    template<class>         class close_op;
    template<class>         class handshake_op;
    template<class>         class ping_op;
    template<class>         class idle_ping_op;
    template<class, class>  class read_some_op;
    template<class, class>  class read_op;
    template<class>         class response_op;
    template<class, class>  class write_some_op;
    template<class, class>  class write_op;

    struct run_accept_op;
    struct run_close_op;
    struct run_handshake_op;
    struct run_ping_op;
    struct run_idle_ping_op;
    struct run_read_some_op;
    struct run_read_op;
    struct run_response_op;
    struct run_write_some_op;
    struct run_write_op;

    static void default_decorate_req(request_type&) {}
    static void default_decorate_res(response_type&) {}

    //
    // accept / handshake
    //

    template<class Buffers, class Decorator>
    void
    do_accept(
        Buffers const& buffers,
        Decorator const& decorator,
        error_code& ec);

    template<
        class Body, class Allocator,
        class Decorator>
    void
    do_accept(
        http::request<Body,
            http::basic_fields<Allocator>> const& req,
        Decorator const& decorator,
        error_code& ec);

    template<class RequestDecorator>
    void
    do_handshake(response_type* res_p,
        string_view host, string_view target,
            RequestDecorator const& decorator,
                error_code& ec);

    //
    // fail
    //

    void
    do_fail(
        std::uint16_t code,
        error_code ev,
        error_code& ec);
};

/** Manually provide a one-time seed to initialize the PRNG

    This function invokes the specified seed sequence to produce a seed
    suitable for use with the pseudo-random number generator used to
    create masks and perform WebSocket protocol handshakes.

    If a seed is not manually provided, the implementation will
    perform a one-time seed generation using `std::random_device`. This
    function may be used when the application runs in an environment
    where the random device is unreliable or does not provide sufficient
    entropy.

    @par Preconditions

    This function may not be called after any websocket @ref stream objects
    have been constructed.

    @param ss A reference to a `std::seed_seq` which will be used to seed
    the pseudo-random number generator. The seed sequence should have at
    least 256 bits of entropy.

    @see stream::secure_prng
*/
inline
void
seed_prng(std::seed_seq& ss)
{
    detail::prng_seed(&ss);
}

} // websocket
} // beast
} // boost

#include <boost/beast/websocket/impl/stream_impl.hpp> // must be first
#include <boost/beast/websocket/impl/accept.hpp>
#include <boost/beast/websocket/impl/close.hpp>
#include <boost/beast/websocket/impl/handshake.hpp>
#include <boost/beast/websocket/impl/ping.hpp>
#include <boost/beast/websocket/impl/read.hpp>
#include <boost/beast/websocket/impl/stream.hpp>
#include <boost/beast/websocket/impl/write.hpp>

#endif
