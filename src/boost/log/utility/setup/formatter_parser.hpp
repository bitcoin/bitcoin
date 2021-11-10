/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatter_parser.hpp
 * \author Andrey Semashev
 * \date   07.04.2008
 *
 * The header contains definition of a formatter parser function, along with facilities to
 * add support for custom formatters.
 */

#ifndef BOOST_LOG_UTILITY_SETUP_FORMATTER_PARSER_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_SETUP_FORMATTER_PARSER_HPP_INCLUDED_

#include <iosfwd>
#include <map>
#include <string>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/log/detail/setup_config.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/expressions/formatter.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/expressions/formatters/stream.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * Formatter factory base interface.
 */
template< typename CharT >
struct formatter_factory
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! The formatter function object
    typedef basic_formatter< char_type > formatter_type;
    /*!
     * Type of the map of formatter factory arguments [argument name -> argument value].
     * This type of maps will be passed to formatter factories on attempt to create a formatter.
     */
    typedef std::map< string_type, string_type > args_map;

    /*!
     * Default constructor
     */
    BOOST_DEFAULTED_FUNCTION(formatter_factory(), {})

    /*!
     * Virtual destructor
     */
    virtual ~formatter_factory() {}

    /*!
     * The function creates a formatter for the specified attribute.
     *
     * \param name Attribute name
     * \param args Formatter arguments
     */
    virtual formatter_type create_formatter(attribute_name const& name, args_map const& args) = 0;

    BOOST_DELETED_FUNCTION(formatter_factory(formatter_factory const&))
    BOOST_DELETED_FUNCTION(formatter_factory& operator= (formatter_factory const&))
};

/*!
 * Base class for formatter factories. This class provides default implementation of formatter expressions for
 * types supporting stream output. The factory does not take into account any additional parameters that may be specified.
 */
template< typename CharT, typename AttributeValueT >
class basic_formatter_factory :
    public formatter_factory< CharT >
{
private:
    typedef formatter_factory< CharT > base_type;

public:
    //! Attribute value type
    typedef AttributeValueT value_type;
    //  Type imports from the base class
    typedef typename base_type::formatter_type formatter_type;
    typedef typename base_type::args_map args_map;

    /*!
     * The function creates a formatter for the specified attribute.
     *
     * \param name Attribute name
     * \param args Formatter arguments
     */
    formatter_type create_formatter(attribute_name const& name, args_map const& args)
    {
        return formatter_type(expressions::stream << expressions::attr< value_type >(name));
    }
};

/*!
 * \brief The function registers a user-defined formatter factory
 *
 * The function registers a user-defined formatter factory. The registered factory function will be
 * called when the formatter parser detects the specified attribute name in the formatter string.
 *
 * \pre <tt>!!attr_name && !!factory</tt>.
 *
 * \param attr_name Attribute name
 * \param factory Pointer to the formatter factory
 */
template< typename CharT >
BOOST_LOG_SETUP_API void register_formatter_factory(
    attribute_name const& attr_name, shared_ptr< formatter_factory< CharT > > const& factory);

/*!
 * \brief The function registers a user-defined formatter factory
 *
 * The function registers a user-defined formatter factory. The registered factory function will be
 * called when the formatter parser detects the specified attribute name in the formatter string.
 *
 * \pre <tt>!!attr_name && !!factory</tt>.
 *
 * \param attr_name Attribute name
 * \param factory Pointer to the formatter factory
 */
template< typename FactoryT >
inline typename boost::enable_if_c<
    is_base_and_derived< formatter_factory< typename FactoryT::char_type >, FactoryT >::value
>::type register_formatter_factory(attribute_name const& attr_name, shared_ptr< FactoryT > const& factory)
{
    typedef formatter_factory< typename FactoryT::char_type > factory_base;
    register_formatter_factory(attr_name, boost::static_pointer_cast< factory_base >(factory));
}

/*!
 * \brief The function registers a simple formatter factory
 *
 * The function registers a simple formatter factory. The registered factory will generate formatters
 * that will be equivalent to the <tt>log::expressions::attr</tt> formatter (i.e. that will use the
 * native \c operator<< to format the attribute value). The factory does not use any arguments from the format string,
 * if specified.
 *
 * \pre <tt>!!attr_name</tt>.
 *
 * \param attr_name Attribute name
 */
template< typename AttributeValueT, typename CharT >
inline void register_simple_formatter_factory(attribute_name const& attr_name)
{
    shared_ptr< formatter_factory< CharT > > factory =
        boost::make_shared< basic_formatter_factory< CharT, AttributeValueT > >();
    register_formatter_factory(attr_name, factory);
}

/*!
 * The function parses a formatter from the sequence of characters
 *
 * \pre <tt>begin <= end</tt>, both pointers must not be NULL
 * \param begin Pointer to the first character of the sequence
 * \param end Pointer to the after-the-last character of the sequence
 * \return The parsed formatter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a formatter cannot be recognized in the character sequence.
 */
template< typename CharT >
BOOST_LOG_SETUP_API basic_formatter< CharT > parse_formatter(const CharT* begin, const CharT* end);

/*!
 * The function parses a formatter from the string
 *
 * \param str A string that contains format description
 * \return The parsed formatter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a formatter cannot be recognized in the character sequence.
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline basic_formatter< CharT > parse_formatter(std::basic_string< CharT, TraitsT, AllocatorT > const& str)
{
    const CharT* p = str.c_str();
    return parse_formatter(p, p + str.size());
}

/*!
 * The function parses a formatter from the string
 *
 * \pre <tt>str != NULL</tt>, <tt>str</tt> points to a zero-terminated string
 * \param str A string that contains format description.
 * \return The parsed formatter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a formatter cannot be recognized in the character sequence.
 */
template< typename CharT >
inline basic_formatter< CharT > parse_formatter(const CharT* str)
{
    return parse_formatter(str, str + std::char_traits< CharT >::length(str));
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_SETUP_FORMATTER_PARSER_HPP_INCLUDED_
