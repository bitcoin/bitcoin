/*-----------------------------------------------------------------------------+
Author: Joachim Faulhaber
Copyright (c) 2009-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------+
Template parameters of major itl class templates can be designed as
template template parameters or
template type parameter
by setting defines in this file.
+-----------------------------------------------------------------------------*/
#ifndef  BOOST_ICL_DESIGN_CONFIG_HPP_JOFA_090214
#define  BOOST_ICL_DESIGN_CONFIG_HPP_JOFA_090214

// If this macro is defined, right_open_interval with static interval borders
// will be used as default for all interval containers. 
// BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS should be defined in the application
// before other includes from the ITL
//#define BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS
// If BOOST_ICL_USE_STATIC_BOUNDED_INTERVALS is NOT defined, ITL uses intervals
// with dynamic borders as default.


//------------------------------------------------------------------------------
// Auxiliary macros for denoting template signatures.
// Purpose:
// (1) Shorten the lenthy and redundant template signatures.
// (2) Name anonymous template types according to their meaning ...
// (3) Making easier to refactor by redefinitin of the macros
// (4) Being able to check template template parameter variants against
//     template type parameter variants.

#define ICL_USE_COMPARE_TEMPLATE_TEMPLATE
#define ICL_USE_COMBINE_TEMPLATE_TEMPLATE
#define ICL_USE_SECTION_TEMPLATE_TEMPLATE
//      ICL_USE_INTERVAL_TEMPLATE_TYPE

//------------------------------------------------------------------------------
// template parameter Compare can not be a template type parameter as long as
// Compare<Interval<DomainT,Compare> >() is called in std::lexicographical_compare
// implementing operator< for interval_base_{set,map}. see NOTE DESIGN TTP
#ifdef ICL_USE_COMPARE_TEMPLATE_TEMPLATE
#   define ICL_COMPARE template<class>class
#   define ICL_COMPARE_DOMAIN(itl_compare, domain_type) itl_compare<domain_type> 
#   define ICL_COMPARE_INSTANCE(compare_instance, domain_type) compare_instance
#   define ICL_EXCLUSIVE_LESS(interval_type) exclusive_less_than
#else//ICL_USE_COMPARE_TEMPLATE_TYPE
#   define ICL_COMPARE class
#   define ICL_COMPARE_DOMAIN(itl_compare, domain_type) itl_compare 
#   define ICL_COMPARE_INSTANCE(compare_instance, domain_type) compare_instance<domain_type> 
#   define ICL_EXCLUSIVE_LESS(interval_type) exclusive_less_than<interval_type>
#endif

//------------------------------------------------------------------------------
// template parameter Combine could be a template type parameter.
#ifdef ICL_USE_COMBINE_TEMPLATE_TEMPLATE
#   define ICL_COMBINE template<class>class
#   define ICL_COMBINE_CODOMAIN(itl_combine, codomain_type) itl_combine<codomain_type> 
#   define ICL_COMBINE_INSTANCE(combine_instance,codomain_type) combine_instance
#else//ICL_USE_COMBINE_TEMPLATE_TYPE
#   define ICL_COMBINE class
#   define ICL_COMBINE_CODOMAIN(itl_combine, codomain_type) itl_combine 
#   define ICL_COMBINE_INSTANCE(combine_instance,codomain_type) combine_instance<codomain_type>
#endif

//------------------------------------------------------------------------------
// template parameter Section could be a template type parameter.
#ifdef ICL_USE_SECTION_TEMPLATE_TEMPLATE
#   define ICL_SECTION template<class>class
#   define ICL_SECTION_CODOMAIN(itl_intersect, codomain_type) itl_intersect<codomain_type> 
#   define ICL_SECTION_INSTANCE(section_instance,codomain_type) section_instance
#else//ICL_USE_SECTION_TEMPLATE_TYPE
#   define ICL_SECTION class
#   define ICL_SECTION_CODOMAIN(itl_intersect, codomain_type) itl_intersect 
#   define ICL_SECTION_INSTANCE(section_instance,codomain_type) section_instance<codomain_type>
#endif


//------------------------------------------------------------------------------
// template parameter Interval could be a template type parameter.
#ifdef ICL_USE_INTERVAL_TEMPLATE_TEMPLATE
#   define ICL_INTERVAL(itl_compare) template<class,itl_compare>class
#   define ICL_INTERVAL2(itl_compare) template<class DomT2,itl_compare>class
#   define ICL_INTERVAL_TYPE(itl_interval, domain_type, itl_compare) itl_interval<domain_type,itl_compare> 
#   define ICL_INTERVAL_INSTANCE(interval_instance,domain_type,itl_compare) interval_instance
#else//ICL_USE_INTERVAL_TEMPLATE_TYPE
#   define ICL_INTERVAL(itl_compare) class
#   define ICL_INTERVAL2(itl_compare) class
#   define ICL_INTERVAL_TYPE(itl_interval, domain_type, itl_compare) itl_interval  
#   define ICL_INTERVAL_INSTANCE(interval_instance,domain_type,itl_compare) typename interval_instance<domain_type,itl_compare>::type
#endif


//------------------------------------------------------------------------------
#define ICL_ALLOC    template<class>class

//------------------------------------------------------------------------------
#define ICL_INTERVAL_DEFAULT boost::icl::interval_type_default

#ifndef BOOST_ICL_USE_COMPARE_STD_GREATER
#   define ICL_COMPARE_DEFAULT std::less
#else
#   define ICL_COMPARE_DEFAULT std::greater
#endif

//------------------------------------------------------------------------------

#endif // BOOST_ICL_DESIGN_CONFIG_HPP_JOFA_090214


