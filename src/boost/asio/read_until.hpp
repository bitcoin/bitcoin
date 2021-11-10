//
// read_until.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_READ_UNTIL_HPP
#define BOOST_ASIO_READ_UNTIL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <string>
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/detail/regex_fwd.hpp>
#include <boost/asio/detail/string_view.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/error.hpp>

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
# include <boost/asio/basic_streambuf_fwd.hpp>
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

namespace detail
{
  char (&has_result_type_helper(...))[2];

  template <typename T>
  char has_result_type_helper(T*, typename T::result_type* = 0);

  template <typename T>
  struct has_result_type
  {
    enum { value = (sizeof((has_result_type_helper)((T*)(0))) == 1) };
  };
} // namespace detail

/// Type trait used to determine whether a type can be used as a match condition
/// function with read_until and async_read_until.
template <typename T>
struct is_match_condition
{
#if defined(GENERATING_DOCUMENTATION)
  /// The value member is true if the type may be used as a match condition.
  static const bool value;
#else
  enum
  {
    value = boost::asio::is_function<
        typename boost::asio::remove_pointer<T>::type>::value
      || detail::has_result_type<T>::value
  };
#endif
};

/**
 * @defgroup read_until boost::asio::read_until
 *
 * @brief The @c read_until function is a composed operation that reads data
 * into a dynamic buffer sequence, or into a streambuf, until it contains a
 * delimiter, matches a regular expression, or a function object indicates a
 * match.
 */
/*@{*/

#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

/// Read data into a dynamic buffer sequence until it contains a specified
/// delimiter.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains the specified
 * delimiter. The call will block until one of the following conditions is
 * true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains the delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 *
 * @param delim The delimiter character.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the delimiter.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond the delimiter. An application will
 * typically leave that data in the dynamic buffer sequence for a subsequent
 * read_until operation to examine.
 *
 * @par Example
 * To read data into a @c std::string until a newline is encountered:
 * @code std::string data;
 * std::size_t n = boost::asio::read_until(s,
 *     boost::asio::dynamic_buffer(data), '\n');
 * std::string line = data.substr(0, n);
 * data.erase(0, n); @endcode
 * After the @c read_until operation completes successfully, the string @c data
 * contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the buffer @c b as
 * follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c read_until operation.
 */
template <typename SyncReadStream, typename DynamicBuffer_v1>
std::size_t read_until(SyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers, char delim,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until it contains a specified
/// delimiter.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains the specified
 * delimiter. The call will block until one of the following conditions is
 * true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains the delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 *
 * @param delim The delimiter character.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the delimiter. Returns 0 if an error occurred.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond the delimiter. An application will
 * typically leave that data in the dynamic buffer sequence for a subsequent
 * read_until operation to examine.
 */
template <typename SyncReadStream, typename DynamicBuffer_v1>
std::size_t read_until(SyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    char delim, boost::system::error_code& ec,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until it contains a specified
/// delimiter.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains the specified
 * delimiter. The call will block until one of the following conditions is
 * true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains the delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 *
 * @param delim The delimiter string.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the delimiter.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond the delimiter. An application will
 * typically leave that data in the dynamic buffer sequence for a subsequent
 * read_until operation to examine.
 *
 * @par Example
 * To read data into a @c std::string until a CR-LF sequence is encountered:
 * @code std::string data;
 * std::size_t n = boost::asio::read_until(s,
 *     boost::asio::dynamic_buffer(data), "\r\n");
 * std::string line = data.substr(0, n);
 * data.erase(0, n); @endcode
 * After the @c read_until operation completes successfully, the string @c data
 * contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the buffer @c b as
 * follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c read_until operation.
 */
template <typename SyncReadStream, typename DynamicBuffer_v1>
std::size_t read_until(SyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    BOOST_ASIO_STRING_VIEW_PARAM delim,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until it contains a specified
/// delimiter.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains the specified
 * delimiter. The call will block until one of the following conditions is
 * true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains the delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 *
 * @param delim The delimiter string.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the delimiter. Returns 0 if an error occurred.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond the delimiter. An application will
 * typically leave that data in the dynamic buffer sequence for a subsequent
 * read_until operation to examine.
 */
template <typename SyncReadStream, typename DynamicBuffer_v1>
std::size_t read_until(SyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    BOOST_ASIO_STRING_VIEW_PARAM delim,
    boost::system::error_code& ec,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if defined(BOOST_ASIO_HAS_BOOST_REGEX) \
  || defined(GENERATING_DOCUMENTATION)

/// Read data into a dynamic buffer sequence until some part of the data it
/// contains matches a regular expression.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains some data
 * that matches a regular expression. The call will block until one of the
 * following conditions is true:
 *
 * @li A substring of the dynamic buffer sequence's get area matches the
 * regular expression.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains data that matches the regular expression, the function returns
 * immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers A dynamic buffer sequence into which the data will be read.
 *
 * @param expr The regular expression.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the substring that matches the regular expression.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond that which matched the regular
 * expression. An application will typically leave that data in the dynamic
 * buffer sequence for a subsequent read_until operation to examine.
 *
 * @par Example
 * To read data into a @c std::string until a CR-LF sequence is encountered:
 * @code std::string data;
 * std::size_t n = boost::asio::read_until(s,
 *     boost::asio::dynamic_buffer(data), boost::regex("\r\n"));
 * std::string line = data.substr(0, n);
 * data.erase(0, n); @endcode
 * After the @c read_until operation completes successfully, the string @c data
 * contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the buffer @c b as
 * follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c read_until operation.
 */
template <typename SyncReadStream, typename DynamicBuffer_v1>
std::size_t read_until(SyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    const boost::regex& expr,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until some part of the data it
/// contains matches a regular expression.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains some data
 * that matches a regular expression. The call will block until one of the
 * following conditions is true:
 *
 * @li A substring of the dynamic buffer sequence's get area matches the
 * regular expression.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains data that matches the regular expression, the function returns
 * immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers A dynamic buffer sequence into which the data will be read.
 *
 * @param expr The regular expression.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the substring that matches the regular expression. Returns 0
 * if an error occurred.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond that which matched the regular
 * expression. An application will typically leave that data in the dynamic
 * buffer sequence for a subsequent read_until operation to examine.
 */
template <typename SyncReadStream, typename DynamicBuffer_v1>
std::size_t read_until(SyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    const boost::regex& expr, boost::system::error_code& ec,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

#endif // defined(BOOST_ASIO_HAS_BOOST_REGEX)
       // || defined(GENERATING_DOCUMENTATION)

/// Read data into a dynamic buffer sequence until a function object indicates a
/// match.

/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until a user-defined match condition function object, when applied
 * to the data contained in the dynamic buffer sequence, indicates a successful
 * match. The call will block until one of the following conditions is true:
 *
 * @li The match condition function object returns a std::pair where the second
 * element evaluates to true.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the match condition function object already indicates
 * a match, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers A dynamic buffer sequence into which the data will be read.
 *
 * @param match_condition The function object to be called to determine whether
 * a match exists. The signature of the function object must be:
 * @code pair<iterator, bool> match_condition(iterator begin, iterator end);
 * @endcode
 * where @c iterator represents the type:
 * @code buffers_iterator<typename DynamicBuffer_v1::const_buffers_type>
 * @endcode
 * The iterator parameters @c begin and @c end define the range of bytes to be
 * scanned to determine whether there is a match. The @c first member of the
 * return value is an iterator marking one-past-the-end of the bytes that have
 * been consumed by the match function. This iterator is used to calculate the
 * @c begin parameter for any subsequent invocation of the match condition. The
 * @c second member of the return value is true if a match has been found, false
 * otherwise.
 *
 * @returns The number of bytes in the dynamic_buffer's get area that
 * have been fully consumed by the match function.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond that which matched the function object.
 * An application will typically leave that data in the dynamic buffer sequence
 * for a subsequent read_until operation to examine.

 * @note The default implementation of the @c is_match_condition type trait
 * evaluates to true for function pointers and function objects with a
 * @c result_type typedef. It must be specialised for other user-defined
 * function objects.
 *
 * @par Examples
 * To read data into a dynamic buffer sequence until whitespace is encountered:
 * @code typedef boost::asio::buffers_iterator<
 *     boost::asio::const_buffers_1> iterator;
 *
 * std::pair<iterator, bool>
 * match_whitespace(iterator begin, iterator end)
 * {
 *   iterator i = begin;
 *   while (i != end)
 *     if (std::isspace(*i++))
 *       return std::make_pair(i, true);
 *   return std::make_pair(i, false);
 * }
 * ...
 * std::string data;
 * boost::asio::read_until(s, data, match_whitespace);
 * @endcode
 *
 * To read data into a @c std::string until a matching character is found:
 * @code class match_char
 * {
 * public:
 *   explicit match_char(char c) : c_(c) {}
 *
 *   template <typename Iterator>
 *   std::pair<Iterator, bool> operator()(
 *       Iterator begin, Iterator end) const
 *   {
 *     Iterator i = begin;
 *     while (i != end)
 *       if (c_ == *i++)
 *         return std::make_pair(i, true);
 *     return std::make_pair(i, false);
 *   }
 *
 * private:
 *   char c_;
 * };
 *
 * namespace asio {
 *   template <> struct is_match_condition<match_char>
 *     : public boost::true_type {};
 * } // namespace asio
 * ...
 * std::string data;
 * boost::asio::read_until(s, data, match_char('a'));
 * @endcode
 */
template <typename SyncReadStream,
    typename DynamicBuffer_v1, typename MatchCondition>
std::size_t read_until(SyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    MatchCondition match_condition,
    typename constraint<
      is_match_condition<MatchCondition>::value
    >::type = 0,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until a function object indicates a
/// match.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until a user-defined match condition function object, when applied
 * to the data contained in the dynamic buffer sequence, indicates a successful
 * match. The call will block until one of the following conditions is true:
 *
 * @li The match condition function object returns a std::pair where the second
 * element evaluates to true.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the match condition function object already indicates
 * a match, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers A dynamic buffer sequence into which the data will be read.
 *
 * @param match_condition The function object to be called to determine whether
 * a match exists. The signature of the function object must be:
 * @code pair<iterator, bool> match_condition(iterator begin, iterator end);
 * @endcode
 * where @c iterator represents the type:
 * @code buffers_iterator<DynamicBuffer_v1::const_buffers_type>
 * @endcode
 * The iterator parameters @c begin and @c end define the range of bytes to be
 * scanned to determine whether there is a match. The @c first member of the
 * return value is an iterator marking one-past-the-end of the bytes that have
 * been consumed by the match function. This iterator is used to calculate the
 * @c begin parameter for any subsequent invocation of the match condition. The
 * @c second member of the return value is true if a match has been found, false
 * otherwise.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area that
 * have been fully consumed by the match function. Returns 0 if an error
 * occurred.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond that which matched the function object.
 * An application will typically leave that data in the dynamic buffer sequence
 * for a subsequent read_until operation to examine.
 *
 * @note The default implementation of the @c is_match_condition type trait
 * evaluates to true for function pointers and function objects with a
 * @c result_type typedef. It must be specialised for other user-defined
 * function objects.
 */
template <typename SyncReadStream,
    typename DynamicBuffer_v1, typename MatchCondition>
std::size_t read_until(SyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    MatchCondition match_condition, boost::system::error_code& ec,
    typename constraint<
      is_match_condition<MatchCondition>::value
    >::type = 0,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

#if !defined(BOOST_ASIO_NO_IOSTREAM)

/// Read data into a streambuf until it contains a specified delimiter.
/**
 * This function is used to read data into the specified streambuf until the
 * streambuf's get area contains the specified delimiter. The call will block
 * until one of the following conditions is true:
 *
 * @li The get area of the streambuf contains the specified delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the streambuf's get area already contains the
 * delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read.
 *
 * @param delim The delimiter character.
 *
 * @returns The number of bytes in the streambuf's get area up to and including
 * the delimiter.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the streambuf may contain
 * additional data beyond the delimiter. An application will typically leave
 * that data in the streambuf for a subsequent read_until operation to examine.
 *
 * @par Example
 * To read data into a streambuf until a newline is encountered:
 * @code boost::asio::streambuf b;
 * boost::asio::read_until(s, b, '\n');
 * std::istream is(&b);
 * std::string line;
 * std::getline(is, line); @endcode
 * After the @c read_until operation completes successfully, the buffer @c b
 * contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\n', 'd', 'e', ... } @endcode
 * The call to @c std::getline then extracts the data up to and including the
 * newline (which is discarded), so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c' } @endcode
 * The remaining data is left in the buffer @c b as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c read_until operation.
 */
template <typename SyncReadStream, typename Allocator>
std::size_t read_until(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, char delim);

/// Read data into a streambuf until it contains a specified delimiter.
/**
 * This function is used to read data into the specified streambuf until the
 * streambuf's get area contains the specified delimiter. The call will block
 * until one of the following conditions is true:
 *
 * @li The get area of the streambuf contains the specified delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the streambuf's get area already contains the
 * delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read.
 *
 * @param delim The delimiter character.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the streambuf's get area up to and including
 * the delimiter. Returns 0 if an error occurred.
 *
 * @note After a successful read_until operation, the streambuf may contain
 * additional data beyond the delimiter. An application will typically leave
 * that data in the streambuf for a subsequent read_until operation to examine.
 */
template <typename SyncReadStream, typename Allocator>
std::size_t read_until(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, char delim,
    boost::system::error_code& ec);

/// Read data into a streambuf until it contains a specified delimiter.
/**
 * This function is used to read data into the specified streambuf until the
 * streambuf's get area contains the specified delimiter. The call will block
 * until one of the following conditions is true:
 *
 * @li The get area of the streambuf contains the specified delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the streambuf's get area already contains the
 * delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read.
 *
 * @param delim The delimiter string.
 *
 * @returns The number of bytes in the streambuf's get area up to and including
 * the delimiter.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the streambuf may contain
 * additional data beyond the delimiter. An application will typically leave
 * that data in the streambuf for a subsequent read_until operation to examine.
 *
 * @par Example
 * To read data into a streambuf until a newline is encountered:
 * @code boost::asio::streambuf b;
 * boost::asio::read_until(s, b, "\r\n");
 * std::istream is(&b);
 * std::string line;
 * std::getline(is, line); @endcode
 * After the @c read_until operation completes successfully, the buffer @c b
 * contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c std::getline then extracts the data up to and including the
 * newline (which is discarded), so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r' } @endcode
 * The remaining data is left in the buffer @c b as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c read_until operation.
 */
template <typename SyncReadStream, typename Allocator>
std::size_t read_until(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    BOOST_ASIO_STRING_VIEW_PARAM delim);

/// Read data into a streambuf until it contains a specified delimiter.
/**
 * This function is used to read data into the specified streambuf until the
 * streambuf's get area contains the specified delimiter. The call will block
 * until one of the following conditions is true:
 *
 * @li The get area of the streambuf contains the specified delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the streambuf's get area already contains the
 * delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read.
 *
 * @param delim The delimiter string.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the streambuf's get area up to and including
 * the delimiter. Returns 0 if an error occurred.
 *
 * @note After a successful read_until operation, the streambuf may contain
 * additional data beyond the delimiter. An application will typically leave
 * that data in the streambuf for a subsequent read_until operation to examine.
 */
template <typename SyncReadStream, typename Allocator>
std::size_t read_until(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    BOOST_ASIO_STRING_VIEW_PARAM delim, boost::system::error_code& ec);

#if defined(BOOST_ASIO_HAS_BOOST_REGEX) \
  || defined(GENERATING_DOCUMENTATION)

/// Read data into a streambuf until some part of the data it contains matches
/// a regular expression.
/**
 * This function is used to read data into the specified streambuf until the
 * streambuf's get area contains some data that matches a regular expression.
 * The call will block until one of the following conditions is true:
 *
 * @li A substring of the streambuf's get area matches the regular expression.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the streambuf's get area already contains data that
 * matches the regular expression, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read.
 *
 * @param expr The regular expression.
 *
 * @returns The number of bytes in the streambuf's get area up to and including
 * the substring that matches the regular expression.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the streambuf may contain
 * additional data beyond that which matched the regular expression. An
 * application will typically leave that data in the streambuf for a subsequent
 * read_until operation to examine.
 *
 * @par Example
 * To read data into a streambuf until a CR-LF sequence is encountered:
 * @code boost::asio::streambuf b;
 * boost::asio::read_until(s, b, boost::regex("\r\n"));
 * std::istream is(&b);
 * std::string line;
 * std::getline(is, line); @endcode
 * After the @c read_until operation completes successfully, the buffer @c b
 * contains the data which matched the regular expression:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c std::getline then extracts the data up to and including the
 * newline (which is discarded), so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r' } @endcode
 * The remaining data is left in the buffer @c b as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c read_until operation.
 */
template <typename SyncReadStream, typename Allocator>
std::size_t read_until(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, const boost::regex& expr);

/// Read data into a streambuf until some part of the data it contains matches
/// a regular expression.
/**
 * This function is used to read data into the specified streambuf until the
 * streambuf's get area contains some data that matches a regular expression.
 * The call will block until one of the following conditions is true:
 *
 * @li A substring of the streambuf's get area matches the regular expression.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the streambuf's get area already contains data that
 * matches the regular expression, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read.
 *
 * @param expr The regular expression.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the streambuf's get area up to and including
 * the substring that matches the regular expression. Returns 0 if an error
 * occurred.
 *
 * @note After a successful read_until operation, the streambuf may contain
 * additional data beyond that which matched the regular expression. An
 * application will typically leave that data in the streambuf for a subsequent
 * read_until operation to examine.
 */
template <typename SyncReadStream, typename Allocator>
std::size_t read_until(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, const boost::regex& expr,
    boost::system::error_code& ec);

#endif // defined(BOOST_ASIO_HAS_BOOST_REGEX)
       // || defined(GENERATING_DOCUMENTATION)

/// Read data into a streambuf until a function object indicates a match.
/**
 * This function is used to read data into the specified streambuf until a
 * user-defined match condition function object, when applied to the data
 * contained in the streambuf, indicates a successful match. The call will
 * block until one of the following conditions is true:
 *
 * @li The match condition function object returns a std::pair where the second
 * element evaluates to true.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the match condition function object already indicates
 * a match, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read.
 *
 * @param match_condition The function object to be called to determine whether
 * a match exists. The signature of the function object must be:
 * @code pair<iterator, bool> match_condition(iterator begin, iterator end);
 * @endcode
 * where @c iterator represents the type:
 * @code buffers_iterator<basic_streambuf<Allocator>::const_buffers_type>
 * @endcode
 * The iterator parameters @c begin and @c end define the range of bytes to be
 * scanned to determine whether there is a match. The @c first member of the
 * return value is an iterator marking one-past-the-end of the bytes that have
 * been consumed by the match function. This iterator is used to calculate the
 * @c begin parameter for any subsequent invocation of the match condition. The
 * @c second member of the return value is true if a match has been found, false
 * otherwise.
 *
 * @returns The number of bytes in the streambuf's get area that have been fully
 * consumed by the match function.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the streambuf may contain
 * additional data beyond that which matched the function object. An application
 * will typically leave that data in the streambuf for a subsequent read_until
 * operation to examine.
 *
 * @note The default implementation of the @c is_match_condition type trait
 * evaluates to true for function pointers and function objects with a
 * @c result_type typedef. It must be specialised for other user-defined
 * function objects.
 *
 * @par Examples
 * To read data into a streambuf until whitespace is encountered:
 * @code typedef boost::asio::buffers_iterator<
 *     boost::asio::streambuf::const_buffers_type> iterator;
 *
 * std::pair<iterator, bool>
 * match_whitespace(iterator begin, iterator end)
 * {
 *   iterator i = begin;
 *   while (i != end)
 *     if (std::isspace(*i++))
 *       return std::make_pair(i, true);
 *   return std::make_pair(i, false);
 * }
 * ...
 * boost::asio::streambuf b;
 * boost::asio::read_until(s, b, match_whitespace);
 * @endcode
 *
 * To read data into a streambuf until a matching character is found:
 * @code class match_char
 * {
 * public:
 *   explicit match_char(char c) : c_(c) {}
 *
 *   template <typename Iterator>
 *   std::pair<Iterator, bool> operator()(
 *       Iterator begin, Iterator end) const
 *   {
 *     Iterator i = begin;
 *     while (i != end)
 *       if (c_ == *i++)
 *         return std::make_pair(i, true);
 *     return std::make_pair(i, false);
 *   }
 *
 * private:
 *   char c_;
 * };
 *
 * namespace asio {
 *   template <> struct is_match_condition<match_char>
 *     : public boost::true_type {};
 * } // namespace asio
 * ...
 * boost::asio::streambuf b;
 * boost::asio::read_until(s, b, match_char('a'));
 * @endcode
 */
template <typename SyncReadStream, typename Allocator, typename MatchCondition>
std::size_t read_until(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, MatchCondition match_condition,
    typename constraint<is_match_condition<MatchCondition>::value>::type = 0);

/// Read data into a streambuf until a function object indicates a match.
/**
 * This function is used to read data into the specified streambuf until a
 * user-defined match condition function object, when applied to the data
 * contained in the streambuf, indicates a successful match. The call will
 * block until one of the following conditions is true:
 *
 * @li The match condition function object returns a std::pair where the second
 * element evaluates to true.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the match condition function object already indicates
 * a match, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read.
 *
 * @param match_condition The function object to be called to determine whether
 * a match exists. The signature of the function object must be:
 * @code pair<iterator, bool> match_condition(iterator begin, iterator end);
 * @endcode
 * where @c iterator represents the type:
 * @code buffers_iterator<basic_streambuf<Allocator>::const_buffers_type>
 * @endcode
 * The iterator parameters @c begin and @c end define the range of bytes to be
 * scanned to determine whether there is a match. The @c first member of the
 * return value is an iterator marking one-past-the-end of the bytes that have
 * been consumed by the match function. This iterator is used to calculate the
 * @c begin parameter for any subsequent invocation of the match condition. The
 * @c second member of the return value is true if a match has been found, false
 * otherwise.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the streambuf's get area that have been fully
 * consumed by the match function. Returns 0 if an error occurred.
 *
 * @note After a successful read_until operation, the streambuf may contain
 * additional data beyond that which matched the function object. An application
 * will typically leave that data in the streambuf for a subsequent read_until
 * operation to examine.
 *
 * @note The default implementation of the @c is_match_condition type trait
 * evaluates to true for function pointers and function objects with a
 * @c result_type typedef. It must be specialised for other user-defined
 * function objects.
 */
template <typename SyncReadStream, typename Allocator, typename MatchCondition>
std::size_t read_until(SyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    MatchCondition match_condition, boost::system::error_code& ec,
    typename constraint<is_match_condition<MatchCondition>::value>::type = 0);

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

/// Read data into a dynamic buffer sequence until it contains a specified
/// delimiter.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains the specified
 * delimiter. The call will block until one of the following conditions is
 * true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains the delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 *
 * @param delim The delimiter character.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the delimiter.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond the delimiter. An application will
 * typically leave that data in the dynamic buffer sequence for a subsequent
 * read_until operation to examine.
 *
 * @par Example
 * To read data into a @c std::string until a newline is encountered:
 * @code std::string data;
 * std::size_t n = boost::asio::read_until(s,
 *     boost::asio::dynamic_buffer(data), '\n');
 * std::string line = data.substr(0, n);
 * data.erase(0, n); @endcode
 * After the @c read_until operation completes successfully, the string @c data
 * contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the buffer @c b as
 * follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c read_until operation.
 */
template <typename SyncReadStream, typename DynamicBuffer_v2>
std::size_t read_until(SyncReadStream& s, DynamicBuffer_v2 buffers, char delim,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until it contains a specified
/// delimiter.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains the specified
 * delimiter. The call will block until one of the following conditions is
 * true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains the delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 *
 * @param delim The delimiter character.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the delimiter. Returns 0 if an error occurred.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond the delimiter. An application will
 * typically leave that data in the dynamic buffer sequence for a subsequent
 * read_until operation to examine.
 */
template <typename SyncReadStream, typename DynamicBuffer_v2>
std::size_t read_until(SyncReadStream& s, DynamicBuffer_v2 buffers,
    char delim, boost::system::error_code& ec,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until it contains a specified
/// delimiter.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains the specified
 * delimiter. The call will block until one of the following conditions is
 * true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains the delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 *
 * @param delim The delimiter string.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the delimiter.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond the delimiter. An application will
 * typically leave that data in the dynamic buffer sequence for a subsequent
 * read_until operation to examine.
 *
 * @par Example
 * To read data into a @c std::string until a CR-LF sequence is encountered:
 * @code std::string data;
 * std::size_t n = boost::asio::read_until(s,
 *     boost::asio::dynamic_buffer(data), "\r\n");
 * std::string line = data.substr(0, n);
 * data.erase(0, n); @endcode
 * After the @c read_until operation completes successfully, the string @c data
 * contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the buffer @c b as
 * follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c read_until operation.
 */
template <typename SyncReadStream, typename DynamicBuffer_v2>
std::size_t read_until(SyncReadStream& s, DynamicBuffer_v2 buffers,
    BOOST_ASIO_STRING_VIEW_PARAM delim,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until it contains a specified
/// delimiter.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains the specified
 * delimiter. The call will block until one of the following conditions is
 * true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains the delimiter, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 *
 * @param delim The delimiter string.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the delimiter. Returns 0 if an error occurred.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond the delimiter. An application will
 * typically leave that data in the dynamic buffer sequence for a subsequent
 * read_until operation to examine.
 */
template <typename SyncReadStream, typename DynamicBuffer_v2>
std::size_t read_until(SyncReadStream& s, DynamicBuffer_v2 buffers,
    BOOST_ASIO_STRING_VIEW_PARAM delim, boost::system::error_code& ec,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if defined(BOOST_ASIO_HAS_BOOST_REGEX) \
  || defined(GENERATING_DOCUMENTATION)

/// Read data into a dynamic buffer sequence until some part of the data it
/// contains matches a regular expression.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains some data
 * that matches a regular expression. The call will block until one of the
 * following conditions is true:
 *
 * @li A substring of the dynamic buffer sequence's get area matches the
 * regular expression.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains data that matches the regular expression, the function returns
 * immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers A dynamic buffer sequence into which the data will be read.
 *
 * @param expr The regular expression.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the substring that matches the regular expression.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond that which matched the regular
 * expression. An application will typically leave that data in the dynamic
 * buffer sequence for a subsequent read_until operation to examine.
 *
 * @par Example
 * To read data into a @c std::string until a CR-LF sequence is encountered:
 * @code std::string data;
 * std::size_t n = boost::asio::read_until(s,
 *     boost::asio::dynamic_buffer(data), boost::regex("\r\n"));
 * std::string line = data.substr(0, n);
 * data.erase(0, n); @endcode
 * After the @c read_until operation completes successfully, the string @c data
 * contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the buffer @c b as
 * follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c read_until operation.
 */
template <typename SyncReadStream, typename DynamicBuffer_v2>
std::size_t read_until(SyncReadStream& s, DynamicBuffer_v2 buffers,
    const boost::regex& expr,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until some part of the data it
/// contains matches a regular expression.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until the dynamic buffer sequence's get area contains some data
 * that matches a regular expression. The call will block until one of the
 * following conditions is true:
 *
 * @li A substring of the dynamic buffer sequence's get area matches the
 * regular expression.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the dynamic buffer sequence's get area already
 * contains data that matches the regular expression, the function returns
 * immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers A dynamic buffer sequence into which the data will be read.
 *
 * @param expr The regular expression.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area up to
 * and including the substring that matches the regular expression. Returns 0
 * if an error occurred.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond that which matched the regular
 * expression. An application will typically leave that data in the dynamic
 * buffer sequence for a subsequent read_until operation to examine.
 */
template <typename SyncReadStream, typename DynamicBuffer_v2>
std::size_t read_until(SyncReadStream& s, DynamicBuffer_v2 buffers,
    const boost::regex& expr, boost::system::error_code& ec,
    typename constraint<
        is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

#endif // defined(BOOST_ASIO_HAS_BOOST_REGEX)
       // || defined(GENERATING_DOCUMENTATION)

/// Read data into a dynamic buffer sequence until a function object indicates a
/// match.

/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until a user-defined match condition function object, when applied
 * to the data contained in the dynamic buffer sequence, indicates a successful
 * match. The call will block until one of the following conditions is true:
 *
 * @li The match condition function object returns a std::pair where the second
 * element evaluates to true.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the match condition function object already indicates
 * a match, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers A dynamic buffer sequence into which the data will be read.
 *
 * @param match_condition The function object to be called to determine whether
 * a match exists. The signature of the function object must be:
 * @code pair<iterator, bool> match_condition(iterator begin, iterator end);
 * @endcode
 * where @c iterator represents the type:
 * @code buffers_iterator<typename DynamicBuffer_v2::const_buffers_type>
 * @endcode
 * The iterator parameters @c begin and @c end define the range of bytes to be
 * scanned to determine whether there is a match. The @c first member of the
 * return value is an iterator marking one-past-the-end of the bytes that have
 * been consumed by the match function. This iterator is used to calculate the
 * @c begin parameter for any subsequent invocation of the match condition. The
 * @c second member of the return value is true if a match has been found, false
 * otherwise.
 *
 * @returns The number of bytes in the dynamic_buffer's get area that
 * have been fully consumed by the match function.
 *
 * @throws boost::system::system_error Thrown on failure.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond that which matched the function object.
 * An application will typically leave that data in the dynamic buffer sequence
 * for a subsequent read_until operation to examine.

 * @note The default implementation of the @c is_match_condition type trait
 * evaluates to true for function pointers and function objects with a
 * @c result_type typedef. It must be specialised for other user-defined
 * function objects.
 *
 * @par Examples
 * To read data into a dynamic buffer sequence until whitespace is encountered:
 * @code typedef boost::asio::buffers_iterator<
 *     boost::asio::const_buffers_1> iterator;
 *
 * std::pair<iterator, bool>
 * match_whitespace(iterator begin, iterator end)
 * {
 *   iterator i = begin;
 *   while (i != end)
 *     if (std::isspace(*i++))
 *       return std::make_pair(i, true);
 *   return std::make_pair(i, false);
 * }
 * ...
 * std::string data;
 * boost::asio::read_until(s, data, match_whitespace);
 * @endcode
 *
 * To read data into a @c std::string until a matching character is found:
 * @code class match_char
 * {
 * public:
 *   explicit match_char(char c) : c_(c) {}
 *
 *   template <typename Iterator>
 *   std::pair<Iterator, bool> operator()(
 *       Iterator begin, Iterator end) const
 *   {
 *     Iterator i = begin;
 *     while (i != end)
 *       if (c_ == *i++)
 *         return std::make_pair(i, true);
 *     return std::make_pair(i, false);
 *   }
 *
 * private:
 *   char c_;
 * };
 *
 * namespace asio {
 *   template <> struct is_match_condition<match_char>
 *     : public boost::true_type {};
 * } // namespace asio
 * ...
 * std::string data;
 * boost::asio::read_until(s, data, match_char('a'));
 * @endcode
 */
template <typename SyncReadStream,
    typename DynamicBuffer_v2, typename MatchCondition>
std::size_t read_until(SyncReadStream& s, DynamicBuffer_v2 buffers,
    MatchCondition match_condition,
    typename constraint<
      is_match_condition<MatchCondition>::value
    >::type = 0,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

/// Read data into a dynamic buffer sequence until a function object indicates a
/// match.
/**
 * This function is used to read data into the specified dynamic buffer
 * sequence until a user-defined match condition function object, when applied
 * to the data contained in the dynamic buffer sequence, indicates a successful
 * match. The call will block until one of the following conditions is true:
 *
 * @li The match condition function object returns a std::pair where the second
 * element evaluates to true.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * read_some function. If the match condition function object already indicates
 * a match, the function returns immediately.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the SyncReadStream concept.
 *
 * @param buffers A dynamic buffer sequence into which the data will be read.
 *
 * @param match_condition The function object to be called to determine whether
 * a match exists. The signature of the function object must be:
 * @code pair<iterator, bool> match_condition(iterator begin, iterator end);
 * @endcode
 * where @c iterator represents the type:
 * @code buffers_iterator<DynamicBuffer_v2::const_buffers_type>
 * @endcode
 * The iterator parameters @c begin and @c end define the range of bytes to be
 * scanned to determine whether there is a match. The @c first member of the
 * return value is an iterator marking one-past-the-end of the bytes that have
 * been consumed by the match function. This iterator is used to calculate the
 * @c begin parameter for any subsequent invocation of the match condition. The
 * @c second member of the return value is true if a match has been found, false
 * otherwise.
 *
 * @param ec Set to indicate what error occurred, if any.
 *
 * @returns The number of bytes in the dynamic buffer sequence's get area that
 * have been fully consumed by the match function. Returns 0 if an error
 * occurred.
 *
 * @note After a successful read_until operation, the dynamic buffer sequence
 * may contain additional data beyond that which matched the function object.
 * An application will typically leave that data in the dynamic buffer sequence
 * for a subsequent read_until operation to examine.
 *
 * @note The default implementation of the @c is_match_condition type trait
 * evaluates to true for function pointers and function objects with a
 * @c result_type typedef. It must be specialised for other user-defined
 * function objects.
 */
template <typename SyncReadStream,
    typename DynamicBuffer_v2, typename MatchCondition>
std::size_t read_until(SyncReadStream& s, DynamicBuffer_v2 buffers,
    MatchCondition match_condition, boost::system::error_code& ec,
    typename constraint<
      is_match_condition<MatchCondition>::value
    >::type = 0,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)

/*@}*/
/**
 * @defgroup async_read_until boost::asio::async_read_until
 *
 * @brief The @c async_read_until function is a composed asynchronous operation
 * that reads data into a dynamic buffer sequence, or into a streambuf, until
 * it contains a delimiter, matches a regular expression, or a function object
 * indicates a match.
 */
/*@{*/

#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

/// Start an asynchronous operation to read data into a dynamic buffer sequence
/// until it contains a specified delimiter.
/**
 * This function is used to asynchronously read data into the specified dynamic
 * buffer sequence until the dynamic buffer sequence's get area contains the
 * specified delimiter. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions
 * is true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the dynamic buffer sequence's get area already contains the delimiter, this
 * asynchronous operation completes immediately. The program must ensure that
 * the stream performs no other read operations (such as async_read,
 * async_read_until, the stream's async_read_some function, or any other
 * composed operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param delim The delimiter character.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the dynamic buffer sequence's
 *   // get area up to and including the delimiter.
 *   // 0 if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the dynamic buffer
 * sequence may contain additional data beyond the delimiter. An application
 * will typically leave that data in the dynamic buffer sequence for a
 * subsequent async_read_until operation to examine.
 *
 * @par Example
 * To asynchronously read data into a @c std::string until a newline is
 * encountered:
 * @code std::string data;
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size)
 * {
 *   if (!e)
 *   {
 *     std::string line = data.substr(0, n);
 *     data.erase(0, n);
 *     ...
 *   }
 * }
 * ...
 * boost::asio::async_read_until(s, data, '\n', handler); @endcode
 * After the @c async_read_until operation completes successfully, the buffer
 * @c data contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the buffer @c data
 * as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c async_read_until operation.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename DynamicBuffer_v1,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers, char delim,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type),
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

/// Start an asynchronous operation to read data into a dynamic buffer sequence
/// until it contains a specified delimiter.
/**
 * This function is used to asynchronously read data into the specified dynamic
 * buffer sequence until the dynamic buffer sequence's get area contains the
 * specified delimiter. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions
 * is true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the dynamic buffer sequence's get area already contains the delimiter, this
 * asynchronous operation completes immediately. The program must ensure that
 * the stream performs no other read operations (such as async_read,
 * async_read_until, the stream's async_read_some function, or any other
 * composed operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param delim The delimiter string.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the dynamic buffer sequence's
 *   // get area up to and including the delimiter.
 *   // 0 if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the dynamic buffer
 * sequence may contain additional data beyond the delimiter. An application
 * will typically leave that data in the dynamic buffer sequence for a
 * subsequent async_read_until operation to examine.
 *
 * @par Example
 * To asynchronously read data into a @c std::string until a CR-LF sequence is
 * encountered:
 * @code std::string data;
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size)
 * {
 *   if (!e)
 *   {
 *     std::string line = data.substr(0, n);
 *     data.erase(0, n);
 *     ...
 *   }
 * }
 * ...
 * boost::asio::async_read_until(s, data, "\r\n", handler); @endcode
 * After the @c async_read_until operation completes successfully, the string
 * @c data contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the string @c data
 * as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c async_read_until operation.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename DynamicBuffer_v1,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    BOOST_ASIO_STRING_VIEW_PARAM delim,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type),
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if defined(BOOST_ASIO_HAS_BOOST_REGEX) \
  || defined(GENERATING_DOCUMENTATION)

/// Start an asynchronous operation to read data into a dynamic buffer sequence
/// until some part of its data matches a regular expression.
/**
 * This function is used to asynchronously read data into the specified dynamic
 * buffer sequence until the dynamic buffer sequence's get area contains some
 * data that matches a regular expression. The function call always returns
 * immediately. The asynchronous operation will continue until one of the
 * following conditions is true:
 *
 * @li A substring of the dynamic buffer sequence's get area matches the regular
 * expression.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the dynamic buffer sequence's get area already contains data that matches
 * the regular expression, this asynchronous operation completes immediately.
 * The program must ensure that the stream performs no other read operations
 * (such as async_read, async_read_until, the stream's async_read_some
 * function, or any other composed operations that perform reads) until this
 * operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param expr The regular expression.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the dynamic buffer
 *   // sequence's get area up to and including the
 *   // substring that matches the regular expression.
 *   // 0 if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the dynamic buffer
 * sequence may contain additional data beyond that which matched the regular
 * expression. An application will typically leave that data in the dynamic
 * buffer sequence for a subsequent async_read_until operation to examine.
 *
 * @par Example
 * To asynchronously read data into a @c std::string until a CR-LF sequence is
 * encountered:
 * @code std::string data;
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size)
 * {
 *   if (!e)
 *   {
 *     std::string line = data.substr(0, n);
 *     data.erase(0, n);
 *     ...
 *   }
 * }
 * ...
 * boost::asio::async_read_until(s, data,
 *     boost::regex("\r\n"), handler); @endcode
 * After the @c async_read_until operation completes successfully, the string
 * @c data contains the data which matched the regular expression:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the match,
 * so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the string @c data
 * as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c async_read_until operation.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename DynamicBuffer_v1,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    const boost::regex& expr,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type),
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

#endif // defined(BOOST_ASIO_HAS_BOOST_REGEX)
       // || defined(GENERATING_DOCUMENTATION)

/// Start an asynchronous operation to read data into a dynamic buffer sequence
/// until a function object indicates a match.
/**
 * This function is used to asynchronously read data into the specified dynamic
 * buffer sequence until a user-defined match condition function object, when
 * applied to the data contained in the dynamic buffer sequence, indicates a
 * successful match. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions
 * is true:
 *
 * @li The match condition function object returns a std::pair where the second
 * element evaluates to true.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the match condition function object already indicates a match, this
 * asynchronous operation completes immediately. The program must ensure that
 * the stream performs no other read operations (such as async_read,
 * async_read_until, the stream's async_read_some function, or any other
 * composed operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param match_condition The function object to be called to determine whether
 * a match exists. The signature of the function object must be:
 * @code pair<iterator, bool> match_condition(iterator begin, iterator end);
 * @endcode
 * where @c iterator represents the type:
 * @code buffers_iterator<typename DynamicBuffer_v1::const_buffers_type>
 * @endcode
 * The iterator parameters @c begin and @c end define the range of bytes to be
 * scanned to determine whether there is a match. The @c first member of the
 * return value is an iterator marking one-past-the-end of the bytes that have
 * been consumed by the match function. This iterator is used to calculate the
 * @c begin parameter for any subsequent invocation of the match condition. The
 * @c second member of the return value is true if a match has been found, false
 * otherwise.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the dynamic buffer sequence's
 *   // get area that have been fully consumed by the match
 *   // function. O if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the dynamic buffer
 * sequence may contain additional data beyond that which matched the function
 * object. An application will typically leave that data in the dynamic buffer
 * sequence for a subsequent async_read_until operation to examine.
 *
 * @note The default implementation of the @c is_match_condition type trait
 * evaluates to true for function pointers and function objects with a
 * @c result_type typedef. It must be specialised for other user-defined
 * function objects.
 *
 * @par Examples
 * To asynchronously read data into a @c std::string until whitespace is
 * encountered:
 * @code typedef boost::asio::buffers_iterator<
 *     boost::asio::const_buffers_1> iterator;
 *
 * std::pair<iterator, bool>
 * match_whitespace(iterator begin, iterator end)
 * {
 *   iterator i = begin;
 *   while (i != end)
 *     if (std::isspace(*i++))
 *       return std::make_pair(i, true);
 *   return std::make_pair(i, false);
 * }
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size);
 * ...
 * std::string data;
 * boost::asio::async_read_until(s, data, match_whitespace, handler);
 * @endcode
 *
 * To asynchronously read data into a @c std::string until a matching character
 * is found:
 * @code class match_char
 * {
 * public:
 *   explicit match_char(char c) : c_(c) {}
 *
 *   template <typename Iterator>
 *   std::pair<Iterator, bool> operator()(
 *       Iterator begin, Iterator end) const
 *   {
 *     Iterator i = begin;
 *     while (i != end)
 *       if (c_ == *i++)
 *         return std::make_pair(i, true);
 *     return std::make_pair(i, false);
 *   }
 *
 * private:
 *   char c_;
 * };
 *
 * namespace asio {
 *   template <> struct is_match_condition<match_char>
 *     : public boost::true_type {};
 * } // namespace asio
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size);
 * ...
 * std::string data;
 * boost::asio::async_read_until(s, data, match_char('a'), handler);
 * @endcode
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream,
    typename DynamicBuffer_v1, typename MatchCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    MatchCondition match_condition,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type),
    typename constraint<
      is_match_condition<MatchCondition>::value
    >::type = 0,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type = 0);

#if !defined(BOOST_ASIO_NO_IOSTREAM)

/// Start an asynchronous operation to read data into a streambuf until it
/// contains a specified delimiter.
/**
 * This function is used to asynchronously read data into the specified
 * streambuf until the streambuf's get area contains the specified delimiter.
 * The function call always returns immediately. The asynchronous operation
 * will continue until one of the following conditions is true:
 *
 * @li The get area of the streambuf contains the specified delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the streambuf's get area already contains the delimiter, this asynchronous
 * operation completes immediately. The program must ensure that the stream
 * performs no other read operations (such as async_read, async_read_until, the
 * stream's async_read_some function, or any other composed operations that
 * perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read. Ownership of
 * the streambuf is retained by the caller, which must guarantee that it remains
 * valid until the handler is called.
 *
 * @param delim The delimiter character.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the streambuf's get
 *   // area up to and including the delimiter.
 *   // 0 if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the streambuf may
 * contain additional data beyond the delimiter. An application will typically
 * leave that data in the streambuf for a subsequent async_read_until operation
 * to examine.
 *
 * @par Example
 * To asynchronously read data into a streambuf until a newline is encountered:
 * @code boost::asio::streambuf b;
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size)
 * {
 *   if (!e)
 *   {
 *     std::istream is(&b);
 *     std::string line;
 *     std::getline(is, line);
 *     ...
 *   }
 * }
 * ...
 * boost::asio::async_read_until(s, b, '\n', handler); @endcode
 * After the @c async_read_until operation completes successfully, the buffer
 * @c b contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\n', 'd', 'e', ... } @endcode
 * The call to @c std::getline then extracts the data up to and including the
 * newline (which is discarded), so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c' } @endcode
 * The remaining data is left in the buffer @c b as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c async_read_until operation.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename Allocator,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, char delim,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type));

/// Start an asynchronous operation to read data into a streambuf until it
/// contains a specified delimiter.
/**
 * This function is used to asynchronously read data into the specified
 * streambuf until the streambuf's get area contains the specified delimiter.
 * The function call always returns immediately. The asynchronous operation
 * will continue until one of the following conditions is true:
 *
 * @li The get area of the streambuf contains the specified delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the streambuf's get area already contains the delimiter, this asynchronous
 * operation completes immediately. The program must ensure that the stream
 * performs no other read operations (such as async_read, async_read_until, the
 * stream's async_read_some function, or any other composed operations that
 * perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read. Ownership of
 * the streambuf is retained by the caller, which must guarantee that it remains
 * valid until the handler is called.
 *
 * @param delim The delimiter string.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the streambuf's get
 *   // area up to and including the delimiter.
 *   // 0 if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the streambuf may
 * contain additional data beyond the delimiter. An application will typically
 * leave that data in the streambuf for a subsequent async_read_until operation
 * to examine.
 *
 * @par Example
 * To asynchronously read data into a streambuf until a newline is encountered:
 * @code boost::asio::streambuf b;
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size)
 * {
 *   if (!e)
 *   {
 *     std::istream is(&b);
 *     std::string line;
 *     std::getline(is, line);
 *     ...
 *   }
 * }
 * ...
 * boost::asio::async_read_until(s, b, "\r\n", handler); @endcode
 * After the @c async_read_until operation completes successfully, the buffer
 * @c b contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c std::getline then extracts the data up to and including the
 * newline (which is discarded), so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r' } @endcode
 * The remaining data is left in the buffer @c b as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c async_read_until operation.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename Allocator,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    BOOST_ASIO_STRING_VIEW_PARAM delim,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type));

#if defined(BOOST_ASIO_HAS_BOOST_REGEX) \
  || defined(GENERATING_DOCUMENTATION)

/// Start an asynchronous operation to read data into a streambuf until some
/// part of its data matches a regular expression.
/**
 * This function is used to asynchronously read data into the specified
 * streambuf until the streambuf's get area contains some data that matches a
 * regular expression. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions
 * is true:
 *
 * @li A substring of the streambuf's get area matches the regular expression.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the streambuf's get area already contains data that matches the regular
 * expression, this asynchronous operation completes immediately. The program
 * must ensure that the stream performs no other read operations (such as
 * async_read, async_read_until, the stream's async_read_some function, or any
 * other composed operations that perform reads) until this operation
 * completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read. Ownership of
 * the streambuf is retained by the caller, which must guarantee that it remains
 * valid until the handler is called.
 *
 * @param expr The regular expression.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the streambuf's get
 *   // area up to and including the substring
 *   // that matches the regular. expression.
 *   // 0 if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the streambuf may
 * contain additional data beyond that which matched the regular expression. An
 * application will typically leave that data in the streambuf for a subsequent
 * async_read_until operation to examine.
 *
 * @par Example
 * To asynchronously read data into a streambuf until a CR-LF sequence is
 * encountered:
 * @code boost::asio::streambuf b;
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size)
 * {
 *   if (!e)
 *   {
 *     std::istream is(&b);
 *     std::string line;
 *     std::getline(is, line);
 *     ...
 *   }
 * }
 * ...
 * boost::asio::async_read_until(s, b, boost::regex("\r\n"), handler); @endcode
 * After the @c async_read_until operation completes successfully, the buffer
 * @c b contains the data which matched the regular expression:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c std::getline then extracts the data up to and including the
 * newline (which is discarded), so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r' } @endcode
 * The remaining data is left in the buffer @c b as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c async_read_until operation.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename Allocator,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, const boost::regex& expr,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type));

#endif // defined(BOOST_ASIO_HAS_BOOST_REGEX)
       // || defined(GENERATING_DOCUMENTATION)

/// Start an asynchronous operation to read data into a streambuf until a
/// function object indicates a match.
/**
 * This function is used to asynchronously read data into the specified
 * streambuf until a user-defined match condition function object, when applied
 * to the data contained in the streambuf, indicates a successful match. The
 * function call always returns immediately. The asynchronous operation will
 * continue until one of the following conditions is true:
 *
 * @li The match condition function object returns a std::pair where the second
 * element evaluates to true.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the match condition function object already indicates a match, this
 * asynchronous operation completes immediately. The program must ensure that
 * the stream performs no other read operations (such as async_read,
 * async_read_until, the stream's async_read_some function, or any other
 * composed operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param b A streambuf object into which the data will be read.
 *
 * @param match_condition The function object to be called to determine whether
 * a match exists. The signature of the function object must be:
 * @code pair<iterator, bool> match_condition(iterator begin, iterator end);
 * @endcode
 * where @c iterator represents the type:
 * @code buffers_iterator<basic_streambuf<Allocator>::const_buffers_type>
 * @endcode
 * The iterator parameters @c begin and @c end define the range of bytes to be
 * scanned to determine whether there is a match. The @c first member of the
 * return value is an iterator marking one-past-the-end of the bytes that have
 * been consumed by the match function. This iterator is used to calculate the
 * @c begin parameter for any subsequent invocation of the match condition. The
 * @c second member of the return value is true if a match has been found, false
 * otherwise.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the streambuf's get
 *   // area that have been fully consumed by the
 *   // match function. O if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the streambuf may
 * contain additional data beyond that which matched the function object. An
 * application will typically leave that data in the streambuf for a subsequent
 * async_read_until operation to examine.
 *
 * @note The default implementation of the @c is_match_condition type trait
 * evaluates to true for function pointers and function objects with a
 * @c result_type typedef. It must be specialised for other user-defined
 * function objects.
 *
 * @par Examples
 * To asynchronously read data into a streambuf until whitespace is encountered:
 * @code typedef boost::asio::buffers_iterator<
 *     boost::asio::streambuf::const_buffers_type> iterator;
 *
 * std::pair<iterator, bool>
 * match_whitespace(iterator begin, iterator end)
 * {
 *   iterator i = begin;
 *   while (i != end)
 *     if (std::isspace(*i++))
 *       return std::make_pair(i, true);
 *   return std::make_pair(i, false);
 * }
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size);
 * ...
 * boost::asio::streambuf b;
 * boost::asio::async_read_until(s, b, match_whitespace, handler);
 * @endcode
 *
 * To asynchronously read data into a streambuf until a matching character is
 * found:
 * @code class match_char
 * {
 * public:
 *   explicit match_char(char c) : c_(c) {}
 *
 *   template <typename Iterator>
 *   std::pair<Iterator, bool> operator()(
 *       Iterator begin, Iterator end) const
 *   {
 *     Iterator i = begin;
 *     while (i != end)
 *       if (c_ == *i++)
 *         return std::make_pair(i, true);
 *     return std::make_pair(i, false);
 *   }
 *
 * private:
 *   char c_;
 * };
 *
 * namespace asio {
 *   template <> struct is_match_condition<match_char>
 *     : public boost::true_type {};
 * } // namespace asio
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size);
 * ...
 * boost::asio::streambuf b;
 * boost::asio::async_read_until(s, b, match_char('a'), handler);
 * @endcode
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename Allocator, typename MatchCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    MatchCondition match_condition,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type),
    typename constraint<is_match_condition<MatchCondition>::value>::type = 0);

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

/// Start an asynchronous operation to read data into a dynamic buffer sequence
/// until it contains a specified delimiter.
/**
 * This function is used to asynchronously read data into the specified dynamic
 * buffer sequence until the dynamic buffer sequence's get area contains the
 * specified delimiter. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions
 * is true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the dynamic buffer sequence's get area already contains the delimiter, this
 * asynchronous operation completes immediately. The program must ensure that
 * the stream performs no other read operations (such as async_read,
 * async_read_until, the stream's async_read_some function, or any other
 * composed operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param delim The delimiter character.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the dynamic buffer sequence's
 *   // get area up to and including the delimiter.
 *   // 0 if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the dynamic buffer
 * sequence may contain additional data beyond the delimiter. An application
 * will typically leave that data in the dynamic buffer sequence for a
 * subsequent async_read_until operation to examine.
 *
 * @par Example
 * To asynchronously read data into a @c std::string until a newline is
 * encountered:
 * @code std::string data;
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size)
 * {
 *   if (!e)
 *   {
 *     std::string line = data.substr(0, n);
 *     data.erase(0, n);
 *     ...
 *   }
 * }
 * ...
 * boost::asio::async_read_until(s, data, '\n', handler); @endcode
 * After the @c async_read_until operation completes successfully, the buffer
 * @c data contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the buffer @c data
 * as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c async_read_until operation.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename DynamicBuffer_v2,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s, DynamicBuffer_v2 buffers, char delim,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type),
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

/// Start an asynchronous operation to read data into a dynamic buffer sequence
/// until it contains a specified delimiter.
/**
 * This function is used to asynchronously read data into the specified dynamic
 * buffer sequence until the dynamic buffer sequence's get area contains the
 * specified delimiter. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions
 * is true:
 *
 * @li The get area of the dynamic buffer sequence contains the specified
 * delimiter.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the dynamic buffer sequence's get area already contains the delimiter, this
 * asynchronous operation completes immediately. The program must ensure that
 * the stream performs no other read operations (such as async_read,
 * async_read_until, the stream's async_read_some function, or any other
 * composed operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param delim The delimiter string.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the dynamic buffer sequence's
 *   // get area up to and including the delimiter.
 *   // 0 if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the dynamic buffer
 * sequence may contain additional data beyond the delimiter. An application
 * will typically leave that data in the dynamic buffer sequence for a
 * subsequent async_read_until operation to examine.
 *
 * @par Example
 * To asynchronously read data into a @c std::string until a CR-LF sequence is
 * encountered:
 * @code std::string data;
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size)
 * {
 *   if (!e)
 *   {
 *     std::string line = data.substr(0, n);
 *     data.erase(0, n);
 *     ...
 *   }
 * }
 * ...
 * boost::asio::async_read_until(s, data, "\r\n", handler); @endcode
 * After the @c async_read_until operation completes successfully, the string
 * @c data contains the delimiter:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the
 * delimiter, so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the string @c data
 * as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c async_read_until operation.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename DynamicBuffer_v2,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s, DynamicBuffer_v2 buffers,
    BOOST_ASIO_STRING_VIEW_PARAM delim,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type),
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if defined(BOOST_ASIO_HAS_BOOST_REGEX) \
  || defined(GENERATING_DOCUMENTATION)

/// Start an asynchronous operation to read data into a dynamic buffer sequence
/// until some part of its data matches a regular expression.
/**
 * This function is used to asynchronously read data into the specified dynamic
 * buffer sequence until the dynamic buffer sequence's get area contains some
 * data that matches a regular expression. The function call always returns
 * immediately. The asynchronous operation will continue until one of the
 * following conditions is true:
 *
 * @li A substring of the dynamic buffer sequence's get area matches the regular
 * expression.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the dynamic buffer sequence's get area already contains data that matches
 * the regular expression, this asynchronous operation completes immediately.
 * The program must ensure that the stream performs no other read operations
 * (such as async_read, async_read_until, the stream's async_read_some
 * function, or any other composed operations that perform reads) until this
 * operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param expr The regular expression.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the dynamic buffer
 *   // sequence's get area up to and including the
 *   // substring that matches the regular expression.
 *   // 0 if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the dynamic buffer
 * sequence may contain additional data beyond that which matched the regular
 * expression. An application will typically leave that data in the dynamic
 * buffer sequence for a subsequent async_read_until operation to examine.
 *
 * @par Example
 * To asynchronously read data into a @c std::string until a CR-LF sequence is
 * encountered:
 * @code std::string data;
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size)
 * {
 *   if (!e)
 *   {
 *     std::string line = data.substr(0, n);
 *     data.erase(0, n);
 *     ...
 *   }
 * }
 * ...
 * boost::asio::async_read_until(s, data,
 *     boost::regex("\r\n"), handler); @endcode
 * After the @c async_read_until operation completes successfully, the string
 * @c data contains the data which matched the regular expression:
 * @code { 'a', 'b', ..., 'c', '\r', '\n', 'd', 'e', ... } @endcode
 * The call to @c substr then extracts the data up to and including the match,
 * so that the string @c line contains:
 * @code { 'a', 'b', ..., 'c', '\r', '\n' } @endcode
 * After the call to @c erase, the remaining data is left in the string @c data
 * as follows:
 * @code { 'd', 'e', ... } @endcode
 * This data may be the start of a new line, to be extracted by a subsequent
 * @c async_read_until operation.
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream, typename DynamicBuffer_v2,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s, DynamicBuffer_v2 buffers,
    const boost::regex& expr,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type),
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

#endif // defined(BOOST_ASIO_HAS_BOOST_REGEX)
       // || defined(GENERATING_DOCUMENTATION)

/// Start an asynchronous operation to read data into a dynamic buffer sequence
/// until a function object indicates a match.
/**
 * This function is used to asynchronously read data into the specified dynamic
 * buffer sequence until a user-defined match condition function object, when
 * applied to the data contained in the dynamic buffer sequence, indicates a
 * successful match. The function call always returns immediately. The
 * asynchronous operation will continue until one of the following conditions
 * is true:
 *
 * @li The match condition function object returns a std::pair where the second
 * element evaluates to true.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. If
 * the match condition function object already indicates a match, this
 * asynchronous operation completes immediately. The program must ensure that
 * the stream performs no other read operations (such as async_read,
 * async_read_until, the stream's async_read_some function, or any other
 * composed operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers The dynamic buffer sequence into which the data will be read.
 * Although the buffers object may be copied as necessary, ownership of the
 * underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param match_condition The function object to be called to determine whether
 * a match exists. The signature of the function object must be:
 * @code pair<iterator, bool> match_condition(iterator begin, iterator end);
 * @endcode
 * where @c iterator represents the type:
 * @code buffers_iterator<typename DynamicBuffer_v2::const_buffers_type>
 * @endcode
 * The iterator parameters @c begin and @c end define the range of bytes to be
 * scanned to determine whether there is a match. The @c first member of the
 * return value is an iterator marking one-past-the-end of the bytes that have
 * been consumed by the match function. This iterator is used to calculate the
 * @c begin parameter for any subsequent invocation of the match condition. The
 * @c second member of the return value is true if a match has been found, false
 * otherwise.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   // Result of operation.
 *   const boost::system::error_code& error,
 *
 *   // The number of bytes in the dynamic buffer sequence's
 *   // get area that have been fully consumed by the match
 *   // function. O if an error occurred.
 *   std::size_t bytes_transferred
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. On
 * immediate completion, invocation of the handler will be performed in a
 * manner equivalent to using boost::asio::post().
 *
 * @note After a successful async_read_until operation, the dynamic buffer
 * sequence may contain additional data beyond that which matched the function
 * object. An application will typically leave that data in the dynamic buffer
 * sequence for a subsequent async_read_until operation to examine.
 *
 * @note The default implementation of the @c is_match_condition type trait
 * evaluates to true for function pointers and function objects with a
 * @c result_type typedef. It must be specialised for other user-defined
 * function objects.
 *
 * @par Examples
 * To asynchronously read data into a @c std::string until whitespace is
 * encountered:
 * @code typedef boost::asio::buffers_iterator<
 *     boost::asio::const_buffers_1> iterator;
 *
 * std::pair<iterator, bool>
 * match_whitespace(iterator begin, iterator end)
 * {
 *   iterator i = begin;
 *   while (i != end)
 *     if (std::isspace(*i++))
 *       return std::make_pair(i, true);
 *   return std::make_pair(i, false);
 * }
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size);
 * ...
 * std::string data;
 * boost::asio::async_read_until(s, data, match_whitespace, handler);
 * @endcode
 *
 * To asynchronously read data into a @c std::string until a matching character
 * is found:
 * @code class match_char
 * {
 * public:
 *   explicit match_char(char c) : c_(c) {}
 *
 *   template <typename Iterator>
 *   std::pair<Iterator, bool> operator()(
 *       Iterator begin, Iterator end) const
 *   {
 *     Iterator i = begin;
 *     while (i != end)
 *       if (c_ == *i++)
 *         return std::make_pair(i, true);
 *     return std::make_pair(i, false);
 *   }
 *
 * private:
 *   char c_;
 * };
 *
 * namespace asio {
 *   template <> struct is_match_condition<match_char>
 *     : public boost::true_type {};
 * } // namespace asio
 * ...
 * void handler(const boost::system::error_code& e, std::size_t size);
 * ...
 * std::string data;
 * boost::asio::async_read_until(s, data, match_char('a'), handler);
 * @endcode
 *
 * @par Per-Operation Cancellation
 * This asynchronous operation supports cancellation for the following
 * boost::asio::cancellation_type values:
 *
 * @li @c cancellation_type::terminal
 *
 * @li @c cancellation_type::partial
 *
 * if they are also supported by the @c AsyncReadStream type's
 * @c async_read_some operation.
 */
template <typename AsyncReadStream,
    typename DynamicBuffer_v2, typename MatchCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) ReadHandler
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(
          typename AsyncReadStream::executor_type)>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s, DynamicBuffer_v2 buffers,
    MatchCondition match_condition,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(
        typename AsyncReadStream::executor_type),
    typename constraint<
      is_match_condition<MatchCondition>::value
    >::type = 0,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type = 0);

#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)

/*@}*/

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/read_until.hpp>

#endif // BOOST_ASIO_READ_UNTIL_HPP
