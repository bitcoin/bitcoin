/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attr_output_impl.hpp
 * \author Andrey Semashev
 * \date   12.08.2012
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_ATTR_OUTPUT_IMPL_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_ATTR_OUTPUT_IMPL_HPP_INCLUDED_

#include <boost/mpl/is_sequence.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/utility/functional/bind_to_log.hpp>
#include <boost/log/detail/attr_output_terminal.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

template< typename LeftT, typename T, typename FallbackPolicyT, typename TagT >
struct make_output_expression
{
    //! Resulting expression
    typedef attribute_output_terminal< LeftT, T, FallbackPolicyT, to_log_fun< TagT > > type;

    //! Creates the output expression
    template< typename RightT >
    static BOOST_FORCEINLINE type make(LeftT const& left, RightT const& right)
    {
        return type(left, right.get_name(), to_log_fun< TagT >(), right.get_fallback_policy());
    }
};

template< typename LeftT, typename RightT, typename ValueT = typename RightT::value_type, bool IsSequenceV = mpl::is_sequence< ValueT >::value >
struct make_output_actor;

template< template< typename > class ActorT, typename LeftExprT, typename RightT, typename ValueT >
struct make_output_actor< ActorT< LeftExprT >, RightT, ValueT, false >
{
    typedef make_output_expression<
        ActorT< LeftExprT >,
        ValueT,
        typename RightT::fallback_policy,
        typename RightT::tag_type
    > make_expression;

    typedef ActorT< typename make_expression::type > type;

    static BOOST_FORCEINLINE type make(ActorT< LeftExprT > const& left, RightT const& right)
    {
        type res = {{ make_expression::make(left, right) }};
        return res;
    }
};

template< template< typename > class ActorT, typename LeftExprT, typename RightT, typename ValueT >
struct make_output_actor< ActorT< LeftExprT >, RightT, ValueT, true >
{
    typedef attribute_output_terminal< ActorT< LeftExprT >, ValueT, typename RightT::fallback_policy, to_log_fun< typename RightT::tag_type > > expression_type;

    typedef ActorT< expression_type > type;

    static BOOST_FORCEINLINE type make(ActorT< LeftExprT > const& left, RightT const& right)
    {
        type res = {{ expression_type(left, right.get_name(), to_log_fun< typename RightT::tag_type >(), right.get_fallback_policy()) }};
        return res;
    }
};

} // namespace aux

#define BOOST_LOG_AUX_OVERLOAD(left_ref, right_ref)\
    template< typename LeftExprT, typename T, typename FallbackPolicyT, typename TagT >\
    BOOST_FORCEINLINE typename aux::make_output_actor< phoenix::actor< LeftExprT >, attribute_actor< T, FallbackPolicyT, TagT, phoenix::actor > >::type\
    operator<< (phoenix::actor< LeftExprT > left_ref left, attribute_actor< T, FallbackPolicyT, TagT, phoenix::actor > right_ref right)\
    {\
        return aux::make_output_actor< phoenix::actor< LeftExprT >, attribute_actor< T, FallbackPolicyT, TagT, phoenix::actor > >::make(left, right);\
    }

#include <boost/log/detail/generate_overloads.hpp>

#undef BOOST_LOG_AUX_OVERLOAD

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_ATTR_OUTPUT_IMPL_HPP_INCLUDED_
