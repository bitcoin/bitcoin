#ifndef DATE_TIME_FILETIME_FUNCTIONS_HPP__
#define DATE_TIME_FILETIME_FUNCTIONS_HPP__

/* Copyright (c) 2004 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

/*! @file filetime_functions.hpp
 * Function(s) for converting between a FILETIME structure and a
 * time object. This file is only available on systems that have
 * BOOST_HAS_FTIME defined.
 */

#include <boost/date_time/compiler_config.hpp>

#if defined(BOOST_HAS_FTIME) // skip this file if no FILETIME

#include <boost/cstdint.hpp>
#include <boost/date_time/time.hpp>
#include <boost/date_time/date_defs.hpp>

namespace boost {

namespace date_time {

//! Create a time object from an initialized FILETIME struct.
/*!
 * Create a time object from an initialized FILETIME struct.
 * A FILETIME struct holds 100-nanosecond units (0.0000001). When
 * built with microsecond resolution the file_time's sub second value
 * will be truncated. Nanosecond resolution has no truncation.
 *
 * \note The function is templated on the FILETIME type, so that
 *       it can be used with both native FILETIME and the ad-hoc
 *       boost::detail::winapi::FILETIME_ type.
 */
template< typename TimeT, typename FileTimeT >
inline
TimeT time_from_ftime(const FileTimeT& ft)
{
    typedef typename TimeT::date_type date_type;
    typedef typename TimeT::date_duration_type date_duration_type;
    typedef typename TimeT::time_duration_type time_duration_type;

    // https://svn.boost.org/trac/boost/ticket/2523
    // Since this function can be called with arbitrary times, including ones that
    // are before 1970-Jan-01, we'll have to cast the time a bit differently,
    // than it is done in the microsec_clock::file_time_to_microseconds function. This allows to
    // avoid integer wrapping for dates before 1970-Jan-01.

    // 100-nanos since 1601-Jan-01
    uint64_t ft_as_integer = (static_cast< uint64_t >(ft.dwHighDateTime) << 32) | static_cast< uint64_t >(ft.dwLowDateTime);
    uint64_t sec = ft_as_integer / 10000000UL;
    uint32_t sub_sec = static_cast< uint32_t >(ft_as_integer % 10000000UL) // 100-nanoseconds since the last second
#if !defined(BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)
        / 10U; // microseconds since the last second
#else
        * 100U; // nanoseconds since the last second
#endif

    // split sec into usable chunks: days, hours, minutes, & seconds
    const uint32_t sec_per_day = 86400; // seconds per day
    uint32_t days = static_cast< uint32_t >(sec / sec_per_day);
    uint32_t tmp = static_cast< uint32_t >(sec % sec_per_day);
    uint32_t hours = tmp / 3600; // sec_per_hour
    tmp %= 3600;
    uint32_t minutes = tmp / 60; // sec_per_min
    tmp %= 60;
    uint32_t seconds = tmp; // seconds

    date_duration_type dd(days);
    date_type d = date_type(1601, Jan, 01) + dd;
    return TimeT(d, time_duration_type(hours, minutes, seconds, sub_sec));
}

}} // boost::date_time

#endif // BOOST_HAS_FTIME

#endif // DATE_TIME_FILETIME_FUNCTIONS_HPP__
