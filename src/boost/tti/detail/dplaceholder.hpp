
//  (C) Copyright Edward Diener 2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_PLACEHOLDER_HPP)
#define BOOST_TTI_DETAIL_PLACEHOLDER_HPP

#include <boost/mpl/lambda.hpp>
#include <boost/mpl/not.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost
  {
  namespace tti
    {
    namespace detail
      {
      template <class BOOST_TTI_DETAIL_TP_T>
      struct is_placeholder_expression :
        boost::mpl::not_
          <
          boost::is_same
            <
            typename boost::mpl::lambda<BOOST_TTI_DETAIL_TP_T>::type,
            BOOST_TTI_DETAIL_TP_T
            >
          >
        {
        };
      }
    }
  }
  
#endif // BOOST_TTI_DETAIL_PLACEHOLDER_HPP
