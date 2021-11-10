/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_HAS_INVERSE_HPP_JOFA_090205
#define BOOST_ICL_TYPE_TRAITS_HAS_INVERSE_HPP_JOFA_090205

#include <boost/type_traits/is_signed.hpp>
#include <boost/type_traits/is_floating_point.hpp>

namespace boost{ namespace icl
{

    template <class Type> struct has_inverse
    { 
        typedef has_inverse<Type> type;
        BOOST_STATIC_CONSTANT(bool, 
            value = (   boost::is_signed<Type>::value 
                     || is_floating_point<Type>::value ) ); 
    };

}} // namespace boost icl

#endif // BOOST_ICL_TYPE_TRAITS_HAS_INVERSE_HPP_JOFA_090205


