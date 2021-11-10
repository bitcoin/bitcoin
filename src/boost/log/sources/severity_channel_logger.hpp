/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   severity_channel_logger.hpp
 * \author Andrey Semashev
 * \date   28.02.2008
 *
 * The header contains implementation of a logger with severity level and channel support.
 */

#ifndef BOOST_LOG_SOURCES_SEVERITY_CHANNEL_LOGGER_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_SEVERITY_CHANNEL_LOGGER_HPP_INCLUDED_

#include <string>
#include <boost/log/detail/config.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/detail/light_rw_mutex.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/sources/features.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/sources/threading_models.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/channel_feature.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sources {

#ifndef BOOST_LOG_DOXYGEN_PASS

#ifdef BOOST_LOG_USE_CHAR

//! Narrow-char logger with severity level and channel support
template< typename LevelT = int, typename ChannelT = std::string >
class severity_channel_logger :
    public basic_composite_logger<
        char,
        severity_channel_logger< LevelT, ChannelT >,
        single_thread_model,
        features<
            severity< LevelT >,
            channel< ChannelT >
        >
    >
{
    BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(severity_channel_logger)
};

#if !defined(BOOST_LOG_NO_THREADS)

//! Narrow-char thread-safe logger with severity level and channel support
template< typename LevelT = int, typename ChannelT = std::string >
class severity_channel_logger_mt :
    public basic_composite_logger<
        char,
        severity_channel_logger_mt< LevelT, ChannelT >,
        multi_thread_model< boost::log::aux::light_rw_mutex >,
        features<
            severity< LevelT >,
            channel< ChannelT >
        >
    >
{
    BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(severity_channel_logger_mt)
};

#endif // !defined(BOOST_LOG_NO_THREADS)

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

//! Wide-char logger with severity level and channel support
template< typename LevelT = int, typename ChannelT = std::wstring >
class wseverity_channel_logger :
    public basic_composite_logger<
        wchar_t,
        wseverity_channel_logger< LevelT, ChannelT >,
        single_thread_model,
        features<
            severity< LevelT >,
            channel< ChannelT >
        >
    >
{
    BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(wseverity_channel_logger)
};

#if !defined(BOOST_LOG_NO_THREADS)

//! Wide-char thread-safe logger with severity level and channel support
template< typename LevelT = int, typename ChannelT = std::wstring >
class wseverity_channel_logger_mt :
    public basic_composite_logger<
        wchar_t,
        wseverity_channel_logger_mt< LevelT, ChannelT >,
        multi_thread_model< boost::log::aux::light_rw_mutex >,
        features<
            severity< LevelT >,
            channel< ChannelT >
        >
    >
{
    BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(wseverity_channel_logger_mt)
};

#endif // !defined(BOOST_LOG_NO_THREADS)

#endif // BOOST_LOG_USE_WCHAR_T

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief Narrow-char logger. Functionally equivalent to \c basic_severity_logger and \c basic_channel_logger.
 *
 * See \c severity and \c channel class templates for a more detailed description
 */
template< typename LevelT = int, typename ChannelT = std::string >
class severity_channel_logger :
    public basic_composite_logger<
        char,
        severity_channel_logger< LevelT, ChannelT >,
        single_thread_model,
        features<
            severity< LevelT >,
            channel< ChannelT >
        >
    >
{
public:
    /*!
     * Default constructor
     */
    severity_channel_logger();
    /*!
     * Copy constructor
     */
    severity_channel_logger(severity_channel_logger const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit severity_channel_logger(ArgsT... const& args);
    /*!
     * Assignment operator
     */
    severity_channel_logger& operator= (severity_channel_logger const& that)
    /*!
     * Swaps two loggers
     */
    void swap(severity_channel_logger& that);
};

/*!
 * \brief Narrow-char thread-safe logger. Functionally equivalent to \c basic_severity_logger and \c basic_channel_logger.
 *
 * See \c severity and \c channel class templates for a more detailed description
 */
template< typename LevelT = int, typename ChannelT = std::string >
class severity_channel_logger_mt :
    public basic_composite_logger<
        char,
        severity_channel_logger_mt< LevelT, ChannelT >,
        multi_thread_model< implementation_defined >,
        features<
            severity< LevelT >,
            channel< ChannelT >
        >
    >
{
public:
    /*!
     * Default constructor
     */
    severity_channel_logger_mt();
    /*!
     * Copy constructor
     */
    severity_channel_logger_mt(severity_channel_logger_mt const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit severity_channel_logger_mt(ArgsT... const& args);
    /*!
     * Assignment operator
     */
    severity_channel_logger_mt& operator= (severity_channel_logger_mt const& that)
    /*!
     * Swaps two loggers
     */
    void swap(severity_channel_logger_mt& that);
};

/*!
 * \brief Wide-char logger. Functionally equivalent to \c basic_severity_logger and \c basic_channel_logger.
 *
 * See \c severity and \c channel class templates for a more detailed description
 */
template< typename LevelT = int, typename ChannelT = std::wstring >
class wseverity_channel_logger :
    public basic_composite_logger<
        wchar_t,
        wseverity_channel_logger< LevelT, ChannelT >,
        single_thread_model,
        features<
            severity< LevelT >,
            channel< ChannelT >
        >
    >
{
public:
    /*!
     * Default constructor
     */
    wseverity_channel_logger();
    /*!
     * Copy constructor
     */
    wseverity_channel_logger(wseverity_channel_logger const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit wseverity_channel_logger(ArgsT... const& args);
    /*!
     * Assignment operator
     */
    wseverity_channel_logger& operator= (wseverity_channel_logger const& that)
    /*!
     * Swaps two loggers
     */
    void swap(wseverity_channel_logger& that);
};

/*!
 * \brief Wide-char thread-safe logger. Functionally equivalent to \c basic_severity_logger and \c basic_channel_logger.
 *
 * See \c severity and \c channel class templates for a more detailed description
 */
template< typename LevelT = int, typename ChannelT = std::wstring >
class wseverity_channel_logger_mt :
    public basic_composite_logger<
        wchar_t,
        wseverity_channel_logger_mt< LevelT, ChannelT >,
        multi_thread_model< implementation_defined >,
        features<
            severity< LevelT >,
            channel< ChannelT >
        >
    >
{
public:
    /*!
     * Default constructor
     */
    wseverity_channel_logger_mt();
    /*!
     * Copy constructor
     */
    wseverity_channel_logger_mt(wseverity_channel_logger_mt const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit wseverity_channel_logger_mt(ArgsT... const& args);
    /*!
     * Assignment operator
     */
    wseverity_channel_logger_mt& operator= (wseverity_channel_logger_mt const& that)
    /*!
     * Swaps two loggers
     */
    void swap(wseverity_channel_logger_mt& that);
};

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace sources

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

//! The macro allows to put a record with a specific channel name into log
#define BOOST_LOG_STREAM_CHANNEL_SEV(logger, chan, lvl)\
    BOOST_LOG_STREAM_WITH_PARAMS((logger), (::boost::log::keywords::channel = (chan))(::boost::log::keywords::severity = (lvl)))

#ifndef BOOST_LOG_NO_SHORTHAND_NAMES

//! An equivalent to BOOST_LOG_STREAM_CHANNEL_SEV(logger, chan, lvl)
#define BOOST_LOG_CHANNEL_SEV(logger, chan, lvl) BOOST_LOG_STREAM_CHANNEL_SEV(logger, chan, lvl)

#endif // BOOST_LOG_NO_SHORTHAND_NAMES

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SOURCES_SEVERITY_CHANNEL_LOGGER_HPP_INCLUDED_
