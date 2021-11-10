
//  (C) Copyright Edward Diener 2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_DETAIL_METAFUNC_HPP)
#define BOOST_TTI_DETAIL_METAFUNC_HPP

#include <boost/mpl/has_xxx.hpp>
#include <boost/tti/gen/namespace_gen.hpp>

namespace boost
  {
  namespace tti
    {
    namespace detail
      {
      BOOST_MPL_HAS_XXX_TEMPLATE_NAMED_DEF(is_metafunction_class_apply, apply, false)
      template <class BOOST_TTI_DETAIL_TP_T>
      struct is_metafunction_class :
        BOOST_TTI_NAMESPACE::detail::is_metafunction_class_apply<BOOST_TTI_DETAIL_TP_T>
        {
        };
      }
    }
  }
  
#endif // BOOST_TTI_DETAIL_METAFUNC_HPP
