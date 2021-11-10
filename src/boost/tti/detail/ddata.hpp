
//  (C) Copyright Edward Diener 2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_DATA_HPP)
#define BOOST_TTI_DETAIL_DATA_HPP

#include <boost/mpl/or.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/tti/detail/dmem_data.hpp>
#include <boost/tti/detail/dstatic_mem_data.hpp>

#define BOOST_TTI_DETAIL_TRAIT_HAS_DATA(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_DATA(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_STATIC_MEMBER_DATA(trait,name) \
  template<class BOOST_TTI_DETAIL_TP_ET,class BOOST_TTI_DETAIL_TP_DT> \
  struct BOOST_PP_CAT(trait,_detail_hd) : \
    boost::mpl::or_ \
        < \
        BOOST_PP_CAT(trait,_detail_hmd_with_enclosing_class)<BOOST_TTI_DETAIL_TP_ET,BOOST_TTI_DETAIL_TP_DT>, \
        BOOST_PP_CAT(trait,_detail_hsd)<BOOST_TTI_DETAIL_TP_ET,BOOST_TTI_DETAIL_TP_DT> \
        > \
    { \
    }; \
/**/

#endif // BOOST_TTI_DETAIL_DATA_HPP
