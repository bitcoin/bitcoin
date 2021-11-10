//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_GRAY_HPP
#define BOOST_GIL_GRAY_HPP

#include <boost/gil/utilities.hpp>
#include <boost/gil/detail/mp11.hpp>

namespace boost { namespace gil {

/// \ingroup ColorNameModel
/// \brief Gray
struct gray_color_t {};

/// \ingroup ColorSpaceModel
using gray_t = mp11::mp_list<gray_color_t>;

/// \ingroup LayoutModel
using gray_layout_t = layout<gray_t>;

}}  // namespace boost::gil

#endif

