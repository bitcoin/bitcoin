/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_IS_MAP_HPP_JOFA_081107
#define BOOST_ICL_TYPE_TRAITS_IS_MAP_HPP_JOFA_081107

#include <boost/config.hpp>

namespace boost{ namespace icl
{
    template <class Type> struct is_map
    {
        typedef is_map<Type> type;
        BOOST_STATIC_CONSTANT(bool, value = false); 
    };

}} // namespace boost icl

#endif


