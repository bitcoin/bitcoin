/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters/named_scope.hpp
 * \author Andrey Semashev
 * \date   11.11.2012
 *
 * The header contains a formatter function for named scope attribute values.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTERS_NAMED_SCOPE_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTERS_NAMED_SCOPE_HPP_INCLUDED_

#include <string>
#include <iterator>
#include <utility>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/parameter/binding.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/terminal_fwd.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/fallback_policy.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/detail/custom_terminal_spec.hpp>
#include <boost/log/detail/deduce_char_type.hpp>
#include <boost/log/detail/attr_output_terminal.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/expressions/keyword_fwd.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/string_literal_fwd.hpp>
#include <boost/log/utility/functional/bind.hpp>
#include <boost/log/keywords/format.hpp>
#include <boost/log/keywords/delimiter.hpp>
#include <boost/log/keywords/depth.hpp>
#include <boost/log/keywords/iteration.hpp>
#include <boost/log/keywords/empty_marker.hpp>
#include <boost/log/keywords/incomplete_marker.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

//! Scope iteration directions
enum scope_iteration_direction
{
    forward,    //!< Iterate through scopes from outermost to innermost
    reverse     //!< Iterate through scopes from innermost to outermost
};

namespace aux {

#ifdef BOOST_LOG_USE_CHAR
//! Parses the named scope format string and constructs the formatter function
BOOST_LOG_API boost::log::aux::light_function< void (basic_formatting_ostream< char >&, attributes::named_scope::value_type::value_type const&) >
parse_named_scope_format(const char* begin, const char* end);
#endif

#ifdef BOOST_LOG_USE_WCHAR_T
//! Parses the named scope format string and constructs the formatter function
BOOST_LOG_API boost::log::aux::light_function< void (basic_formatting_ostream< wchar_t >&, attributes::named_scope::value_type::value_type const&) >
parse_named_scope_format(const wchar_t* begin, const wchar_t* end);
#endif

//! Parses the named scope format string and constructs the formatter function
template< typename CharT >
inline boost::log::aux::light_function< void (basic_formatting_ostream< CharT >&, attributes::named_scope::value_type::value_type const&) >
parse_named_scope_format(const CharT* format)
{
    return parse_named_scope_format(format, format + std::char_traits< CharT >::length(format));
}

//! Parses the named scope format string and constructs the formatter function
template< typename CharT, typename TraitsT, typename AllocatorT >
inline boost::log::aux::light_function< void (basic_formatting_ostream< CharT >&, attributes::named_scope::value_type::value_type const&) >
parse_named_scope_format(std::basic_string< CharT, TraitsT, AllocatorT > const& format)
{
    const CharT* p = format.c_str();
    return parse_named_scope_format(p, p + format.size());
}

//! Parses the named scope format string and constructs the formatter function
template< typename CharT, typename TraitsT >
inline boost::log::aux::light_function< void (basic_formatting_ostream< CharT >&, attributes::named_scope::value_type::value_type const&) >
parse_named_scope_format(basic_string_literal< CharT, TraitsT > const& format)
{
    const CharT* p = format.c_str();
    return parse_named_scope_format(p, p + format.size());
}

template< typename CharT >
class format_named_scope_impl
{
public:
    //! Function result type
    typedef void result_type;

    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Formatting stream type
    typedef basic_formatting_ostream< char_type > stream_type;
    //! Attribute value type
    typedef attributes::named_scope::value_type value_type;
    //! Named scope formatter
    typedef boost::log::aux::light_function< void (stream_type&, value_type::value_type const&) > element_formatter_type;

private:
    //! Element formatting function
    element_formatter_type m_element_formatter;
    //! Element delimiter
    string_type m_delimiter;
    //! Incomplete list marker
    string_type m_incomplete_marker;
    //! Empty list marker
    string_type m_empty_marker;
    //! Maximum number of elements to output
    value_type::size_type m_depth;
    //! Iteration direction
    scope_iteration_direction m_direction;

public:
    //! Initializing constructor
    format_named_scope_impl
    (
        element_formatter_type const& element_formatter,
        string_type const& delimiter,
        string_type const& incomplete_marker,
        string_type const& empty_marker,
        value_type::size_type depth,
        scope_iteration_direction direction
    ) :
        m_element_formatter(element_formatter),
        m_delimiter(delimiter),
        m_incomplete_marker(incomplete_marker),
        m_empty_marker(empty_marker),
        m_depth(depth),
        m_direction(direction)
    {
    }
    //! Copy constructor
    format_named_scope_impl(format_named_scope_impl const& that) :
        m_element_formatter(that.m_element_formatter),
        m_delimiter(that.m_delimiter),
        m_incomplete_marker(that.m_incomplete_marker),
        m_empty_marker(that.m_empty_marker),
        m_depth(that.m_depth),
        m_direction(that.m_direction)
    {
    }

    //! Formatting operator
    result_type operator() (stream_type& strm, value_type const& scopes) const
    {
        if (!scopes.empty())
        {
            if (m_direction == expressions::forward)
                format_forward(strm, scopes);
            else
                format_reverse(strm, scopes);
        }
        else
        {
            strm << m_empty_marker;
        }
    }

private:
    //! The function performs formatting of the extracted scope stack in forward direction
    void format_forward(stream_type& strm, value_type const& scopes) const
    {
        value_type::const_iterator it, end = scopes.end();
        if (m_depth > 0)
        {
            value_type::size_type const scopes_to_iterate = (std::min)(m_depth, scopes.size());
            it = scopes.end();
            std::advance(it, -static_cast< value_type::difference_type >(scopes_to_iterate));
        }
        else
        {
            it = scopes.begin();
        }

        if (it != end)
        {
            if (it != scopes.begin())
                strm << m_incomplete_marker;

            m_element_formatter(strm, *it);
            for (++it; it != end; ++it)
            {
                strm << m_delimiter;
                m_element_formatter(strm, *it);
            }
        }
    }
    //! The function performs formatting of the extracted scope stack in reverse direction
    void format_reverse(stream_type& strm, value_type const& scopes) const
    {
        value_type::const_reverse_iterator it = scopes.rbegin(), end;
        if (m_depth > 0)
        {
            value_type::size_type const scopes_to_iterate = (std::min)(m_depth, scopes.size());
            end = it;
            std::advance(end, static_cast< value_type::difference_type >(scopes_to_iterate));
        }
        else
        {
            end = scopes.rend();
        }

        if (it != end)
        {
            m_element_formatter(strm, *it);
            for (++it; it != end; ++it)
            {
                strm << m_delimiter;
                m_element_formatter(strm, *it);
            }

            if (it != scopes.rend())
                strm << m_incomplete_marker;
        }
    }
};

} // namespace aux

/*!
 * Named scope formatter terminal.
 */
template< typename FallbackPolicyT, typename CharT >
class format_named_scope_terminal
{
public:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Internal typedef for type categorization
    typedef void _is_boost_log_terminal;
#endif

    //! Attribute value type
    typedef attributes::named_scope::value_type value_type;
    //! Fallback policy
    typedef FallbackPolicyT fallback_policy;
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Formatting stream type
    typedef basic_formatting_ostream< char_type > stream_type;
    //! Formatter function
    typedef aux::format_named_scope_impl< char_type > formatter_function_type;

    //! Function result type
    typedef string_type result_type;

private:
    //! Attribute value visitor invoker
    typedef value_visitor_invoker< value_type, fallback_policy > visitor_invoker_type;

private:
    //! Attribute name
    attribute_name m_name;
    //! Formatter function
    formatter_function_type m_formatter;
    //! Attribute value visitor invoker
    visitor_invoker_type m_visitor_invoker;

public:
    //! Initializing constructor
    template< typename FormatT >
    format_named_scope_terminal
    (
        attribute_name const& name,
        fallback_policy const& fallback,
        FormatT const& element_format,
        string_type const& delimiter,
        string_type const& incomplete_marker,
        string_type const& empty_marker,
        value_type::size_type depth,
        scope_iteration_direction direction
    ) :
        m_name(name), m_formatter(aux::parse_named_scope_format(element_format), delimiter, incomplete_marker, empty_marker, depth, direction), m_visitor_invoker(fallback)
    {
    }
    //! Copy constructor
    format_named_scope_terminal(format_named_scope_terminal const& that) :
        m_name(that.m_name), m_formatter(that.m_formatter), m_visitor_invoker(that.m_visitor_invoker)
    {
    }

    //! Returns attribute name
    attribute_name get_name() const
    {
        return m_name;
    }

    //! Returns fallback policy
    fallback_policy const& get_fallback_policy() const
    {
        return m_visitor_invoker.get_fallback_policy();
    }

    //! Retruns formatter function
    formatter_function_type const& get_formatter_function() const
    {
        return m_formatter;
    }

    //! Invokation operator
    template< typename ContextT >
    result_type operator() (ContextT const& ctx)
    {
        string_type str;
        stream_type strm(str);
        m_visitor_invoker(m_name, fusion::at_c< 0 >(phoenix::env(ctx).args()), binder1st< formatter_function_type&, stream_type& >(m_formatter, strm));
        strm.flush();
        return BOOST_LOG_NRVO_RESULT(str);
    }

    //! Invokation operator
    template< typename ContextT >
    result_type operator() (ContextT const& ctx) const
    {
        string_type str;
        stream_type strm(str);
        m_visitor_invoker(m_name, fusion::at_c< 0 >(phoenix::env(ctx).args()), binder1st< formatter_function_type const&, stream_type& >(m_formatter, strm));
        strm.flush();
        return BOOST_LOG_NRVO_RESULT(str);
    }

    BOOST_DELETED_FUNCTION(format_named_scope_terminal())
};

/*!
 * Named scope formatter actor.
 */
template< typename FallbackPolicyT, typename CharT, template< typename > class ActorT = phoenix::actor >
class format_named_scope_actor :
    public ActorT< format_named_scope_terminal< FallbackPolicyT, CharT > >
{
public:
    //! Character type
    typedef CharT char_type;
    //! Fallback policy
    typedef FallbackPolicyT fallback_policy;
    //! Base terminal type
    typedef format_named_scope_terminal< fallback_policy, char_type > terminal_type;
    //! Attribute value type
    typedef typename terminal_type::value_type value_type;
    //! Formatter function
    typedef typename terminal_type::formatter_function_type formatter_function_type;

    //! Base actor type
    typedef ActorT< terminal_type > base_type;

public:
    //! Initializing constructor
    explicit format_named_scope_actor(base_type const& act) : base_type(act)
    {
    }

    /*!
     * \returns The attribute name
     */
    attribute_name get_name() const
    {
        return this->proto_expr_.child0.get_name();
    }

    /*!
     * \returns Fallback policy
     */
    fallback_policy const& get_fallback_policy() const
    {
        return this->proto_expr_.child0.get_fallback_policy();
    }

    /*!
     * \returns Formatter function
     */
    formatter_function_type const& get_formatter_function() const
    {
        return this->proto_expr_.child0.get_formatter_function();
    }
};

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_AUX_OVERLOAD(left_ref, right_ref)\
    template< typename LeftExprT, typename FallbackPolicyT, typename CharT >\
    BOOST_FORCEINLINE phoenix::actor< aux::attribute_output_terminal< phoenix::actor< LeftExprT >, attributes::named_scope::value_type, FallbackPolicyT, typename format_named_scope_actor< FallbackPolicyT, CharT >::formatter_function_type > >\
    operator<< (phoenix::actor< LeftExprT > left_ref left, format_named_scope_actor< FallbackPolicyT, CharT > right_ref right)\
    {\
        typedef aux::attribute_output_terminal< phoenix::actor< LeftExprT >, attributes::named_scope::value_type, FallbackPolicyT, typename format_named_scope_actor< FallbackPolicyT, CharT >::formatter_function_type > terminal_type;\
        phoenix::actor< terminal_type > actor = {{ terminal_type(left, right.get_name(), right.get_formatter_function(), right.get_fallback_policy()) }};\
        return actor;\
    }

#include <boost/log/detail/generate_overloads.hpp>

#undef BOOST_LOG_AUX_OVERLOAD

#endif // BOOST_LOG_DOXYGEN_PASS

namespace aux {

//! Auxiliary traits to acquire default formatter parameters depending on the character type
template< typename CharT >
struct default_named_scope_params;

#ifdef BOOST_LOG_USE_CHAR
template< >
struct default_named_scope_params< char >
{
    static const char* forward_delimiter() { return "->"; }
    static const char* reverse_delimiter() { return "<-"; }
    static const char* incomplete_marker() { return "..."; }
    static const char* empty_marker() { return ""; }
};
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
template< >
struct default_named_scope_params< wchar_t >
{
    static const wchar_t* forward_delimiter() { return L"->"; }
    static const wchar_t* reverse_delimiter() { return L"<-"; }
    static const wchar_t* incomplete_marker() { return L"..."; }
    static const wchar_t* empty_marker() { return L""; }
};
#endif

template< typename CharT, template< typename > class ActorT, typename FallbackPolicyT, typename ArgsT >
BOOST_FORCEINLINE format_named_scope_actor< FallbackPolicyT, CharT, ActorT > format_named_scope(attribute_name const& name, FallbackPolicyT const& fallback, ArgsT const& args)
{
    typedef format_named_scope_actor< FallbackPolicyT, CharT, ActorT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typedef default_named_scope_params< CharT > default_params;
    scope_iteration_direction dir = args[keywords::iteration | expressions::forward];
    const CharT* default_delimiter = (dir == expressions::forward ? default_params::forward_delimiter() : default_params::reverse_delimiter());
    typename actor_type::base_type act =
    {{
         terminal_type
         (
             name,
             fallback,
             args[keywords::format],
             args[keywords::delimiter | default_delimiter],
             args[keywords::incomplete_marker | default_params::incomplete_marker()],
             args[keywords::empty_marker | default_params::empty_marker()],
             args[keywords::depth | static_cast< attributes::named_scope::value_type::size_type >(0)],
             dir
         )
    }};
    return actor_type(act);
}

} // namespace aux

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param name Attribute name
 * \param element_format Format string for a single named scope
 */
template< typename CharT >
BOOST_FORCEINLINE format_named_scope_actor< fallback_to_none, CharT > format_named_scope(attribute_name const& name, const CharT* element_format)
{
    typedef format_named_scope_actor< fallback_to_none, CharT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(name, fallback_to_none(), element_format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param name Attribute name
 * \param element_format Format string for a single named scope
 */
template< typename CharT >
BOOST_FORCEINLINE format_named_scope_actor< fallback_to_none, CharT > format_named_scope(attribute_name const& name, std::basic_string< CharT > const& element_format)
{
    typedef format_named_scope_actor< fallback_to_none, CharT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(name, fallback_to_none(), element_format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param keyword Attribute keyword
 * \param element_format Format string for a single named scope
 */
template< typename DescriptorT, template< typename > class ActorT, typename CharT >
BOOST_FORCEINLINE format_named_scope_actor< fallback_to_none, CharT, ActorT >
format_named_scope(attribute_keyword< DescriptorT, ActorT > const& keyword, const CharT* element_format)
{
    BOOST_STATIC_ASSERT_MSG((is_same< typename DescriptorT::value_type, attributes::named_scope::value_type >::value),\
        "Boost.Log: Named scope formatter only accepts attribute values of type attributes::named_scope::value_type.");

    typedef format_named_scope_actor< fallback_to_none, CharT, ActorT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(keyword.get_name(), fallback_to_none(), element_format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param keyword Attribute keyword
 * \param element_format Format string for a single named scope
 */
template< typename DescriptorT, template< typename > class ActorT, typename CharT >
BOOST_FORCEINLINE format_named_scope_actor< fallback_to_none, CharT, ActorT >
format_named_scope(attribute_keyword< DescriptorT, ActorT > const& keyword, std::basic_string< CharT > const& element_format)
{
    BOOST_STATIC_ASSERT_MSG((is_same< typename DescriptorT::value_type, attributes::named_scope::value_type >::value),\
        "Boost.Log: Named scope formatter only accepts attribute values of type attributes::named_scope::value_type.");

    typedef format_named_scope_actor< fallback_to_none, CharT, ActorT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(keyword.get_name(), fallback_to_none(), element_format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param placeholder Attribute placeholder
 * \param element_format Format string for a single named scope
 */
template< typename T, typename FallbackPolicyT, typename TagT, template< typename > class ActorT, typename CharT >
BOOST_FORCEINLINE format_named_scope_actor< FallbackPolicyT, CharT, ActorT >
format_named_scope(attribute_actor< T, FallbackPolicyT, TagT, ActorT > const& placeholder, const CharT* element_format)
{
    BOOST_STATIC_ASSERT_MSG((is_same< T, attributes::named_scope::value_type >::value),\
        "Boost.Log: Named scope formatter only accepts attribute values of type attributes::named_scope::value_type.");

    typedef format_named_scope_actor< FallbackPolicyT, CharT, ActorT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(placeholder.get_name(), placeholder.get_fallback_policy(), element_format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param placeholder Attribute placeholder
 * \param element_format Format string for a single named scope
 */
template< typename T, typename FallbackPolicyT, typename TagT, template< typename > class ActorT, typename CharT >
BOOST_FORCEINLINE format_named_scope_actor< FallbackPolicyT, CharT, ActorT >
format_named_scope(attribute_actor< T, FallbackPolicyT, TagT, ActorT > const& placeholder, std::basic_string< CharT > const& element_format)
{
    BOOST_STATIC_ASSERT_MSG((is_same< T, attributes::named_scope::value_type >::value),\
        "Boost.Log: Named scope formatter only accepts attribute values of type attributes::named_scope::value_type.");

    typedef format_named_scope_actor< FallbackPolicyT, CharT, ActorT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(placeholder.get_name(), placeholder.get_fallback_policy(), element_format) }};
    return actor_type(act);
}

#if !defined(BOOST_LOG_DOXYGEN_PASS)

#   define BOOST_PP_FILENAME_1 <boost/log/detail/named_scope_fmt_pp.hpp>
#   define BOOST_PP_ITERATION_LIMITS (1, 6)
#   include BOOST_PP_ITERATE()

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * Formatter generator. Construct the named scope formatter with the specified formatting parameters.
 *
 * \param name Attribute name
 * \param args An set of named parameters. Supported parameters:
 *             \li \c format - A format string for named scopes. The string can contain "%n", "%f" and "%l" placeholders for the scope name, file and line number, respectively. This parameter is mandatory.
 *             \li \c delimiter - A string that is used to delimit the formatted scope names. Default: "->" or "<-", depending on the iteration direction.
 *             \li \c incomplete_marker - A string that is used to indicate that the list was printed incomplete because of depth limitation. Default: "...".
 *             \li \c empty_marker - A string that is output in case if the scope list is empty. Default: "", i.e. nothing is output.
 *             \li \c iteration - Iteration direction, see \c scope_iteration_direction enumeration. Default: forward.
 *             \li \c depth - Iteration depth. Default: unlimited.
 */
template< typename... ArgsT >
unspecified format_named_scope(attribute_name const& name, ArgsT... const& args);

/*! \overload */
template< typename DescriptorT, template< typename > class ActorT, typename... ArgsT >
unspecified format_named_scope(attribute_keyword< DescriptorT, ActorT > const& keyword, ArgsT... const& args);

/*! \overload */
template< typename T, typename FallbackPolicyT, typename TagT, template< typename > class ActorT, typename... ArgsT >
unspecified format_named_scope(attribute_actor< T, FallbackPolicyT, TagT, ActorT > const& placeholder, ArgsT... const& args);

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace phoenix {

namespace result_of {

template< typename FallbackPolicyT, typename CharT >
struct is_nullary< custom_terminal< boost::log::expressions::format_named_scope_terminal< FallbackPolicyT, CharT > > > :
    public mpl::false_
{
};

} // namespace result_of

} // namespace phoenix

#endif

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_FORMATTERS_NAMED_SCOPE_HPP_INCLUDED_
