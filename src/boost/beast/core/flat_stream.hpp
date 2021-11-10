//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_FLAT_STREAM_HPP
#define BOOST_BEAST_CORE_FLAT_STREAM_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/detail/flat_stream.hpp>
#include <boost/asio/async_result.hpp>
#include <cstdlib>
#include <utility>

namespace boost {
namespace beast {

/** Stream wrapper to improve write performance.

    This wrapper flattens writes for buffer sequences having length
    greater than 1 and total size below a predefined amount, using
    a dynamic memory allocation. It is primarily designed to overcome
    a performance limitation of the current version of `net::ssl::stream`,
    which does not use OpenSSL's scatter/gather interface for its
    low-level read some and write some operations.

    It is normally not necessary to use this class directly if you
    are already using @ref ssl_stream. The following examples shows
    how to use this class with the ssl stream that comes with
    networking:

    @par Example

    To use the @ref flat_stream template with SSL streams, declare
    a variable of the correct type. Parameters passed to the constructor
    will be forwarded to the next layer's constructor:

    @code
        flat_stream<net::ssl::stream<ip::tcp::socket>> fs{ioc, ctx};
    @endcode
    Alternatively you can write
    @code
        ssl::stream<ip::tcp::socket> ss{ioc, ctx};
        flat_stream<net::ssl::stream<ip::tcp::socket>&> fs{ss};
    @endcode

    The resulting stream may be passed to any stream algorithms which
    operate on synchronous or asynchronous read or write streams,
    examples include:
    
    @li `net::read`, `net::async_read`

    @li `net::write`, `net::async_write`

    @li `net::read_until`, `net::async_read_until`

    The stream may also be used as a template parameter in other
    stream wrappers, such as for websocket:
    @code
        websocket::stream<flat_stream<net::ssl::stream<ip::tcp::socket>>> ws{ioc, ctx};
    @endcode

    @tparam NextLayer The type representing the next layer, to which
    data will be read and written during operations. For synchronous
    operations, the type must support the @b SyncStream concept. For
    asynchronous operations, the type must support the @b AsyncStream
    concept. This type will usually be some variation of
    `net::ssl::stream`.

    @par Concepts
        @li SyncStream
        @li AsyncStream

    @see
        @li https://github.com/boostorg/asio/issues/100
        @li https://github.com/boostorg/beast/issues/1108
        @li https://stackoverflow.com/questions/38198638/openssl-ssl-write-from-multiple-buffers-ssl-writev
        @li https://stackoverflow.com/questions/50026167/performance-drop-on-port-from-beast-1-0-0-b66-to-boost-1-67-0-beast
*/
template<class NextLayer>
class flat_stream
#if ! BOOST_BEAST_DOXYGEN
    : private detail::flat_stream_base
#endif
{
    NextLayer stream_;
    flat_buffer buffer_;

    BOOST_STATIC_ASSERT(has_get_executor<NextLayer>::value);

    struct ops;

    template<class ConstBufferSequence>
    std::size_t
    stack_write_some(
        std::size_t size,
        ConstBufferSequence const& buffers,
        error_code& ec);

public:
    /// The type of the next layer.
    using next_layer_type =
        typename std::remove_reference<NextLayer>::type;

    /// The type of the executor associated with the object.
    using executor_type = beast::executor_type<next_layer_type>;

    flat_stream(flat_stream&&) = default;
    flat_stream(flat_stream const&) = default;
    flat_stream& operator=(flat_stream&&) = default;
    flat_stream& operator=(flat_stream const&) = default;

    /** Destructor

        The treatment of pending operations will be the same as that
        of the next layer.
    */
    ~flat_stream() = default;

    /** Constructor

        Arguments, if any, are forwarded to the next layer's constructor.
    */
    template<class... Args>
    explicit
    flat_stream(Args&&... args);

    //--------------------------------------------------------------------------

    /** Get the executor associated with the object.
    
        This function may be used to obtain the executor object that the
        stream uses to dispatch handlers for asynchronous operations.

        @return A copy of the executor that stream will use to dispatch handlers.
    */
    executor_type
    get_executor() noexcept
    {
        return stream_.get_executor();
    }

    /** Get a reference to the next layer

        This function returns a reference to the next layer
        in a stack of stream layers.

        @return A reference to the next layer in the stack of
        stream layers.
    */
    next_layer_type&
    next_layer() noexcept
    {
        return stream_;
    }

    /** Get a reference to the next layer

        This function returns a reference to the next layer in a
        stack of stream layers.

        @return A reference to the next layer in the stack of
        stream layers.
    */
    next_layer_type const&
    next_layer() const noexcept
    {
        return stream_;
    }

    //--------------------------------------------------------------------------

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
    read_some(
        MutableBufferSequence const& buffers,
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
            error_code const& error,        // Result of operation.
            std::size_t bytes_transferred   // Number of bytes read.
        );
        @endcode
        Regardless of whether the asynchronous operation completes
        immediately or not, the handler will not be invoked from within
        this function. Invocation of the handler will be performed in a
        manner equivalent to using `net::post`.
        
        @note The `read_some` operation may not read all of the requested number of
        bytes. Consider using the function `net::async_read` if you need
        to ensure that the requested amount of data is read before the asynchronous
        operation completes.
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
        ConstBufferSequence const& buffers,
        error_code& ec);

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

#include <boost/beast/core/impl/flat_stream.hpp>

#endif
