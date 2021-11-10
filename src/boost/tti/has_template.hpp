
//  (C) Copyright Edward Diener 2011,2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_HAS_TEMPLATE_HPP)
#define BOOST_TTI_HAS_TEMPLATE_HPP

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

#include <boost/tti/gen/has_template_gen.hpp>
#include <boost/preprocessor/config/config.hpp>
#include <boost/preprocessor/control/iif.hpp>

#if BOOST_PP_VARIADICS

#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/variadic/elem.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/tti/detail/dvm_template_params.hpp>

/// A macro which expands to a metafunction which tests whether an inner class template with a particular name exists.
/**

    BOOST_TTI_TRAIT_HAS_TEMPLATE is a macro which expands to a metafunction.
    The metafunction tests whether an inner class template with a particular name exists.
    The macro takes the form of BOOST_TTI_TRAIT_HAS_TEMPLATE(trait,...) where
    
    trait = the name of the metafunction <br/>
    ...   = variadic parameters.
    
    The first variadic parameter is the inner class template name.
    
    Following variadic parameters are optional.
    
    If no following variadic parameters exist, then the inner class template 
    being introspected must be all template type parameters ( template parameters 
    starting with `class` or `typename` ) and any number of template type parameters
    can occur.
    
    If the second variadic parameter is BOOST_PP_NIL and no other variadic 
    parameter is given, then just as in the previous case the inner class template 
    being introspected must be all template type parameters ( template parameters 
    starting with `class` or `typename` ) and any number of template type parameters
    can occur. This form is allowed in order to be consistent with using the 
    non-variadic form of this macro.
    
    If the second variadic parameter is a Boost preprocessor library array and no other 
    variadic parameter is given, then the inner class template must have its template 
    parameters matching the sequence in the tuple portion of the Boost PP array. This 
    form is allowed in order to be consistent with using the non-variadic form of this 
    macro.
    
    Otherwise the inner class template must have its template parameters matching the 
    sequence of the optional variadic parameters.
    
    BOOST_TTI_TRAIT_HAS_TEMPLATE generates a metafunction called "trait" where 'trait' is the first macro parameter.
    
  @code
  
              template<class BOOST_TTI_TP_T>
              struct trait
                {
                static const value = unspecified;
                typedef mpl::bool_<true-or-false> type;
                };

              The metafunction types and return:
    
                BOOST_TTI_TP_T = the enclosing type in which to look for our 'name'.
                                 The enclosing type can be a class, struct, or union.
                
                returns = 'value' is true if the 'name' template exists within the enclosing type,
                          otherwise 'value' is false.
                          
  @endcode
  
    Examples:
    
  @code
  
    1) Search for an inner class template called 'MyTemplate', with all template type parameters,
       nested within the class 'MyClass' using a metafunction name of 'MyMeta'.
    
       BOOST_TTI_TRAIT_HAS_TEMPLATE(MyMeta,MyTemplate)
    
       or
    
       BOOST_TTI_TRAIT_HAS_TEMPLATE(MyMeta,MyTemplate,BOOST_PP_NIL) // Non-variadic macro form
    
       MyMeta<MyClass>::value
    
       is a compile time boolean constant which is either 'true' or 'false'
       if the nested template exists.
    
    2) Search for an inner class template called 'MyTemplate', with template parameters 
       of 'class T,int x,template<class> class U', nested within the class 'MyClass' 
       using a metafunction name of 'MyMeta'.
    
       BOOST_TTI_TRAIT_HAS_TEMPLATE(MyMeta,MyTemplate,class,int,template<class> class)
    
       or
    
       BOOST_TTI_TRAIT_HAS_TEMPLATE(MyMeta,MyTemplate,(3,(class,int,template<class> class))) // Non-variadic macro form
    
       MyMeta<MyClass>::value
    
       is a compile time boolean constant which is either 'true' or 'false'
       if the nested template exists.
    
  @endcode
  
*/
#define BOOST_TTI_TRAIT_HAS_TEMPLATE(trait,...) \
  BOOST_PP_IIF \
    ( \
    BOOST_PP_EQUAL \
      ( \
      BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), \
      1 \
      ), \
    BOOST_TTI_DETAIL_VM_TRAIT_HAS_TEMPLATE, \
    BOOST_TTI_DETAIL_VM_CHECK_MORE_THAN_TWO \
    ) \
    (trait,__VA_ARGS__) \
/**/

/// A macro which expands to a metafunction which tests whether an inner class template with a particular name exists.
/**

    BOOST_TTI_HAS_TEMPLATE is a macro which expands to a metafunction.
    The metafunction tests whether an inner class template with a particular name exists.
    The macro takes the form of BOOST_TTI_HAS_TEMPLATE(...) where
    
    ...   = variadic parameters.
    
    The first variadic parameter is the inner class template name.
    
    Following variadic parameters are optional.
    
    If no following variadic parameters exist, then the inner class template 
    being introspected must be all template type parameters ( template parameters 
    starting with `class` or `typename` ) and any number of template type parameters
    can occur.
    
    If the second variadic parameter is BOOST_PP_NIL and no other variadic 
    parameter is given, then just as in the previous case the inner class template 
    being introspected must be all template type parameters ( template parameters 
    starting with `class` or `typename` ) and any number of template type parameters
    can occur. This form is allowed in order to be consistent with using the 
    non-variadic form of this macro.
    
    If the second variadic parameter is a Boost preprocessor library array and no other 
    variadic parameter is given, then the inner class template must have its template 
    parameters matching the sequence in the tuple portion of the Boost PP array. This 
    form is allowed in order to be consistent with using the non-variadic form of this 
    macro.
    
    Otherwise the inner class template must have its template parameters matching the 
    sequence of the optional variadic parameters.
    
    BOOST_TTI_HAS_TEMPLATE generates a metafunction called "has_template_'name'" where 'name' is the first variadic parameter.
    
  @code
  
              template<class BOOST_TTI_TP_T>
              struct has_template_'name'
                {
                static const value = unspecified;
                typedef mpl::bool_<true-or-false> type;
                };

              The metafunction types and return:
    
                BOOST_TTI_TP_T = the enclosing type in which to look for our 'name'.
                                 The enclosing type can be a class, struct, or union.
                
                returns = 'value' is true if the 'name' template exists within the enclosing type,
                          otherwise 'value' is false.
                          
  @endcode
  
    Examples:
    
  @code
  
    1) Search for an inner class template called 'MyTemplate', with all template type parameters,
       nested within the class 'MyClass'.
    
       BOOST_TTI_HAS_TEMPLATE(MyTemplate)
    
       or
    
       BOOST_TTI_HAS_TEMPLATE(MyTemplate,BOOST_PP_NIL) // Non-variadic macro form
    
       has_template_MyTemplate<MyClass>::value
    
       is a compile time boolean constant which is either 'true' or 'false'
       if the nested template exists.
    
    2) Search for an inner class template called 'MyTemplate' with template parameters 
       of 'class T,int x,template<class> class U' nested within the class 'MyClass'.
    
       BOOST_TTI_HAS_TEMPLATE(MyTemplate,class,int,template<class> class)
    
       or
    
       BOOST_TTI_HAS_TEMPLATE(MyTemplate,(3,(class,int,template<class> class))) // Non-variadic macro form
    
       has_template_MyTemplate<MyClass>::value
    
       is a compile time boolean constant which is either 'true' or 'false'
       if the nested template exists.
    
  @endcode
  
*/
#define BOOST_TTI_HAS_TEMPLATE(...) \
  BOOST_TTI_TRAIT_HAS_TEMPLATE \
    ( \
    BOOST_TTI_HAS_TEMPLATE_GEN \
      ( \
      BOOST_PP_VARIADIC_ELEM(0,__VA_ARGS__) \
      ), \
    __VA_ARGS__ \
    ) \
/**/

#else // !BOOST_PP_VARIADICS

#include <boost/preprocessor/detail/is_binary.hpp>
#include <boost/tti/detail/dtemplate.hpp>
#include <boost/tti/detail/dtemplate_params.hpp>

/// A macro which expands to a metafunction which tests whether an inner class template with a particular name exists.
/**

    BOOST_TTI_TRAIT_HAS_TEMPLATE is a macro which expands to a metafunction.
    The metafunction tests whether an inner class template with a particular name exists.
    The macro takes the form of BOOST_TTI_TRAIT_HAS_TEMPLATE(trait,name,params) where
    
    trait  = the name of the metafunction <br/>
    name   = the inner class template name <br/>
    params = If the  parameter is BOOST_PP_NIL the inner class template 
             being introspected must be all template type parameters ( template parameters 
             starting with `class` or `typename` ) and any number of template type parameters
             can occur.
            
    If the parameter is a Boost preprocessor library array, then the inner class 
    template must have its template parameters matching the sequence in the tuple portion 
    of the Boost PP array.
    
    If the parameter is anything else a compiler error occurs.
    
    BOOST_TTI_TRAIT_HAS_TEMPLATE generates a metafunction called "trait" where 'trait' is the first macro parameter.
    
  @code
  
              template<class BOOST_TTI_TP_T>
              struct trait
                {
                static const value = unspecified;
                typedef mpl::bool_<true-or-false> type;
                };

              The metafunction types and return:
    
                BOOST_TTI_TP_T = the enclosing type in which to look for our 'name'.
                                 The enclosing type can be a class, struct, or union.
                
                returns = 'value' is true if the 'name' template exists within the enclosing type,
                          otherwise 'value' is false.
                          
  @endcode
  
    Examples:
    
  @code
  
    1) Search for an inner class template called 'MyTemplate', with all template type parameters,
       nested within the class 'MyClass' using a metafunction name of 'MyMeta'.
    
       BOOST_TTI_TRAIT_HAS_TEMPLATE(MyMeta,MyTemplate,BOOST_PP_NIL)
    
       MyMeta<MyClass>::value
    
       is a compile time boolean constant which is either 'true' or 'false'
       if the nested template exists.
    
    2) Search for an inner class template called 'MyTemplate', with template parameters 
       of 'class T,int x,template<class> class U', nested within the class 'MyClass' 
       using a metafunction name of 'MyMeta'.
    
       BOOST_TTI_TRAIT_HAS_TEMPLATE(MyMeta,MyTemplate,(3,(class,int,template<class> class)))
    
       MyMeta<MyClass>::value
    
       is a compile time boolean constant which is either 'true' or 'false'
       if the nested template exists.
    
  @endcode
  
*/
#define BOOST_TTI_TRAIT_HAS_TEMPLATE(trait,name,params) \
  BOOST_PP_IIF \
    ( \
    BOOST_PP_IS_BINARY(params), \
    BOOST_TTI_DETAIL_TRAIT_HAS_TEMPLATE_CHECK_PARAMS, \
    BOOST_TTI_DETAIL_TRAIT_CHECK_IS_NIL \
    ) \
    (trait,name,params) \
/**/
  
/// A macro which expands to a metafunction which tests whether an inner class template with a particular name exists.
/**

    BOOST_TTI_HAS_TEMPLATE is a macro which expands to a metafunction.
    The metafunction tests whether an inner class template with a particular name exists.
    The macro takes the form of BOOST_TTI_HAS_TEMPLATE(name,params) where
    
    name   = the inner class template name <br/>
    params = If the  parameter is BOOST_PP_NIL the inner class template 
             being introspected must be all template type parameters ( template parameters 
             starting with `class` or `typename` ) and any number of template type parameters
             can occur.
            
    If the parameter is a Boost preprocessor library array, then the inner class 
    template must have its template parameters matching the sequence in the tuple portion 
    of the Boost PP array.
    
    If the parameter is anything else a compiler error occurs.
    
    BOOST_TTI_HAS_TEMPLATE generates a metafunction called "has_template_'name'" where 'name' is the first macro parameter.
    
  @code
  
              template<class BOOST_TTI_TP_T>
              struct has_template_'name'
                {
                static const value = unspecified;
                typedef mpl::bool_<true-or-false> type;
                };

              The metafunction types and return:
    
                BOOST_TTI_TP_T = the enclosing type in which to look for our 'name'.
                                 The enclosing type can be a class, struct, or union.
                
                returns = 'value' is true if the 'name' template exists within the enclosing type,
                          otherwise 'value' is false.
                          
  @endcode
  
    Examples:
    
  @code
  
    1) Search for an inner class template called 'MyTemplate', with all template type parameters,
       nested within the class 'MyClass'.
    
       BOOST_TTI_HAS_TEMPLATE(MyTemplate,BOOST_PP_NIL)
    
       has_template_MyTemplate<MyClass>::value
    
       is a compile time boolean constant which is either 'true' or 'false'
       if the nested template exists.
    
    2) Search for an inner class template called 'MyTemplate' with template parameters 
       of 'class T,int x,template<class> class U' nested within the class 'MyClass'.
    
       BOOST_TTI_HAS_TEMPLATE(MyTemplate,(3,(class,int,template<class> class)))
    
       has_template_MyTemplate<MyClass>::value
    
       is a compile time boolean constant which is either 'true' or 'false'
       if the nested template exists.
       
  @endcode
  
*/
#define BOOST_TTI_HAS_TEMPLATE(name,params) \
  BOOST_TTI_TRAIT_HAS_TEMPLATE \
  ( \
  BOOST_TTI_HAS_TEMPLATE_GEN(name), \
  name, \
  params \
  ) \
/**/

#endif // BOOST_PP_VARIADICS
#endif // BOOST_TTI_HAS_TEMPLATE_HPP
