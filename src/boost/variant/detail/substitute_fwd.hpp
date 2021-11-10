//-----------------------------------------------------------------------------
// boost variant/detail/substitute_fwd.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003
// Eric Friedman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_DETAIL_SUBSTITUTE_FWD_HPP
#define BOOST_VARIANT_DETAIL_SUBSTITUTE_FWD_HPP

#include <boost/mpl/aux_/lambda_arity_param.hpp>
#include <boost/mpl/aux_/template_arity.hpp>
#include <boost/mpl/int_fwd.hpp>


///////////////////////////////////////////////////////////////////////////////
// BOOST_VARIANT_DETAIL_NO_SUBSTITUTE
//
// Defined if 'substitute' is not implementable on the current compiler.
//

#include <boost/mpl/aux_/config/ctps.hpp>
#include <boost/mpl/aux_/config/ttp.hpp>

#if defined(BOOST_NO_TEMPLATE_TEMPLATE_PARAMETERS) \
 && !defined(BOOST_VARIANT_DETAIL_NO_SUBSTITUTE)
#   define BOOST_VARIANT_DETAIL_NO_SUBSTITUTE
#endif

namespace boost {
namespace detail { namespace variant {

#if !defined(BOOST_VARIANT_DETAIL_NO_SUBSTITUTE)

///////////////////////////////////////////////////////////////////////////////
// metafunction substitute
//
// Substitutes one type for another in the given type expression.
//
template <
      typename T, typename Dest, typename Source
      BOOST_MPL_AUX_LAMBDA_ARITY_PARAM(
          typename Arity = mpl::int_< mpl::aux::template_arity<T>::value >
        )
    >
struct substitute;

#endif // !defined(BOOST_VARIANT_DETAIL_NO_SUBSTITUTE)

}} // namespace detail::variant
} // namespace boost

#endif // BOOST_VARIANT_DETAIL_SUBSTITUTE_FWD_HPP
