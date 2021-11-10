//
// write_at.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_WRITE_AT_HPP
#define BOOST_ASIO_WRITE_AT_HPP

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
 * @defgroup write_at boost::asio::write_at
 *
 * @brief The @c write_at function is a composed operation that writes a
 * certain amount of data at a specified offset before returning.
 */
/*@{*/

/// Write all of the supplied data at the specified offset before returning.
/**
 * This function is used to write a certain number of bytes of data to a random
 * access device at a specified offset. The call will block until one of the
 * following conditions is true:
 *
 * @li All of the data in the supplied buffers has been written. That is, the
 * bytes transferred is equal to the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * write_some_at function.
 *
 * @param d The device to which the data is to be written. The type must support
 * the SyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param buffers One or more buffers containing the data to be written. The sum
 * of the buffer sizes indicates the maximum number of bytes to write to the
 * device.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @par Example
 * To write a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::write_at(d, 42, boost::asio::buffer(data, size)); @endcode
 * See the @ref buffer documentation for information on writing multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::write_at(
 *     d, offset, buffers,
 *     boost::asio::transfer_all()); @endcode
 */
template <typename SyncRandomAccessWriteDevice, typename ConstBufferSequence>
std::size_t write_at(SyncRandomAccessWriteDevice& d,
    uint64_t offset, const ConstBufferSequence& buffers);

/// Write all of the supplied data at the specified offset before returning.
/**
 * This function is used to write a certain number of bytes of data to a random
 * access device at a specified offset. The call will block until one of the
 * following conditions is true:
 *
 * @li All of the data in the supplied buffers has been written. That is, the
 * bytes transferred is equal to the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * write_some_at function.
 *
 * @param d The device to which the data is to be written. The type must support
 * the SyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param buffers One or more buffers containing the data to be written. The sum
 * of the buffer sizes indicates the maximum number of bytes to write to the
 * device.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes transferred.
 *
 * @par Example
 * To write a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::write_at(d, 42,
 *     boost::asio::buffer(data, size), ec); @endcode
 * See the @ref buffer documentation for information on writing multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::write_at(
 *     d, offset, buffers,
 *     boost::asio::transfer_all(), ec); @endcode
 */
template <typename SyncRandomAccessWriteDevice, typename ConstBufferSequence>
std::size_t write_at(SyncRandomAccessWriteDevice& d,
    uint64_t offset, const ConstBufferSequence& buffers,
    boost::system::error_code& ec);

/// Write a certain amount of data at a specified offset before returning.
/**
 * This function is used to write a certain number of bytes of data to a random
 * access device at a specified offset. The call will block until one of the
 * following conditions is true:
 *
 * @li All of the data in the supplied buffers has been written. That is, the
 * bytes transferred is equal to the sum of the buffer sizes.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * write_some_at function.
 *
 * @param d The device to which the data is to be written. The type must support
 * the SyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param buffers One or more buffers containing the data to be written. The sum
 * of the buffer sizes indicates the maximum number of bytes to write to the
 * device.
 *
 * @param completion_condition The function object to be called to determine
 * whether the write operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest write_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the write operation is complete. A
 * non-zero return value indicates the maximum number of bytes to be written on
 * the next call to the device's write_some_at function.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @par Example
 * To write a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::write_at(d, 42, boost::asio::buffer(data, size),
 *     boost::asio::transfer_at_least(32)); @endcode
 * See the @ref buffer documentation for information on writing multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 */
template <typename SyncRandomAccessWriteDevice, typename ConstBufferSequence,
    typename CompletionCondition>
std::size_t write_at(SyncRandomAccessWriteDevice& d,
    uint64_t offset, const ConstBufferSequence& buffers,
    CompletionCondition completion_condition);

/// Write a certain amount of data at a specified offset before returning.
/**
 * This function is used to write a certain number of bytes of data to a random
 * access device at a specified offset. The call will block until one of the
 * following conditions is true:
 *
 * @li All of the data in the supplied buffers has been written. That is, the
 * bytes transferred is equal to the sum of the buffer sizes.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * write_some_at function.
 *
 * @param d The device to which the data is to be written. The type must support
 * the SyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param buffers One or more buffers containing the data to be written. The sum
 * of the buffer sizes indicates the maximum number of bytes to write to the
 * device.
 *
 * @param completion_condition The function object to be called to determine
 * whether the write operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest write_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the write operation is complete. A
 * non-zero return value indicates the maximum number of bytes to be written on
 * the next call to the device's write_some_at function.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes written. If an error occurs, returns the total
 * number of bytes successfully transferred prior to the error.
 */
template <typename SyncRandomAccessWriteDevice, typename ConstBufferSequence,
    typename CompletionCondition>
std::size_t write_at(SyncRandomAccessWriteDevice& d,
    uint64_t offset, const ConstBufferSequence& buffers,
    CompletionCondition completion_condition, boost::system::error_code& ec);

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if !defined(BOOST_ASIO_NO_IOSTREAM)

/// Write all of the supplied data at the specified offset before returning.
/**
 * This function is used to write a certain number of bytes of data to a random
 * access device at a specified offset. The call will block until one of the
 * following conditions is true:
 *
 * @li All of the data in the supplied basic_streambuf has been written.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * write_some_at function.
 *
 * @param d The device to which the data is to be written. The type must support
 * the SyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param b The basic_streambuf object from which data will be written.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::write_at(
 *     d, 42, b,
 *     boost::asio::transfer_all()); @endcode
 */
template <typename SyncRandomAccessWriteDevice, typename Allocator>
std::size_t write_at(SyncRandomAccessWriteDevice& d,
    uint64_t offset, basic_streambuf<Allocator>& b);

/// Write all of the supplied data at the specified offset before returning.
/**
 * This function is used to write a certain number of bytes of data to a random
 * access device at a specified offset. The call will block until one of the
 * following conditions is true:
 *
 * @li All of the data in the supplied basic_streambuf has been written.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * write_some_at function.
 *
 * @param d The device to which the data is to be written. The type must support
 * the SyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param b The basic_streambuf object from which data will be written.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes transferred.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::write_at(
 *     d, 42, b,
 *     boost::asio::transfer_all(), ec); @endcode
 */
template <typename SyncRandomAccessWriteDevice, typename Allocator>
std::size_t write_at(SyncRandomAccessWriteDevice& d,
    uint64_t offset, basic_streambuf<Allocator>& b,
    boost::system::error_code& ec);

/// Write a certain amount of data at a specified offset before returning.
/**
 * This function is used to write a certain number of bytes of data to a random
 * access device at a specified offset. The call will block until one of the
 * following conditions is true:
 *
 * @li All of the data in the supplied basic_streambuf has been written.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * write_some_at function.
 *
 * @param d The device to which the data is to be written. The type must support
 * the SyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param b The basic_streambuf object from which data will be written.
 *
 * @param completion_condition The function object to be called to determine
 * whether the write operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest write_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the write operation is complete. A
 * non-zero return value indicates the maximum number of bytes to be written on
 * the next call to the device's write_some_at function.
 *
 * @returns The number of bytes transferred.
 *
 * @throws boost::system::system_error Thrown on failure.
 */
template <typename SyncRandomAccessWriteDevice, typename Allocator,
    typename CompletionCondition>
std::size_t write_at(SyncRandomAccessWriteDevice& d, uint64_t offset,
    basic_streambuf<Allocator>& b, CompletionCondition completion_condition);

/// Write a certain amount of data at a specified offset before returning.
/**
 * This function is used to write a certain number of bytes of data to a random
 * access device at a specified offset. The call will block until one of the
 * following conditions is true:
 *
 * @li All of the data in the supplied basic_streambuf has been written.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * write_some_at function.
 *
 * @param d The device to which the data is to be written. The type must support
 * the SyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param b The basic_streambuf object from which data will be written.
 *
 * @param completion_condition The function object to be called to determine
 * whether the write operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest write_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the write operation is complete. A
 * non-zero return value indicates the maximum number of bytes to be written on
 * the next call to the device's write_some_at function.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes written. If an error occurs, returns the total
 * number of bytes successfully transferred prior to the error.
 */
template <typename SyncRandomAccessWriteDevice, typename Allocator,
    typename CompletionCondition>
std::size_t write_at(SyncRandomAccessWriteDevice& d, uint64_t offset,
    basic_streambuf<Allocator>& b, CompletionCondition completion_condition,
    boost::system::error_code& ec);

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)

/*@}*/
/**
 * @defgroup async_write_at boost::asio::async_write_at
 *
 * @brief The @c async_write_at function is a composed asynchronous operation
 * that writes a certain amount of data at the specified offset before
 * completion.
 */
/*@{*/

/// Start an asynchronous operation to write all of the supplied data at the
/// specified offset.
/**
 * This function is used to asynchronously write a certain number of bytes of
 * data to a random access device at a specified offset. The function call
 * always returns immediately. The asynchronous operation will continue until
 * one of the following conditions is true:
 *
 * @li All of the data in the supplied buffers has been written. That is, the
 * bytes transferred is equal to the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * async_write_some_at function, and is known as a <em>composed operation</em>.
 * The program must ensure that the device performs no <em>overlapping</em>
 * write operations (such as async_write_at, the device's async_write_some_at
 * function, or any other composed operations that perform writes) until this
 * operation completes. Operations are overlapping if the regions defined by
 * their offsets, and the numbers of bytes to write, intersect.
 *
 * @param d The device to which the data is to be written. The type must support
 * the AsyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param buffers One or more buffers containing the data to be written.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param handler The handler to be called when the write operation completes.
 * Copies will be made of the handler as required. The function signature of
 * the handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes written from the buffers. If an error
 *   // occurred, this will be less than the sum of the buffer sizes.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @par Example
 * To write a single data buffer use the @ref buffer function as follows:
 * @code
 * boost::asio::async_write_at(d, 42, boost::asio::buffer(data, size), handler);
 * @endcode
 * See the @ref buffer documentation for information on writing multiple
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
 * if they are also supported by the @c AsyncRandomAccessWriteDevice type's
 * async_write_some_at operation.
 */
template <typename AsyncRandomAccessWriteDevice, typename ConstBufferSequence,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncRandomAccessWriteDevice::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write_at(AsyncRandomAccessWriteDevice& d, uint64_t offset,
    const ConstBufferSequence& buffers,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncRandomAccessWriteDevice::executor_type));

/// Start an asynchronous operation to write a certain amount of data at the
/// specified offset.
/**
 * This function is used to asynchronously write a certain number of bytes of
 * data to a random access device at a specified offset. The function call
 * always returns immediately. The asynchronous operation will continue until
 * one of the following conditions is true:
 *
 * @li All of the data in the supplied buffers has been written. That is, the
 * bytes transferred is equal to the sum of the buffer sizes.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * async_write_some_at function, and is known as a <em>composed operation</em>.
 * The program must ensure that the device performs no <em>overlapping</em>
 * write operations (such as async_write_at, the device's async_write_some_at
 * function, or any other composed operations that perform writes) until this
 * operation completes. Operations are overlapping if the regions defined by
 * their offsets, and the numbers of bytes to write, intersect.
 *
 * @param d The device to which the data is to be written. The type must support
 * the AsyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param buffers One or more buffers containing the data to be written.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param completion_condition The function object to be called to determine
 * whether the write operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest async_write_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the write operation is complete. A
 * non-zero return value indicates the maximum number of bytes to be written on
 * the next call to the device's async_write_some_at function.
 *
 * @param handler The handler to be called when the write operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes written from the buffers. If an error
 *   // occurred, this will be less than the sum of the buffer sizes.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @par Example
 * To write a single data buffer use the @ref buffer function as follows:
 * @code boost::asio::async_write_at(d, 42,
 *     boost::asio::buffer(data, size),
 *     boost::asio::transfer_at_least(32),
 *     handler); @endcode
 * See the @ref buffer documentation for information on writing multiple
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
 * if they are also supported by the @c AsyncRandomAccessWriteDevice type's
 * async_write_some_at operation.
 */
template <typename AsyncRandomAccessWriteDevice,
    typename ConstBufferSequence, typename CompletionCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncRandomAccessWriteDevice::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write_at(AsyncRandomAccessWriteDevice& d,
    uint64_t offset, const ConstBufferSequence& buffers,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncRandomAccessWriteDevice::executor_type));

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if !defined(BOOST_ASIO_NO_IOSTREAM)

/// Start an asynchronous operation to write all of the supplied data at the
/// specified offset.
/**
 * This function is used to asynchronously write a certain number of bytes of
 * data to a random access device at a specified offset. The function call
 * always returns immediately. The asynchronous operation will continue until
 * one of the following conditions is true:
 *
 * @li All of the data in the supplied basic_streambuf has been written.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * async_write_some_at function, and is known as a <em>composed operation</em>.
 * The program must ensure that the device performs no <em>overlapping</em>
 * write operations (such as async_write_at, the device's async_write_some_at
 * function, or any other composed operations that perform writes) until this
 * operation completes. Operations are overlapping if the regions defined by
 * their offsets, and the numbers of bytes to write, intersect.
 *
 * @param d The device to which the data is to be written. The type must support
 * the AsyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param b A basic_streambuf object from which data will be written. Ownership
 * of the streambuf is retained by the caller, which must guarantee that it
 * remains valid until the handler is called.
 *
 * @param handler The handler to be called when the write operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes written from the buffers. If an error
 *   // occurred, this will be less than the sum of the buffer sizes.
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
 * if they are also supported by the @c AsyncRandomAccessWriteDevice type's
 * async_write_some_at operation.
 */
template <typename AsyncRandomAccessWriteDevice, typename Allocator,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncRandomAccessWriteDevice::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write_at(AsyncRandomAccessWriteDevice& d,
    uint64_t offset, basic_streambuf<Allocator>& b,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncRandomAccessWriteDevice::executor_type));

/// Start an asynchronous operation to write a certain amount of data at the
/// specified offset.
/**
 * This function is used to asynchronously write a certain number of bytes of
 * data to a random access device at a specified offset. The function call
 * always returns immediately. The asynchronous operation will continue until
 * one of the following conditions is true:
 *
 * @li All of the data in the supplied basic_streambuf has been written.
 *
 * @li The completion_condition function object returns 0.
 *
 * This operation is implemented in terms of zero or more calls to the device's
 * async_write_some_at function, and is known as a <em>composed operation</em>.
 * The program must ensure that the device performs no <em>overlapping</em>
 * write operations (such as async_write_at, the device's async_write_some_at
 * function, or any other composed operations that perform writes) until this
 * operation completes. Operations are overlapping if the regions defined by
 * their offsets, and the numbers of bytes to write, intersect.
 *
 * @param d The device to which the data is to be written. The type must support
 * the AsyncRandomAccessWriteDevice concept.
 *
 * @param offset The offset at which the data will be written.
 *
 * @param b A basic_streambuf object from which data will be written. Ownership
 * of the streambuf is retained by the caller, which must guarantee that it
 * remains valid until the handler is called.
 *
 * @param completion_condition The function object to be called to determine
 * whether the write operation is complete. The signature of the function object
 * must be:
 * @code std::size_t completion_condition(
 *   // Result of latest async_write_some_at operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes transferred so far.
 *   std::size_t bytes_transferred
 * ); @endcode
 * A return value of 0 indicates that the write operation is complete. A
 * non-zero return value indicates the maximum number of bytes to be written on
 * the next call to the device's async_write_some_at function.
 *
 * @param handler The handler to be called when the write operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // Number of bytes written from the buffers. If an error
 *   // occurred, this will be less than the sum of the buffer sizes.
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
 * if they are also supported by the @c AsyncRandomAccessWriteDevice type's
 * async_write_some_at operation.
 */
template <typename AsyncRandomAccessWriteDevice,
    typename Allocator, typename CompletionCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncRandomAccessWriteDevice::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write_at(AsyncRandomAccessWriteDevice& d, uint64_t offset,
    basic_streambuf<Allocator>& b, CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncRandomAccessWriteDevice::executor_type));

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)

/*@}*/

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/write_at.hpp>

#endif // BOOST_ASIO_WRITE_AT_HPP
