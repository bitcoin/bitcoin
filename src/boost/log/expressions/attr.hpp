/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr.hpp
 * \author Andrey Semashev
 * \date   21.07.2012
 *
 * The header contains implementation of a generic attribute placeholder in template expressions.
 */

#ifndef BOOST_LOG_EXPRESSIONS_ATTR_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_ATTR_HPP_INCLUDED_

#include <boost/mpl/bool.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/terminal_fwd.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/copy_cv.hpp>
#include <boost/log/detail/custom_terminal_spec.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/attributes/fallback_policy.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

/*!
 * An attribute value extraction terminal
 */
template< typename T, typename FallbackPolicyT, typename TagT >
class attribute_terminal
{
private:
    //! Value extractor type
    typedef value_extractor< T, FallbackPolicyT, TagT > value_extractor_type;
    //! Self type
    typedef attribute_terminal< T, FallbackPolicyT, TagT > this_type;

public:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Internal typedef for type categorization
    typedef void _is_boost_log_terminal;
#endif

    //! Attribute tag type
    typedef TagT tag_type;
    //! Attribute value type
    typedef typename value_extractor_type::value_type value_type;
    //! Fallback policy type
    typedef typename value_extractor_type::fallback_policy fallback_policy;

    //! Function result type
    template< typename >
    struct result;

    template< typename ThisT, typename ContextT >
    struct result< ThisT(ContextT) >
    {
        typedef typename remove_cv<
            typename remove_reference< typename phoenix::result_of::env< ContextT >::type >::type
        >::type env_type;
        typedef typename env_type::args_type args_type;
        typedef typename boost::log::aux::copy_cv< ThisT, value_extractor_type >::type cv_value_extractor_type;

        typedef typename boost::result_of< cv_value_extractor_type(attribute_name const&, typename fusion::result_of::at_c< args_type, 0 >::type) >::type type;
    };

private:
    //! Attribute value name
    const attribute_name m_name;
    //! Attribute value extractor
    value_extractor_type m_value_extractor;

public:
    /*!
     * Initializing constructor
     */
    explicit attribute_terminal(attribute_name const& name) : m_name(name)
    {
    }

    /*!
     * Initializing constructor
     */
    template< typename U >
    attribute_terminal(attribute_name const& name, U const& arg) : m_name(name), m_value_extractor(arg)
    {
    }

    /*!
     * \returns Attribute value name
     */
    attribute_name get_name() const
    {
        return m_name;
    }

    /*!
     * \returns Fallback policy
     */
    fallback_policy const& get_fallback_policy() const
    {
        return m_value_extractor.get_fallback_policy();
    }

    /*!
     * The operator extracts attribute value
     */
    template< typename ContextT >
    typename result< this_type(ContextT const&) >::type
    operator() (ContextT const& ctx)
    {
        return m_value_extractor(m_name, fusion::at_c< 0 >(phoenix::env(ctx).args()));
    }

    /*!
     * The operator extracts attribute value
     */
    template< typename ContextT >
    typename result< const this_type(ContextT const&) >::type
    operator() (ContextT const& ctx) const
    {
        return m_value_extractor(m_name, fusion::at_c< 0 >(phoenix::env(ctx).args()));
    }

    BOOST_DELETED_FUNCTION(attribute_terminal())
};

/*!
 * An attribute value extraction terminal actor
 */
template< typename T, typename FallbackPolicyT, typename TagT, template< typename > class ActorT >
class attribute_actor :
    public ActorT< attribute_terminal< T, FallbackPolicyT, TagT > >
{
public:
    //! Attribute tag type
    typedef TagT tag_type;
    //! Fallback policy
    typedef FallbackPolicyT fallback_policy;
    //! Base terminal type
    typedef attribute_terminal< T, fallback_policy, tag_type > terminal_type;
    //! Attribute value type
    typedef typename terminal_type::value_type value_type;

    //! Base actor type
    typedef ActorT< terminal_type > base_type;

public:
    //! Initializing constructor
    explicit attribute_actor(base_type const& act) : base_type(act)
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

    //! Expression with cached attribute name
    typedef attribute_actor< value_type, fallback_to_none, tag_type, ActorT > or_none_result_type;

    //! Generates an expression that extracts the attribute value or a default value
    or_none_result_type or_none() const
    {
        typedef typename or_none_result_type::terminal_type result_terminal;
        typename or_none_result_type::base_type act = {{ result_terminal(get_name()) }};
        return or_none_result_type(act);
    }

    //! Expression with cached attribute name
    typedef attribute_actor< value_type, fallback_to_throw, tag_type, ActorT > or_throw_result_type;

    //! Generates an expression that extracts the attribute value or throws an exception
    or_throw_result_type or_throw() const
    {
        typedef typename or_throw_result_type::terminal_type result_terminal;
        typename or_throw_result_type::base_type act = {{ result_terminal(get_name()) }};
        return or_throw_result_type(act);
    }

    //! Generates an expression that extracts the attribute value or a default value
    template< typename DefaultT >
    attribute_actor< value_type, fallback_to_default< DefaultT >, tag_type, ActorT > or_default(DefaultT const& def_val) const
    {
        typedef attribute_actor< value_type, fallback_to_default< DefaultT >, tag_type, ActorT > or_default_result_type;
        typedef typename or_default_result_type::terminal_type result_terminal;
        typename or_default_result_type::base_type act = {{ result_terminal(get_name(), def_val) }};
        return or_default_result_type(act);
    }
};

/*!
 * The function generates a terminal node in a template expression. The node will extract the value of the attribute
 * with the specified name and type.
 */
template< typename AttributeValueT >
BOOST_FORCEINLINE attribute_actor< AttributeValueT > attr(attribute_name const& name)
{
    typedef attribute_actor< AttributeValueT > result_type;
    typedef typename result_type::terminal_type result_terminal;
    typename result_type::base_type act = {{ result_terminal(name) }};
    return result_type(act);
}

/*!
 * The function generates a terminal node in a template expression. The node will extract the value of the attribute
 * with the specified name and type.
 */
template< typename AttributeValueT, typename TagT >
BOOST_FORCEINLINE attribute_actor< AttributeValueT, fallback_to_none, TagT > attr(attribute_name const& name)
{
    typedef attribute_actor< AttributeValueT, fallback_to_none, TagT > result_type;
    typedef typename result_type::terminal_type result_terminal;
    typename result_type::base_type act = {{ result_terminal(name) }};
    return result_type(act);
}

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace phoenix {

namespace result_of {

template< typename T, typename FallbackPolicyT, typename TagT >
struct is_nullary< custom_terminal< boost::log::expressions::attribute_terminal< T, FallbackPolicyT, TagT > > > :
    public mpl::false_
{
};

} // namespace result_of

} // namespace phoenix

#endif

} // namespace boost

#include <boost/log/detail/footer.hpp>
#if defined(BOOST_LOG_EXPRESSIONS_FORMATTERS_STREAM_HPP_INCLUDED_)
#include <boost/log/detail/attr_output_impl.hpp>
#endif

#endif // BOOST_LOG_EXPRESSIONS_ATTR_HPP_INCLUDED_
