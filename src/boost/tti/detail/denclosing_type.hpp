
//  (C) Copyright Edward Diener 2019
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_ENCLOSING_TYPE_HPP)
#define BOOST_TTI_DETAIL_ENCLOSING_TYPE_HPP

#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_union.hpp>

namespace boost
  {
  namespace tti
    {
    namespace detail
      {
      template
        <
        class BOOST_TTI_DETAIL_TP_T
        >
      struct enclosing_type :
        boost::mpl::or_
          <
          boost::is_class<BOOST_TTI_DETAIL_TP_T>,
          boost::is_union<BOOST_TTI_DETAIL_TP_T>
          >
        {
        };
      }
    }
  }
  
#endif // BOOST_TTI_DETAIL_ENCLOSING_TYPE_HPP
