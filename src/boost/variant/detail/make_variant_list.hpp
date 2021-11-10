//-----------------------------------------------------------------------------
// boost variant/detail/make_variant_list.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2002-2003 Eric Friedman, Itay Maman
// Copyright (c) 2013-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_DETAIL_MAKE_VARIANT_LIST_HPP
#define BOOST_VARIANT_DETAIL_MAKE_VARIANT_LIST_HPP

#include <boost/variant/variant_fwd.hpp>

#include <boost/mpl/list.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/enum.hpp>

namespace boost {
namespace detail { namespace variant {

///////////////////////////////////////////////////////////////////////////////
// (detail) metafunction make_variant_list
//
// Provides a MPL-compatible sequence with the specified non-void types
// as arguments.
//
// Rationale: see class template convert_void (variant_fwd.hpp) and using-
// declaration workaround (below).
//

#if !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)

template < typename... T >
struct make_variant_list
{
    typedef typename mpl::list< T... >::type type;
};

#else // defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)

template < BOOST_VARIANT_ENUM_PARAMS(typename T) >
struct make_variant_list
{
public: // metafunction result

    // [Define a macro to convert any void(NN) tags to mpl::void...]
#   define BOOST_VARIANT_AUX_CONVERT_VOID(z, N,_)  \
        typename convert_void< BOOST_PP_CAT(T,N) >::type

    // [...so that the specified types can be passed to mpl::list...]
    typedef typename mpl::list< 
          BOOST_PP_ENUM(
              BOOST_VARIANT_LIMIT_TYPES
            , BOOST_VARIANT_AUX_CONVERT_VOID
            , _
            )
        >::type type;

    // [...and, finally, the conversion macro can be undefined:]
#   undef BOOST_VARIANT_AUX_CONVERT_VOID

};

#endif // BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES workaround

}} // namespace detail::variant
} // namespace boost

#endif // BOOST_VARIANT_DETAIL_MAKE_VARIANT_LIST_HPP
