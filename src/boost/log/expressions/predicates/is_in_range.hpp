/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   is_in_range.hpp
 * \author Andrey Semashev
 * \date   02.09.2012
 *
 * The header contains implementation of an \c is_in_range predicate in template expressions.
 */

#ifndef BOOST_LOG_EXPRESSIONS_PREDICATES_IS_IN_RANGE_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_PREDICATES_IS_IN_RANGE_HPP_INCLUDED_

#include <utility>
#include <boost/phoenix/core/actor.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/embedded_string_type.hpp>
#include <boost/log/detail/unary_function_terminal.hpp>
#include <boost/log/detail/attribute_predicate.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/expressions/keyword_fwd.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/fallback_policy.hpp>
#include <boost/log/utility/functional/in_range.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

/*!
 * The predicate checks if the attribute value contains a substring. The attribute value is assumed to be of a string type.
 */
#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

template< typename T, typename BoundaryT, typename FallbackPolicyT = fallback_to_none >
using attribute_is_in_range = aux::attribute_predicate< T, std::pair< BoundaryT, BoundaryT >, in_range_fun, FallbackPolicyT >;

#else // !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

template< typename T, typename BoundaryT, typename FallbackPolicyT = fallback_to_none >
class attribute_is_in_range :
    public aux::attribute_predicate< T, std::pair< BoundaryT, BoundaryT >, in_range_fun, FallbackPolicyT >
{
    typedef aux::attribute_predicate< T, std::pair< BoundaryT, BoundaryT >, in_range_fun, FallbackPolicyT > base_type;

public:
    /*!
     * Initializing constructor
     *
     * \param name Attribute name
     * \param boundaries The expected attribute value boundaries
     */
    attribute_is_in_range(attribute_name const& name, std::pair< BoundaryT, BoundaryT > const& boundaries) : base_type(name, boundaries)
    {
    }

    /*!
     * Initializing constructor
     *
     * \param name Attribute name
     * \param boundaries The expected attribute value boundaries
     * \param arg Additional parameter for the fallback policy
     */
    template< typename U >
    attribute_is_in_range(attribute_name const& name, std::pair< BoundaryT, BoundaryT > const& boundaries, U const& arg) : base_type(name, boundaries, arg)
    {
    }
};

#endif // !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

/*!
 * The function generates a terminal node in a template expression. The node will check if the attribute value
 * is in the specified range. The range must be half-open, that is the predicate will be equivalent to <tt>least <= attr < most</tt>.
 */
template< typename T, typename FallbackPolicyT, typename TagT, template< typename > class ActorT, typename BoundaryT >
BOOST_FORCEINLINE ActorT< aux::unary_function_terminal< attribute_is_in_range< T, typename boost::log::aux::make_embedded_string_type< BoundaryT >::type, FallbackPolicyT > > >
is_in_range(attribute_actor< T, FallbackPolicyT, TagT, ActorT > const& attr, BoundaryT const& least, BoundaryT const& most)
{
    typedef typename boost::log::aux::make_embedded_string_type< BoundaryT >::type boundary_type;
    typedef aux::unary_function_terminal< attribute_is_in_range< T, boundary_type, FallbackPolicyT > > terminal_type;
    ActorT< terminal_type > act = {{ terminal_type(attr.get_name(), std::pair< boundary_type, boundary_type >(least, most), attr.get_fallback_policy()) }};
    return act;
}

/*!
 * The function generates a terminal node in a template expression. The node will check if the attribute value
 * is in the specified range. The range must be half-open, that is the predicate will be equivalent to <tt>least <= attr < most</tt>.
 */
template< typename DescriptorT, template< typename > class ActorT, typename BoundaryT >
BOOST_FORCEINLINE ActorT< aux::unary_function_terminal< attribute_is_in_range< typename DescriptorT::value_type, typename boost::log::aux::make_embedded_string_type< BoundaryT >::type > > >
is_in_range(attribute_keyword< DescriptorT, ActorT > const&, BoundaryT const& least, BoundaryT const& most)
{
    typedef typename boost::log::aux::make_embedded_string_type< BoundaryT >::type boundary_type;
    typedef aux::unary_function_terminal< attribute_is_in_range< typename DescriptorT::value_type, boundary_type > > terminal_type;
    ActorT< terminal_type > act = {{ terminal_type(DescriptorT::get_name(), std::pair< boundary_type, boundary_type >(least, most)) }};
    return act;
}

/*!
 * The function generates a terminal node in a template expression. The node will check if the attribute value
 * is in the specified range. The range must be half-open, that is the predicate will be equivalent to <tt>least <= attr < most</tt>.
 */
template< typename T, typename BoundaryT >
BOOST_FORCEINLINE phoenix::actor< aux::unary_function_terminal< attribute_is_in_range< T, typename boost::log::aux::make_embedded_string_type< BoundaryT >::type > > >
is_in_range(attribute_name const& name, BoundaryT const& least, BoundaryT const& most)
{
    typedef typename boost::log::aux::make_embedded_string_type< BoundaryT >::type boundary_type;
    typedef aux::unary_function_terminal< attribute_is_in_range< T, boundary_type > > terminal_type;
    phoenix::actor< terminal_type > act = {{ terminal_type(name, std::pair< boundary_type, boundary_type >(least, most)) }};
    return act;
}

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_PREDICATES_IS_IN_RANGE_HPP_INCLUDED_
