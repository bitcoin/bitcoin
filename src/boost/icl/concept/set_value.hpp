/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_CONCEPT_SET_VALUE_HPP_JOFA_100924
#define BOOST_ICL_CONCEPT_SET_VALUE_HPP_JOFA_100924

#include <boost/icl/type_traits/is_set.hpp>
#include <boost/icl/type_traits/codomain_type_of.hpp>

namespace boost{ namespace icl
{

//==============================================================================
//= AlgoUnifiers<Set>
//==============================================================================
template<class Type, class Iterator>
inline typename enable_if<is_set<Type>, const typename Type::key_type>::type&
key_value(Iterator it_)
{
    return *it_;
}

template<class Type>
inline typename enable_if<is_set<Type>, typename Type::value_type>::type
make_value(const typename Type::key_type& key_val, 
           const typename codomain_type_of<Type>::type& )
{
    return typename Type::value_type(key_val);
}


}} // namespace boost icl

#endif


