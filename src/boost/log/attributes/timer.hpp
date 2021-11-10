/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   timer.hpp
 * \author Andrey Semashev
 * \date   02.12.2007
 *
 * The header contains implementation of a stop watch attribute.
 */

#ifndef BOOST_LOG_ATTRIBUTES_TIMER_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_TIMER_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/time_traits.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace attributes {

/*!
 * \brief A class of an attribute that makes an attribute value of the time interval since construction
 *
 * The timer attribute calculates the time passed since its construction and returns it on value acquisition.
 * The attribute value type is <tt>boost::posix_time::time_duration</tt>.
 *
 * On Windows platform there are two implementations of the attribute. The default one is more precise but
 * a bit slower. This version uses <tt>QueryPerformanceFrequence</tt>/<tt>QueryPerformanceCounter</tt> API
 * to calculate elapsed time.
 *
 * There are known problems with these functions when used with some CPUs, notably AMD Athlon with
 * Cool'n'Quiet technology enabled. See the following links for more information and possible resolutions:
 *
 * http://support.microsoft.com/?scid=kb;en-us;895980
 * http://support.microsoft.com/?id=896256
 *
 * In case if none of these solutions apply, you are free to define <tt>BOOST_LOG_NO_QUERY_PERFORMANCE_COUNTER</tt> macro to
 * fall back to another implementation based on Boost.DateTime.
 */
class BOOST_LOG_API timer :
    public attribute
{
public:
    //! Attribute value type
    typedef utc_time_traits::time_type::time_duration_type value_type;

private:
    //! Factory implementation
    class BOOST_SYMBOL_VISIBLE impl;

public:
    /*!
     * Constructor. Starts time counting.
     */
    timer();
    /*!
     * Constructor for casting support
     */
    explicit timer(cast_source const& source);
};

} // namespace attributes

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_TIMER_HPP_INCLUDED_
