/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   basic_sink_backend.hpp
 * \author Andrey Semashev
 * \date   04.11.2007
 *
 * The header contains implementation of base classes for sink backends.
 */

#ifndef BOOST_LOG_SINKS_BASIC_SINK_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_BASIC_SINK_BACKEND_HPP_INCLUDED_

#include <string>
#include <boost/type_traits/is_same.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

/*!
 * \brief Base class for a logging sink backend
 *
 * The \c basic_sink_backend class template defines a number of types that
 * all sink backends are required to define. All sink backends have to derive from the class.
 */
template< typename FrontendRequirementsT >
struct basic_sink_backend
{
    //! Frontend requirements tag
    typedef FrontendRequirementsT frontend_requirements;

    BOOST_DEFAULTED_FUNCTION(basic_sink_backend(), {})

    BOOST_DELETED_FUNCTION(basic_sink_backend(basic_sink_backend const&))
    BOOST_DELETED_FUNCTION(basic_sink_backend& operator= (basic_sink_backend const&))
};

/*!
 * \brief A base class for a logging sink backend with message formatting support
 *
 * The \c basic_formatted_sink_backend class template indicates to the frontend that
 * the backend requires logging record formatting.
 *
 * The class allows to request encoding conversion in case if the sink backend
 * requires the formatted string in some particular encoding (e.g. if underlying API
 * supports only narrow or wide characters). In order to perform conversion one
 * should specify the desired final character type in the \c TargetCharT template
 * parameter.
 */
template<
    typename CharT,
    typename FrontendRequirementsT = synchronized_feeding
>
struct basic_formatted_sink_backend :
    public basic_sink_backend<
        typename combine_requirements< FrontendRequirementsT, formatted_records >::type
    >
{
private:
    typedef basic_sink_backend<
        typename combine_requirements< FrontendRequirementsT, formatted_records >::type
    > base_type;

public:
    //! Character type
    typedef CharT char_type;
    //! Formatted string type
    typedef std::basic_string< char_type > string_type;
    //! Frontend requirements
    typedef typename base_type::frontend_requirements frontend_requirements;
};

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_BASIC_SINK_BACKEND_HPP_INCLUDED_
