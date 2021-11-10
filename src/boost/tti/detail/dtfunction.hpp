
//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_TFUNCTION_HPP)
#define BOOST_TTI_DETAIL_TFUNCTION_HPP

#include <boost/mpl/push_front.hpp>
#include <boost/function_types/function_type.hpp>

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
      struct tfunction_seq
        {
        typedef typename boost::mpl::push_front<BOOST_TTI_DETAIL_TP_FS,BOOST_TTI_DETAIL_TP_R>::type ftseq;
        typedef typename boost::function_types::function_type<ftseq,BOOST_TTI_DETAIL_TP_TAG>::type type;
        };
      }
    }
  }
  
#endif // BOOST_TTI_DETAIL_TFUNCTION_HPP
