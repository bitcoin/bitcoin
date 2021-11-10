/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attribute_predicate.hpp
 * \author Andrey Semashev
 * \date   02.09.2012
 *
 * The header contains implementation of a generic predicate in template expressions.
 */

#ifndef BOOST_LOG_DETAIL_ATTRIBUTE_PREDICATE_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_ATTRIBUTE_PREDICATE_HPP_INCLUDED_

#include <boost/phoenix/core/actor.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/attributes/fallback_policy.hpp>
#include <boost/log/utility/functional/bind.hpp>
#include <boost/log/utility/functional/save_result.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

/*!
 * The predicate checks if the attribute value satisfies a predicate.
 */
template< typename T, typename ArgT, typename PredicateT, typename FallbackPolicyT = fallback_to_none >
class attribute_predicate
{
public:
    //! Function result_type
    typedef bool result_type;
    //! Expected attribute value type
    typedef T value_type;
    //! Predicate type
    typedef PredicateT predicate_type;
    //! Argument type for the predicate
    typedef ArgT argument_type;
    //! Fallback policy
    typedef FallbackPolicyT fallback_policy;

private:
    //! Argument for the predicate
    const argument_type m_arg;
    //! Attribute value name
    const attribute_name m_name;
    //! Visitor invoker
    value_visitor_invoker< value_type, fallback_policy > m_visitor_invoker;

public:
    /*!
     * Initializing constructor
     *
     * \param name Attribute name
     * \param pred_arg The predicate argument
     */
    attribute_predicate(attribute_name const& name, argument_type const& pred_arg) : m_arg(pred_arg), m_name(name)
    {
    }

    /*!
     * Initializing constructor
     *
     * \param name Attribute name
     * \param pred_arg The predicate argument
     * \param arg Additional parameter for the fallback policy
     */
    template< typename U >
    attribute_predicate(attribute_name const& name, argument_type const& pred_arg, U const& arg) : m_arg(pred_arg), m_name(name), m_visitor_invoker(arg)
    {
    }

    /*!
     * Checking operator
     *
     * \param arg A set of attribute values or a log record
     * \return \c true if the log record contains the sought attribute value, \c false otherwise
     */
    template< typename ArgumentT >
    result_type operator() (ArgumentT const& arg) const
    {
        typedef binder2nd< predicate_type, argument_type const& > visitor_type;

        bool res = false;
        m_visitor_invoker(m_name, arg, boost::log::save_result(visitor_type(predicate_type(), m_arg), res));
        return res;
    }
};

} // namespace aux

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_ATTRIBUTE_PREDICATE_HPP_INCLUDED_
