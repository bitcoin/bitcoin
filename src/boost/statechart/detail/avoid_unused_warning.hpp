#ifndef BOOST_STATECHART_DETAIL_AVOID_UNUSED_WARNING_HPP_INCLUDED
#define BOOST_STATECHART_DETAIL_AVOID_UNUSED_WARNING_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



namespace boost
{
namespace statechart
{
namespace detail
{



template< typename T >
inline void avoid_unused_warning( const T & ) {}



} // namespace detail
} // namespace statechart
} // namespace boost



#endif
