//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_READ_HPP
#define BOOST_BEAST_DETAIL_READ_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/detail/is_invocable.hpp>
#include <boost/asio/async_result.hpp>
#include <cstdlib>

namespace boost {
namespace beast {
namespace detail {

//------------------------------------------------------------------------------

/** Read data into a dynamic buffer from a stream until a condition is met.
    
    This function is used to read from a stream into a dynamic buffer until
    a condition is met. The call will block until one of the following is true:
    
    @li The specified dynamic buffer sequence is full (that is, it has
        reached its currently configured maximum size).

    @li The `completion_condition` function object returns 0.

    This operation is implemented in terms of zero or more calls to the
    stream's `read_some` function.
    
    @param stream The stream from which the data is to be read. The type
    must support the <em>SyncReadStream</em> requirements.
    
    @param buffer The dynamic buffer sequence into which the data will be read.
 
    @param completion_condition The function object to be called to determine
    whether the read operation is complete. The function object must be invocable
    with this signature:
    @code
    std::size_t
    completion_condition(
        // Modifiable result of latest read_some operation. 
        error_code& ec,
    
        // Number of bytes transferred so far.
        std::size_t bytes_transferred

        // The dynamic buffer used to store the bytes read
        DynamicBuffer& buffer
    );
    @endcode
    A non-zero return value indicates the maximum number of bytes to be read on
    the next call to the stream's `read_some` function. A return value of 0
    from the completion condition indicates that the read operation is complete;
    in this case the optionally modifiable error passed to the completion
    condition will be delivered to the caller as an exception.

    @returns The number of bytes transferred from the stream.

    @throws net::system_error Thrown on failure.
*/    
template<
    class SyncReadStream,
    class DynamicBuffer,
    class CompletionCondition
#if ! BOOST_BEAST_DOXYGEN
    , class = typename std::enable_if<
        is_sync_read_stream<SyncReadStream>::value &&
        net::is_dynamic_buffer<DynamicBuffer>::value &&
        detail::is_invocable<CompletionCondition,
            void(error_code&, std::size_t, DynamicBuffer&)>::value
    >::type
#endif
>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    CompletionCondition completion_condition);

/** Read data into a dynamic buffer from a stream until a condition is met.
    
    This function is used to read from a stream into a dynamic buffer until
    a condition is met. The call will block until one of the following is true:
    
    @li The specified dynamic buffer sequence is full (that is, it has
        reached its currently configured maximum size).

    @li The `completion_condition` function object returns 0.
     
    This operation is implemented in terms of zero or more calls to the
    stream's `read_some` function.
    
    @param stream The stream from which the data is to be read. The type
    must support the <em>SyncReadStream</em> requirements.
    
    @param buffer The dynamic buffer sequence into which the data will be read.
 
    @param completion_condition The function object to be called to determine
    whether the read operation is complete. The function object must be invocable
    with this signature:
    @code
    std::size_t
    completion_condition(
        // Modifiable result of latest read_some operation. 
        error_code& ec,
    
        // Number of bytes transferred so far.
        std::size_t bytes_transferred

        // The dynamic buffer used to store the bytes read
        DynamicBuffer& buffer
    );
    @endcode
    A non-zero return value indicates the maximum number of bytes to be read on
    the next call to the stream's `read_some` function. A return value of 0
    from the completion condition indicates that the read operation is complete;
    in this case the optionally modifiable error passed to the completion
    condition will be delivered to the caller.

    @returns The number of bytes transferred from the stream.
*/    
template<
    class SyncReadStream,
    class DynamicBuffer,
    class CompletionCondition
#if ! BOOST_BEAST_DOXYGEN
    , class = typename std::enable_if<
        is_sync_read_stream<SyncReadStream>::value &&
        net::is_dynamic_buffer<DynamicBuffer>::value &&
        detail::is_invocable<CompletionCondition,
            void(error_code&, std::size_t, DynamicBuffer&)>::value
    >::type
#endif
>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    CompletionCondition completion_condition,
    error_code& ec);

/** Asynchronously read data into a dynamic buffer from a stream until a condition is met.
    
    This function is used to asynchronously read from a stream into a dynamic
    buffer until a condition is met. The function call always returns immediately.
    The asynchronous operation will continue until one of the following is true:
    
    @li The specified dynamic buffer sequence is full (that is, it has
        reached its currently configured maximum size).

    @li The `completion_condition` function object returns 0.
    
    This operation is implemented in terms of zero or more calls to the stream's
    `async_read_some` function, and is known as a <em>composed operation</em>. The
    program must ensure that the stream performs no other read operations (such
    as `async_read`, the stream's `async_read_some` function, or any other composed
    operations that perform reads) until this operation completes.
    
    @param stream The stream from which the data is to be read. The type must
    support the <em>AsyncReadStream</em> requirements.
    
    @param buffer The dynamic buffer sequence into which the data will be read.
    Ownership of the object is retained by the caller, which must guarantee
    that it remains valid until the handler is called.

    @param completion_condition The function object to be called to determine
    whether the read operation is complete. The function object must be invocable
    with this signature:
    @code
    std::size_t
    completion_condition(
        // Modifiable result of latest async_read_some operation. 
        error_code& ec,

        // Number of bytes transferred so far.
        std::size_t bytes_transferred,

        // The dynamic buffer used to store the bytes read
        DynamicBuffer& buffer
    );
    @endcode
    A non-zero return value indicates the maximum number of bytes to be read on
    the next call to the stream's `async_read_some` function. A return value of 0
    from the completion condition indicates that the read operation is complete;
    in this case the optionally modifiable error passed to the completion
    condition will be delivered to the completion handler.

    @param handler The completion handler to invoke when the operation
    completes. The implementation takes ownership of the handler by
    performing a decay-copy. The equivalent function signature of
    the handler must be:
    @code
    void
    handler(
        error_code const& ec,               // Result of operation.

        std::size_t bytes_transferred       // Number of bytes copied into
                                            // the dynamic buffer. If an error
                                            // occurred, this will be the number
                                            // of bytes successfully transferred
                                            // prior to the error.
    );
    @endcode
    Regardless of whether the asynchronous operation completes
    immediately or not, the handler will not be invoked from within
    this function. Invocation of the handler will be performed in a
    manner equivalent to using `net::post`.
*/
template<
    class AsyncReadStream,
    class DynamicBuffer,
    class CompletionCondition,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler
#if ! BOOST_BEAST_DOXYGEN
    , class = typename std::enable_if<
        is_async_read_stream<AsyncReadStream>::value &&
        net::is_dynamic_buffer<DynamicBuffer>::value &&
        detail::is_invocable<CompletionCondition,
            void(error_code&, std::size_t, DynamicBuffer&)>::value
    >::type
#endif
>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    CompletionCondition&& completion_condition,
    ReadHandler&& handler);

} // detail
} // beast
} // boost

#include <boost/beast/core/detail/impl/read.hpp>

#endif
