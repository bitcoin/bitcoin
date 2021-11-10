
//  (C) Copyright Edward Diener 2019
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_MACRO_FVE_HPP)
#define BOOST_TTI_DETAIL_MACRO_FVE_HPP

#include <boost/preprocessor/variadic/elem.hpp>

#define BOOST_TTI_DETAIL_FIRST_VARIADIC_ELEM(...) \
    BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__) \
/**/

#endif // BOOST_TTI_DETAIL_MACRO_FVE_HPP
