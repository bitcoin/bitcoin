
//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_HAS_MEMBER_FUNCTION_HPP)
#define BOOST_TTI_HAS_MEMBER_FUNCTION_HPP

#include <boost/config.hpp>
#include <boost/function_types/property_tags.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/tti/detail/ddeftype.hpp>
#include <boost/tti/detail/dmem_fun.hpp>
#include <boost/tti/gen/has_member_function_gen.hpp>
#include <boost/tti/gen/namespace_gen.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/// A macro which expands to a metafunction which tests whether a member function with a particular name and signature exists.
/**

    BOOST_TTI_TRAIT_HAS_MEMBER_FUNCTION is a macro which expands to a metafunction.
    The metafunction tests whether a member function with a particular name
    and signature exists. The macro takes the form of BOOST_TTI_TRAIT_HAS_MEMBER_FUNCTION(trait,name) where
    
    trait = the name of the metafunction <br/>
    name  = the name of the inner member.

    BOOST_TTI_TRAIT_HAS_MEMBER_FUNCTION generates a metafunction called "trait" where 'trait' is the macro parameter.
    
  @code
  
              template<class BOOST_TTI_TP_T,class BOOST_TTI_R,class BOOST_TTI_FS,class BOOST_TTI_TAG>
              struct trait
                {
                static const value = unspecified;
                typedef mpl::bool_<true-or-false> type;
                };

              The metafunction types and return:
    
                BOOST_TTI_TP_T   = the enclosing type in which to look for our 'name'.
                                   The enclosing type can be a class, struct, or union.
                                            OR
                                   a pointer to member function as a single type.
                
                BOOST_TTI_TP_R   = (optional) the return type of the member function
                          if the first parameter is the enclosing type.
                
                BOOST_TTI_TP_FS  = (optional) the parameters of the member function as a boost::mpl forward sequence
                          if the first parameter is the enclosing type and the member function parameters
                          are not empty.
                
                BOOST_TTI_TP_TAG = (optional) a boost::function_types tag to apply to the member function
                          if the first parameter is the enclosing type and a tag is needed.
                
                returns = 'value' is true if the 'name' exists, 
                          with the appropriate member function type,
                          otherwise 'value' is false.
                          
  @endcode
  
*/
#define BOOST_TTI_TRAIT_HAS_MEMBER_FUNCTION(trait,name) \
  BOOST_TTI_DETAIL_TRAIT_HAS_MEMBER_FUNCTION(trait,name) \
  template<class BOOST_TTI_TP_T,class BOOST_TTI_TP_R = BOOST_TTI_NAMESPACE::detail::deftype,class BOOST_TTI_TP_FS = boost::mpl::vector<>,class BOOST_TTI_TP_TAG = boost::function_types::null_tag> \
  struct trait \
    { \
    typedef typename \
    BOOST_PP_CAT(trait,_detail_hmf)<BOOST_TTI_TP_T,BOOST_TTI_TP_R,BOOST_TTI_TP_FS,BOOST_TTI_TP_TAG>::type type; \
    BOOST_STATIC_CONSTANT(bool,value=type::value); \
    }; \
/**/

/// A macro which expands to a metafunction which tests whether a member function with a particular name and signature exists.
/**

    BOOST_TTI_HAS_MEMBER_FUNCTION is a macro which expands to a metafunction.
    The metafunction tests whether a member function with a particular name
    and signature exists. The macro takes the form of BOOST_TTI_HAS_MEMBER_FUNCTION(name) where
    
    name  = the name of the inner member.

    BOOST_TTI_HAS_MEMBER_FUNCTION generates a metafunction called "has_member_function_name" where 'name' is the macro parameter.
    
  @code
  
              template<class BOOST_TTI_TP_T,class BOOST_TTI_TP_R,class BOOST_TTI_TP_FS,class BOOST_TTI_TP_TAG>
              struct has_member_function_'name'
                {
                static const value = unspecified;
                typedef mpl::bool_<true-or-false> type;
                };

              The metafunction types and return:
    
                BOOST_TTI_TP_T   = the enclosing type in which to look for our 'name'.
                                   The enclosing type can be a class, struct, or union.
                                            OR
                                   a pointer to member function as a single type.
                
                BOOST_TTI_TP_R   = (optional) the return type of the member function
                          if the first parameter is the enclosing type.
                
                BOOST_TTI_TP_FS  = (optional) the parameters of the member function as a boost::mpl forward sequence
                          if the first parameter is the enclosing type and the member function parameters
                          are not empty.
                
                BOOST_TTI_TP_TAG = (optional) a boost::function_types tag to apply to the member function
                          if the first parameter is the enclosing type and a tag is needed.
                
                returns = 'value' is true if the 'name' exists, 
                          with the appropriate member function type,
                          otherwise 'value' is false.
                          
  @endcode
  
*/
#define BOOST_TTI_HAS_MEMBER_FUNCTION(name) \
  BOOST_TTI_TRAIT_HAS_MEMBER_FUNCTION \
  ( \
  BOOST_TTI_HAS_MEMBER_FUNCTION_GEN(name), \
  name \
  ) \
/**/

#endif // BOOST_TTI_HAS_MEMBER_FUNCTION_HPP
