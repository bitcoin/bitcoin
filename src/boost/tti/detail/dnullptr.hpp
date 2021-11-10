
//  (C) Copyright Edward Diener 2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_NULLPTR_HPP)
#define BOOST_TTI_DETAIL_NULLPTR_HPP

#include <boost/config.hpp>

#if defined(BOOST_NO_CXX11_NULLPTR)

#define BOOST_TTI_DETAIL_NULLPTR 0

#else // !BOOST_NO_CXX11_NULLPTR

#define BOOST_TTI_DETAIL_NULLPTR nullptr

#endif // BOOST_NO_CXX11_NULLPTR

#endif // BOOST_TTI_DETAIL_NULLPTR_HPP
