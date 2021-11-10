//
// read_at.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_READ_AT_HPP
#define BOOST_ASIO_READ_AT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/cstdint.hpp>
#include <boost/asio/error.hpp>

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
# include <boost/asio/basic_streambuf_fwd.hpp>
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/**
 * @defgroup read_at boost::asio::read_at
 *
 * @brief The @c read_at function is a composed operation that reads a certain
 * amount of data at the specified offset before returning.
 */
/*@{*/

/// Attempt to read a certain amount of data at the specified offset before
/// returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * random access device at the specified offset. The call will block until one
 * of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the SyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * device.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::read_at(d, 42, boost::asio::buffer(data, size)); @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::read_at(
 *     d, 42, buffers,
 *     boost::asio::transfer_all()); @endcode
 */
template <typename SyncRandomAccessReadDevice, typename MutableBufferSequence>
std::size_t read_at(SyncRandomAccessReadDevice& d,
    uint64_t offset, const MutableBufferSequence& buffers);

/// Attempt to read a certain amount of data at the specified offset before
/// returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * random access device at the specified offset. The call will block until one
 * of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the SyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * device.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes transferred.
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::read_at(d, 42,
 *     boost::asio::buffer(data, size), ec); @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::read_at(
 *     d, 42, buffers,
 *     boost::asio::transfer_all(), ec); @endcode
 */
template <typename SyncRandomAccessReadDevice, typename MutableBufferSequence>
std::size_t read_at(SyncRandomAccessReadDevice& d,
    uint64_t offset, const MutableBufferSequence& buffers,
    boost::system::error_code& ec);

/// Attempt to read a certain amount of data at the specified offset before
/// returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * random access device at the specified offset. The call will block until one
 * of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the SyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * device.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest read_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the device's read_some_at function.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::read_at(d, 42, boost::asio::buffer(data, size),
 *     boost::asio::transfer_at_least(32)); @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 */
template <typename SyncRandomAccessReadDevice, typename MutableBufferSequence,
    typename CompletionCondition>
std::size_t read_at(SyncRandomAccessReadDevice& d,
    uint64_t offset, const MutableBufferSequence& buffers,
    CompletionCondition completion_condition);

/// Attempt to read a certain amount of data at the specified offset before
/// returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * random access device at the specified offset. The call will block until one
 * of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the SyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * device.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest read_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the device's read_some_at function.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes read. If an error occurs, returns the total
 * number of bytes successfully transferred prior to the error.
 */
template <typename SyncRandomAccessReadDevice, typename MutableBufferSequence,
    typename CompletionCondition>
std::size_t read_at(SyncRandomAccessReadDevice& d,
    uint64_t offset, const MutableBufferSequence& buffers,
    CompletionCondition completion_condition, boost::system::error_code& ec);

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if !defined(BOOST_ASIO_NO_IOSTREAM)

/// Attempt to read a certain amount of data at the specified offset before
/// returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * random access device at the specified offset. The call will block until one
 * of the following conditions is true:
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the SyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param b The basic_streambuf object into which the data will be read.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::read_at(
 *     d, 42, b,
 *     boost::asio::transfer_all()); @endcode
 */
template <typename SyncRandomAccessReadDevice, typename Allocator>
std::size_t read_at(SyncRandomAccessReadDevice& d,
    uint64_t offset, basic_streambuf<Allocator>& b);

/// Attempt to read a certain amount of data at the specified offset before
/// returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * random access device at the specified offset. The call will block until one
 * of the following conditions is true:
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the SyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param b The basic_streambuf object into which the data will be read.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes transferred.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::read_at(
 *     d, 42, b,
 *     boost::asio::transfer_all(), ec); @endcode
 */
template <typename SyncRandomAccessReadDevice, typename Allocator>
std::size_t read_at(SyncRandomAccessReadDevice& d,
    uint64_t offset, basic_streambuf<Allocator>& b,
    boost::system::error_code& ec);

/// Attempt to read a certain amount of data at the specified offset before
/// returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * random access device at the specified offset. The call will block until one
 * of the following conditions is true:
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the SyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param b The basic_streambuf object into which the data will be read.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest read_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the device's read_some_at function.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 */
template <typename SyncRandomAccessReadDevice, typename Allocator,
    typename CompletionCondition>
std::size_t read_at(SyncRandomAccessReadDevice& d,
    uint64_t offset, basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition);

/// Attempt to read a certain amount of data at the specified offset before
/// returning.
/**
 * This function is used to read a certain number of bytes of data from a
 * random access device at the specified offset. The call will block until one
 * of the following conditions is true:
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the SyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param b The basic_streambuf object into which the data will be read.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest read_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the device's read_some_at function.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes read. If an error occurs, returns the total
 * number of bytes successfully transferred prior to the error.
 */
template <typename SyncRandomAccessReadDevice, typename Allocator,
    typename CompletionCondition>
std::size_t read_at(SyncRandomAccessReadDevice& d,
    uint64_t offset, basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition, boost::system::error_code& ec);

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)

/*@}*/
/**
 * @defgroup async_read_at boost::asio::async_read_at
 *
 * @brief The @c async_read_at function is a composed asynchronous operation
 * that reads a certain amount of data at the specified offset.
 */
/*@{*/

/// Start an asynchronous operation to read a certain amount of data at the
/// specified offset.
/**
 * This function is used to asynchronously read a certain number of bytes of
 * data from a random access device at the specified offset. The function call
 * always returns immediately. The asynchronous operation will continue until
 * one of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * async_read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the AsyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * device. Although the buffers object may be copied as necessary, ownership of
 * the underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes copied into the buffers. If an error
 *   // occurred, this will be the number of bytes successfully
 *   // transferred prior to the error.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code
 * boost::asio::async_read_at(d, 42, boost::asio::buffer(data, size), handler);
 * @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::async_read_at(
 *     d, 42, buffers,
 *     boost::asio::transfer_all(),
 *     handler); @endcode
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncRandomAccessReadDevice type's
 * async_read_some_at operation.
 */
template <typename AsyncRandomAccessReadDevice, typename MutableBufferSequence,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncRandomAccessReadDevice::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_at(AsyncRandomAccessReadDevice& d, uint64_t offset,
    const MutableBufferSequence& buffers,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncRandomAccessReadDevice::executor_type));

/// Start an asynchronous operation to read a certain amount of data at the
/// specified offset.
/**
 * This function is used to asynchronously read a certain number of bytes of
 * data from a random access device at the specified offset. The function call
 * always returns immediately. The asynchronous operation will continue until
 * one of the following conditions is true:
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li The completion_condition function object returns 0.
 *
 * @param d The device from which the data is to be read. The type must support
 * the AsyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * device. Although the buffers object may be copied as necessary, ownership of
 * the underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest async_read_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the device's async_read_some_at function.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes copied into the buffers. If an error
 *   // occurred, this will be the number of bytes successfully
 *   // transferred prior to the error.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::async_read_at(d, 42,
 *     boost::asio::buffer(data, size),
 *     boost::asio::transfer_at_least(32),
 *     handler); @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncRandomAccessReadDevice type's
 * async_read_some_at operation.
 */
template <typename AsyncRandomAccessReadDevice,
    typename MutableBufferSequence, typename CompletionCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncRandomAccessReadDevice::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_at(AsyncRandomAccessReadDevice& d,
    uint64_t offset, const MutableBufferSequence& buffers,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncRandomAccessReadDevice::executor_type));

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if !defined(BOOST_ASIO_NO_IOSTREAM)

/// Start an asynchronous operation to read a certain amount of data at the
/// specified offset.
/**
 * This function is used to asynchronously read a certain number of bytes of
 * data from a random access device at the specified offset. The function call
 * always returns immediately. The asynchronous operation will continue until
 * one of the following conditions is true:
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * async_read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the AsyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param b A basic_streambuf object into which the data will be read. Ownership
 * of the streambuf is retained by the caller, which must guarantee that it
 * remains valid until the handler is called.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes copied into the buffers. If an error
 *   // occurred, this will be the number of bytes successfully
 *   // transferred prior to the error.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::async_read_at(
 *     d, 42, b,
 *     boost::asio::transfer_all(),
 *     handler); @endcode
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncRandomAccessReadDevice type's
 * async_read_some_at operation.
 */
template <typename AsyncRandomAccessReadDevice, typename Allocator,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncRandomAccessReadDevice::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_at(AsyncRandomAccessReadDevice& d,
    uint64_t offset, basic_streambuf<Allocator>& b,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncRandomAccessReadDevice::executor_type));

/// Start an asynchronous operation to read a certain amount of data at the
/// specified offset.
/**
 * This function is used to asynchronously read a certain number of bytes of
 * data from a random access device at the specified offset. The function call
 * always returns immediately. The asynchronous operation will continue until
 * one of the following conditions is true:
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * async_read_some_at function.
 *
 * @param d The device from which the data is to be read. The type must support
 * the AsyncRandomAccessReadDevice concept.
 *
 * @param offset The offset at which the data will be read.
 *
 * @param b A basic_streambuf object into which the data will be read. Ownership
 * of the streambuf is retained by the caller, which must guarantee that it
 * remains valid until the handler is called.
 *
 * @param completion_condition The function object to be called to determine
 * whether the read operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest async_read_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the read operation is complete. A non-zero
 * return value indicates the maximum number of bytes to be read on the next
 * call to the device's async_read_some_at function.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes copied into the buffers. If an error
 *   // occurred, this will be the number of bytes successfully
 *   // transferred prior to the error.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncRandomAccessReadDevice type's
 * async_read_some_at operation.
 */
template <typename AsyncRandomAccessReadDevice,
    typename Allocator, typename CompletionCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncRandomAccessReadDevice::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_at(AsyncRandomAccessReadDevice& d,
    uint64_t offset, basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncRandomAccessReadDevice::executor_type));

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)

/*@}*/

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/read_at.hpp>

#endif // BOOST_ASIO_READ_AT_HPP
