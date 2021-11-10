
//  (C) Copyright Edward Diener 2019
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_STATIC_FUNCTION_TYPE_HPP)
#define BOOST_TTI_DETAIL_STATIC_FUNCTION_TYPE_HPP

#include <boost/function_types/is_function.hpp>
#include <boost/function_types/property_tags.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost
  {
  namespace tti
    {
    namespace detail
      {
      template
        <
        class BOOST_TTI_DETAIL_TP_R,
        class BOOST_TTI_DETAIL_TP_FS,
        class BOOST_TTI_DETAIL_TP_TAG
        >
      struct static_function_type :
        boost::mpl::and_
          <
          boost::is_same<BOOST_TTI_DETAIL_TP_TAG,boost::function_types::null_tag>,
          boost::is_same<BOOST_TTI_DETAIL_TP_FS,boost::mpl::vector<> >,
          boost::function_types::is_function<BOOST_TTI_DETAIL_TP_R>
          >
        {
        };
      }
    }
  }
  
#endif // BOOST_TTI_DETAIL_STATIC_FUNCTION_TYPE_HPP
