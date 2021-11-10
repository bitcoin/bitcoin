// Copyright Daniel Wallin 2006.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_FORWARD_MATCH_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_IMPL_FUNCTION_FORWARD_MATCH_HPP

#include <boost/parameter/config.hpp>

#if !defined(BOOST_NO_SFINAE) && \
    !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x592))

#include <boost/parameter/aux_/pp_impl/match.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>

// Expands to an extra argument that is well-formed
// iff all Args... satisfy the requirements set by params.
#define BOOST_PARAMETER_FUNCTION_FORWARD_MATCH(params, n, prefix)            \
  , typename ::boost::parameter::aux::match<                                 \
        params BOOST_PP_ENUM_TRAILING_PARAMS(n, prefix)                      \
    >::type = params()
/**/

#define BOOST_PARAMETER_FUNCTION_FORWARD_MATCH_Z(z, params, n, prefix)       \
  , typename ::boost::parameter::aux::match<                                 \
        params BOOST_PP_ENUM_TRAILING_PARAMS_Z(z, n, prefix)                 \
    >::type = params()
/**/

#else   // SFINAE/Borland workarounds needed.

#define BOOST_PARAMETER_FUNCTION_FORWARD_MATCH(params, n, prefix)            \
  , params = params()
/**/

#define BOOST_PARAMETER_FUNCTION_FORWARD_MATCH_Z(z, params, n, prefix)       \
  , params = params()
/**/

#endif
#endif  // include guard

