/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   fallback_policy.hpp
 * \author Andrey Semashev
 * \date   18.08.2012
 *
 * The header contains definition of fallback policies when attribute value visitation or extraction fails.
 */

#ifndef BOOST_LOG_ATTRIBUTES_FALLBACK_POLICY_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_FALLBACK_POLICY_HPP_INCLUDED_

#include <boost/type_index.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/attributes/fallback_policy_fwd.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * The \c fallback_to_none policy results in returning an empty value reference if the attribute value cannot be extracted.
 */
struct fallback_to_none
{
    enum { guaranteed_result = false };

    /*!
     * The method is called in order to apply a function object to the default value.
     */
    template< typename FunT >
    static bool apply_default(FunT&)
    {
        return false;
    }

    /*!
     * The method is called in order to apply a function object to the default value.
     */
    template< typename FunT >
    static bool apply_default(FunT const&)
    {
        return false;
    }

    /*!
     * The method is called when value extraction failed because the attribute value has different type than requested.
     */
    static void on_invalid_type(typeindex::type_index const&)
    {
    }

    /*!
     * The method is called when value extraction failed because the attribute value was not found.
     */
    static void on_missing_value()
    {
    }
};

/*!
 * The \c fallback_to_throw policy results in throwing an exception if the attribute value cannot be extracted.
 */
struct fallback_to_throw
{
    enum { guaranteed_result = true };

    /*!
     * The method is called in order to apply a function object to the default value.
     */
    template< typename FunT >
    static bool apply_default(FunT&)
    {
        return false;
    }

    /*!
     * The method is called in order to apply a function object to the default value.
     */
    template< typename FunT >
    static bool apply_default(FunT const&)
    {
        return false;
    }

    /*!
     * The method is called when value extraction failed because the attribute value has different type than requested.
     */
    static void on_invalid_type(typeindex::type_index const& t)
    {
        BOOST_LOG_THROW_DESCR_PARAMS(invalid_type, "Attribute value has incompatible type", (t));
    }

    /*!
     * The method is called when value extraction failed because the attribute value was not found.
     */
    static void on_missing_value()
    {
        BOOST_LOG_THROW_DESCR(missing_value, "Attribute value not found");
    }
};

/*!
 * The \c fallback_to_default policy results in a default value if the attribute value cannot be extracted.
 */
template< typename DefaultT >
struct fallback_to_default
{
    enum { guaranteed_result = true };

    //! Default value type
    typedef typename remove_cv< typename remove_reference< DefaultT >::type >::type default_type;

    /*!
     * Default constructor.
     */
    fallback_to_default() : m_default()
    {
    }

    /*!
     * Initializing constructor.
     */
    explicit fallback_to_default(default_type const& def_val) : m_default(def_val)
    {
    }

    /*!
     * The method is called in order to apply a function object to the default value.
     */
    template< typename FunT >
    bool apply_default(FunT& fun) const
    {
        fun(m_default);
        return true;
    }

    /*!
     * The method is called in order to apply a function object to the default value.
     */
    template< typename FunT >
    bool apply_default(FunT const& fun) const
    {
        fun(m_default);
        return true;
    }

    /*!
     * The method is called when value extraction failed because the attribute value has different type than requested.
     */
    static void on_invalid_type(typeindex::type_index const&)
    {
    }

    /*!
     * The method is called when value extraction failed because the attribute value was not found.
     */
    static void on_missing_value()
    {
    }

private:
    //! Default value
    DefaultT m_default;
};

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_FALLBACK_POLICY_HPP_INCLUDED_
