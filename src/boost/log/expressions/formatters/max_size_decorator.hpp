/*
 *             Copyright Andrey Semashev 2016.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters/max_size_decorator.hpp
 * \author Andrey Semashev
 * \date   06.07.2016
 *
 * The header contains implementation of a string length limiting decorator.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTERS_MAX_SIZE_DECORATOR_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTERS_MAX_SIZE_DECORATOR_HPP_INCLUDED_

#include <cstddef>
#include <string>
#include <boost/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/core/addressof.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/core/terminal_fwd.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/phoenix/support/vector.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/custom_terminal_spec.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

//! String size limiting decorator stream output terminal
template< typename LeftT, typename SubactorT, typename CharT >
class max_size_decorator_output_terminal
{
private:
    //! Self type
    typedef max_size_decorator_output_terminal< LeftT, SubactorT, CharT > this_type;

public:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Internal typedef for type categorization
    typedef void _is_boost_log_terminal;
#endif

    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! String size type
    typedef std::size_t size_type;
    //! Adopted actor type
    typedef SubactorT subactor_type;

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
    //! Adopted formatter actor
    subactor_type m_subactor;
    //! Max size of the formatted string produced by the adopted formatter
    size_type m_max_size;
    //! Overflow marker
    string_type m_overflow_marker;

public:
    /*!
     * Initializing constructor. Creates decorator of the \a fmt formatter with the specified \a decorations.
     */
    max_size_decorator_output_terminal(LeftT const& left, subactor_type const& sub, size_type max_size, string_type const& overflow_marker = string_type()) :
        m_left(left), m_subactor(sub), m_max_size(max_size), m_overflow_marker(overflow_marker)
    {
        BOOST_ASSERT(overflow_marker.size() <= max_size);
    }
    /*!
     * Copy constructor
     */
    max_size_decorator_output_terminal(max_size_decorator_output_terminal const& that) :
        m_left(that.m_left), m_subactor(that.m_subactor), m_max_size(that.m_max_size), m_overflow_marker(that.m_overflow_marker)
    {
    }

    /*!
     * Invokation operator
     */
    template< typename ContextT >
    typename result< this_type(ContextT const&) >::type operator() (ContextT const& ctx)
    {
        // Flush the stream and keep the current write position in the target string
        typedef typename result< this_type(ContextT const&) >::type result_type;
        result_type strm = phoenix::eval(m_left, ctx);
        strm.flush();

        if (!strm.rdbuf()->storage_overflow())
        {
            const size_type old_max_size = strm.rdbuf()->max_size();
            const size_type start_pos = strm.rdbuf()->storage()->size(), max_size_left = strm.rdbuf()->storage()->max_size() - start_pos;
            strm.rdbuf()->max_size(start_pos + (m_max_size <= max_size_left ? m_max_size : max_size_left));

            try
            {
                // Invoke the adopted formatter
                phoenix::eval(m_subactor, ctx);

                // Flush the buffered characters and apply decorations
                strm.flush();

                if (strm.rdbuf()->storage_overflow())
                {
                    if (!m_overflow_marker.empty())
                    {
                        // Free up space for the overflow marker
                        strm.rdbuf()->max_size(strm.rdbuf()->max_size() - m_overflow_marker.size());

                        // Append the marker
                        strm.rdbuf()->max_size(strm.rdbuf()->storage()->max_size());
                        strm.rdbuf()->storage_overflow(false);
                        strm.rdbuf()->append(m_overflow_marker.data(), m_overflow_marker.size());
                    }
                    else
                    {
                        strm.rdbuf()->storage_overflow(false);
                    }
                }

                // Restore the original size limit
                strm.rdbuf()->max_size(old_max_size);
            }
            catch (...)
            {
                strm.rdbuf()->storage_overflow(false);
                strm.rdbuf()->max_size(old_max_size);
                throw;
            }
        }

        return strm;
    }

    /*!
     * Invokation operator
     */
    template< typename ContextT >
    typename result< const this_type(ContextT const&) >::type operator() (ContextT const& ctx) const
    {
        // Flush the stream and keep the current write position in the target string
        typedef typename result< const this_type(ContextT const&) >::type result_type;
        result_type strm = phoenix::eval(m_left, ctx);
        strm.flush();

        if (!strm.rdbuf()->storage_overflow())
        {
            const size_type old_max_size = strm.rdbuf()->max_size();
            const size_type start_pos = strm.rdbuf()->storage()->size(), max_size_left = strm.rdbuf()->storage()->max_size() - start_pos;
            strm.rdbuf()->max_size(start_pos + (m_max_size <= max_size_left ? m_max_size : max_size_left));

            try
            {
                // Invoke the adopted formatter
                phoenix::eval(m_subactor, ctx);

                // Flush the buffered characters and apply decorations
                strm.flush();

                if (strm.rdbuf()->storage_overflow())
                {
                    if (!m_overflow_marker.empty())
                    {
                        // Free up space for the overflow marker
                        strm.rdbuf()->max_size(strm.rdbuf()->max_size() - m_overflow_marker.size());

                        // Append the marker
                        strm.rdbuf()->max_size(strm.rdbuf()->storage()->max_size());
                        strm.rdbuf()->storage_overflow(false);
                        strm.rdbuf()->append(m_overflow_marker.data(), m_overflow_marker.size());
                    }
                    else
                    {
                        strm.rdbuf()->storage_overflow(false);
                    }
                }

                // Restore the original size limit
                strm.rdbuf()->max_size(old_max_size);
            }
            catch (...)
            {
                strm.rdbuf()->storage_overflow(false);
                strm.rdbuf()->max_size(old_max_size);
                throw;
            }
        }

        return strm;
    }

    BOOST_DELETED_FUNCTION(max_size_decorator_output_terminal())
};

} // namespace aux

/*!
 * String size limiting decorator terminal class. This formatter allows to limit the maximum total length
 * of the strings generated by other formatters.
 *
 * The \c max_size_decorator_terminal class aggregates the formatter being decorated, the maximum string length
 * it can produce and an optional truncation marker string, which will be put at the end of the output if the limit is exceeded. Note that
 * the marker length is included in the limit and as such must not exceed it.
 * The \c max_size_decorator_terminal class is a formatter itself, so it can be used to construct
 * more complex formatters, including nesting decorators.
 */
template< typename SubactorT, typename CharT >
class max_size_decorator_terminal
{
private:
    //! Self type
    typedef max_size_decorator_terminal< SubactorT, CharT > this_type;

public:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Internal typedef for type categorization
    typedef void _is_boost_log_terminal;
#endif

    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! String size type
    typedef std::size_t size_type;
    //! Stream type
    typedef basic_formatting_ostream< char_type > stream_type;
    //! Adopted actor type
    typedef SubactorT subactor_type;

    //! Result type definition
    typedef string_type result_type;

private:
    //! Adopted formatter actor
    subactor_type m_subactor;
    //! Max size of the formatted string produced by the adopted formatter
    size_type m_max_size;
    //! Overflow marker
    string_type m_overflow_marker;

public:
    /*!
     * Initializing constructor.
     */
    max_size_decorator_terminal(subactor_type const& sub, size_type max_size, string_type const& overflow_marker = string_type()) :
        m_subactor(sub), m_max_size(max_size), m_overflow_marker(overflow_marker)
    {
        BOOST_ASSERT(overflow_marker.size() <= max_size);
    }
    /*!
     * Copy constructor
     */
    max_size_decorator_terminal(max_size_decorator_terminal const& that) :
        m_subactor(that.m_subactor), m_max_size(that.m_max_size), m_overflow_marker(that.m_overflow_marker)
    {
    }

    /*!
     * \returns Adopted subactor
     */
    subactor_type const& get_subactor() const
    {
        return m_subactor;
    }

    /*!
     * \returns Max string size limit
     */
    size_type get_max_size() const
    {
        return m_max_size;
    }

    /*!
     * \returns Max string size limit
     */
    string_type const& get_overflow_marker() const
    {
        return m_overflow_marker;
    }

    /*!
     * Invokation operator
     */
    template< typename ContextT >
    result_type operator() (ContextT const& ctx)
    {
        string_type str;
        stream_type strm(str);
        strm.rdbuf()->max_size(m_max_size);

        // Invoke the adopted formatter
        typedef phoenix::vector3<
            subactor_type*,
            typename fusion::result_of::at_c<
                typename remove_cv<
                    typename remove_reference<
                        typename phoenix::result_of::env< ContextT const& >::type
                    >::type
                >::type::args_type,
                0
            >::type,
            stream_type&
        > env_type;
        env_type env = { boost::addressof(m_subactor), fusion::at_c< 0 >(phoenix::env(ctx).args()), strm };
        phoenix::eval(m_subactor, phoenix::make_context(env, phoenix::actions(ctx)));

        // Flush the buffered characters and see of overflow happened
        strm.flush();

        if (strm.rdbuf()->storage_overflow() && !m_overflow_marker.empty())
        {
            strm.rdbuf()->max_size(strm.rdbuf()->max_size() - m_overflow_marker.size());
            strm.rdbuf()->max_size(str.max_size());
            strm.rdbuf()->storage_overflow(false);
            strm.rdbuf()->append(m_overflow_marker.data(), m_overflow_marker.size());
            strm.flush();
        }

        return BOOST_LOG_NRVO_RESULT(str);
    }

    /*!
     * Invokation operator
     */
    template< typename ContextT >
    result_type operator() (ContextT const& ctx) const
    {
        string_type str;
        stream_type strm(str);
        strm.rdbuf()->max_size(m_max_size);

        // Invoke the adopted formatter
        typedef phoenix::vector3<
            const subactor_type*,
            typename fusion::result_of::at_c<
                typename remove_cv<
                    typename remove_reference<
                        typename phoenix::result_of::env< ContextT const& >::type
                    >::type
                >::type::args_type,
                0
            >::type,
            stream_type&
        > env_type;
        env_type env = { boost::addressof(m_subactor), fusion::at_c< 0 >(phoenix::env(ctx).args()), strm };
        phoenix::eval(m_subactor, phoenix::make_context(env, phoenix::actions(ctx)));

        // Flush the buffered characters and see of overflow happened
        strm.flush();

        if (strm.rdbuf()->storage_overflow())
        {
            strm.rdbuf()->max_size(strm.rdbuf()->max_size() - m_overflow_marker.size());
            strm.rdbuf()->max_size(str.max_size());
            strm.rdbuf()->storage_overflow(false);
            strm.rdbuf()->append(m_overflow_marker.data(), m_overflow_marker.size());
            strm.flush();
        }

        return BOOST_LOG_NRVO_RESULT(str);
    }

    BOOST_DELETED_FUNCTION(max_size_decorator_terminal())
};

/*!
 * Character decorator actor
 */
template< typename SubactorT, typename CharT, template< typename > class ActorT = phoenix::actor >
class max_size_decorator_actor :
    public ActorT< max_size_decorator_terminal< SubactorT, CharT > >
{
public:
    //! Base terminal type
    typedef max_size_decorator_terminal< SubactorT, CharT > terminal_type;
    //! Character type
    typedef typename terminal_type::char_type char_type;

    //! Base actor type
    typedef ActorT< terminal_type > base_type;

public:
    //! Initializing constructor
    explicit max_size_decorator_actor(base_type const& act) : base_type(act)
    {
    }

    //! Returns reference to the terminal
    terminal_type const& get_terminal() const
    {
        return this->proto_expr_.child0;
    }
};

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_AUX_OVERLOAD(left_ref, right_ref)\
    template< typename LeftExprT, typename SubactorT, typename CharT, template< typename > class ActorT >\
    BOOST_FORCEINLINE phoenix::actor< aux::max_size_decorator_output_terminal< phoenix::actor< LeftExprT >, SubactorT, CharT > >\
    operator<< (phoenix::actor< LeftExprT > left_ref left, max_size_decorator_actor< SubactorT, CharT, ActorT > right_ref right)\
    {\
        typedef aux::max_size_decorator_output_terminal< phoenix::actor< LeftExprT >, SubactorT, CharT > terminal_type;\
        phoenix::actor< terminal_type > actor = {{ terminal_type(left, right.get_terminal().get_subactor(), right.get_terminal().get_max_size(), right.get_terminal().get_overflow_marker()) }};\
        return actor;\
    }

#include <boost/log/detail/generate_overloads.hpp>

#undef BOOST_LOG_AUX_OVERLOAD

#endif // BOOST_LOG_DOXYGEN_PASS

namespace aux {

template< typename CharT >
class max_size_decorator_gen
{
private:
    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef std::size_t size_type;

private:
    size_type m_max_size;
    string_type m_overflow_marker;

public:
    explicit max_size_decorator_gen(size_type max_size, string_type const& overflow_marker = string_type()) :
        m_max_size(max_size), m_overflow_marker(overflow_marker)
    {
        BOOST_ASSERT(overflow_marker.size() <= max_size);
    }

    template< typename SubactorT >
    BOOST_FORCEINLINE max_size_decorator_actor< SubactorT, char_type > operator[] (SubactorT const& subactor) const
    {
        typedef max_size_decorator_actor< SubactorT, char_type > result_type;
        typedef typename result_type::terminal_type terminal_type;
        typename result_type::base_type act = {{ terminal_type(subactor, m_max_size, m_overflow_marker) }};
        return result_type(act);
    }
};

} // namespace aux

/*!
 * The function returns a decorator generator object. The generator provides <tt>operator[]</tt> that can be used
 * to construct the actual decorator.
 *
 * \param max_size The maximum number of characters (i.e. string element objects) that the decorated formatter can produce.
 */
template< typename CharT >
BOOST_FORCEINLINE aux::max_size_decorator_gen< CharT > max_size_decor(std::size_t max_size)
{
    return aux::max_size_decorator_gen< CharT >(max_size);
}

/*!
 * The function returns a decorator generator object. The generator provides <tt>operator[]</tt> that can be used
 * to construct the actual decorator.
 *
 * \param max_size The maximum number of characters (i.e. string element objects) that the decorated formatter can produce.
 * \param overflow_marker The marker string which is appended to the output if the \a max_size limit is exceeded. Must be
 *                        a non-null pointer to a zero-terminated string.
 *
 * \pre The \a overflow_marker length must not exceed the \a max_size limit.
 */
template< typename CharT >
BOOST_FORCEINLINE aux::max_size_decorator_gen< CharT > max_size_decor(std::size_t max_size, const CharT* overflow_marker)
{
    return aux::max_size_decorator_gen< CharT >(max_size, overflow_marker);
}

/*!
 * The function returns a decorator generator object. The generator provides <tt>operator[]</tt> that can be used
 * to construct the actual decorator.
 *
 * \param max_size The maximum number of characters (i.e. string element objects) that the decorated formatter can produce.
 * \param overflow_marker The marker string which is appended to the output if the \a max_size limit is exceeded.
 *
 * \pre The \a overflow_marker length must not exceed the \a max_size limit.
 */
template< typename CharT >
BOOST_FORCEINLINE aux::max_size_decorator_gen< CharT > max_size_decor(std::size_t max_size, std::basic_string< CharT > const& overflow_marker)
{
    return aux::max_size_decorator_gen< CharT >(max_size, overflow_marker);
}

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace phoenix {

namespace result_of {

template< typename SubactorT, typename CharT >
struct is_nullary< custom_terminal< boost::log::expressions::max_size_decorator_terminal< SubactorT, CharT > > > :
    public mpl::false_
{
};

template< typename LeftT, typename SubactorT, typename CharT >
struct is_nullary< custom_terminal< boost::log::expressions::aux::max_size_decorator_output_terminal< LeftT, SubactorT, CharT > > > :
    public mpl::false_
{
};

} // namespace result_of

} // namespace phoenix

#endif

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_FORMATTERS_MAX_SIZE_DECORATOR_HPP_INCLUDED_
