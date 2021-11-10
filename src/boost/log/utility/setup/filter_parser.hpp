/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   filter_parser.hpp
 * \author Andrey Semashev
 * \date   31.03.2008
 *
 * The header contains definition of a filter parser function.
 */

#ifndef BOOST_LOG_UTILITY_SETUP_FILTER_PARSER_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_SETUP_FILTER_PARSER_HPP_INCLUDED_

#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/phoenix/operator/comparison.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/log/detail/setup_config.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/expressions/filter.hpp>
#include <boost/log/expressions/keyword_fwd.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/expressions/predicates/has_attr.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * The interface class for all filter factories.
 */
template< typename CharT >
struct filter_factory
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;

    /*!
     * Default constructor
     */
    BOOST_DEFAULTED_FUNCTION(filter_factory(), {})

    /*!
     * Virtual destructor
     */
    virtual ~filter_factory() {}

    /*!
     * The callback for filter for the attribute existence test
     */
    virtual filter on_exists_test(attribute_name const& name)
    {
        return filter(expressions::has_attr(name));
    }

    /*!
     * The callback for equality relation filter
     */
    virtual filter on_equality_relation(attribute_name const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "The equality attribute value relation is not supported", (name));
        BOOST_LOG_UNREACHABLE_RETURN(filter());
    }
    /*!
     * The callback for inequality relation filter
     */
    virtual filter on_inequality_relation(attribute_name const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "The inequality attribute value relation is not supported", (name));
        BOOST_LOG_UNREACHABLE_RETURN(filter());
    }
    /*!
     * The callback for less relation filter
     */
    virtual filter on_less_relation(attribute_name const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "The less attribute value relation is not supported", (name));
        BOOST_LOG_UNREACHABLE_RETURN(filter());
    }
    /*!
     * The callback for greater relation filter
     */
    virtual filter on_greater_relation(attribute_name const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "The greater attribute value relation is not supported", (name));
        BOOST_LOG_UNREACHABLE_RETURN(filter());
    }
    /*!
     * The callback for less or equal relation filter
     */
    virtual filter on_less_or_equal_relation(attribute_name const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "The less-or-equal attribute value relation is not supported", (name));
        BOOST_LOG_UNREACHABLE_RETURN(filter());
    }
    /*!
     * The callback for greater or equal relation filter
     */
    virtual filter on_greater_or_equal_relation(attribute_name const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "The greater-or-equal attribute value relation is not supported", (name));
        BOOST_LOG_UNREACHABLE_RETURN(filter());
    }

    /*!
     * The callback for custom relation filter
     */
    virtual filter on_custom_relation(attribute_name const& name, string_type const& rel, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "The custom attribute value relation \"" + boost::log::aux::to_narrow(rel) + "\" is not supported", (name));
        BOOST_LOG_UNREACHABLE_RETURN(filter());
    }

    BOOST_DELETED_FUNCTION(filter_factory(filter_factory const&))
    BOOST_DELETED_FUNCTION(filter_factory& operator= (filter_factory const&))
};

/*!
 * The base class for filter factories. The class defines default implementations for most
 * filter expressions. In order to be able to construct filters, the attribute value type must
 * support reading from a stream. Also, the default filters will rely on relational operators for
 * the type, so these operators must also be defined.
 */
template< typename CharT, typename AttributeValueT >
class basic_filter_factory :
    public filter_factory< CharT >
{
    //! Base type
    typedef filter_factory< CharT > base_type;

public:
    //! The type(s) of the attribute value expected
    typedef AttributeValueT value_type;
    //  Type imports
    typedef typename base_type::string_type string_type;

    /*!
     * The callback for filter for the attribute existence test
     */
    virtual filter on_exists_test(attribute_name const& name)
    {
        return filter(expressions::has_attr< value_type >(name));
    }

    /*!
     * The callback for equality relation filter
     */
    virtual filter on_equality_relation(attribute_name const& name, string_type const& arg)
    {
        return filter(expressions::attr< value_type >(name) == parse_argument(arg));
    }
    /*!
     * The callback for inequality relation filter
     */
    virtual filter on_inequality_relation(attribute_name const& name, string_type const& arg)
    {
        return filter(expressions::attr< value_type >(name) != parse_argument(arg));
    }
    /*!
     * The callback for less relation filter
     */
    virtual filter on_less_relation(attribute_name const& name, string_type const& arg)
    {
        return filter(expressions::attr< value_type >(name) < parse_argument(arg));
    }
    /*!
     * The callback for greater relation filter
     */
    virtual filter on_greater_relation(attribute_name const& name, string_type const& arg)
    {
        return filter(expressions::attr< value_type >(name) > parse_argument(arg));
    }
    /*!
     * The callback for less or equal relation filter
     */
    virtual filter on_less_or_equal_relation(attribute_name const& name, string_type const& arg)
    {
        return filter(expressions::attr< value_type >(name) <= parse_argument(arg));
    }
    /*!
     * The callback for greater or equal relation filter
     */
    virtual filter on_greater_or_equal_relation(attribute_name const& name, string_type const& arg)
    {
        return filter(expressions::attr< value_type >(name) >= parse_argument(arg));
    }

    /*!
     * The callback for custom relation filter
     */
    virtual filter on_custom_relation(attribute_name const& name, string_type const& rel, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR_PARAMS(parse_error, "The custom attribute value relation \"" + boost::log::aux::to_narrow(rel) + "\" is not supported", (name));
        BOOST_LOG_UNREACHABLE_RETURN(filter());
    }

    /*!
     * The function parses the argument value for a binary relation
     */
    virtual value_type parse_argument(string_type const& arg)
    {
        return boost::lexical_cast< value_type >(arg);
    }
};

/*!
 * The function registers a filter factory object for the specified attribute name. The factory will be
 * used to construct a filter during parsing the filter string.
 *
 * \pre <tt>name != NULL && factory != NULL</tt>, <tt>name</tt> points to a zero-terminated string
 * \param name Attribute name to associate the factory with
 * \param factory The filter factory
 */
template< typename CharT >
BOOST_LOG_SETUP_API void register_filter_factory(
    attribute_name const& name, shared_ptr< filter_factory< CharT > > const& factory);

/*!
 * The function registers a filter factory object for the specified attribute name. The factory will be
 * used to construct a filter during parsing the filter string.
 *
 * \pre <tt>name != NULL && factory != NULL</tt>, <tt>name</tt> points to a zero-terminated string
 * \param name Attribute name to associate the factory with
 * \param factory The filter factory
 */
template< typename FactoryT >
inline typename boost::enable_if_c<
    is_base_and_derived< filter_factory< typename FactoryT::char_type >, FactoryT >::value
>::type register_filter_factory(attribute_name const& name, shared_ptr< FactoryT > const& factory)
{
    typedef filter_factory< typename FactoryT::char_type > factory_base;
    register_filter_factory(name, boost::static_pointer_cast< factory_base >(factory));
}

/*!
 * The function registers a simple filter factory object for the specified attribute name. The factory will
 * support attribute values of type \c AttributeValueT, which must support all relation operations, such as
 * equality comparison and less/greater ordering, and also extraction from stream.
 *
 * \pre <tt>name != NULL</tt>, <tt>name</tt> points to a zero-terminated string
 * \param name Attribute name to associate the factory with
 */
template< typename AttributeValueT, typename CharT >
inline void register_simple_filter_factory(attribute_name const& name)
{
    shared_ptr< filter_factory< CharT > > factory =
        boost::make_shared< basic_filter_factory< CharT, AttributeValueT > >();
    register_filter_factory(name, factory);
}

/*!
 * The function registers a simple filter factory object for the specified attribute name. The factory will
 * support attribute values of type \c AttributeValueT, which must support all relation operations, such as
 * equality comparison and less/greater ordering, and also extraction from stream.
 *
 * \pre <tt>name != NULL</tt>, <tt>name</tt> points to a zero-terminated string
 * \param name Attribute name to associate the factory with
 */
template< typename AttributeValueT >
inline void register_simple_filter_factory(attribute_name const& name)
{
    register_simple_filter_factory< AttributeValueT, char >(name);
}

/*!
 * The function registers a simple filter factory object for the specified attribute keyword. The factory will
 * support attribute values described by the keyword. The values must support all relation operations, such as
 * equality comparison and less/greater ordering, and also extraction from stream.
 *
 * \pre <tt>name != NULL</tt>, <tt>name</tt> points to a zero-terminated string
 * \param keyword Attribute keyword to associate the factory with
 */
template< typename CharT, typename DescriptorT, template< typename > class ActorT >
inline void register_simple_filter_factory(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword)
{
    register_simple_filter_factory< typename DescriptorT::value_type, CharT >(keyword.get_name());
}

/*!
 * The function parses a filter from the sequence of characters
 *
 * \pre <tt>begin <= end</tt>, both pointers must not be \c NULL
 * \param begin Pointer to the first character of the sequence
 * \param end Pointer to the after-the-last character of the sequence
 * \return A function object that can be used as a filter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a filter cannot be recognized in the character sequence.
 */
template< typename CharT >
BOOST_LOG_SETUP_API filter parse_filter(const CharT* begin, const CharT* end);

/*!
 * The function parses a filter from the string
 *
 * \param str A string that contains filter description
 * \return A function object that can be used as a filter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a filter cannot be recognized in the character sequence.
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline filter parse_filter(std::basic_string< CharT, TraitsT, AllocatorT > const& str)
{
    const CharT* p = str.c_str();
    return parse_filter(p, p + str.size());
}

/*!
 * The function parses a filter from the string
 *
 * \pre <tt>str != NULL</tt>, <tt>str</tt> points to a zero-terminated string.
 * \param str A string that contains filter description.
 * \return A function object that can be used as a filter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a filter cannot be recognized in the character sequence.
 */
template< typename CharT >
inline filter parse_filter(const CharT* str)
{
    return parse_filter(str, str + std::char_traits< CharT >::length(str));
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_SETUP_FILTER_PARSER_HPP_INCLUDED_
