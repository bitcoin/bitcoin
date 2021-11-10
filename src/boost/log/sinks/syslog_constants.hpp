/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   syslog_constants.hpp
 * \author Andrey Semashev
 * \date   08.01.2008
 *
 * The header contains definition of constants related to Syslog API. The constants can be
 * used in other places without the Syslog backend.
 */

#ifndef BOOST_LOG_SINKS_SYSLOG_CONSTANTS_HPP_INCLUDED_HPP_
#define BOOST_LOG_SINKS_SYSLOG_CONSTANTS_HPP_INCLUDED_HPP_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifndef BOOST_LOG_WITHOUT_SYSLOG

#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

namespace syslog {

    //! Syslog record levels
    enum level
    {
        emergency = 0,                //!< Equivalent to LOG_EMERG in syslog API
        alert = 1,                    //!< Equivalent to LOG_ALERT in syslog API
        critical = 2,                 //!< Equivalent to LOG_CRIT in syslog API
        error = 3,                    //!< Equivalent to LOG_ERROR in syslog API
        warning = 4,                  //!< Equivalent to LOG_WARNING in syslog API
        notice = 5,                   //!< Equivalent to LOG_NOTICE in syslog API
        info = 6,                     //!< Equivalent to LOG_INFO in syslog API
        debug = 7                     //!< Equivalent to LOG_DEBUG in syslog API
    };

    /*!
     * The function constructs log record level from an integer
     */
    BOOST_LOG_API level make_level(int lev);

    //! Syslog facility codes
    enum facility
    {
        kernel = 0 * 8,               //!< Kernel messages
        user = 1 * 8,                 //!< User-level messages. Equivalent to LOG_USER in syslog API.
        mail = 2 * 8,                 //!< Mail system messages. Equivalent to LOG_MAIL in syslog API.
        daemon = 3 * 8,               //!< System daemons. Equivalent to LOG_DAEMON in syslog API.
        security0 = 4 * 8,            //!< Security/authorization messages
        syslogd = 5 * 8,              //!< Messages from the syslogd daemon. Equivalent to LOG_SYSLOG in syslog API.
        printer = 6 * 8,              //!< Line printer subsystem. Equivalent to LOG_LPR in syslog API.
        news = 7 * 8,                 //!< Network news subsystem. Equivalent to LOG_NEWS in syslog API.
        uucp = 8 * 8,                 //!< Messages from UUCP subsystem. Equivalent to LOG_UUCP in syslog API.
        clock0 = 9 * 8,               //!< Messages from the clock daemon
        security1 = 10 * 8,           //!< Security/authorization messages
        ftp = 11 * 8,                 //!< Messages from FTP daemon
        ntp = 12 * 8,                 //!< Messages from NTP daemon
        log_audit = 13 * 8,           //!< Security/authorization messages
        log_alert = 14 * 8,           //!< Security/authorization messages
        clock1 = 15 * 8,              //!< Messages from the clock daemon
        local0 = 16 * 8,              //!< For local use. Equivalent to LOG_LOCAL0 in syslog API
        local1 = 17 * 8,              //!< For local use. Equivalent to LOG_LOCAL1 in syslog API
        local2 = 18 * 8,              //!< For local use. Equivalent to LOG_LOCAL2 in syslog API
        local3 = 19 * 8,              //!< For local use. Equivalent to LOG_LOCAL3 in syslog API
        local4 = 20 * 8,              //!< For local use. Equivalent to LOG_LOCAL4 in syslog API
        local5 = 21 * 8,              //!< For local use. Equivalent to LOG_LOCAL5 in syslog API
        local6 = 22 * 8,              //!< For local use. Equivalent to LOG_LOCAL6 in syslog API
        local7 = 23 * 8               //!< For local use. Equivalent to LOG_LOCAL7 in syslog API
    };

    /*!
     * The function constructs log source facility from an integer
     */
    BOOST_LOG_API facility make_facility(int fac);

} // namespace syslog

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_WITHOUT_SYSLOG

#endif // BOOST_LOG_SINKS_SYSLOG_CONSTANTS_HPP_INCLUDED_HPP_
