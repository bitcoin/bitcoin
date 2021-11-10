/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   text_ostream_backend.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of a text output stream sink backend.
 */

#ifndef BOOST_LOG_SINKS_TEXT_OSTREAM_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_TEXT_OSTREAM_BACKEND_HPP_INCLUDED_

#include <ostream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/keywords/auto_flush.hpp>
#include <boost/log/keywords/auto_newline_mode.hpp>
#include <boost/log/sinks/auto_newline_mode.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

/*!
 * \brief An implementation of a text output stream logging sink backend
 *
 * The sink backend puts formatted log records to one or more text streams.
 */
template< typename CharT >
class basic_text_ostream_backend :
    public basic_formatted_sink_backend<
        CharT,
        combine_requirements< synchronized_feeding, flushing >::type
    >
{
    //! Base type
    typedef basic_formatted_sink_backend<
        CharT,
        combine_requirements< synchronized_feeding, flushing >::type
    > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type to be used as a message text holder
    typedef typename base_type::string_type string_type;
    //! Output stream type
    typedef std::basic_ostream< char_type > stream_type;

private:
    //! \cond

    struct implementation;
    implementation* m_pImpl;

    //! \endcond

public:
    /*!
     * Constructor. No streams attached to the constructed backend, auto flush feature disabled.
     */
    BOOST_LOG_API basic_text_ostream_backend();

    /*!
     * Constructor. Creates a sink backend with the specified named parameters.
     * The following named parameters are supported:
     *
     * \li \c auto_flush - Specifies a flag, whether or not to automatically flush the attached streams after each
     *                     written log record. By default, is \c false.
     * \li \c auto_newline_mode - Specifies automatic trailing newline insertion mode. Must be a value of
     *                            the \c auto_newline_mode enum. By default, is <tt>auto_newline_mode::insert_if_missing</tt>.
     */
#ifndef BOOST_LOG_DOXYGEN_PASS
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_CALL(basic_text_ostream_backend, construct)
#else
    template< typename... ArgsT >
    explicit basic_text_ostream_backend(ArgsT... const& args);
#endif

    /*!
     * Destructor
     */
    BOOST_LOG_API ~basic_text_ostream_backend();

    /*!
     * The method adds a new stream to the sink.
     *
     * \param strm Pointer to the stream. Must not be NULL.
     */
    BOOST_LOG_API void add_stream(shared_ptr< stream_type > const& strm);
    /*!
     * The method removes a stream from the sink. If the stream is not attached to the sink,
     * the method has no effect.
     *
     * \param strm Pointer to the stream. Must not be NULL.
     */
    BOOST_LOG_API void remove_stream(shared_ptr< stream_type > const& strm);

    /*!
     * Sets the flag to automatically flush buffers of all attached streams after each log record.
     *
     * \param enable The flag indicates whether the automatic buffer flush should be performed.
     */
    BOOST_LOG_API void auto_flush(bool enable = true);

    /*!
     * Selects whether a trailing newline should be automatically inserted after every log record. See
     * \c auto_newline_mode description for the possible modes of operation.
     *
     * \param mode The trailing newline insertion mode.
     */
    BOOST_LOG_API void set_auto_newline_mode(auto_newline_mode mode);

    /*!
     * The method writes the message to the sink.
     */
    BOOST_LOG_API void consume(record_view const& rec, string_type const& formatted_message);

    /*!
     * The method flushes all attached streams.
     */
    BOOST_LOG_API void flush();

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Constructor implementation
    template< typename ArgsT >
    void construct(ArgsT const& args)
    {
        construct(
            args[keywords::auto_newline_mode | insert_if_missing],
            args[keywords::auto_flush | false]);
    }
    //! Constructor implementation
    BOOST_LOG_API void construct(auto_newline_mode auto_newline, bool auto_flush);
#endif // BOOST_LOG_DOXYGEN_PASS
};

#ifdef BOOST_LOG_USE_CHAR
typedef basic_text_ostream_backend< char > text_ostream_backend;        //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_text_ostream_backend< wchar_t > wtext_ostream_backend;    //!< Convenience typedef for wide-character logging
#endif

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_TEXT_OSTREAM_BACKEND_HPP_INCLUDED_
