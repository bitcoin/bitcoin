// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_IS_DIMENSION_LIST_HPP
#define BOOST_UNITS_IS_DIMENSION_LIST_HPP

///
/// \file
/// \brief Check that a type is a valid dimension list.
///

#include <boost/mpl/bool.hpp>

#include <boost/units/units_fwd.hpp>

namespace boost {

namespace units {

/// Check that a type is a valid dimension list.
template<typename Seq>
struct is_dimension_list :
    public mpl::false_
{ };

template<typename Item, typename Next>
struct is_dimension_list<list<Item, Next> > :
    public mpl::true_
{ };

template<>
struct is_dimension_list<dimensionless_type> :
    public mpl::true_
{ };

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_IS_DIMENSION_LIST_HPP
