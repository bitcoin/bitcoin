/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

BOOST_LOG_AUX_OVERLOAD(const&, const&)
BOOST_LOG_AUX_OVERLOAD(&, const&)
BOOST_LOG_AUX_OVERLOAD(const&, &)
BOOST_LOG_AUX_OVERLOAD(&, &)

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

BOOST_LOG_AUX_OVERLOAD(const&&, const&&)
BOOST_LOG_AUX_OVERLOAD(&&, const&&)
BOOST_LOG_AUX_OVERLOAD(const&&, &&)
BOOST_LOG_AUX_OVERLOAD(&&, &&)

BOOST_LOG_AUX_OVERLOAD(const&&, const&)
BOOST_LOG_AUX_OVERLOAD(&&, const&)
BOOST_LOG_AUX_OVERLOAD(const&&, &)
BOOST_LOG_AUX_OVERLOAD(&&, &)

BOOST_LOG_AUX_OVERLOAD(const&, const&&)
BOOST_LOG_AUX_OVERLOAD(&, const&&)
BOOST_LOG_AUX_OVERLOAD(const&, &&)
BOOST_LOG_AUX_OVERLOAD(&, &&)

#endif // !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
