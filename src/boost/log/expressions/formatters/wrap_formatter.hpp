/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters/wrap_formatter.hpp
 * \author Andrey Semashev
 * \date   24.11.2012
 *
 * The header contains a formatter function wrapper that enables third-party functions to participate in formatting expressions.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTERS_WRAP_FORMATTER_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTERS_WRAP_FORMATTER_HPP_INCLUDED_

#include <string>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/terminal_fwd.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/custom_terminal_spec.hpp>
#include <boost/log/detail/function_traits.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

//! Wrapped formatter stream output terminal
template< typename LeftT, typename FunT >
class wrapped_formatter_output_terminal
{
private:
    //! Self type
    typedef wrapped_formatter_output_terminal< LeftT, FunT > this_type;

public:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Internal typedef for type categorization
    typedef void _is_boost_log_terminal;
#endif

    //! Wrapped function type
    typedef FunT function_type;

    //! Result type definition
    template< typename >
    struct result;

    template< typename ThisT, typename ContextT >
    struct result< ThisT(ContextT) >
    {
        typedef typename remove_cv< typename remove_reference< ContextT >::type >::type context_type;
        typedef typename phoenix::evaluator::impl<
            typename LeftT::proto_base_expr&,
            context_type,
            phoenix::unused
        >::result_type type;
    };

private:
    //! Left argument actor
    LeftT m_left;
    //! Wrapped function
    function_type m_fun;

public:
    //! Initializing constructor
    wrapped_formatter_output_terminal(LeftT const& left, function_type const& fun) : m_left(left), m_fun(fun)
    {
    }
    //! Copy constructor
    wrapped_formatter_output_terminal(wrapped_formatter_output_terminal const& that) : m_left(that.m_left), m_fun(that.m_fun)
    {
    }

    //! Invokation operator
    template< typename ContextT >
    typename result< this_type(ContextT const&) >::type operator() (ContextT const& ctx)
    {
        typedef typename result< this_type(ContextT const&) >::type result_type;
        result_type strm = phoenix::eval(m_left, ctx);
        m_fun(fusion::at_c< 0 >(phoenix::env(ctx).args()), strm);
        return strm;
    }

    //! Invokation operator
    template< typename ContextT >
    typename result< const this_type(ContextT const&) >::type operator() (ContextT const& ctx) const
    {
        typedef typename result< const this_type(ContextT const&) >::type result_type;
        result_type strm = phoenix::eval(m_left, ctx);
        m_fun(fusion::at_c< 0 >(phoenix::env(ctx).args()), strm);
        return strm;
    }

    BOOST_DELETED_FUNCTION(wrapped_formatter_output_terminal())
};

BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(has_char_type, char_type, false)

template<
    typename FunT,
    bool HasCharTypeV = has_char_type< FunT >::value,
    bool HasSecondArgumentV = boost::log::aux::has_second_argument_type< FunT >::value,
    bool HasArg2V = boost::log::aux::has_arg2_type< FunT >::value
>
struct default_char_type
{
    // Use this char type if all detection fails
    typedef char type;
};

template< typename FunT, bool HasSecondArgumentV, bool HasArg2V >
struct default_char_type< FunT, true, HasSecondArgumentV, HasArg2V >
{
    typedef typename FunT::char_type type;
};

template< typename FunT, bool HasArg2V >
struct default_char_type< FunT, false, true, HasArg2V >
{
    typedef typename remove_cv< typename remove_reference< typename FunT::second_argument_type >::type >::type argument_type;
    typedef typename argument_type::char_type type;
};

template< typename FunT >
struct default_char_type< FunT, false, false, true >
{
    typedef typename remove_cv< typename remove_reference< typename FunT::arg2_type >::type >::type argument_type;
    typedef typename argument_type::char_type type;
};

} // namespace aux

/*!
 * Formatter function wrapper terminal.
 */
template< typename FunT, typename CharT >
class wrapped_formatter_terminal
{
public:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Internal typedef for type categorization
    typedef void _is_boost_log_terminal;
#endif

    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Formatting stream type
    typedef basic_formatting_ostream< char_type > stream_type;
    //! Wrapped function type
    typedef FunT function_type;

    //! Formatter result type
    typedef string_type result_type;

private:
    //! Wrapped function
    function_type m_fun;

public:
    //! Initializing construction
    explicit wrapped_formatter_terminal(function_type const& fun) : m_fun(fun)
    {
    }
    //! Copy constructor
    wrapped_formatter_terminal(wrapped_formatter_terminal const& that) : m_fun(that.m_fun)
    {
    }

    //! Returns the wrapped function
    function_type const& get_function() const
    {
        return m_fun;
    }

    //! Invokation operator
    template< typename ContextT >
    result_type operator() (ContextT const& ctx)
    {
        string_type str;
        stream_type strm(str);
        m_fun(fusion::at_c< 0 >(phoenix::env(ctx).args()), strm);
        strm.flush();
        return BOOST_LOG_NRVO_RESULT(str);
    }

    //! Invokation operator
    template< typename ContextT >
    result_type operator() (ContextT const& ctx) const
    {
        string_type str;
        stream_type strm(str);
        m_fun(fusion::at_c< 0 >(phoenix::env(ctx).args()), strm);
        strm.flush();
        return BOOST_LOG_NRVO_RESULT(str);
    }
};

/*!
 * Wrapped formatter function actor.
 */
template< typename FunT, typename CharT, template< typename > class ActorT = phoenix::actor >
class wrapped_formatter_actor :
    public ActorT< wrapped_formatter_terminal< FunT, CharT > >
{
public:
    //! Character type
    typedef CharT char_type;
    //! Wrapped function type
    typedef FunT function_type;
    //! Base terminal type
    typedef wrapped_formatter_terminal< function_type, char_type > terminal_type;

    //! Base actor type
    typedef ActorT< terminal_type > base_type;

public:
    //! Initializing constructor
    explicit wrapped_formatter_actor(base_type const& act) : base_type(act)
    {
    }

    /*!
     * \returns The wrapped function
     */
    function_type const& get_function() const
    {
        return this->proto_expr_.child0.get_function();
    }
};

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_AUX_OVERLOAD(left_ref, right_ref)\
    template< typename LeftExprT, typename FunT, typename CharT >\
    BOOST_FORCEINLINE phoenix::actor< aux::wrapped_formatter_output_terminal< phoenix::actor< LeftExprT >, FunT > >\
    operator<< (phoenix::actor< LeftExprT > left_ref left, wrapped_formatter_actor< FunT, CharT > right_ref right)\
    {\
        typedef aux::wrapped_formatter_output_terminal< phoenix::actor< LeftExprT >, FunT > terminal_type;\
        phoenix::actor< terminal_type > actor = {{ terminal_type(left, right.get_function()) }};\
        return actor;\
    }

#include <boost/log/detail/generate_overloads.hpp>

#undef BOOST_LOG_AUX_OVERLOAD

#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * The function wraps a function object in order it to be able to participate in formatting expressions. The wrapped
 * function object must be compatible with the following signature:
 *
 * <pre>
 * void (record_view const&, basic_formatting_ostream< CharT >&)
 * </pre>
 *
 * where \c CharT is the character type of the formatting expression.
 */
template< typename FunT >
BOOST_FORCEINLINE wrapped_formatter_actor< FunT, typename aux::default_char_type< FunT >::type > wrap_formatter(FunT const& fun)
{
    typedef wrapped_formatter_actor< FunT, typename aux::default_char_type< FunT >::type > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(fun) }};
    return actor_type(act);
}

/*!
 * The function wraps a function object in order it to be able to participate in formatting expressions. The wrapped
 * function object must be compatible with the following signature:
 *
 * <pre>
 * void (record_view const&, basic_formatting_ostream< CharT >&)
 * </pre>
 *
 * where \c CharT is the character type of the formatting expression.
 */
template< typename CharT, typename FunT >
BOOST_FORCEINLINE wrapped_formatter_actor< FunT, CharT > wrap_formatter(FunT const& fun)
{
    typedef wrapped_formatter_actor< FunT, CharT > actor_type;
    typedef typename actor_type::terminal_type terminal_type;
    typename actor_type::base_type act = {{ terminal_type(fun) }};
    return actor_type(act);
}

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace phoenix {

namespace result_of {

template< typename LeftT, typename FunT >
struct is_nullary< custom_terminal< boost::log::expressions::aux::wrapped_formatter_output_terminal< LeftT, FunT > > > :
    public mpl::false_
{
};

template< typename FunT, typename CharT >
struct is_nullary< custom_terminal< boost::log::expressions::wrapped_formatter_terminal< FunT, CharT > > > :
    public mpl::false_
{
};

} // namespace result_of

} // namespace phoenix

#endif

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_FORMATTERS_WRAP_FORMATTER_HPP_INCLUDED_
