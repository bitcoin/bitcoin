/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_DYNAMIC_INTERVAL_TRAITS_HPP_JOFA_100926
#define BOOST_ICL_DYNAMIC_INTERVAL_TRAITS_HPP_JOFA_100926

namespace boost{ namespace icl
{

class interval_bounds;
template<class DomainT> class bounded_value;


//------------------------------------------------------------------------------
//- Adapter class
//------------------------------------------------------------------------------
template<class Type>
struct dynamic_interval_traits
{
    typedef typename Type::domain_type    domain_type;
    typedef typename Type::domain_compare domain_compare;

    static Type construct(const domain_type& lo, const domain_type& up, interval_bounds bounds);
    static Type construct_bounded(const bounded_value<domain_type>& lo, 
                                  const bounded_value<domain_type>& up);
};


}} // namespace boost icl

#endif


