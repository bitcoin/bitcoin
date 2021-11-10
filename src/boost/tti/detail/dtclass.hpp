
//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_TCLASS_HPP)
#define BOOST_TTI_DETAIL_TCLASS_HPP

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/is_class.hpp>

namespace boost
  {
  namespace tti
    {
    namespace detail
      {
      template <class BOOST_TTI_DETAIL_TP_T>
      struct tclass :
        boost::mpl::eval_if
          <
          boost::is_class<BOOST_TTI_DETAIL_TP_T>,
          BOOST_TTI_DETAIL_TP_T,
          boost::mpl::identity<BOOST_TTI_DETAIL_TP_T>
          >
        {
        };
      }
    }
  }
  
#endif // BOOST_TTI_DETAIL_TCLASS_HPP
