/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   channel_logger.hpp
 * \author Andrey Semashev
 * \date   28.02.2008
 *
 * The header contains implementation of a logger with channel support.
 */

#ifndef BOOST_LOG_SOURCES_CHANNEL_LOGGER_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_CHANNEL_LOGGER_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/detail/light_rw_mutex.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/sources/features.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/sources/threading_models.hpp>
#include <boost/log/sources/channel_feature.hpp>
#include <boost/log/keywords/channel.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sources {

#ifndef BOOST_LOG_DOXYGEN_PASS

#ifdef BOOST_LOG_USE_CHAR

//! Narrow-char logger with channel support
template< typename ChannelT = std::string >
class channel_logger :
    public basic_composite_logger<
        char,
        channel_logger< ChannelT >,
        single_thread_model,
        features< channel< ChannelT > >
    >
{
    typedef typename channel_logger::logger_base base_type;

public:
    BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(channel_logger)

    explicit channel_logger(ChannelT const& channel) : base_type(keywords::channel = channel)
    {
    }
};

#if !defined(BOOST_LOG_NO_THREADS)

//! Narrow-char thread-safe logger with channel support
template< typename ChannelT = std::string >
class channel_logger_mt :
    public basic_composite_logger<
        char,
        channel_logger_mt< ChannelT >,
        multi_thread_model< boost::log::aux::light_rw_mutex >,
        features< channel< ChannelT > >
    >
{
    typedef typename channel_logger_mt::logger_base base_type;

public:
    BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(channel_logger_mt)

    explicit channel_logger_mt(ChannelT const& channel) : base_type(keywords::channel = channel)
    {
    }
};

#endif // !defined(BOOST_LOG_NO_THREADS)

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

//! Wide-char logger with channel support
template< typename ChannelT = std::wstring >
class wchannel_logger :
    public basic_composite_logger<
        wchar_t,
        wchannel_logger< ChannelT >,
        single_thread_model,
        features< channel< ChannelT > >
    >
{
    typedef typename wchannel_logger::logger_base base_type;

public:
    BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(wchannel_logger)

    explicit wchannel_logger(ChannelT const& channel) : base_type(keywords::channel = channel)
    {
    }
};

#if !defined(BOOST_LOG_NO_THREADS)

//! Wide-char thread-safe logger with channel support
template< typename ChannelT = std::wstring >
class wchannel_logger_mt :
    public basic_composite_logger<
        wchar_t,
        wchannel_logger< ChannelT >,
        multi_thread_model< boost::log::aux::light_rw_mutex >,
        features< channel< ChannelT > >
    >
{
    typedef typename wchannel_logger_mt::logger_base base_type;

public:
    BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(wchannel_logger_mt)

    explicit wchannel_logger_mt(ChannelT const& channel) : base_type(keywords::channel = channel)
    {
    }
};

#endif // !defined(BOOST_LOG_NO_THREADS)

#endif // BOOST_LOG_USE_WCHAR_T

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief Narrow-char logger. Functionally equivalent to \c basic_channel_logger.
 *
 * See \c channel class template for a more detailed description
 */
template< typename ChannelT = std::string >
class channel_logger :
    public basic_composite_logger<
        char,
        channel_logger< ChannelT >,
        single_thread_model,
        features< channel< ChannelT > >
    >
{
public:
    /*!
     * Default constructor
     */
    channel_logger();
    /*!
     * Copy constructor
     */
    channel_logger(channel_logger const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit channel_logger(ArgsT... const& args);
    /*!
     * The constructor creates the logger with the specified channel name
     *
     * \param channel The channel name
     */
    explicit channel_logger(ChannelT const& channel);
    /*!
     * Assignment operator
     */
    channel_logger& operator= (channel_logger const& that)
    /*!
     * Swaps two loggers
     */
    void swap(channel_logger& that);
};

/*!
 * \brief Narrow-char thread-safe logger. Functionally equivalent to \c basic_channel_logger.
 *
 * See \c channel class template for a more detailed description
 */
template< typename ChannelT = std::string >
class channel_logger_mt :
    public basic_composite_logger<
        char,
        channel_logger_mt< ChannelT >,
        multi_thread_model< implementation_defined >,
        features< channel< ChannelT > >
    >
{
public:
    /*!
     * Default constructor
     */
    channel_logger_mt();
    /*!
     * Copy constructor
     */
    channel_logger_mt(channel_logger_mt const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit channel_logger_mt(ArgsT... const& args);
    /*!
     * The constructor creates the logger with the specified channel name
     *
     * \param channel The channel name
     */
    explicit channel_logger_mt(ChannelT const& channel);
    /*!
     * Assignment operator
     */
    channel_logger_mt& operator= (channel_logger_mt const& that)
    /*!
     * Swaps two loggers
     */
    void swap(channel_logger_mt& that);
};

/*!
 * \brief Wide-char logger. Functionally equivalent to \c basic_channel_logger.
 *
 * See \c channel class template for a more detailed description
 */
template< typename ChannelT = std::wstring >
class wchannel_logger :
    public basic_composite_logger<
        wchar_t,
        wchannel_logger< ChannelT >,
        single_thread_model,
        features< channel< ChannelT > >
    >
{
public:
    /*!
     * Default constructor
     */
    wchannel_logger();
    /*!
     * Copy constructor
     */
    wchannel_logger(wchannel_logger const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit wchannel_logger(ArgsT... const& args);
    /*!
     * The constructor creates the logger with the specified channel name
     *
     * \param channel The channel name
     */
    explicit wchannel_logger(ChannelT const& channel);
    /*!
     * Assignment operator
     */
    wchannel_logger& operator= (wchannel_logger const& that)
    /*!
     * Swaps two loggers
     */
    void swap(wchannel_logger& that);
};

/*!
 * \brief Wide-char thread-safe logger. Functionally equivalent to \c basic_channel_logger.
 *
 * See \c channel class template for a more detailed description
 */
template< typename ChannelT = std::wstring >
class wchannel_logger_mt :
    public basic_composite_logger<
        wchar_t,
        wchannel_logger< ChannelT >,
        multi_thread_model< implementation_defined >,
        features< channel< ChannelT > >
    >
{
public:
    /*!
     * Default constructor
     */
    wchannel_logger_mt();
    /*!
     * Copy constructor
     */
    wchannel_logger_mt(wchannel_logger_mt const& that);
    /*!
     * Constructor with named arguments
     */
    template< typename... ArgsT >
    explicit wchannel_logger_mt(ArgsT... const& args);
    /*!
     * The constructor creates the logger with the specified channel name
     *
     * \param channel The channel name
     */
    explicit wchannel_logger_mt(ChannelT const& channel);
    /*!
     * Assignment operator
     */
    wchannel_logger_mt& operator= (wchannel_logger_mt const& that)
    /*!
     * Swaps two loggers
     */
    void swap(wchannel_logger_mt& that);
};

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace sources

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SOURCES_CHANNEL_LOGGER_HPP_INCLUDED_
