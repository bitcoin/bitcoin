/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   channel_feature.hpp
 * \author Andrey Semashev
 * \date   28.02.2008
 *
 * The header contains implementation of a channel support feature.
 */

#ifndef BOOST_LOG_SOURCES_CHANNEL_FEATURE_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_CHANNEL_FEATURE_HPP_INCLUDED_

#include <string>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/locks.hpp>
#include <boost/log/detail/default_attribute_names.hpp>
#include <boost/log/keywords/channel.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/utility/strictest_lock.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sources {

/*!
 * \brief Channel feature implementation
 */
template< typename BaseT, typename ChannelT >
class basic_channel_logger :
    public BaseT
{
    //! Base type
    typedef BaseT base_type;
    typedef basic_channel_logger this_type;
    BOOST_COPYABLE_AND_MOVABLE_ALT(this_type)

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! Final type
    typedef typename base_type::final_type final_type;
    //! Threading model being used
    typedef typename base_type::threading_model threading_model;

    //! Channel type
    typedef ChannelT channel_type;
    //! Channel attribute type
    typedef attributes::mutable_constant< channel_type > channel_attribute;

    //! Lock requirement for the \c open_record_unlocked method
    typedef typename strictest_lock<
        typename base_type::open_record_lock,
#ifndef BOOST_LOG_NO_THREADS
        boost::log::aux::exclusive_lock_guard< threading_model >
#else
        no_lock< threading_model >
#endif // !defined(BOOST_LOG_NO_THREADS)
    >::type open_record_lock;

    //! Lock requirement for the \c swap_unlocked method
    typedef typename strictest_lock<
        typename base_type::swap_lock,
#ifndef BOOST_LOG_NO_THREADS
        boost::log::aux::multiple_unique_lock2< threading_model, threading_model >
#else
        no_lock< threading_model >
#endif // !defined(BOOST_LOG_NO_THREADS)
    >::type swap_lock;

private:
    //! Default channel name generator
    struct make_default_channel_name
    {
        typedef channel_type result_type;
        result_type operator() () const { return result_type(); }
    };

private:
    //! Channel attribute
    channel_attribute m_ChannelAttr;

public:
    /*!
     * Default constructor. The constructed logger has the default-constructed channel name.
     */
    basic_channel_logger() : base_type(), m_ChannelAttr(channel_type())
    {
        base_type::add_attribute_unlocked(boost::log::aux::default_attribute_names::channel(), m_ChannelAttr);
    }
    /*!
     * Copy constructor
     */
    basic_channel_logger(basic_channel_logger const& that) :
        base_type(static_cast< base_type const& >(that)),
        m_ChannelAttr(that.m_ChannelAttr.get())
    {
        // Our attributes must refer to our channel attribute
        base_type::attributes()[boost::log::aux::default_attribute_names::channel()] = m_ChannelAttr;
    }
    /*!
     * Move constructor
     */
    basic_channel_logger(BOOST_RV_REF(basic_channel_logger) that) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible< base_type >::value && boost::is_nothrow_move_constructible< channel_attribute >::value) :
        base_type(boost::move(static_cast< base_type& >(that))),
        m_ChannelAttr(boost::move(that.m_ChannelAttr))
    {
    }
    /*!
     * Constructor with arguments. Allows to register a channel name attribute on construction.
     *
     * \param args A set of named arguments. The following arguments are supported:
     *             \li \c channel - a string that represents the channel name
     */
    template< typename ArgsT >
    explicit basic_channel_logger(ArgsT const& args) :
        base_type(args),
        m_ChannelAttr(args[keywords::channel || make_default_channel_name()])
    {
        base_type::add_attribute_unlocked(boost::log::aux::default_attribute_names::channel(), m_ChannelAttr);
    }

    /*!
     * The observer of the channel name
     *
     * \return The channel name that was set by the logger
     */
    channel_type channel() const
    {
        BOOST_LOG_EXPR_IF_MT(boost::log::aux::shared_lock_guard< const threading_model > lock(this->get_threading_model());)
        return m_ChannelAttr.get();
    }

    /*!
     * The setter of the channel name
     *
     * \param ch The channel name to be set for the logger
     */
    void channel(channel_type const& ch)
    {
        BOOST_LOG_EXPR_IF_MT(boost::log::aux::exclusive_lock_guard< threading_model > lock(this->get_threading_model());)
        m_ChannelAttr.set(ch);
    }

protected:
    /*!
     * Channel attribute accessor
     */
    channel_attribute const& get_channel_attribute() const { return m_ChannelAttr; }

    /*!
     * Unlocked \c open_record
     */
    template< typename ArgsT >
    record open_record_unlocked(ArgsT const& args)
    {
        return open_record_with_channel_unlocked(args, args[keywords::channel | parameter::void_()]);
    }

    /*!
     * Unlocked swap
     */
    void swap_unlocked(basic_channel_logger& that)
    {
        base_type::swap_unlocked(static_cast< base_type& >(that));
        m_ChannelAttr.swap(that.m_ChannelAttr);
    }

private:
    //! The \c open_record implementation for the case when the channel is specified in log statement
    template< typename ArgsT, typename T >
    record open_record_with_channel_unlocked(ArgsT const& args, T const& ch)
    {
        m_ChannelAttr.set(ch);
        return base_type::open_record_unlocked(args);
    }
    //! The \c open_record implementation for the case when the channel is not specified in log statement
    template< typename ArgsT >
    record open_record_with_channel_unlocked(ArgsT const& args, parameter::void_)
    {
        return base_type::open_record_unlocked(args);
    }
};

/*!
 * \brief Channel support feature
 *
 * The logger with this feature automatically registers an attribute with the specified
 * on construction value, which is a channel name. The channel name can be modified
 * through the logger life time, either by calling the \c channel method or by specifying
 * the name in the logging statement.
 *
 * The type of the channel name can be customized by providing it as a template parameter
 * to the feature template. By default, a string will be used.
 */
template< typename ChannelT = std::string >
struct channel
{
    template< typename BaseT >
    struct apply
    {
        typedef basic_channel_logger<
            BaseT,
            ChannelT
        > type;
    };
};

} // namespace sources

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

//! The macro allows to put a record with a specific channel name into log
#define BOOST_LOG_STREAM_CHANNEL(logger, chan)\
    BOOST_LOG_STREAM_WITH_PARAMS((logger), (::boost::log::keywords::channel = (chan)))

#ifndef BOOST_LOG_NO_SHORTHAND_NAMES

//! An equivalent to BOOST_LOG_STREAM_CHANNEL(logger, chan)
#define BOOST_LOG_CHANNEL(logger, chan) BOOST_LOG_STREAM_CHANNEL(logger, chan)

#endif // BOOST_LOG_NO_SHORTHAND_NAMES

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SOURCES_CHANNEL_FEATURE_HPP_INCLUDED_
