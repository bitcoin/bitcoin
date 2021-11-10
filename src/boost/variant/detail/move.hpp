//-----------------------------------------------------------------------------
// boost variant/detail/move.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2003 Eric Friedman
//  Copyright (c) 2002 by Andrei Alexandrescu
//  Copyright (c) 2013-2021 Antony Polukhin
//
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  This file derivative of MoJO. Much thanks to Andrei for his initial work.
//  See <http://www.cuj.com/experts/2102/alexandr.htm> for information on MOJO.
//  Re-issued here under the Boost Software License, with permission of the original
//  author (Andrei Alexandrescu).


#ifndef BOOST_VARIANT_DETAIL_MOVE_HPP
#define BOOST_VARIANT_DETAIL_MOVE_HPP

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/move/utility_core.hpp> // for boost::move
#include <boost/move/adl_move_swap.hpp>

namespace boost { namespace detail { namespace variant {

using boost::move;

//////////////////////////////////////////////////////////////////////////
// function template move_swap
//
// Swaps using Koenig lookup but falls back to move-swap for primitive
// types and on non-conforming compilers.
//

template <typename T>
inline void move_swap(T& lhs, T& rhs)
{
    ::boost::adl_move_swap(lhs, rhs);
}

}}} // namespace boost::detail::variant

#endif // BOOST_VARIANT_DETAIL_MOVE_HPP



