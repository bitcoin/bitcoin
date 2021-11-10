
//  (C) Copyright Edward Diener 2011,2012,2013
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_TTI_MEMBER_TYPE_HPP)
#define BOOST_TTI_MEMBER_TYPE_HPP
  
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/not.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/tti/gen/member_type_gen.hpp>
#include <boost/tti/gen/namespace_gen.hpp>
#include <boost/tti/detail/dmem_type.hpp>
#include <boost/tti/detail/dnotype.hpp>

/*

  The succeeding comments in this file are in doxygen format.

*/

/** \file
*/

/// A macro expands to a metafunction whose typedef 'type' is either the named type or a marker type.
/**

    BOOST_TTI_TRAIT_MEMBER_TYPE is a macro which expands to a metafunction.
    The metafunction tests whether an inner type with a particular name exists
    by returning the inner type or a marker type.
    The macro takes the form of BOOST_TTI_TRAIT_MEMBER_TYPE(trait,name) where
    
    trait = the name of the metafunction <br/>
    name  = the name of the inner type.

    BOOST_TTI_TRAIT_MEMBER_TYPE generates a metafunction called "trait" where 'trait' is the macro parameter.
    
  @code
  
              template<class BOOST_TTI_TP_T,class BOOST_TTI_TP_MARKER_TYPE = boost::tti::detail::notype>
              struct trait
                {
                typedef unspecified type;
                typedef BOOST_TTI_TP_MARKER_TYPE boost_tti_marker_type;
                };

              The metafunction types and return:
              
                BOOST_TTI_TP_T           = the enclosing type.
                                           The enclosing type can be a class, struct, or union.
                
                BOOST_TTI_TP_MARKER_TYPE = (optional) a type to use as the marker type.
                                           defaults to the internal boost::tti::detail::notype.
                
                returns                  = 'type' is the inner type of 'name' if the inner type exists
                                           within the enclosing type, else 'type' is a marker type.
                                           if the end-user does not specify a marker type then
                                           an internal boost::tti::detail::notype marker type, 
                                           which is empty, is used.
                          
                The metafunction also encapsulates the type of the marker type as
                a nested 'boost_tti_marker_type'.
                          
  @endcode
  
    The purpose of this macro is to encapsulate the 'name' type as the typedef 'type'
    of a metafunction, but only if it exists within the enclosing type. This allows for
    an evaluation of inner type existence, without generating a compiler error,
    which can be used by other metafunctions in this library.
    
*/
#define BOOST_TTI_TRAIT_MEMBER_TYPE(trait,name) \
    BOOST_TTI_DETAIL_TRAIT_HAS_TYPE_MEMBER_TYPE(trait,name) \
    BOOST_TTI_DETAIL_TRAIT_MEMBER_TYPE(trait,name) \
    template<class BOOST_TTI_TP_T,class BOOST_TTI_TP_MARKER_TYPE = BOOST_TTI_NAMESPACE::detail::notype> \
    struct trait : \
      boost::mpl::eval_if \
        < \
        BOOST_PP_CAT(trait,_detail_has_type_member_type)<BOOST_TTI_TP_T>, \
        BOOST_PP_CAT(trait,_detail_member_type)<BOOST_TTI_TP_T>, \
        boost::mpl::identity<BOOST_TTI_TP_MARKER_TYPE> \
        > \
      { \
      typedef BOOST_TTI_TP_MARKER_TYPE boost_tti_marker_type; \
      }; \
/**/

/// A macro which expands to a metafunction whose typedef 'type' is either the named type or a marker type.
/**

    BOOST_TTI_MEMBER_TYPE is a macro which expands to a metafunction.
    The metafunction tests whether an inner type with a particular name exists
    by returning the inner type or a marker type.
    The macro takes the form of BOOST_TTI_MEMBER_TYPE(name) where
    
    name  = the name of the inner type.

    BOOST_TTI_MEMBER_TYPE generates a metafunction called "member_type_name" where 'name' is the macro parameter.
    
  @code
  
              template<class BOOST_TTI_TP_T,class BOOST_TTI_TP_MARKER_TYPE = boost::tti::detail::notype>
              struct member_type_'name'
                {
                typedef unspecified type;
                typedef BOOST_TTI_TP_MARKER_TYPE boost_tti_marker_type;
                };

              The metafunction types and return:
              
                BOOST_TTI_TP_T           = the enclosing type.
                                           The enclosing type can be a class, struct, or union.
                
                BOOST_TTI_TP_MARKER_TYPE = (optional) a type to use as the marker type.
                                           defaults to the internal boost::tti::detail::notype.
                
                returns                  = 'type' is the inner type of 'name' if the inner type exists
                                           within the enclosing type, else 'type' is a marker type.
                                           if the end-user does not specify a marker type then
                                           an internal boost::tti::detail::notype marker type is used.
                          
                The metafunction also encapsulates the type of the marker type as
                a nested 'boost_tti_marker_type'.
                          
  @endcode
  
    The purpose of this macro is to encapsulate the 'name' type as the typedef 'type'
    of a metafunction, but only if it exists within the enclosing type. This allows for
    an evaluation of inner type existence, without generating a compiler error,
    which can be used by other metafunctions in this library.
    
*/
#define BOOST_TTI_MEMBER_TYPE(name) \
  BOOST_TTI_TRAIT_MEMBER_TYPE \
  ( \
  BOOST_TTI_MEMBER_TYPE_GEN(name), \
  name \
  ) \
/**/
  
namespace boost
  {
  namespace tti
    {
  
    /// A metafunction which checks whether the member 'type' returned from invoking the macro metafunction generated by BOOST_TTI_MEMBER_TYPE ( BOOST_TTI_TRAIT_MEMBER_TYPE ) is a valid type.
    /**
    
        The metafunction 'valid_member_type', which is in the boost::tti namespace, takes the form of:

  @code
  
        template<class BOOST_TTI_TP_T,class BOOST_TTI_TP_MARKER_TYPE = boost::tti::detail::notype>
        struct valid_member_type
          {
          static const value = unspecified;
          typedef mpl::bool_<true-or-false> type;
          };

        The metafunction types and return:

          BOOST_TTI_TP_T           = returned inner 'type' from invoking the macro metafunction generated by BOOST_TTI_MEMBER_TYPE ( BOOST_TTI_TRAIT_MEMBER_TYPE ).
          
          BOOST_TTI_TP_MARKER_TYPE = (optional) a type to use as the marker type.
                                     defaults to the internal boost::tti::detail::notype.
      
          returns                  = 'value' is true if the type is valid, otherwise 'value' is false.
                                     A valid type means that the returned inner 'type' is not the marker type.
                          
  @endcode
  
    */
    template<class BOOST_TTI_TP_T,class BOOST_TTI_TP_MARKER_TYPE = BOOST_TTI_NAMESPACE::detail::notype>
    struct valid_member_type :
      boost::mpl::not_
        <
        boost::is_same<BOOST_TTI_TP_T,BOOST_TTI_TP_MARKER_TYPE>
        >
      {
      };
      
    /// A metafunction which checks whether the invoked macro metafunction generated by BOOST_TTI_MEMBER_TYPE ( BOOST_TTI_TRAIT_MEMBER_TYPE ) hold a valid type.
    /**

        The metafunction 'valid_member_metafunction', which is in the boost::tti namespace, takes the form of:

  @code
  
        template<class BOOST_TTI_METAFUNCTION>
        struct valid_member_metafunction
          {
          static const value = unspecified;
          typedef mpl::bool_<true-or-false> type;
          };

        The metafunction types and return:

          BOOST_TTI_METAFUNCTION = The invoked macro metafunction generated by BOOST_TTI_MEMBER_TYPE ( BOOST_TTI_TRAIT_MEMBER_TYPE ).
      
          returns          = 'value' is true if the nested type of the invoked metafunction is valid, otherwise 'value' is false.
                             A valid type means that the invoked metafunction's inner 'type' is not the marker type.
                          
  @endcode
  
    */
    template<class BOOST_TTI_METAFUNCTION>
    struct valid_member_metafunction :
      boost::mpl::not_
        <
        boost::is_same<typename BOOST_TTI_METAFUNCTION::type,typename BOOST_TTI_METAFUNCTION::boost_tti_marker_type>
        >
      {
      };
    }
  }
  
#endif // BOOST_TTI_MEMBER_TYPE_HPP
