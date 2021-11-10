/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_UNIT_ELEMENT_HPP_JOFA_080912
#define BOOST_ICL_TYPE_TRAITS_UNIT_ELEMENT_HPP_JOFA_080912

#include <string>
#include <boost/icl/type_traits/identity_element.hpp>
#include <boost/icl/type_traits/succ_pred.hpp>

namespace boost{ namespace icl
{
    template <class Type> struct unit_element{ static Type value(); };

    template<> inline bool   unit_element<bool>::value()   { return true;}
    template<> inline float  unit_element<float>::value()  { return 1.0; }
    template<> inline double unit_element<double>::value() { return 1.0; }
    template<> inline long double unit_element<long double>::value() 
                                                           { return 1.0; }
    
    // Smallest 'visible' string that is greater than the empty string.
    template <>    
    inline std::string unit_element<std::string>::value()
    { return std::string(" "); }

    template <class Type> 
    inline Type unit_element<Type>::value()
    { return icl::succ(identity_element<Type>::value()); }

}} // namespace boost icl

#endif


