/*!
@file
Defines generally useful preprocessor macros.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_PREPROCESSOR_HPP
#define BOOST_HANA_DETAIL_PREPROCESSOR_HPP

//! @ingroup group-details
//! Expands to the concatenation of its two arguments.
#define BOOST_HANA_PP_CONCAT(x, y) BOOST_HANA_PP_CONCAT_PRIMITIVE(x, y)
#define BOOST_HANA_PP_CONCAT_PRIMITIVE(x, y) x ## y

//! @ingroup group-details
//! Expands to the stringized version of its argument.
#define BOOST_HANA_PP_STRINGIZE(...) BOOST_HANA_PP_STRINGIZE_PRIMITIVE(__VA_ARGS__)
#define BOOST_HANA_PP_STRINGIZE_PRIMITIVE(...) #__VA_ARGS__

//! @ingroup group-details
//! Expands to its first argument.
#ifdef BOOST_HANA_WORKAROUND_MSVC_PREPROCESSOR_616033
#define BOOST_HANA_PP_FRONT(...) BOOST_HANA_PP_FRONT_IMPL_I(__VA_ARGS__)
#define BOOST_HANA_PP_FRONT_IMPL_I(...) BOOST_HANA_PP_CONCAT(BOOST_HANA_PP_FRONT_IMPL(__VA_ARGS__, ),)
#else
#define BOOST_HANA_PP_FRONT(...) BOOST_HANA_PP_FRONT_IMPL(__VA_ARGS__, )
#endif
#define BOOST_HANA_PP_FRONT_IMPL(e0, ...) e0

//! @ingroup group-details
//! Expands to all of its arguments, except for the first one.
//!
//! This macro may not be called with less than 2 arguments.
#define BOOST_HANA_PP_DROP_FRONT(e0, ...) __VA_ARGS__

#endif // !BOOST_HANA_DETAIL_PREPROCESSOR_HPP
