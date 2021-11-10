//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_BASIC_STREAM_HPP
#define BOOST_BEAST_CORE_BASIC_STREAM_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/detail/stream_base.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/rate_policy.hpp>
#include <boost/beast/core/role.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/is_executor.hpp>
#include <boost/core/empty_value.hpp>
#include <boost/config/workaround.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <chrono>
#include <limits>
#include <memory>

#if ! BOOST_BEAST_DOXYGEN
namespace boost {
namespace asio {
namespace ssl {
template<typename> class stream;
} // ssl
} // asio
} // boost
#endif

namespace boost {
namespace beast {

/** A stream socket wrapper with timeouts, an executor, and a rate limit policy.

    This stream wraps a `net::basic_stream_socket` to provide
    the following features:

    @li An <em>Executor</em> may be associated with the stream, which will
    be used to invoke any completion handlers which do not already have
    an associated executor. This achieves support for
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1322r0.html">[P1322R0] Networking TS enhancement to enable custom I/O executors</a>.

    @li Timeouts may be specified for each logical asynchronous operation
    performing any reading, writing, or connecting.

    @li A <em>RatePolicy</em> may be associated with the stream, to implement
    rate limiting through the policy's interface.

    Although the stream supports multiple concurrent outstanding asynchronous
    operations, the stream object is not thread-safe. The caller is responsible
    for ensuring that the stream is accessed from only one thread at a time.
    This includes the times when the stream, and its underlying socket, are
    accessed by the networking implementation. To meet this thread safety
    requirement, all asynchronous operations must be performed by the stream
    within the same implicit strand (only one thread `net::io_context::run`)
    or within the same explicit strand, such as an instance of `net::strand`.

    Completion handlers with explicit associated executors (such as those
    arising from use of `net::bind_executor`) will be invoked by the stream
    using the associated executor. Otherwise, the completion handler will
    be invoked by the executor associated with the stream upon construction.
    The type of executor used with this stream must meet the following
    requirements:

    @li Function objects submitted to the executor shall never run
        concurrently with each other.

    The executor type `net::strand` meets these requirements. Use of a
    strand as the executor in the stream class template offers an additional
    notational convenience: the strand does not need to be specified in
    each individual initiating function call.

    Unlike other stream wrappers, the underlying socket is accessed
    through the @ref socket member function instead of `next_layer`.
    This causes the @ref basic_stream to be returned in calls
    to @ref get_lowest_layer.

    @par Usage

    To use this stream declare an instance of the class. Then, before
    each logical operation for which a timeout is desired, call
    @ref expires_after with a duration, or call @ref expires_at with a
    time point. Alternatively, call @ref expires_never to disable the
    timeout for subsequent logical operations. A logical operation
    is any series of one or more direct or indirect calls to the timeout
    stream's asynchronous read, asynchronous write, or asynchronous connect
    functions.

    When a timeout is set and a mixed operation is performed (one that
    includes both reads and writes, for example) the timeout applies
    to all of the intermediate asynchronous operations used in the
    enclosing operation. This allows timeouts to be applied to stream
    algorithms which were not written specifically to allow for timeouts,
    when those algorithms are passed a timeout stream with a timeout set.

    When a timeout occurs the socket will be closed, canceling any
    pending I/O operations. The completion handlers for these canceled
    operations will be invoked with the error @ref beast::error::timeout.

    @par Examples

    This function reads an HTTP request with a timeout, then sends the
    HTTP response with a different timeout.

    @code
    void process_http_1 (tcp_stream& stream, net::yield_context yield)
    {
        flat_buffer buffer;
        http::request<http::empty_body> req;

        // Read the request, with a 15 second timeout
        stream.expires_after(std::chrono::seconds(15));
        http::async_read(stream, buffer, req, yield);

        // Calculate the response
        http::response<http::string_body> res = make_response(req);

        // Send the response, with a 30 second timeout.
        stream.expires_after (std::chrono::seconds(30));
        http::async_write (stream, res, yield);
    }
    @endcode

    The example above could be expressed using a single timeout with a
    simple modification. The function that follows first reads an HTTP
    request then sends the HTTP response, with a single timeout that
    applies to the entire combined operation of reading and writing:

    @code
    void process_http_2 (tcp_stream& stream, net::yield_context yield)
    {
        flat_buffer buffer;
        http::request<http::empty_body> req;

        // Require that the read and write combined take no longer than 30 seconds
        stream.expires_after(std::chrono::seconds(30));

        http::async_read(stream, buffer, req, yield);

        http::response<http::string_body> res = make_response(req);
        http::async_write (stream, res, yield);
    }
    @endcode

    Some stream algorithms, such as `ssl::stream::async_handshake` perform
    both reads and writes. A timeout set before calling the initiating function
    of such composite stream algorithms will apply to the entire composite
    operation. For example, a timeout may be set on performing the SSL handshake
    thusly:

    @code
    void do_ssl_handshake (net::ssl::stream<tcp_stream>& stream, net::yield_context yield)
    {
        // Require that the SSL handshake take no longer than 10 seconds
        stream.expires_after(std::chrono::seconds(10));

        stream.async_handshake(net::ssl::stream_base::client, yield);
    }
    @endcode

    @par Blocking I/O

    Synchronous functions behave identically as that of the wrapped
    `net::basic_stream_socket`. Timeouts are not available when performing
    blocking calls.

    @tparam Protocol A type meeting the requirements of <em>Protocol</em>
    representing the protocol the protocol to use for the basic stream socket.
    A common choice is `net::ip::tcp`.

    @tparam Executor A type meeting the requirements of <em>Executor</em> to
    be used for submitting all completion handlers which do not already have an
    associated executor. If this type is omitted, the default of `net::any_io_executor`
    will be used.

    @par Thread Safety
    <em>Distinct objects</em>: Safe.@n
    <em>Shared objects</em>: Unsafe. The application must also ensure
    that all asynchronous operations are performed within the same
    implicit or explicit strand.

    @see

    @li <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1322r0.html">[P1322R0] Networking TS enhancement to enable custom I/O executors</a>.
*/
template<
    class Protocol,
    class Executor = net::any_io_executor,
    class RatePolicy = unlimited_rate_policy
>
class basic_stream
#if ! BOOST_BEAST_DOXYGEN
    : private detail::stream_base
#endif
{
public:
    /// The type of the underlying socket.
    using socket_type =
        net::basic_stream_socket<Protocol, Executor>;

    /** The type of the executor associated with the stream.

        This will be the type of executor used to invoke completion
        handlers which do not have an explicit associated executor.
    */
    using executor_type = beast::executor_type<socket_type>;

    /// Rebinds the stream type to another executor.
    template<class Executor1>
    struct rebind_executor
    {
        /// The stream type when rebound to the specified executor.
        using other = basic_stream<
            Protocol, Executor1, RatePolicy>;
    };

    /// The protocol type.
    using protocol_type = Protocol;

    /// The endpoint type.
    using endpoint_type = typename Protocol::endpoint;

private:
    static_assert(
        net::is_executor<Executor>::value || net::execution::is_executor<Executor>::value,
        "Executor type requirements not met");

    struct impl_type
        : boost::enable_shared_from_this<impl_type>
        , boost::empty_value<RatePolicy>
    {
        // must come first
        net::basic_stream_socket<
            Protocol, Executor> socket;

        op_state read;
        op_state write;
#if 0
        net::basic_waitable_timer<
            std::chrono::steady_clock,
            net::wait_traits<
                std::chrono::steady_clock>,
            Executor> timer; // rate timer;
#else
        net::steady_timer timer;
#endif
        int waiting = 0;

        impl_type(impl_type&&) = default;

        template<class... Args>
        explicit
        impl_type(std::false_type, Args&&...);

        template<class RatePolicy_, class... Args>
        explicit
        impl_type(std::true_type,
            RatePolicy_&& policy, Args&&...);

        impl_type& operator=(impl_type&&) = delete;

        beast::executor_type<socket_type>
        ex() noexcept
        {
            return this->socket.get_executor();
        }

        RatePolicy&
        policy() noexcept
        {
            return this->boost::empty_value<RatePolicy>::get();
        }

        RatePolicy const&
        policy() const noexcept
        {
            return this->boost::empty_value<RatePolicy>::get();
        }

        template<class Executor2>
        void on_timer(Executor2 const& ex2);

        void reset();           // set timeouts to never
        void close() noexcept;  // cancel everything
    };

    // We use shared ownership for the state so it can
    // outlive the destruction of the stream_socket object,
    // in the case where there is no outstanding read or write
    // but the implementation is still waiting on a timer.
    boost::shared_ptr<impl_type> impl_;

    template<class Executor2>
    struct timeout_handler;

    struct ops;

#if ! BOOST_BEAST_DOXYGEN
    // boost::asio::ssl::stream needs these
    // DEPRECATED
    template<class>
    friend class boost::asio::ssl::stream;
    // DEPRECATED
    using lowest_layer_type = socket_type;
    // DEPRECATED
    lowest_layer_type&
    lowest_layer() noexcept
    {
        return impl_->socket;
    }
    // DEPRECATED
    lowest_layer_type const&
    lowest_layer() const noexcept
    {
        return impl_->socket;
    }
#endif

public:
    /** Destructor

        This function destroys the stream, cancelling any outstanding
        asynchronous operations associated with the socket as if by
        calling cancel.
    */
    ~basic_stream();

    /** Constructor

        This constructor creates the stream by forwarding all arguments
        to the underlying socket. The socket then needs to be open and
        connected or accepted before data can be sent or received on it.

        @param args A list of parameters forwarded to the constructor of
        the underlying socket.
    */
#if BOOST_BEAST_DOXYGEN
    template<class... Args>
    explicit
    basic_stream(Args&&... args);
#else
    template<class Arg0, class... Args,
        class = typename std::enable_if<
        ! std::is_constructible<RatePolicy, Arg0>::value>::type>
    explicit
    basic_stream(Arg0&& argo, Args&&... args);
#endif

    /** Constructor

        This constructor creates the stream with the specified rate
        policy, and forwards all remaining arguments to the underlying
        socket. The socket then needs to be open and connected or
        accepted before data can be sent or received on it.

        @param policy The rate policy object to use. The stream will
        take ownership of this object by decay-copy.

        @param args A list of parameters forwarded to the constructor of
        the underlying socket.
    */
#if BOOST_BEAST_DOXYGEN
    template<class RatePolicy_, class... Args>
    explicit
    basic_stream(RatePolicy_&& policy, Args&&... args);
#else
    template<class RatePolicy_, class Arg0, class... Args,
        class = typename std::enable_if<
            std::is_constructible<
                RatePolicy, RatePolicy_>::value>::type>
    basic_stream(
        RatePolicy_&& policy, Arg0&& arg, Args&&... args);
#endif

    /** Move constructor

        @param other The other object from which the move will occur.

        @note Following the move, the moved-from object is in the
        same state as if newly constructed.
    */
    basic_stream(basic_stream&& other);

    /// Move assignment (deleted).
    basic_stream& operator=(basic_stream&&) = delete;

    /// Return a reference to the underlying socket
    socket_type&
    socket() noexcept
    {
        return impl_->socket;
    }

    /// Return a reference to the underlying socket
    socket_type const&
    socket() const noexcept
    {
        return impl_->socket;
    }

    /** Release ownership of the underlying socket.

        This function causes all outstanding asynchronous connect,
        read, and write operations to be canceled as if by a call
        to @ref cancel. Ownership of the underlying socket is then
        transferred to the caller.
    */
    socket_type
    release_socket();

    //--------------------------------------------------------------------------

    /// Returns the rate policy associated with the object
    RatePolicy&
    rate_policy() noexcept
    {
        return impl_->policy();
    }

    /// Returns the rate policy associated with the object
    RatePolicy const&
    rate_policy() const noexcept
    {
        return impl_->policy();
    }

    /** Set the timeout for the next logical operation.

        This sets either the read timer, the write timer, or
        both timers to expire after the specified amount of time
        has elapsed. If a timer expires when the corresponding
        asynchronous operation is outstanding, the stream will be
        closed and any outstanding operations will complete with the
        error @ref beast::error::timeout. Otherwise, if the timer
        expires while no operations are outstanding, and the expiraton
        is not set again, the next operation will time out immediately.

        The timer applies collectively to any asynchronous reads
        or writes initiated after the expiration is set, until the
        expiration is set again. A call to @ref async_connect
        counts as both a read and a write.

        @param expiry_time The amount of time after which a logical
        operation should be considered timed out.
    */
    void
    expires_after(
        net::steady_timer::duration expiry_time);

    /** Set the timeout for the next logical operation.

        This sets either the read timer, the write timer, or both
        timers to expire at the specified time point. If a timer
        expires when the corresponding asynchronous operation is
        outstanding, the stream will be closed and any outstanding
        operations will complete with the error @ref beast::error::timeout.
        Otherwise, if the timer expires while no operations are outstanding,
        and the expiraton is not set again, the next operation will time out
        immediately.

        The timer applies collectively to any asynchronous reads
        or writes initiated after the expiration is set, until the
        expiration is set again. A call to @ref async_connect
        counts as both a read and a write.

        @param expiry_time The time point after which a logical
        operation should be considered timed out.
    */
    void
    expires_at(net::steady_timer::time_point expiry_time);

    /// Disable the timeout for the next logical operation.
    void
    expires_never();

    /** Cancel all asynchronous operations associated with the socket.

        This function causes all outstanding asynchronous connect,
        read, and write operations to finish immediately. Completion
        handlers for cancelled operations will receive the error
        `net::error::operation_aborted`. Completion handlers not
        yet invoked whose operations have completed, will receive
        the error corresponding to the result of the operation (which
        may indicate success).
    */
    void
    cancel();

    /** Close the timed stream.

        This cancels all of the outstanding asynchronous operations
        as if by calling @ref cancel, and closes the underlying socket.
    */
    void
    close();

    //--------------------------------------------------------------------------

    /** Get the executor associated with the object.

        This function may be used to obtain the executor object that the
        stream uses to dispatch completion handlers without an assocaited
        executor.

        @return A copy of the executor that stream will use to dispatch handlers.
    */
    executor_type
    get_executor() noexcept
    {
        return impl_->ex();
    }

    /** Connect the stream to the specified endpoint.

        This function is used to connect the underlying socket to the
        specified remote endpoint. The function call will block until
        the connection is successfully made or an error occurs.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        @param ep The remote endpoint to connect to.

        @throws system_error Thrown on failure.

        @see connect
    */
    void
    connect(endpoint_type const& ep)
    {
        socket().connect(ep);
    }

    /** Connect the stream to the specified endpoint.

        This function is used to connect the underlying socket to the
        specified remote endpoint. The function call will block until
        the connection is successfully made or an error occurs.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        @param ep The remote endpoint to connect to.

        @param ec Set to indicate what error occurred, if any.

        @see connect
    */
    void
    connect(endpoint_type const& ep, error_code& ec)
    {
        socket().connect(ep, ec);
    }

    /** Establishes a connection by trying each endpoint in a sequence.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the underlying socket's `connect` function.

        @param endpoints A sequence of endpoints.

        @returns The successfully connected endpoint.

        @throws system_error Thrown on failure. If the sequence is
        empty, the associated error code is `net::error::not_found`.
        Otherwise, contains the error from the last connection attempt.
    */
    template<class EndpointSequence
    #if ! BOOST_BEAST_DOXYGEN
        ,class = typename std::enable_if<
            net::is_endpoint_sequence<
                EndpointSequence>::value>::type
    #endif
    >
    typename Protocol::endpoint
    connect(EndpointSequence const& endpoints)
    {
        return net::connect(socket(), endpoints);
    }

    /** Establishes a connection by trying each endpoint in a sequence.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the underlying socket's `connect` function.

        @param endpoints A sequence of endpoints.

        @param ec Set to indicate what error occurred, if any. If the sequence is
        empty, set to `net::error::not_found`. Otherwise, contains the error
        from the last connection attempt.

        @returns On success, the successfully connected endpoint. Otherwise, a
        default-constructed endpoint.
    */
    template<class EndpointSequence
    #if ! BOOST_BEAST_DOXYGEN
        ,class = typename std::enable_if<
            net::is_endpoint_sequence<
                EndpointSequence>::value>::type
    #endif
    >
    typename Protocol::endpoint
    connect(
        EndpointSequence const& endpoints,
        error_code& ec
    )
    {
        return net::connect(socket(), endpoints, ec);
    }

    /** Establishes a connection by trying each endpoint in a sequence.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the underlying socket's `connect` function.

        @param begin An iterator pointing to the start of a sequence of endpoints.

        @param end An iterator pointing to the end of a sequence of endpoints.

        @returns An iterator denoting the successfully connected endpoint.

        @throws system_error Thrown on failure. If the sequence is
        empty, the associated error code is `net::error::not_found`.
        Otherwise, contains the error from the last connection attempt.
    */
    template<class Iterator>
    Iterator
    connect(
        Iterator begin, Iterator end)
    {
        return net::connect(socket(), begin, end);
    }

    /** Establishes a connection by trying each endpoint in a sequence.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the underlying socket's `connect` function.

        @param begin An iterator pointing to the start of a sequence of endpoints.

        @param end An iterator pointing to the end of a sequence of endpoints.

        @param ec Set to indicate what error occurred, if any. If the sequence is
        empty, set to boost::asio::error::not_found. Otherwise, contains the error
        from the last connection attempt.

        @returns On success, an iterator denoting the successfully connected
        endpoint. Otherwise, the end iterator.
    */
    template<class Iterator>
    Iterator
    connect(
        Iterator begin, Iterator end,
        error_code& ec)
    {
        return net::connect(socket(), begin, end, ec);
    }

    /** Establishes a connection by trying each endpoint in a sequence.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the underlying socket's `connect` function.

        @param endpoints A sequence of endpoints.

        @param connect_condition A function object that is called prior to each
        connection attempt. The signature of the function object must be:
        @code
        bool connect_condition(
            error_code const& ec,
            typename Protocol::endpoint const& next);
        @endcode
        The @c ec parameter contains the result from the most recent connect
        operation. Before the first connection attempt, @c ec is always set to
        indicate success. The @c next parameter is the next endpoint to be tried.
        The function object should return true if the next endpoint should be tried,
        and false if it should be skipped.

        @returns The successfully connected endpoint.

        @throws boost::system::system_error Thrown on failure. If the sequence is
        empty, the associated error code is `net::error::not_found`.
        Otherwise, contains the error from the last connection attempt.
    */
    template<
        class EndpointSequence, class ConnectCondition
    #if ! BOOST_BEAST_DOXYGEN
        ,class = typename std::enable_if<
            net::is_endpoint_sequence<
                EndpointSequence>::value>::type
    #endif
    >
    typename Protocol::endpoint
    connect(
        EndpointSequence const& endpoints,
        ConnectCondition connect_condition
    )
    {
        return net::connect(socket(), endpoints, connect_condition);
    }

    /** Establishes a connection by trying each endpoint in a sequence.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the underlying socket's `connect` function.

        @param endpoints A sequence of endpoints.

        @param connect_condition A function object that is called prior to each
        connection attempt. The signature of the function object must be:
        @code
        bool connect_condition(
            error_code const& ec,
            typename Protocol::endpoint const& next);
        @endcode
        The @c ec parameter contains the result from the most recent connect
        operation. Before the first connection attempt, @c ec is always set to
        indicate success. The @c next parameter is the next endpoint to be tried.
        The function object should return true if the next endpoint should be tried,
        and false if it should be skipped.

        @param ec Set to indicate what error occurred, if any. If the sequence is
        empty, set to `net::error::not_found`. Otherwise, contains the error
        from the last connection attempt.

        @returns On success, the successfully connected endpoint. Otherwise, a
        default-constructed endpoint.
    */
    template<
        class EndpointSequence, class ConnectCondition
    #if ! BOOST_BEAST_DOXYGEN
        ,class = typename std::enable_if<
            net::is_endpoint_sequence<
                EndpointSequence>::value>::type
    #endif
    >
    typename Protocol::endpoint
    connect(
        EndpointSequence const& endpoints,
        ConnectCondition connect_condition,
        error_code& ec)
    {
        return net::connect(socket(), endpoints, connect_condition, ec);
    }

    /** Establishes a connection by trying each endpoint in a sequence.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the underlying socket's `connect` function.

        @param begin An iterator pointing to the start of a sequence of endpoints.

        @param end An iterator pointing to the end of a sequence of endpoints.

        @param connect_condition A function object that is called prior to each
        connection attempt. The signature of the function object must be:
        @code
        bool connect_condition(
            error_code const& ec,
            typename Protocol::endpoint const& next);
        @endcode
        The @c ec parameter contains the result from the most recent connect
        operation. Before the first connection attempt, @c ec is always set to
        indicate success. The @c next parameter is the next endpoint to be tried.
        The function object should return true if the next endpoint should be tried,
        and false if it should be skipped.

        @returns An iterator denoting the successfully connected endpoint.

        @throws boost::system::system_error Thrown on failure. If the sequence is
        empty, the associated @c error_code is `net::error::not_found`.
        Otherwise, contains the error from the last connection attempt.
    */
    template<
        class Iterator, class ConnectCondition>
    Iterator
    connect(
        Iterator begin, Iterator end,
        ConnectCondition connect_condition)
    {
        return net::connect(socket(), begin, end, connect_condition);
    }

    /** Establishes a connection by trying each endpoint in a sequence.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed operation</em>, is implemented
        in terms of calls to the underlying socket's `connect` function.

        @param begin An iterator pointing to the start of a sequence of endpoints.

        @param end An iterator pointing to the end of a sequence of endpoints.

        @param connect_condition A function object that is called prior to each
        connection attempt. The signature of the function object must be:
        @code
        bool connect_condition(
            error_code const& ec,
            typename Protocol::endpoint const& next);
        @endcode
        The @c ec parameter contains the result from the most recent connect
        operation. Before the first connection attempt, @c ec is always set to
        indicate success. The @c next parameter is the next endpoint to be tried.
        The function object should return true if the next endpoint should be tried,
        and false if it should be skipped.

        @param ec Set to indicate what error occurred, if any. If the sequence is
        empty, set to `net::error::not_found`. Otherwise, contains the error
        from the last connection attempt.

        @returns On success, an iterator denoting the successfully connected
        endpoint. Otherwise, the end iterator.
    */
    template<
        class Iterator, class ConnectCondition>
    Iterator
    connect(
        Iterator begin, Iterator end,
        ConnectCondition connect_condition,
        error_code& ec)
    {
        return net::connect(socket(), begin, end, connect_condition, ec);
    }

    /** Connect the stream to the specified endpoint asynchronously.

        This function is used to asynchronously connect the underlying
        socket to the specified remote endpoint. The function call always
        returns immediately.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        If the timeout timer expires while the operation is outstanding,
        the operation will be canceled and the completion handler will be
        invoked with the error @ref error::timeout.

        @param ep The remote endpoint to which the underlying socket will be
        connected. Copies will be made of the endpoint object as required.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code ec         // Result of operation
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @see async_connect
    */
    template<
        BOOST_BEAST_ASYNC_TPARAM1 ConnectHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT1(ConnectHandler)
    async_connect(
        endpoint_type const& ep,
        ConnectHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    /** Establishes a connection by trying each endpoint in a sequence asynchronously.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed asynchronous operation</em>, is
        implemented in terms of calls to the underlying socket's `async_connect`
        function.

        If the timeout timer expires while the operation is outstanding,
        the current connection attempt will be canceled and the completion
        handler will be invoked with the error @ref error::timeout.

        @param endpoints A sequence of endpoints. This this object must meet
        the requirements of <em>EndpointSequence</em>.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            // Result of operation. if the sequence is empty, set to
            // net::error::not_found. Otherwise, contains the
            // error from the last connection attempt.
            error_code const& error,

            // On success, the successfully connected endpoint.
            // Otherwise, a default-constructed endpoint.
            typename Protocol::endpoint const& endpoint
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        class EndpointSequence,
        BOOST_ASIO_COMPLETION_TOKEN_FOR(
            void(error_code, typename Protocol::endpoint))
            RangeConnectHandler =
                net::default_completion_token_t<executor_type>
    #if ! BOOST_BEAST_DOXYGEN
        ,class = typename std::enable_if<
            net::is_endpoint_sequence<
                EndpointSequence>::value>::type
    #endif
    >
    BOOST_ASIO_INITFN_RESULT_TYPE(
        RangeConnectHandler,
        void(error_code, typename Protocol::endpoint))
    async_connect(
        EndpointSequence const& endpoints,
        RangeConnectHandler&& handler =
            net::default_completion_token_t<executor_type>{});

    /** Establishes a connection by trying each endpoint in a sequence asynchronously.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed asynchronous operation</em>, is
        implemented in terms of calls to the underlying socket's `async_connect`
        function.

        If the timeout timer expires while the operation is outstanding,
        the current connection attempt will be canceled and the completion
        handler will be invoked with the error @ref error::timeout.

        @param endpoints A sequence of endpoints. This this object must meet
        the requirements of <em>EndpointSequence</em>.

        @param connect_condition A function object that is called prior to each
        connection attempt. The signature of the function object must be:
        @code
        bool connect_condition(
            error_code const& ec,
            typename Protocol::endpoint const& next);
        @endcode
        The @c ec parameter contains the result from the most recent connect
        operation. Before the first connection attempt, @c ec is always set to
        indicate success. The @c next parameter is the next endpoint to be tried.
        The function object should return true if the next endpoint should be tried,
        and false if it should be skipped.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            // Result of operation. if the sequence is empty, set to
            // net::error::not_found. Otherwise, contains the
            // error from the last connection attempt.
            error_code const& error,

            // On success, the successfully connected endpoint.
            // Otherwise, a default-constructed endpoint.
            typename Protocol::endpoint const& endpoint
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @par Example
        The following connect condition function object can be used to output
        information about the individual connection attempts:
        @code
        struct my_connect_condition
        {
            bool operator()(
                error_code const& ec,
                net::ip::tcp::endpoint const& next)
            {
                if (ec)
                    std::cout << "Error: " << ec.message() << std::endl;
                std::cout << "Trying: " << next << std::endl;
                return true;
            }
        };
        @endcode
    */
    template<
        class EndpointSequence,
        class ConnectCondition,
        BOOST_ASIO_COMPLETION_TOKEN_FOR(
            void(error_code, typename Protocol::endpoint))
            RangeConnectHandler =
                net::default_completion_token_t<executor_type>
    #if ! BOOST_BEAST_DOXYGEN
        ,class = typename std::enable_if<
            net::is_endpoint_sequence<
                EndpointSequence>::value>::type
    #endif
    >
    BOOST_ASIO_INITFN_RESULT_TYPE(
        RangeConnectHandler,
        void(error_code, typename Protocol::endpoint))
    async_connect(
        EndpointSequence const& endpoints,
        ConnectCondition connect_condition,
        RangeConnectHandler&& handler =
            net::default_completion_token_t<
                executor_type>{});

    /** Establishes a connection by trying each endpoint in a sequence asynchronously.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The underlying socket is automatically opened if needed.
        An automatically opened socket is not returned to the
        closed state upon failure.

        The algorithm, known as a <em>composed asynchronous operation</em>, is
        implemented in terms of calls to the underlying socket's `async_connect`
        function.

        If the timeout timer expires while the operation is outstanding,
        the current connection attempt will be canceled and the completion
        handler will be invoked with the error @ref error::timeout.

        @param begin An iterator pointing to the start of a sequence of endpoints.

        @param end An iterator pointing to the end of a sequence of endpoints.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            // Result of operation. if the sequence is empty, set to
            // net::error::not_found. Otherwise, contains the
            // error from the last connection attempt.
            error_code const& error,

            // On success, an iterator denoting the successfully
            // connected endpoint. Otherwise, the end iterator.
            Iterator iterator
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        class Iterator,
        BOOST_ASIO_COMPLETION_TOKEN_FOR(
            void(error_code, Iterator))
            IteratorConnectHandler =
                net::default_completion_token_t<executor_type>>
    BOOST_ASIO_INITFN_RESULT_TYPE(
        IteratorConnectHandler,
        void(error_code, Iterator))
    async_connect(
        Iterator begin, Iterator end,
        IteratorConnectHandler&& handler =
            net::default_completion_token_t<executor_type>{});

    /** Establishes a connection by trying each endpoint in a sequence asynchronously.

        This function attempts to connect the stream to one of a sequence of
        endpoints by trying each endpoint until a connection is successfully
        established.
        The algorithm, known as a <em>composed asynchronous operation</em>, is
        implemented in terms of calls to the underlying socket's `async_connect`
        function.

        If the timeout timer expires while the operation is outstanding,
        the current connection attempt will be canceled and the completion
        handler will be invoked with the error @ref error::timeout.

        @param begin An iterator pointing to the start of a sequence of endpoints.

        @param end An iterator pointing to the end of a sequence of endpoints.

        @param connect_condition A function object that is called prior to each
        connection attempt. The signature of the function object must be:
        @code
        bool connect_condition(
            error_code const& ec,
            typename Protocol::endpoint const& next);
        @endcode

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            // Result of operation. if the sequence is empty, set to
            // net::error::not_found. Otherwise, contains the
            // error from the last connection attempt.
            error_code const& error,

            // On success, an iterator denoting the successfully
            // connected endpoint. Otherwise, the end iterator.
            Iterator iterator
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
    */
    template<
        class Iterator,
        class ConnectCondition,
        BOOST_ASIO_COMPLETION_TOKEN_FOR(
            void(error_code, Iterator))
            IteratorConnectHandler =
                net::default_completion_token_t<executor_type>>
    BOOST_ASIO_INITFN_RESULT_TYPE(
        IteratorConnectHandler,
        void(error_code, Iterator))
    async_connect(
        Iterator begin, Iterator end,
        ConnectCondition connect_condition,
        IteratorConnectHandler&& handler =
            net::default_completion_token_t<executor_type>{});

    //--------------------------------------------------------------------------

    /** Read some data.

        This function is used to read some data from the stream.

        The call blocks until one of the following is true:

        @li One or more bytes are read from the stream.

        @li An error occurs.

        @param buffers The buffers into which the data will be read. If the
        size of the buffers is zero bytes, the call always returns
        immediately with no error.

        @returns The number of bytes read.

        @throws system_error Thrown on failure.

        @note The `read_some` operation may not receive all of the requested
        number of bytes. Consider using the function `net::read` if you need
        to ensure that the requested amount of data is read before the
        blocking operation completes.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers)
    {
        return impl_->socket.read_some(buffers);
    }

    /** Read some data.

        This function is used to read some data from the underlying socket.

        The call blocks until one of the following is true:

        @li One or more bytes are read from the stream.

        @li An error occurs.

        @param buffers The buffers into which the data will be read. If the
        size of the buffers is zero bytes, the call always returns
        immediately with no error.

        @param ec Set to indicate what error occurred, if any.

        @returns The number of bytes read.

        @note The `read_some` operation may not receive all of the requested
        number of bytes. Consider using the function `net::read` if you need
        to ensure that the requested amount of data is read before the
        blocking operation completes.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(
        MutableBufferSequence const& buffers,
        error_code& ec)
    {
        return impl_->socket.read_some(buffers, ec);
    }

    /** Read some data asynchronously.

        This function is used to asynchronously read data from the stream.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li One or more bytes are read from the stream.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_read_some`
        function. The program must ensure that no other calls to @ref read_some
        or @ref async_read_some are performed until this operation completes.

        If the timeout timer expires while the operation is outstanding,
        the operation will be canceled and the completion handler will be
        invoked with the error @ref error::timeout.

        @param buffers The buffers into which the data will be read. If the size
        of the buffers is zero bytes, the operation always completes immediately
        with no error.
        Although the buffers object may be copied as necessary, ownership of the
        underlying memory blocks is retained by the caller, which must guarantee
        that they remain valid until the handler is called.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code error,               // Result of operation.
            std::size_t bytes_transferred   // Number of bytes read.
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @note The `async_read_some` operation may not receive all of the requested
        number of bytes. Consider using the function `net::async_read` if you need
        to ensure that the requested amount of data is read before the asynchronous
        operation completes.
    */
    template<
        class MutableBufferSequence,
        BOOST_BEAST_ASYNC_TPARAM2 ReadHandler =
            net::default_completion_token_t<executor_type>
    >
    BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
    async_read_some(
        MutableBufferSequence const& buffers,
        ReadHandler&& handler =
            net::default_completion_token_t<executor_type>{}
    );

    /** Write some data.

        This function is used to write some data to the stream.

        The call blocks until one of the following is true:

        @li One or more bytes are written to the stream.

        @li An error occurs.

        @param buffers The buffers from which the data will be written. If the
        size of the buffers is zero bytes, the call always returns immediately
        with no error.

        @returns The number of bytes written.

        @throws system_error Thrown on failure.

        @note The `write_some` operation may not transmit all of the requested
        number of bytes. Consider using the function `net::write` if you need
        to ensure that the requested amount of data is written before the
        blocking operation completes.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers)
    {
        return impl_->socket.write_some(buffers);
    }

    /** Write some data.

        This function is used to write some data to the stream.

        The call blocks until one of the following is true:

        @li One or more bytes are written to the stream.

        @li An error occurs.

        @param buffers The buffers from which the data will be written. If the
        size of the buffers is zero bytes, the call always returns immediately
        with no error.

        @param ec Set to indicate what error occurred, if any.

        @returns The number of bytes written.

        @throws system_error Thrown on failure.

        @note The `write_some` operation may not transmit all of the requested
        number of bytes. Consider using the function `net::write` if you need
        to ensure that the requested amount of data is written before the
        blocking operation completes.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(
        ConstBufferSequence const& buffers,
        error_code& ec)
    {
        return impl_->socket.write_some(buffers, ec);
    }

    /** Write some data asynchronously.

        This function is used to asynchronously write data to the underlying socket.

        This call always returns immediately. The asynchronous operation
        will continue until one of the following conditions is true:

        @li One or more bytes are written to the stream.

        @li An error occurs.

        The algorithm, known as a <em>composed asynchronous operation</em>,
        is implemented in terms of calls to the next layer's `async_write_some`
        function. The program must ensure that no other calls to @ref async_write_some
        are performed until this operation completes.

        If the timeout timer expires while the operation is outstanding,
        the operation will be canceled and the completion handler will be
        invoked with the error @ref error::timeout.

        @param buffers The buffers from which the data will be written. If the
        size of the buffers is zero bytes, the operation  always completes
        immediately with no error.
        Although the buffers object may be copied as necessary, ownership of the
        underlying memory blocks is retained by the caller, which must guarantee
        that they remain valid until the handler is called.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code error,               // Result of operation.
            std::size_t bytes_transferred   // Number of bytes written.
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @note The `async_write_some` operation may not transmit all of the requested
        number of bytes. Consider using the function `net::async_write` if you need
        to ensure that the requested amount of data is sent before the asynchronous
        operation completes.
    */
    template<
        class ConstBufferSequence,
        BOOST_BEAST_ASYNC_TPARAM2 WriteHandler =
            net::default_completion_token_t<Executor>
    >
    BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
    async_write_some(
        ConstBufferSequence const& buffers,
        WriteHandler&& handler =
            net::default_completion_token_t<Executor>{});
};

} // beast
} // boost

#include <boost/beast/core/impl/basic_stream.hpp>

#endif
