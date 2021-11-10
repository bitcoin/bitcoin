/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   time_traits.hpp
 * \author Andrey Semashev
 * \date   01.12.2007
 *
 * The header contains implementation of time traits that are used in various parts of the
 * library to acquire current time.
 */

#ifndef BOOST_LOG_ATTRIBUTES_TIME_TRAITS_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_TIME_TRAITS_HPP_INCLUDED_

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace attributes {

//! Base class for time traits involving Boost.DateTime.
struct basic_time_traits
{
    //! Time type
    typedef posix_time::ptime time_type;

    //! Current time source
#if defined(BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK)
    typedef posix_time::microsec_clock clock_source;
#else
    typedef posix_time::second_clock clock_source;
#endif // defined(BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK)
};

//! Time traits that describes UTC time acquirement via Boost.DateTime facilities
struct utc_time_traits :
    public basic_time_traits
{
    /*!
     * \return Current time stamp
     */
    static time_type get_clock()
    {
        return clock_source::universal_time();
    }
};

//! Time traits that describes local time acquirement via Boost.DateTime facilities
struct local_time_traits :
    public basic_time_traits
{
    /*!
     * \return Current time stamp
     */
    static time_type get_clock()
    {
        return clock_source::local_time();
    }
};

} // namespace attributes

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_TIME_TRAITS_HPP_INCLUDED_
