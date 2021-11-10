///////////////////////////////////////////////////////////////////////////////
// weight.hpp
//
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_VARIATES_COVARIATE_HPP_EAN_03_11_2005
#define BOOST_ACCUMULATORS_STATISTICS_VARIATES_COVARIATE_HPP_EAN_03_11_2005

#include <boost/parameter/keyword.hpp>
#include <boost/accumulators/accumulators_fwd.hpp>

namespace boost { namespace accumulators
{

BOOST_PARAMETER_KEYWORD(tag, covariate1)
BOOST_PARAMETER_KEYWORD(tag, covariate2)

BOOST_ACCUMULATORS_IGNORE_GLOBAL(covariate1)
BOOST_ACCUMULATORS_IGNORE_GLOBAL(covariate2)

}} // namespace boost::accumulators

#endif
