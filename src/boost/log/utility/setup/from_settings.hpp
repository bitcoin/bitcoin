/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   from_settings.hpp
 * \author Andrey Semashev
 * \date   11.10.2009
 *
 * The header contains definition of facilities that allows to initialize the library from
 * settings.
 */

#ifndef BOOST_LOG_UTILITY_SETUP_FROM_SETTINGS_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_SETUP_FROM_SETTINGS_HPP_INCLUDED_

#include <string>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/log/detail/setup_config.hpp>
#include <boost/log/sinks/sink.hpp>
#include <boost/log/utility/setup/settings.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * The function initializes the logging library from a settings container
 *
 * \param setts Library settings container
 *
 * \b Throws: An <tt>std::exception</tt>-based exception if the provided settings are not valid.
 */
template< typename CharT >
BOOST_LOG_SETUP_API void init_from_settings(basic_settings_section< CharT > const& setts);


/*!
 * Sink factory base interface
 */
template< typename CharT >
struct sink_factory
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Settings section type
    typedef basic_settings_section< char_type > settings_section;

    /*!
     * Default constructor
     */
    BOOST_DEFAULTED_FUNCTION(sink_factory(), {})

    /*!
     * Virtual destructor
     */
    virtual ~sink_factory() {}

    /*!
     * The function creates a formatter for the specified attribute.
     *
     * \param settings Sink parameters
     */
    virtual shared_ptr< sinks::sink > create_sink(settings_section const& settings) = 0;

    BOOST_DELETED_FUNCTION(sink_factory(sink_factory const&))
    BOOST_DELETED_FUNCTION(sink_factory& operator= (sink_factory const&))
};

/*!
 * \brief The function registers a factory for a custom sink
 *
 * The function registers a factory for a sink. The factory will be called to create sink
 * instance when the parser discovers the specified sink type in the settings file. The
 * factory must accept a map of parameters [parameter name -> parameter value] that it
 * may use to initialize the sink. The factory must return a non-NULL pointer to the
 * constructed sink instance.
 *
 * \param sink_name The custom sink name. Must point to a zero-terminated sequence of characters,
 *                  must not be NULL.
 * \param factory Pointer to the custom sink factory. Must not be NULL.
 */
template< typename CharT >
BOOST_LOG_SETUP_API void register_sink_factory(const char* sink_name, shared_ptr< sink_factory< CharT > > const& factory);

/*!
 * \brief The function registers a factory for a custom sink
 *
 * The function registers a factory for a sink. The factory will be called to create sink
 * instance when the parser discovers the specified sink type in the settings file. The
 * factory must accept a map of parameters [parameter name -> parameter value] that it
 * may use to initialize the sink. The factory must return a non-NULL pointer to the
 * constructed sink instance.
 *
 * \param sink_name The custom sink name
 * \param factory Pointer to the custom sink factory. Must not be NULL.
 */
template< typename CharT >
inline void register_sink_factory(std::string const& sink_name, shared_ptr< sink_factory< CharT > > const& factory)
{
    register_sink_factory(sink_name.c_str(), factory);
}

/*!
 * \brief The function registers a factory for a custom sink
 *
 * The function registers a factory for a sink. The factory will be called to create sink
 * instance when the parser discovers the specified sink type in the settings file. The
 * factory must accept a map of parameters [parameter name -> parameter value] that it
 * may use to initialize the sink. The factory must return a non-NULL pointer to the
 * constructed sink instance.
 *
 * \param sink_name The custom sink name. Must point to a zero-terminated sequence of characters,
 *                  must not be NULL.
 * \param factory Pointer to the custom sink factory. Must not be NULL.
 */
template< typename FactoryT >
inline typename boost::enable_if_c<
    is_base_and_derived< sink_factory< typename FactoryT::char_type >, FactoryT >::value
>::type register_sink_factory(const char* sink_name, shared_ptr< FactoryT > const& factory)
{
    typedef sink_factory< typename FactoryT::char_type > factory_base;
    register_sink_factory(sink_name, boost::static_pointer_cast< factory_base >(factory));
}

/*!
 * \brief The function registers a factory for a custom sink
 *
 * The function registers a factory for a sink. The factory will be called to create sink
 * instance when the parser discovers the specified sink type in the settings file. The
 * factory must accept a map of parameters [parameter name -> parameter value] that it
 * may use to initialize the sink. The factory must return a non-NULL pointer to the
 * constructed sink instance.
 *
 * \param sink_name The custom sink name
 * \param factory Pointer to the custom sink factory. Must not be NULL.
 */
template< typename FactoryT >
inline typename boost::enable_if_c<
    is_base_and_derived< sink_factory< typename FactoryT::char_type >, FactoryT >::value
>::type register_sink_factory(std::string const& sink_name, shared_ptr< FactoryT > const& factory)
{
    typedef sink_factory< typename FactoryT::char_type > factory_base;
    register_sink_factory(sink_name.c_str(), boost::static_pointer_cast< factory_base >(factory));
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_SETUP_FROM_SETTINGS_HPP_INCLUDED_
