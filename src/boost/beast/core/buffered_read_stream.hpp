//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_BUFFERED_READ_STREAM_HPP
#define BOOST_BEAST_BUFFERED_READ_STREAM_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <cstdint>
#include <utility>

namespace boost {
namespace beast {

/** A <em>Stream</em> with attached <em>DynamicBuffer</em> to buffer reads.

    This wraps a <em>Stream</em> implementation so that calls to write are
    passed through to the underlying stream, while calls to read will
    first consume the input sequence stored in a <em>DynamicBuffer</em> which
    is part of the object.

    The use-case for this class is different than that of the
    `net::buffered_read_stream`. It is designed to facilitate
    the use of `net::read_until`, and to allow buffers
    acquired during detection of handshakes to be made transparently
    available to callers. A hypothetical implementation of the
    buffered version of `net::ssl::stream::async_handshake`
    could make use of this wrapper.

    Uses:

    @li Transparently leave untouched input acquired in calls
      to `net::read_until` behind for subsequent callers.

    @li "Preload" a stream with handshake input data acquired
      from other sources.

    Example:
    @code
    // Process the next HTTP header on the stream,
    // leaving excess bytes behind for the next call.
    //
    template<class Stream, class DynamicBuffer>
    void process_http_message(
        buffered_read_stream<Stream, DynamicBuffer>& stream)
    {
        // Read up to and including the end of the HTTP
        // header, leaving the sequence in the stream's
        // buffer. read_until may read past the end of the
        // headers; the return value will include only the
        // part up to the end of the delimiter.
        //
        std::size_t bytes_transferred =
            net::read_until(
                stream.next_layer(), stream.buffer(), "\r\n\r\n");

        // Use buffers_prefix() to limit the input
        // sequence to only the data up to and including
        // the trailing "\r\n\r\n".
        //
        auto header_buffers = buffers_prefix(
            bytes_transferred, stream.buffer().data());

        ...

        // Discard the portion of the input corresponding
        // to the HTTP headers.
        //
        stream.buffer().consume(bytes_transferred);

        // Everything we read from the stream
        // is part of the content-body.
    }
    @endcode

    @tparam Stream The type of stream to wrap.

    @tparam DynamicBuffer The type of stream buffer to use.
*/
template<class Stream, class DynamicBuffer>
class buffered_read_stream
{
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");

    struct ops;

    DynamicBuffer buffer_;
    std::size_t capacity_ = 0;
    Stream next_layer_;

public:
    /// The type of the internal buffer
    using buffer_type = DynamicBuffer;

    /// The type of the next layer.
    using next_layer_type =
        typename std::remove_reference<Stream>::type;

    /** Move constructor.

        @note The behavior of move assignment on or from streams
        with active or pending operations is undefined.
    */
    buffered_read_stream(buffered_read_stream&&) = default;

    /** Move assignment.

        @note The behavior of move assignment on or from streams
        with active or pending operations is undefined.
    */
    buffered_read_stream& operator=(buffered_read_stream&&) = default;

    /** Construct the wrapping stream.

        @param args Parameters forwarded to the `Stream` constructor.
    */
    template<class... Args>
    explicit
    buffered_read_stream(Args&&... args);

    /// Get a reference to the next layer.
    next_layer_type&
    next_layer() noexcept
    {
        return next_layer_;
    }

    /// Get a const reference to the next layer.
    next_layer_type const&
    next_layer() const noexcept
    {
        return next_layer_;
    }
    
    using executor_type =
        beast::executor_type<next_layer_type>;

    /** Get the executor associated with the object.
    
        This function may be used to obtain the executor object that the stream
        uses to dispatch handlers for asynchronous operations.

        @return A copy of the executor that stream will use to dispatch handlers.
    */
    executor_type
    get_executor() noexcept
    {
        return next_layer_.get_executor();
    }

    /** Access the internal buffer.

        The internal buffer is returned. It is possible for the
        caller to break invariants with this function. For example,
        by causing the internal buffer size to increase beyond
        the caller defined maximum.
    */
    DynamicBuffer&
    buffer() noexcept
    {
        return buffer_;
    }

    /// Access the internal buffer
    DynamicBuffer const&
    buffer() const noexcept
    {
        return buffer_;
    }

    /** Set the maximum buffer size.

        This changes the maximum size of the internal buffer used
        to hold read data. No bytes are discarded by this call. If
        the buffer size is set to zero, no more data will be buffered.

        Thread safety:
            The caller is responsible for making sure the call is
            made from the same implicit or explicit strand.

        @param size The number of bytes in the read buffer.

        @note This is a soft limit. If the new maximum size is smaller
        than the amount of data in the buffer, no bytes are discarded.
    */
    void
    capacity(std::size_t size) noexcept
    {
        capacity_ = size;
    }

    /** Read some data from the stream.

        This function is used to read data from the stream.
        The function call will block until one or more bytes of
        data has been read successfully, or until an error occurs.

        @param buffers One or more buffers into which the data will be read.

        @return The number of bytes read.

        @throws system_error Thrown on failure.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers);

    /** Read some data from the stream.

        This function is used to read data from the stream.
        The function call will block until one or more bytes of
        data has been read successfully, or until an error occurs.

        @param buffers One or more buffers into which the data will be read.

        @param ec Set to the error, if any occurred.

        @return The number of bytes read, or 0 on error.
    */
    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers,
        error_code& ec);

    /** Start an asynchronous read.

        This function is used to asynchronously read data from
        the stream. The function call always returns immediately.

        @param buffers One or more buffers into which the data
        will be read. Although the buffers object may be copied
        as necessary, ownership of the underlying memory blocks
        is retained by the caller, which must guarantee that they
        remain valid until the handler is called.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& error,      // result of operation
            std::size_t bytes_transferred // number of bytes transferred
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
            net::default_completion_token_t<executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
    async_read_some(
        MutableBufferSequence const& buffers,
        ReadHandler&& handler =
            net::default_completion_token_t<executor_type>{});

    /** Write some data to the stream.

        This function is used to write data to the stream.
        The function call will block until one or more bytes of the
        data has been written successfully, or until an error occurs.

        @param buffers One or more data buffers to be written to the stream.

        @return The number of bytes written.

        @throws system_error Thrown on failure.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers)
    {
        static_assert(is_sync_write_stream<next_layer_type>::value,
            "SyncWriteStream type requirements not met");
        return next_layer_.write_some(buffers);
    }

    /** Write some data to the stream.

        This function is used to write data to the stream.
        The function call will block until one or more bytes of the
        data has been written successfully, or until an error occurs.

        @param buffers One or more data buffers to be written to the stream.

        @param ec Set to the error, if any occurred.

        @return The number of bytes written.
    */
    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers,
        error_code& ec)
    {
        static_assert(is_sync_write_stream<next_layer_type>::value,
            "SyncWriteStream type requirements not met");
        return next_layer_.write_some(buffers, ec);
    }

    /** Start an asynchronous write.

        This function is used to asynchronously write data from
        the stream. The function call always returns immediately.

        @param buffers One or more data buffers to be written to
        the stream. Although the buffers object may be copied as
        necessary, ownership of the underlying memory blocks is
        retained by the caller, which must guarantee that they
        remain valid until the handler is called.

        @param handler The completion handler to invoke when the operation
        completes. The implementation takes ownership of the handler by
        performing a decay-copy. The equivalent function signature of
        the handler must be:
        @code
        void handler(
            error_code const& error,      // result of operation
            std::size_t bytes_transferred // number of bytes transferred
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
            net::default_completion_token_t<executor_type>>
    BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
    async_write_some(
        ConstBufferSequence const& buffers,
        WriteHandler&& handler =
            net::default_completion_token_t<executor_type>{});
};

} // beast
} // boost

#include <boost/beast/core/impl/buffered_read_stream.hpp>

#endif
