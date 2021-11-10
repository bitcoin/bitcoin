/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_IS_INTERVAL_HPP_JOFA_100327
#define BOOST_ICL_TYPE_TRAITS_IS_INTERVAL_HPP_JOFA_100327

#include <boost/mpl/or.hpp>
#include <boost/icl/interval_bounds.hpp>
#include <boost/icl/interval_traits.hpp>

namespace boost{ namespace icl
{

template <class Type>
struct interval_bound_type
{
    typedef interval_bound_type type;
    BOOST_STATIC_CONSTANT(bound_type, value = (interval_bounds::undefined)); 
};

template <class Type> struct is_interval
{ 
    typedef is_interval<Type> type;
    BOOST_STATIC_CONSTANT(bool, 
        value = ((interval_bound_type<Type>::value) < interval_bounds::undefined)); 
};


template <class Type> struct has_static_bounds
{ 
    typedef has_static_bounds<Type> type;
    BOOST_STATIC_CONSTANT(bool, 
        value = ((interval_bound_type<Type>::value) < interval_bounds::dynamic)); 
};

template <class Type> struct has_dynamic_bounds
{ 
    typedef has_dynamic_bounds<Type> type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (interval_bound_type<Type>::value == interval_bounds::dynamic));
};

template <class Type> struct has_asymmetric_bounds
{ 
    typedef has_asymmetric_bounds<Type> type;
    BOOST_STATIC_CONSTANT(bound_type, bounds = (interval_bound_type<Type>::value)); 
    BOOST_STATIC_CONSTANT(bool, 
        value = (   bounds == interval_bounds::static_left_open 
                 || bounds == interval_bounds::static_right_open)); 
};

template <class Type> struct has_symmetric_bounds
{ 
    typedef has_symmetric_bounds<Type> type;
    BOOST_STATIC_CONSTANT(bound_type, bounds = (interval_bound_type<Type>::value)); 
    BOOST_STATIC_CONSTANT(bool, 
        value = (   bounds == interval_bounds::static_closed 
                 || bounds == interval_bounds::static_open)); 
};

//------------------------------------------------------------------------------
template <class Type> struct is_discrete_static
{ 
    typedef is_discrete_static type;
    typedef typename interval_traits<Type>::domain_type domain_type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (mpl::and_< has_static_bounds<Type>
                          , is_discrete<domain_type> >::value) ); 
};

//------------------------------------------------------------------------------
template <class Type> struct is_continuous_static
{ 
    typedef is_continuous_static type;
    typedef typename interval_traits<Type>::domain_type domain_type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (mpl::and_< has_static_bounds<Type>
                          , is_continuous<domain_type>
                          , has_asymmetric_bounds<Type> >::value) ); 
};

//------------------------------------------------------------------------------
template <class Type> struct is_static_right_open
{ 
    typedef is_static_right_open<Type> type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (interval_bound_type<Type>::value == interval_bounds::static_right_open)); 
};

template <class Type> struct is_static_left_open
{ 
    typedef is_static_left_open<Type> type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (interval_bound_type<Type>::value == interval_bounds::static_left_open)); 
};

template <class Type> struct is_static_open
{ 
    typedef is_static_open<Type> type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (interval_bound_type<Type>::value == interval_bounds::static_open)); 
};

template <class Type> struct is_static_closed
{ 
    typedef is_static_closed<Type> type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (interval_bound_type<Type>::value == interval_bounds::static_closed)); 
};

template <class Type> struct is_discrete_static_closed
{ 
    typedef is_static_closed<Type> type;
    typedef typename interval_traits<Type>::domain_type domain_type;
    
    BOOST_STATIC_CONSTANT( bool, 
        value = (mpl::and_< is_static_closed<Type>
                          , is_discrete<domain_type> >::value) ); 
};

template <class Type> struct is_discrete_static_open
{ 
    typedef is_static_closed<Type> type;
    typedef typename interval_traits<Type>::domain_type domain_type;
    
    BOOST_STATIC_CONSTANT( bool, 
        value = (mpl::and_< is_static_open<Type>
                          , is_discrete<domain_type> >::value) ); 
};

//------------------------------------------------------------------------------
template <class Type> struct is_continuous_right_open
{ 
    typedef is_continuous_right_open<Type> type;
    typedef typename interval_traits<Type>::domain_type domain_type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (mpl::and_<is_static_right_open<Type>, is_continuous<domain_type> >::value)); 
};

template <class Type> struct is_continuous_left_open
{ 
    typedef is_continuous_left_open<Type> type;
    typedef typename interval_traits<Type>::domain_type domain_type;
    BOOST_STATIC_CONSTANT(bool, 
        value = (mpl::and_<is_static_left_open<Type>, is_continuous<domain_type> >::value)); 
};

//------------------------------------------------------------------------------

template <class Type> struct is_singelizable
{
    typedef is_singelizable type;
    typedef typename interval_traits<Type>::domain_type domain_type;

    BOOST_STATIC_CONSTANT(bool, 
        value =
            (mpl::or_< has_dynamic_bounds<Type>
                     , is_discrete<domain_type> 
                     >::value)
    );
};

}} // namespace boost icl

#endif


