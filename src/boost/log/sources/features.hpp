/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   sources/features.hpp
 * \author Andrey Semashev
 * \date   17.07.2009
 *
 * The header contains definition of a features list class template.
 */

#ifndef BOOST_LOG_SOURCES_FEATURES_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_FEATURES_HPP_INCLUDED_

#include <boost/mpl/lambda.hpp>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>

//! The macro defines the maximum number of features that can be specified for a logger
#ifndef BOOST_LOG_FEATURES_LIMIT
#define BOOST_LOG_FEATURES_LIMIT 10
#endif // BOOST_LOG_FEATURES_LIMIT

#endif

#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sources {

#if defined(BOOST_LOG_DOXYGEN_PASS) || !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

/*!
 * \brief A type sequence of logger features
 *
 * This class template can be used to specify logger features in a \c basic_composite_logger instantiation.
 */
template< typename... FeaturesT >
struct features
{
};

namespace aux {

//! The metafunction produces the inherited features hierarchy with \c RootT as the ultimate base type
template< typename RootT, typename FeaturesT >
struct inherit_features;

template< typename RootT, typename FeatureT0, typename... FeaturesT >
struct inherit_features< RootT, features< FeatureT0, FeaturesT... > >
{
    typedef typename mpl::lambda<
        FeatureT0
    >::type::BOOST_NESTED_TEMPLATE apply<
        typename inherit_features<
            RootT,
            features< FeaturesT... >
        >::type
    >::type type;
};

template< typename RootT, typename FeatureT0 >
struct inherit_features< RootT, features< FeatureT0 > >
{
    typedef typename mpl::lambda<
        FeatureT0
    >::type::BOOST_NESTED_TEMPLATE apply<
        RootT
    >::type type;
};

template< typename RootT >
struct inherit_features< RootT, features< > >
{
    typedef RootT type;
};

} // namespace aux

#else

//! A type sequence of logger features
template< BOOST_PP_ENUM_BINARY_PARAMS(BOOST_LOG_FEATURES_LIMIT, typename FeatureT, = void BOOST_PP_INTERCEPT) >
struct features
{
};

namespace aux {

template< typename RootT, typename FeaturesT >
struct inherit_features;

template< typename RootT, BOOST_PP_ENUM_PARAMS(BOOST_LOG_FEATURES_LIMIT, typename FeatureT) >
struct inherit_features< RootT, features< BOOST_PP_ENUM_PARAMS(BOOST_LOG_FEATURES_LIMIT, FeatureT) > >
{
    typedef typename mpl::lambda<
        FeatureT0
    >::type::BOOST_NESTED_TEMPLATE apply<
        typename inherit_features<
            RootT,
            features< BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_LOG_FEATURES_LIMIT, FeatureT) >
        >::type
    >::type type;
};

template< typename RootT, typename FeatureT0 >
struct inherit_features< RootT, features< FeatureT0, BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_LOG_FEATURES_LIMIT, void BOOST_PP_INTERCEPT) > >
{
    typedef typename mpl::lambda<
        FeatureT0
    >::type::BOOST_NESTED_TEMPLATE apply<
        RootT
    >::type type;
};

template< typename RootT >
struct inherit_features< RootT, features< BOOST_PP_ENUM_PARAMS(BOOST_LOG_FEATURES_LIMIT, void BOOST_PP_INTERCEPT) > >
{
    typedef RootT type;
};

} // namespace aux

#endif

} // namespace sources

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SOURCES_FEATURES_HPP_INCLUDED_
