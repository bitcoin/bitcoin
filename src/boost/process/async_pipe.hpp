// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PROCESS_ASYNC_PIPE_HPP
#define BOOST_PROCESS_ASYNC_PIPE_HPP

#include <boost/config.hpp>
#include <boost/process/detail/config.hpp>

#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/async_pipe.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/async_pipe.hpp>
#endif

namespace boost { namespace process {


#if defined(BOOST_PROCESS_DOXYGEN)


/** Class implementing and asnychronous I/O-Object for use with boost.asio.
 *  It is based on the corresponding I/O Object, that is either boost::asio::windows::stream_handle or
 *  boost::asio::posix::stream_descriptor.
 *
 *  It can be used directly with boost::asio::async_read or async_write.
 *
 * \note The object is copyable, but that does invoke a handle duplicate.
 */
class async_pipe
{
public:
    /** Typedef for the native handle representation.
     * \note This is the handle on the system, not the boost.asio class.
     *
     */
    typedef platform_specific native_handle_type;
    /** Typedef for the handle representation of boost.asio.
     *
     */
    typedef platform_specific handle_type;

    typedef typename handle_type::executor_type executor_type;

    /** Construct a new async_pipe, does automatically open the pipe.
     * Initializes source and sink with the same io_context.
     * @note Windows creates a named pipe here, where the name is automatically generated.
     */
    inline async_pipe(boost::asio::io_context & ios);

    /** Construct a new async_pipe, does automatically open the pipe.
     * @note Windows creates a named pipe here, where the name is automatically generated.
     */
    inline async_pipe(boost::asio::io_context & ios_source,
                      boost::asio::io_context & ios_sink);

    /** Construct a new async_pipe, does automatically open.
     * Initializes source and sink with the same io_context.
     *
     * @note Windows restricts possible names.
     */
    inline async_pipe(boost::asio::io_context & ios, const std::string & name);


    /** Construct a new async_pipe, does automatically open.
     *
     * @note Windows restricts possible names.
     */
    inline async_pipe(boost::asio::io_context & ios_source,
                      boost::asio::io_context & ios_sink, const std::string & name);

    /** Copy-Constructor of the async pipe.
     * @note Windows requires a named pipe for this, if a the wrong type is used an exception is thrown.
     *
     */
    async_pipe(const async_pipe& lhs);

    /** Move-Constructor of the async pipe.
     */
    async_pipe(async_pipe&& lhs);

    /** Construct the async-pipe from a pipe.
     * @note Windows requires a named pipe for this, if a the wrong type is used an exception is thrown.
     *
     */
    template<class CharT, class Traits = std::char_traits<CharT>>
    explicit async_pipe(boost::asio::io_context & ios, const basic_pipe<CharT, Traits> & p);

    /** Construct the async-pipe from a pipe, with two different io_context objects.
     * @note Windows requires a named pipe for this, if a the wrong type is used an exception is thrown.
     *
     */
    template<class CharT, class Traits = std::char_traits<CharT>>
    explicit async_pipe(boost::asio::io_context & ios_source,
                        boost::asio::io_context & ios_sink,
                        const basic_pipe<CharT, Traits> & p);


    /** Assign a basic_pipe.
     * @note Windows requires a named pipe for this, if a the wrong type is used an exception is thrown.
     *
     */
    template<class CharT, class Traits = std::char_traits<CharT>>
    inline async_pipe& operator=(const basic_pipe<CharT, Traits>& p);

    /** Copy Assign a pipe.
     * @note Duplicates the handles.
     */
    async_pipe& operator=(const async_pipe& lhs);
    /** Move assign a pipe */
    async_pipe& operator=(async_pipe&& lhs);

    /** Destructor. Closes the pipe handles. */
    ~async_pipe();

    /** Explicit cast to basic_pipe.  */
    template<class CharT, class Traits = std::char_traits<CharT>>
    inline explicit operator basic_pipe<CharT, Traits>() const;

    /** Cancel the current asynchronous operations. */
    void cancel();
    /** Close the pipe handles. */
    void close();
    /** Close the pipe handles. While passing an error_code
     *
     */
    void close(std::error_code & ec);

    /** Check if the pipes are open. */
    bool is_open() const;

    /** Async close, i.e. close after current operation is completed.
     *
     *  \note There is no guarantee that this will indeed read the entire pipe-buffer
     */
    void async_close();

    /** Read some data from the handle.

     * See the boost.asio documentation for more details.
     */
    template<typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence & buffers);

    /** Write some data to the handle.

     * See the boost.asio documentation for more details.
     */
    template<typename MutableBufferSequence>
    std::size_t write_some(const MutableBufferSequence & buffers);

    /** Get the native handle of the source. */
    native_handle native_source() const {return const_cast<boost::asio::windows::stream_handle&>(_source).native();}
    /** Get the native handle of the sink. */
    native_handle native_sink  () const {return const_cast<boost::asio::windows::stream_handle&>(_sink  ).native();}

    /** Start an asynchronous read.
     *
     * See the [boost.asio documentation](http://www.boost.org/doc/libs/1_60_0/doc/html/boost_asio/reference/AsyncReadStream.html) for more details.
     */
    template<typename MutableBufferSequence,
             typename ReadHandler>
    detail::dummy async_read_some(
        const MutableBufferSequence & buffers,
              ReadHandler &&handler);

    /** Start an asynchronous write.

     * See the [boost.asio documentation](http://www.boost.org/doc/libs/1_60_0/doc/html/boost_asio/reference/AsyncWriteStream.html) for more details.
     */
    template<typename ConstBufferSequence,
             typename WriteHandler>
    detail::dummy async_write_some(
        const ConstBufferSequence & buffers,
        WriteHandler && handler);

    ///Get the asio handle of the pipe sink.
    const handle_type & sink  () const &;
    ///Get the asio handle of the pipe source.
    const handle_type & source() const &;

    ///Get the asio handle of the pipe sink. Qualified as rvalue
    handle_type && sink  () &&;
    ///Get the asio handle of the pipe source. Qualified as rvalue
    handle_type && source() &&;

    /// Move the source out of this class and change the io_context. Qualified as rvalue. \attention Will always move.
    handle_type source(::boost::asio::io_context& ios) &&;
    /// Move the sink out of this class and change the io_context. Qualified as rvalue. \attention Will always move
    handle_type sink  (::boost::asio::io_context& ios) &&;

    /// Copy the source out of this class and change the io_context. \attention Will always copy.
    handle_type source(::boost::asio::io_context& ios) const &;
    /// Copy the sink out of this class and change the io_context. \attention Will always copy
    handle_type sink  (::boost::asio::io_context& ios) const &;



};

#else
using ::boost::process::detail::api::async_pipe;
#endif


}}



#endif
