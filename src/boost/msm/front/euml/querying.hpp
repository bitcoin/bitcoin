// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_EUML_QUERYING_H
#define BOOST_MSM_FRONT_EUML_QUERYING_H

#include <algorithm>
#include <boost/msm/front/euml/common.hpp>

namespace boost { namespace msm { namespace front { namespace euml
{

BOOST_MSM_EUML_FUNCTION(Find_ , std::find , find_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(FindIf_ , std::find_if , find_if_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(LowerBound_ , std::lower_bound , lower_bound_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(UpperBound_ , std::upper_bound , upper_bound_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(BinarySearch_ , std::binary_search , binary_search_ , bool , bool )
BOOST_MSM_EUML_FUNCTION(MinElement_ , std::min_element , min_element_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(MaxElement_ , std::max_element , max_element_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(AdjacentFind_ , std::adjacent_find , adjacent_find_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(FindEnd_ , std::find_end , find_end_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(FindFirstOf_ , std::find_first_of , find_first_of_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(Equal_ , std::equal , equal_ , bool , bool )
BOOST_MSM_EUML_FUNCTION(Search_ , std::search , search_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(Includes_ , std::includes , includes_ , bool , bool )
BOOST_MSM_EUML_FUNCTION(LexicographicalCompare_ , std::lexicographical_compare , lexicographical_compare_ , bool , bool )
BOOST_MSM_EUML_FUNCTION(Count_ , std::count , count_ , RESULT_TYPE_DIFF_TYPE_ITER_TRAITS_PARAM1 , RESULT_TYPE2_DIFF_TYPE_ITER_TRAITS_PARAM1 )
BOOST_MSM_EUML_FUNCTION(CountIf_ , std::count_if , count_if_ , RESULT_TYPE_DIFF_TYPE_ITER_TRAITS_PARAM1 , RESULT_TYPE2_DIFF_TYPE_ITER_TRAITS_PARAM1 )
BOOST_MSM_EUML_FUNCTION(Distance_ , std::distance , distance_ , RESULT_TYPE_DIFF_TYPE_ITER_TRAITS_PARAM1 , RESULT_TYPE2_DIFF_TYPE_ITER_TRAITS_PARAM1 )
BOOST_MSM_EUML_FUNCTION(EqualRange_ , std::equal_range , equal_range_ , RESULT_TYPE_PAIR_REMOVE_REF_PARAM1 , RESULT_TYPE2_PAIR_REMOVE_REF_PARAM1 )
BOOST_MSM_EUML_FUNCTION(Mismatch_ , std::mismatch , mismatch_ , RESULT_TYPE_PAIR_REMOVE_REF_PARAM1 , RESULT_TYPE2_PAIR_REMOVE_REF_PARAM1 )


}}}}

#endif //BOOST_MSM_FRONT_EUML_QUERYING_H
