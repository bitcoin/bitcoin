/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters/date_time.hpp
 * \author Andrey Semashev
 * \date   16.09.2012
 *
 * The header contains a formatter function for date and time attribute values.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTERS_DATE_TIME_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTERS_DATE_TIME_HPP_INCLUDED_

#include <string>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/terminal_fwd.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/fallback_policy.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/detail/date_time_fmt_gen_traits_fwd.hpp>
#include <boost/log/detail/custom_terminal_spec.hpp>
#include <boost/log/detail/attr_output_terminal.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/expressions/keyword_fwd.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/functional/bind.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

/*!
 * Date and time formatter terminal.
 */
template< typename T, typename FallbackPolicyT, typename CharT >
class format_date_time_terminal
{
public:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Internal typedef for type categorization
    typedef void _is_boost_log_terminal;
#endif

    //! Attribute value type
    typedef T value_type;
    //! Fallback policy
    typedef FallbackPolicyT fallback_policy;
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Formatting stream type
    typedef basic_formatting_ostream< char_type > stream_type;

    //! Formatter function
    typedef boost::log::aux::light_function< void (stream_type&, value_type const&) > formatter_function_type;

    //! Function result type
    typedef string_type result_type;

private:
    //! Formatter generator traits
    typedef aux::date_time_formatter_generator_traits< value_type, char_type > formatter_generator;
    //! Attribute value visitor invoker
    typedef value_visitor_invoker< value_type, fallback_policy > visitor_invoker_type;

private:
    //! Attribute name
    attribute_name m_name;
    //! Formattr function
    formatter_function_type m_formatter;
    //! Attribute value visitor invoker
    visitor_invoker_type m_visitor_invoker;

public:
    //! Initializing constructor
    format_date_time_terminal(attribute_name const& name, fallback_policy const& fallback, string_type const& format) :
        m_name(name), m_formatter(formatter_generator::parse(format)), m_visitor_invoker(fallback)
    {
    }
    //! Copy constructor
    format_date_time_terminal(format_date_time_terminal const& that) :
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

    BOOST_DELETED_FUNCTION(format_date_time_terminal())
};

/*!
 * Date and time formatter actor.
 */
template< typename T, typename FallbackPolicyT, typename CharT, template< typename > class ActorT = phoenix::actor >
class format_date_time_actor :
    public ActorT< format_date_time_terminal< T, FallbackPolicyT, CharT > >
{
public:
    //! Attribute value type
    typedef T value_type;
    //! Character type
    typedef CharT char_type;
    //! Fallback policy
    typedef FallbackPolicyT fallback_policy;
    //! Base terminal type
    typedef format_date_time_terminal< value_type, fallback_policy, char_type > terminal_type;
    //! Formatter function
    typedef typename terminal_type::formatter_function_type formatter_function_type;

    //! Base actor type
    typedef ActorT< terminal_type > base_type;

public:
    //! Initializing constructor
    explicit format_date_time_actor(base_type const& act) : base_type(act)
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
    template< typename LeftExprT, typename T, typename FallbackPolicyT, typename CharT >\
    BOOST_FORCEINLINE phoenix::actor< aux::attribute_output_terminal< phoenix::actor< LeftExprT >, T, FallbackPolicyT, typename format_date_time_actor< T, FallbackPolicyT, CharT >::formatter_function_type > >\
    operator<< (phoenix::actor< LeftExprT > left_ref left, format_date_time_actor< T, FallbackPolicyT, CharT > right_ref right)\
    {\
        typedef aux::attribute_output_terminal< phoenix::actor< LeftExprT >, T, FallbackPolicyT, typename format_date_time_actor< T, FallbackPolicyT, CharT >::formatter_function_type > terminal_type;\
        phoenix::actor< terminal_type > actor = {{ terminal_type(left, right.get_name(), right.get_formatter_function(), right.get_fallback_policy()) }};\
        return actor;\
    }

#include <boost/log/detail/generate_overloads.hpp>

#undef BOOST_LOG_AUX_OVERLOAD

#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param name Attribute name
 * \param format Format string
 */
template< typename AttributeValueT, typename CharT >
BOOST_FORCEINLINE format_date_time_actor< AttributeValueT, fallback_to_none, CharT > format_date_time(attribute_name const& name, const CharT* format)
{
    typedef format_date_time_actor< AttributeValueT, fallback_to_none, CharT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(name, fallback_to_none(), format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param name Attribute name
 * \param format Format string
 */
template< typename AttributeValueT, typename CharT >
BOOST_FORCEINLINE format_date_time_actor< AttributeValueT, fallback_to_none, CharT > format_date_time(attribute_name const& name, std::basic_string< CharT > const& format)
{
    typedef format_date_time_actor< AttributeValueT, fallback_to_none, CharT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(name, fallback_to_none(), format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param keyword Attribute keyword
 * \param format Format string
 */
template< typename DescriptorT, template< typename > class ActorT, typename CharT >
BOOST_FORCEINLINE format_date_time_actor< typename DescriptorT::value_type, fallback_to_none, CharT, ActorT >
format_date_time(attribute_keyword< DescriptorT, ActorT > const& keyword, const CharT* format)
{
    typedef format_date_time_actor< typename DescriptorT::value_type, fallback_to_none, CharT, ActorT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(keyword.get_name(), fallback_to_none(), format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param keyword Attribute keyword
 * \param format Format string
 */
template< typename DescriptorT, template< typename > class ActorT, typename CharT >
BOOST_FORCEINLINE format_date_time_actor< typename DescriptorT::value_type, fallback_to_none, CharT, ActorT >
format_date_time(attribute_keyword< DescriptorT, ActorT > const& keyword, std::basic_string< CharT > const& format)
{
    typedef format_date_time_actor< typename DescriptorT::value_type, fallback_to_none, CharT, ActorT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(keyword.get_name(), fallback_to_none(), format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param placeholder Attribute placeholder
 * \param format Format string
 */
template< typename T, typename FallbackPolicyT, typename TagT, template< typename > class ActorT, typename CharT >
BOOST_FORCEINLINE format_date_time_actor< T, FallbackPolicyT, CharT, ActorT >
format_date_time(attribute_actor< T, FallbackPolicyT, TagT, ActorT > const& placeholder, const CharT* format)
{
    typedef format_date_time_actor< T, FallbackPolicyT, CharT, ActorT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(placeholder.get_name(), placeholder.get_fallback_policy(), format) }};
    return actor_type(act);
}

/*!
 * The function generates a manipulator node in a template expression. The manipulator must participate in a formatting
 * expression (stream output or \c format placeholder filler).
 *
 * \param placeholder Attribute placeholder
 * \param format Format string
 */
template< typename T, typename FallbackPolicyT, typename TagT, template< typename > class ActorT, typename CharT >
BOOST_FORCEINLINE format_date_time_actor< T, FallbackPolicyT, CharT, ActorT >
format_date_time(attribute_actor< T, FallbackPolicyT, TagT, ActorT > const& placeholder, std::basic_string< CharT > const& format)
{
    typedef format_date_time_actor< T, FallbackPolicyT, CharT, ActorT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(placeholder.get_name(), placeholder.get_fallback_policy(), format) }};
    return actor_type(act);
}

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace phoenix {

namespace result_of {

template< typename T, typename FallbackPolicyT, typename CharT >
struct is_nullary< custom_terminal< boost::log::expressions::format_date_time_terminal< T, FallbackPolicyT, CharT > > > :
    public mpl::false_
{
};

} // namespace result_of

} // namespace phoenix

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_FORMATTERS_DATE_TIME_HPP_INCLUDED_
