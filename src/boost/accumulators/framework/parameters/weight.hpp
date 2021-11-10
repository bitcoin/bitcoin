///////////////////////////////////////////////////////////////////////////////
// weight.hpp
//
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_FRAMEWORK_PARAMETERS_WEIGHT_HPP_EAN_31_10_2005
#define BOOST_ACCUMULATORS_FRAMEWORK_PARAMETERS_WEIGHT_HPP_EAN_31_10_2005

#include <boost/parameter/name.hpp>
#include <boost/accumulators/accumulators_fwd.hpp>

namespace boost { namespace accumulators
{

// The weight of a single sample
BOOST_PARAMETER_NAME((weight, tag) weight)
BOOST_ACCUMULATORS_IGNORE_GLOBAL(weight)

}} // namespace boost::accumulators

#endif
