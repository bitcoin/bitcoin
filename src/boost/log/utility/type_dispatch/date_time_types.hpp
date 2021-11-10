/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   date_time_types.hpp
 * \author Andrey Semashev
 * \date   13.03.2008
 *
 * The header contains definition of date and time-related types supported by the library by default.
 */

#ifndef BOOST_LOG_DATE_TIME_TYPES_HPP_INCLUDED_
#define BOOST_LOG_DATE_TIME_TYPES_HPP_INCLUDED_

#include <ctime>
#include <boost/mpl/vector.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/local_time/local_time_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

//! Boost.Preprocessor sequence of the standard C date/time types
#define BOOST_LOG_NATIVE_DATE_TIME_TYPES()\
    (std::time_t)(std::tm)

//! Boost.Preprocessor sequence of the standard C date types
#define BOOST_LOG_NATIVE_DATE_TYPES()\
    BOOST_LOG_NATIVE_DATE_TIME_TYPES()

//! Boost.Preprocessor sequence of the Boost date/time types
#define BOOST_LOG_BOOST_DATE_TIME_TYPES()\
    (boost::posix_time::ptime)(boost::local_time::local_date_time)

//! Boost.Preprocessor sequence of date/time types
#define BOOST_LOG_DATE_TIME_TYPES()\
    BOOST_LOG_NATIVE_DATE_TIME_TYPES()BOOST_LOG_BOOST_DATE_TIME_TYPES()\

//! Boost.Preprocessor sequence of the Boost date types
#define BOOST_LOG_BOOST_DATE_TYPES()\
    BOOST_LOG_BOOST_DATE_TIME_TYPES()(boost::gregorian::date)

//! Boost.Preprocessor sequence of date types
#define BOOST_LOG_DATE_TYPES()\
    BOOST_LOG_NATIVE_DATE_TYPES()BOOST_LOG_BOOST_DATE_TYPES()


//! Boost.Preprocessor sequence of the standard time duration types
#define BOOST_LOG_NATIVE_TIME_DURATION_TYPES()\
    (double)  /* result of difftime() */

//! Boost.Preprocessor sequence of the Boost time duration types
#define BOOST_LOG_BOOST_TIME_DURATION_TYPES()\
    (boost::posix_time::time_duration)(boost::gregorian::date_duration)

//! Boost.Preprocessor sequence of time duration types
#define BOOST_LOG_TIME_DURATION_TYPES()\
    BOOST_LOG_NATIVE_TIME_DURATION_TYPES()BOOST_LOG_BOOST_TIME_DURATION_TYPES()


//! Boost.Preprocessor sequence of the Boost time period types
#define BOOST_LOG_BOOST_TIME_PERIOD_TYPES()\
    (boost::posix_time::time_period)(boost::local_time::local_time_period)(boost::gregorian::date_period)

//! Boost.Preprocessor sequence of time period types
#define BOOST_LOG_TIME_PERIOD_TYPES()\
    BOOST_LOG_BOOST_TIME_PERIOD_TYPES()


/*!
 * An MPL-sequence of natively supported date and time types of attributes
 */
typedef mpl::vector<
    BOOST_PP_SEQ_ENUM(BOOST_LOG_NATIVE_DATE_TIME_TYPES())
> native_date_time_types;

/*!
 * An MPL-sequence of Boost date and time types of attributes
 */
typedef mpl::vector<
    BOOST_PP_SEQ_ENUM(BOOST_LOG_BOOST_DATE_TIME_TYPES())
> boost_date_time_types;

/*!
 * An MPL-sequence with the complete list of the supported date and time types
 */
typedef mpl::vector<
    BOOST_PP_SEQ_ENUM(BOOST_LOG_DATE_TIME_TYPES())
> date_time_types;

/*!
 * An MPL-sequence of natively supported date types of attributes
 */
typedef native_date_time_types native_date_types;

/*!
 * An MPL-sequence of Boost date types of attributes
 */
typedef mpl::vector<
    BOOST_PP_SEQ_ENUM(BOOST_LOG_BOOST_DATE_TYPES())
> boost_date_types;

/*!
 * An MPL-sequence with the complete list of the supported date types
 */
typedef mpl::vector<
    BOOST_PP_SEQ_ENUM(BOOST_LOG_DATE_TYPES())
> date_types;

/*!
 * An MPL-sequence of natively supported time types
 */
typedef native_date_time_types native_time_types;

//! An MPL-sequence of Boost time types
typedef boost_date_time_types boost_time_types;

/*!
 * An MPL-sequence with the complete list of the supported time types
 */
typedef date_time_types time_types;

/*!
 * An MPL-sequence of natively supported time duration types of attributes
 */
typedef mpl::vector<
    BOOST_PP_SEQ_ENUM(BOOST_LOG_NATIVE_TIME_DURATION_TYPES())
> native_time_duration_types;

/*!
 * An MPL-sequence of Boost time duration types of attributes
 */
typedef mpl::vector<
    BOOST_PP_SEQ_ENUM(BOOST_LOG_BOOST_TIME_DURATION_TYPES())
> boost_time_duration_types;

/*!
 * An MPL-sequence with the complete list of the supported time duration types
 */
typedef mpl::vector<
    BOOST_PP_SEQ_ENUM(BOOST_LOG_TIME_DURATION_TYPES())
> time_duration_types;

/*!
 * An MPL-sequence of Boost time duration types of attributes
 */
typedef mpl::vector<
    BOOST_PP_SEQ_ENUM(BOOST_LOG_BOOST_TIME_PERIOD_TYPES())
> boost_time_period_types;

/*!
 * An MPL-sequence with the complete list of the supported time period types
 */
typedef boost_time_period_types time_period_types;

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DATE_TIME_TYPES_HPP_INCLUDED_
