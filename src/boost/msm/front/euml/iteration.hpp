// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_EUML_ITERATION_H
#define BOOST_MSM_FRONT_EUML_ITERATION_H

#include <algorithm>
#include <numeric>
#include <boost/msm/front/euml/common.hpp>

namespace boost { namespace msm { namespace front { namespace euml
{

BOOST_MSM_EUML_FUNCTION(ForEach_ , std::for_each , for_each_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(Accumulate_ , std::accumulate , accumulate_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )

}}}}

#endif //BOOST_MSM_FRONT_EUML_ITERATION_H
