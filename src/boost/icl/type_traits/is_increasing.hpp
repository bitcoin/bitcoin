/*-----------------------------------------------------------------------------+    
Copyright (c) 2011-2011: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_IS_INCREASING_HPP_JOFA_110416
#define BOOST_ICL_TYPE_TRAITS_IS_INCREASING_HPP_JOFA_110416

#include <functional>

namespace boost{ namespace icl
{

template <class DomainT, class Compare>
struct is_increasing
{
    typedef is_increasing type;
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template <class DomainT>
struct is_increasing<DomainT, std::greater<DomainT> >
{
    typedef is_increasing type;
    BOOST_STATIC_CONSTANT(bool, value = false);
};

}} // namespace boost icl

#endif


