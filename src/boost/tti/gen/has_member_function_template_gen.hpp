
//  (C) Copyright Edward Diener 2019
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_MEMBER_FUNCTION_TEMPLATE_GEN_HPP)
#define BOOST_TTI_MEMBER_FUNCTION_TEMPLATE_GEN_HPP

#include <boost/preprocessor/cat.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/// Generates the macro metafunction name for BOOST_TTI_HAS_MEMBER_FUNCTION_TEMPLATE.
/**
    name  = the name of the member function template.

    returns = the generated macro metafunction name.
*/
#define BOOST_TTI_HAS_MEMBER_FUNCTION_TEMPLATE_GEN(name) \
  BOOST_PP_CAT(has_member_function_template_,name) \
/**/

#endif // BOOST_TTI_MEMBER_FUNCTION_TEMPLATE_GEN_HPP
