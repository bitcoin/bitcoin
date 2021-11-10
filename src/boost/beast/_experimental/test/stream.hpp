//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_TEST_STREAM_HPP
#define BOOST_BEAST_TEST_STREAM_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/role.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/_experimental/test/fail_count.hpp>
#include <boost/beast/_experimental/test/detail/stream_state.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <condition_variable>
#include <limits>
#include <memory>
#include <mutex>
#include <utility>

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
namespace test {

/** A two-way socket useful for unit testing

    An instance of this class simulates a traditional socket,
    while also providing features useful for unit testing.
    Each endpoint maintains an independent buffer called
    the input area. Writes from one endpoint append data
    to the peer's pending input area. When an endpoint performs
    a read and data is present in the input area, the data is
    delivered to the blocking or asynchronous operation. Otherwise
    the operation is blocked or deferred until data is made
    available, or until the endpoints become disconnected.

    These streams may be used anywhere an algorithm accepts a
    reference to a synchronous or asynchronous read or write
    stream. It is possible to use a test stream in a call to
    `net::read_until`, or in a call to
    @ref boost::beast::http::async_write for example.

    As with Boost.Asio I/O objects, a @ref stream constructs
    with a reference to the `net::io_context` to use for
    handling asynchronous I/O. For asynchronous operations, the
    stream follows the same rules as a traditional asio socket
    with respect to how completion handlers for asynchronous
    operations are performed.

    To facilitate testing, these streams support some additional
    features:

    @li The input area, represented by a @ref beast::basic_flat_buffer,
    may be directly accessed by the caller to inspect the contents
    before or after the remote endpoint writes data. This allows
    a unit test to verify that the received data matches.

    @li Data may be manually appended to the input area. This data
    will delivered in the next call to
    @ref stream::read_some or @ref stream::async_read_some.
    This allows predefined test vectors to be set up for testing
    read algorithms.

    @li The stream may be constructed with a fail count. The
    stream will eventually fail with a predefined error after a
    certain number of operations, where the number of operations
    is controlled by the test. When a test loops over a range of
    operation counts, it is possible to exercise every possible
    point of failure in the algorithm being tested. When used
    correctly the technique allows the tests to reach a high
    percentage of code coverage.

    @par Thread Safety
        @e Distinct @e objects: Safe.@n
        @e Shared @e objects: Unsafe.
        The application must also ensure that all asynchronous
        operations are performed within the same implicit or explicit strand.

    @par Concepts
        @li <em>SyncReadStream</em>
        @li <em>SyncWriteStream</em>
        @li <em>AsyncReadStream</em>
        @li <em>AsyncWriteStream</em>
*/
template<class Executor = net::any_io_executor>
class basic_stream;

template<class Executor>
void
teardown(
    role_type,
    basic_stream<Executor>& s,
    boost::system::error_code& ec);

template<class Executor, class TeardownHandler>
void
async_teardown(
    role_type role,
    basic_stream<Executor>& s,
    TeardownHandler&& handler);

template<class Executor>
class basic_stream
{
public:
    /// The type of the executor associated with the object.
    using executor_type =
        Executor;

      /// Rebinds the socket type to another executor.
    template <typename Executor1>
    struct rebind_executor
    {
        /// The socket type when rebound to the specified executor.
        typedef basic_stream<Executor1> other;
    };

private:
    template<class Executor2>
    friend class basic_stream;

    boost::shared_ptr<detail::stream_state> in_;
    boost::weak_ptr<detail::stream_state> out_;

    template<class Handler, class Buffers>
    class read_op;

    struct run_read_op;
    struct run_write_op;

    static
    void
    initiate_read(
        boost::shared_ptr<detail::stream_state> const& in,
        std::unique_ptr<detail::stream_read_op_base>&& op,
        std::size_t buf_size);

#if ! BOOST_BEAST_DOXYGEN
    // boost::asio::ssl::stream needs these
    // DEPRECATED
    template<class>
    friend class boost::asio::ssl::stream;
    // DEPRECATED
    using lowest_layer_type = basic_stream;
    // DEPRECATED
    lowest_layer_type&
    lowest_layer() noexcept
    {
        return *this;
    }
    // DEPRECATED
    lowest_layer_type const&
    lowest_layer() const noexcept
    {
        return *this;
    }
#endif

public:
    using buffer_type = flat_buffer;

    /** Destructor

        If an asynchronous read operation is pending, it will
        simply be discarded with no notification to the completion
        handler.

        If a connection is established while the stream is destroyed,
        the peer will see the error `net::error::connection_reset`
        when performing any reads or writes.
    */
    ~basic_stream();

    /** Move Constructor

        Moving the stream while asynchronous operations are pending
        results in undefined behavior.
    */
    basic_stream(basic_stream&& other);

    /** Move Constructor

        Moving the stream while asynchronous operations are pending
        results in undefined behavior.
    */
    template<class Executor2>
    basic_stream(basic_stream<Executor2>&& other)
    : in_(std::move(other.in_))
    , out_(std::move(other.out_))
    {
        assert(in_->exec.target_type() == typeid(Executor2));
        in_->exec = executor_type(*in_->exec.template target<Executor2>());
    }

    /** Move Assignment

        Moving the stream while asynchronous operations are pending
        results in undefined behavior.
    */
    basic_stream&
    operator=(basic_stream&& other);

    template<class Executor2>
    basic_stream&
    operator==(basic_stream<Executor2>&& other);

    /** Construct a stream

        The stream will be created in a disconnected state.

        @param ioc The `io_context` object that the stream will use to
        dispatch handlers for any asynchronous operations.
    */
    template <typename ExecutionContext>
    explicit basic_stream(ExecutionContext& context,
        typename std::enable_if<
            std::is_convertible<ExecutionContext&, net::execution_context&>::value
        >::type* = 0)
    : basic_stream(context.get_executor())
    {
    }

    /** Construct a stream

        The stream will be created in a disconnected state.

        @param exec The `executor` object that the stream will use to
        dispatch handlers for any asynchronous operations.
    */
    explicit
    basic_stream(executor_type exec);

    /** Construct a stream

        The stream will be created in a disconnected state.

        @param ioc The `io_context` object that the stream will use to
        dispatch handlers for any asynchronous operations.

        @param fc The @ref fail_count to associate with the stream.
        Each I/O operation performed on the stream will increment the
        fail count.  When the fail count reaches its internal limit,
        a simulated failure error will be raised.
    */
    basic_stream(
        net::io_context& ioc,
        fail_count& fc);

    /** Construct a stream

        The stream will be created in a disconnected state.

        @param ioc The `io_context` object that the stream will use to
        dispatch handlers for any asynchronous operations.

        @param s A string which will be appended to the input area, not
        including the null terminator.
    */
    basic_stream(
        net::io_context& ioc,
        string_view s);

    /** Construct a stream

        The stream will be created in a disconnected state.

        @param ioc The `io_context` object that the stream will use to
        dispatch handlers for any asynchronous operations.

        @param fc The @ref fail_count to associate with the stream.
        Each I/O operation performed on the stream will increment the
        fail count.  When the fail count reaches its internal limit,
        a simulated failure error will be raised.

        @param s A string which will be appended to the input area, not
        including the null terminator.
    */
    basic_stream(
        net::io_context& ioc,
        fail_count& fc,
        string_view s);

    /// Establish a connection
    void
    connect(basic_stream& remote);

    /// Return the executor associated with the object.
    executor_type
    get_executor() noexcept;

    /// Set the maximum number of bytes returned by read_some
    void
    read_size(std::size_t n) noexcept
    {
        in_->read_max = n;
    }

    /// Set the maximum number of bytes returned by write_some
    void
    write_size(std::size_t n) noexcept
    {
        in_->write_max = n;
    }

    /// Direct input buffer access
    buffer_type&
    buffer() noexcept
    {
        return in_->b;
    }

    /// Returns a string view representing the pending input data
    string_view
    str() const;

    /// Appends a string to the pending input data
    void
    append(string_view s);

    /// Clear the pending input area
    void
    clear();

    /// Return the number of reads
    std::size_t
    nread() const noexcept
    {
        return in_->nread;
    }

    /// Return the number of bytes read
    std::size_t
    nread_bytes() const noexcept
    {
        return in_->nread_bytes;
    }

    /// Return the number of writes
    std::size_t
    nwrite() const noexcept
    {
        return in_->nwrite;
    }

    /// Return the number of bytes written
    std::size_t
    nwrite_bytes() const noexcept
    {
        return in_->nwrite_bytes;
    }

    /** Close the stream.

        The other end of the connection will see
        `error::eof` after reading all the remaining data.
    */
    void
    close();

    /** Close the other end of the stream.

        This end of the connection will see
        `error::eof` after reading all the remaining data.
    */
    void
    close_remote();

    /** Read some data from the stream.

        This function is used to read data from the stream. The function call will
        block until one or more bytes of data has been read successfully, or until
        an error occurs.

        @param buffers The buffers into which the data will be read.

        @returns The number of bytes read.

        @throws boost::system::system_error Thrown on failure.

        @note The `read_some` operation may not read all of the requested number of
        bytes. Consider using the function `net::read` if you need to ensure
        that the requested amount of data is read before the blocking operation
        completes.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers);

    /** Read some data from the stream.

        This function is used to read data from the stream. The function call will
        block until one or more bytes of data has been read successfully, or until
        an error occurs.

        @param buffers The buffers into which the data will be read.

        @param ec Set to indicate what error occurred, if any.

        @returns The number of bytes read.

        @note The `read_some` operation may not read all of the requested number of
        bytes. Consider using the function `net::read` if you need to ensure
        that the requested amount of data is read before the blocking operation
        completes.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers,
        error_code& ec);

    /** Start an asynchronous read.

        This function is used to asynchronously read one or more bytes of data from
        the stream. The function call always returns immediately.

        @param buffers The buffers into which the data will be read. Although the
        buffers object may be copied as necessary, ownership of the underlying
        buffers is retained by the caller, which must guarantee that they remain
        valid until the handler is called.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec,           // Result of operation.
            std::size_t bytes_transferred   // Number of bytes read.
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @note The `async_read_some` operation may not read all of the requested number of
        bytes. Consider using the function `net::async_read` if you need
        to ensure that the requested amount of data is read before the asynchronous
        operation completes.
    */
    template<
        class MutableBufferSequence,
        BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, std::size_t)) ReadHandler
            BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
    BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(error_code, std::size_t))
    async_read_some(
        MutableBufferSequence const& buffers,
        ReadHandler&& handler BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type));

    /** Write some data to the stream.

        This function is used to write data on the stream. The function call will
        block until one or more bytes of data has been written successfully, or
        until an error occurs.

        @param buffers The data to be written.

        @returns The number of bytes written.

        @throws boost::system::system_error Thrown on failure.

        @note The `write_some` operation may not transmit all of the data to the
        peer. Consider using the function `net::write` if you need to
        ensure that all data is written before the blocking operation completes.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers);

    /** Write some data to the stream.

        This function is used to write data on the stream. The function call will
        block until one or more bytes of data has been written successfully, or
        until an error occurs.

        @param buffers The data to be written.

        @param ec Set to indicate what error occurred, if any.

        @returns The number of bytes written.

        @note The `write_some` operation may not transmit all of the data to the
        peer. Consider using the function `net::write` if you need to
        ensure that all data is written before the blocking operation completes.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(
        ConstBufferSequence const& buffers, error_code& ec);

    /** Start an asynchronous write.

        This function is used to asynchronously write one or more bytes of data to
        the stream. The function call always returns immediately.

        @param buffers The data to be written to the stream. Although the buffers
        object may be copied as necessary, ownership of the underlying buffers is
        retained by the caller, which must guarantee that they remain valid until
        the handler is called.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& ec,           // Result of operation.
            std::size_t bytes_transferred   // Number of bytes written.
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.

        @note The `async_write_some` operation may not transmit all of the data to
        the peer. Consider using the function `net::async_write` if you need
        to ensure that all data is written before the asynchronous operation completes.
    */
    template<
        class ConstBufferSequence,
        BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, std::size_t)) WriteHandler
            BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
    BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void(error_code, std::size_t))
    async_write_some(
        ConstBufferSequence const& buffers,
        WriteHandler&& handler BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(executor_type)
        );

#if ! BOOST_BEAST_DOXYGEN
    friend
    void
    teardown<>(
        role_type,
        basic_stream& s,
        boost::system::error_code& ec);

    template<class Ex2, class TeardownHandler>
    friend
    void
    async_teardown(
        role_type role,
        basic_stream<Ex2>& s,
        TeardownHandler&& handler);
#endif
};

#if ! BOOST_BEAST_DOXYGEN
template<class Executor>
void
beast_close_socket(basic_stream<Executor>& s)
{
    s.close();
}
#endif

#if BOOST_BEAST_DOXYGEN
/** Return a new stream connected to the given stream

    @param to The stream to connect to.

    @param args Optional arguments forwarded to the new stream's constructor.

    @return The new, connected stream.
*/
template<class Executor>
template<class... Args>
bascic_stream
connect(basic_stream& to, Args&&... args);

#else
template<class Executor>
basic_stream<Executor>
connect(basic_stream<Executor>& to);

template<class Executor>
void
connect(basic_stream<Executor>& s1, basic_stream<Executor>& s2);

template<class Executor, class Arg1, class... ArgN>
basic_stream<Executor>
connect(basic_stream<Executor>& to, Arg1&& arg1, ArgN&&... argn);
#endif

using stream = basic_stream<>;

} // test
} // beast
} // boost

#include <boost/beast/_experimental/test/impl/stream.hpp>
//#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/_experimental/test/impl/stream.ipp>
//#endif

#endif
