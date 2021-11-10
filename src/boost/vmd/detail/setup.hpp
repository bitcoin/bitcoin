
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_SETUP_HPP)
#define BOOST_VMD_DETAIL_SETUP_HPP

#include <boost/preprocessor/config/config.hpp>

#if defined(BOOST_VMD_MSVC)
#undef BOOST_VMD_MSVC
#endif
#if defined(BOOST_VMD_MSVC_V8)
#undef BOOST_VMD_MSVC_V8
#endif
#if BOOST_PP_VARIADICS
#define BOOST_VMD_MSVC BOOST_PP_VARIADICS_MSVC
#if BOOST_VMD_MSVC && defined(_MSC_VER) && _MSC_VER == 1400
#define BOOST_VMD_MSVC_V8 1
#else
#define BOOST_VMD_MSVC_V8 0
#endif /* BOOST_VMD_MSVC  && defined(_MSC_VER) && _MSC_VER == 1400 */
#if !defined(BOOST_VMD_ASSERT_DATA)
#if defined(NDEBUG)
#define BOOST_VMD_ASSERT_DATA 0
#else
#define BOOST_VMD_ASSERT_DATA 1
#endif /* NDEBUG */
#endif /* BOOST_VMD_ASSERT_DATA */
#else
#define BOOST_VMD_MSVC 0
#define BOOST_VMD_MSVC_V8 0
#if defined(BOOST_VMD_ASSERT_DATA)
#undef BOOST_VMD_ASSERT_DATA
#endif
#define BOOST_VMD_ASSERT_DATA 0
#endif /* BOOST_PP_VARIADICS */

#endif /* BOOST_VMD_DETAIL_SETUP_HPP */
