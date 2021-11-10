/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   clock.hpp
 * \author Andrey Semashev
 * \date   01.12.2007
 *
 * The header contains wall clock attribute implementation and typedefs.
 */

#ifndef BOOST_LOG_ATTRIBUTES_CLOCK_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_CLOCK_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/attributes/time_traits.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace attributes {

/*!
 * \brief A class of an attribute that makes an attribute value of the current date and time
 *
 * The attribute generates current time stamp as a value. The type of the attribute value
 * is determined with time traits passed to the class template as a template parameter.
 * The time traits provided by the library use \c boost::posix_time::ptime as the time type.
 *
 * Time traits also determine the way time is acquired. There are two types of time traits
 * provided by the library: \c utc_time_traits and \c local_time_traits. The first returns UTC time,
 * the second returns local time.
 */
template< typename TimeTraitsT >
class basic_clock :
    public attribute
{
public:
    //! Generated value type
    typedef typename TimeTraitsT::time_type value_type;

protected:
    //! Attribute factory implementation
    struct BOOST_SYMBOL_VISIBLE impl :
        public attribute::impl
    {
        attribute_value get_value()
        {
            typedef attribute_value_impl< value_type > result_value;
            return attribute_value(new result_value(TimeTraitsT::get_clock()));
        }
    };

public:
    /*!
     * Default constructor
     */
    basic_clock() : attribute(new impl())
    {
    }
    /*!
     * Constructor for casting support
     */
    explicit basic_clock(cast_source const& source) : attribute(source.as< impl >())
    {
    }
};

//! Attribute that returns current UTC time
typedef basic_clock< utc_time_traits > utc_clock;
//! Attribute that returns current local time
typedef basic_clock< local_time_traits > local_clock;

} // namespace attributes

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_CLOCK_HPP_INCLUDED_
