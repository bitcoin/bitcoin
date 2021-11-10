/* Copyright 2003-2020 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include <boost/config.hpp>

#if !defined(BOOST_MULTI_INDEX_DETAIL_UNDEF_IF_CONSTEXPR_MACRO)

#if !defined(BOOST_NO_CXX17_IF_CONSTEXPR)
#define BOOST_MULTI_INDEX_IF_CONSTEXPR if constexpr
#else
#define BOOST_MULTI_INDEX_IF_CONSTEXPR if
#if defined(BOOST_MSVC)
#define BOOST_MULTI_INDEX_DETAIL_C4127_DISABLED
#pragma warning(push)
#pragma warning(disable:4127) /* conditional expression is constant */
#endif
#endif

#else

#undef BOOST_MULTI_INDEX_IF_CONSTEXPR 
#if defined(BOOST_MULTI_INDEX_DETAIL_C4127_DISABLED)
#pragma warning(pop)
#undef BOOST_MULTI_INDEX_DETAIL_C4127_DISABLED
#endif

#endif
