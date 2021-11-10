//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_DYNAMIC_STEP_HPP
#define BOOST_GIL_DYNAMIC_STEP_HPP

#include <boost/gil/concepts/dynamic_step.hpp>

namespace boost { namespace gil {

/// Base template for types that model HasDynamicXStepTypeConcept.
template <typename IteratorOrLocatorOrView>
struct dynamic_x_step_type;

/// Base template for types that model HasDynamicYStepTypeConcept.
template <typename LocatorOrView>
struct dynamic_y_step_type;

/// Base template for types that model both, HasDynamicXStepTypeConcept and HasDynamicYStepTypeConcept.
///
/// \todo TODO: Is Locator allowed or practical to occur?
template <typename View>
struct dynamic_xy_step_type;

}}  // namespace boost::gil

#endif
