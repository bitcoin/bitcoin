/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_IS_COMBINABLE_HPP_JOFA_090115
#define BOOST_ICL_IS_COMBINABLE_HPP_JOFA_090115

#include <boost/mpl/bool.hpp> 
#include <boost/mpl/if.hpp> 
#include <boost/mpl/and.hpp> 
#include <boost/mpl/or.hpp> 
#include <boost/mpl/not.hpp> 
#include <boost/type_traits/is_same.hpp>
#include <boost/icl/type_traits/is_concept_equivalent.hpp>
#include <boost/icl/type_traits/is_interval_container.hpp>

namespace boost{namespace icl
{

template<class Type>
struct is_overloadable
{
    typedef is_overloadable<Type> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (boost::is_same<Type, typename Type::overloadable_type>::value)
        );
};


//------------------------------------------------------------------------------
template<class LeftT, class RightT>
struct is_codomain_equal
{
    typedef is_codomain_equal<LeftT, RightT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (boost::is_same<typename LeftT::codomain_type, 
                        typename RightT::codomain_type>::value)
        );
};

//NOTE: Equality of compare order implies the equality of the domain_types
template<class LeftT, class RightT>
struct is_key_compare_equal
{
    typedef is_key_compare_equal<LeftT, RightT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (boost::is_same<typename LeftT::key_compare, 
                        typename RightT::key_compare>::value)
        );
};

template<class LeftT, class RightT>
struct is_codomain_type_equal
{
    typedef is_codomain_type_equal<LeftT, RightT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::and_<is_key_compare_equal<LeftT, RightT>, 
                   is_codomain_equal<LeftT, RightT> >::value)
        );
};


// For equal containers concepts, domain order and codomain type must match.
template<template<class>class IsConcept, class LeftT, class RightT>
struct is_concept_compatible
{
    typedef is_concept_compatible<IsConcept, LeftT, RightT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::and_<
            IsConcept<LeftT>
          , IsConcept<RightT>
          , is_codomain_type_equal<LeftT, RightT>
        >::value)
        );
};

template<template<class>class LeftConcept, 
         template<class>class RightConcept,         
         class LeftT, class RightT>
struct is_concept_combinable
{
    typedef is_concept_combinable<LeftConcept, RightConcept, LeftT, RightT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::and_<
            LeftConcept<LeftT>
          , RightConcept<RightT>
          , is_key_compare_equal<LeftT, RightT>
        >::value)
        );
};

template<class LeftT, class RightT>
struct is_intra_combinable
{
    typedef is_intra_combinable<LeftT, RightT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::or_<
            is_concept_compatible<is_interval_set, LeftT, RightT>
          , is_concept_compatible<is_interval_map, LeftT, RightT>
        >::value)
        );
};

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
template<class LeftT, class RightT>
struct is_cross_combinable
{
    typedef is_cross_combinable<LeftT, RightT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::or_<
            is_concept_combinable<is_interval_set, is_interval_map, LeftT, RightT>
          , is_concept_combinable<is_interval_map, is_interval_set, LeftT, RightT>
        >::value)
        );
};

template<class LeftT, class RightT>
struct is_inter_combinable
{
    typedef is_inter_combinable<LeftT, RightT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::or_<is_intra_combinable<LeftT,RightT>, 
                  is_cross_combinable<LeftT,RightT> >::value)
        );
};

//------------------------------------------------------------------------------
// is_fragment_of
//------------------------------------------------------------------------------
template<class FragmentT, class Type>
struct is_fragment_of
{
    typedef is_fragment_of type;
    BOOST_STATIC_CONSTANT(bool, value = false);
};

template<class Type>
struct is_fragment_of<typename Type::element_type, Type>
{
    typedef is_fragment_of type;
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template<class Type>
struct is_fragment_of<typename Type::segment_type, Type>
{
    typedef is_fragment_of type;
    BOOST_STATIC_CONSTANT(bool, value = true);
};

//------------------------------------------------------------------------------
// is_key_of
//------------------------------------------------------------------------------
template<class KeyT, class Type>
struct is_key_of
{
    typedef is_key_of type;
    BOOST_STATIC_CONSTANT(bool, value = false);
};

template<class Type>
struct is_key_of<typename Type::domain_type, Type>
{
    typedef is_key_of type;
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template<class Type>
struct is_key_of<typename Type::interval_type, Type>
{
    typedef is_key_of type;
    BOOST_STATIC_CONSTANT(bool, value = true);
};

//------------------------------------------------------------------------------
// is_interval_set_derivative
//------------------------------------------------------------------------------
template<class Type, class AssociateT>
struct is_interval_set_derivative;

template<class Type>
struct is_interval_set_derivative<Type, typename Type::domain_type>
{ 
    typedef is_interval_set_derivative type;
    BOOST_STATIC_CONSTANT(bool, value = (is_interval_container<Type>::value)); 
};

template<class Type>
struct is_interval_set_derivative<Type, typename Type::interval_type>
{ 
    typedef is_interval_set_derivative type;
    BOOST_STATIC_CONSTANT(bool, value = (is_interval_container<Type>::value)); 
};

template<class Type, class AssociateT>
struct is_interval_set_derivative
{
    typedef is_interval_set_derivative<Type, AssociateT> type;
    BOOST_STATIC_CONSTANT(bool, value = false);
};

//------------------------------------------------------------------------------
// is_interval_map_derivative
//------------------------------------------------------------------------------
template<class Type, class AssociateT>
struct is_interval_map_derivative;

template<class Type>
struct is_interval_map_derivative<Type, typename Type::domain_mapping_type>
{
    typedef is_interval_map_derivative type;
    BOOST_STATIC_CONSTANT(bool, value = (is_interval_container<Type>::value));
};

template<class Type>
struct is_interval_map_derivative<Type, typename Type::interval_mapping_type>
{
    typedef is_interval_map_derivative type;
    BOOST_STATIC_CONSTANT(bool, value = (is_interval_container<Type>::value));
};

template<class Type>
struct is_interval_map_derivative<Type, typename Type::value_type>
{
    typedef is_interval_map_derivative type;
    BOOST_STATIC_CONSTANT(bool, value = (is_interval_container<Type>::value));
};

template<class Type, class AssociateT>
struct is_interval_map_derivative
{
    typedef is_interval_map_derivative<Type, AssociateT> type;
    BOOST_STATIC_CONSTANT(bool, value = false);
};

//------------------------------------------------------------------------------
// is_intra_derivative
//------------------------------------------------------------------------------
template<class Type, class AssociateT>
struct is_intra_derivative
{
    typedef is_intra_derivative<Type, AssociateT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::or_
        <     
            mpl::and_<is_interval_set<Type>, 
                      is_interval_set_derivative<Type, AssociateT> >
          , mpl::and_<is_interval_map<Type>, 
                      is_interval_map_derivative<Type, AssociateT> >
        >::value)
        );
};

template<class Type, class AssociateT>
struct is_cross_derivative
{
    typedef is_cross_derivative<Type, AssociateT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::and_<
            is_interval_map<Type>
          , is_interval_set_derivative<Type, AssociateT>
        >::value)
        );
};

template<class Type, class AssociateT>
struct is_inter_derivative
{
    typedef is_inter_derivative<Type, AssociateT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::or_<
            is_intra_derivative<Type, AssociateT> 
          , is_cross_derivative<Type, AssociateT>
        >::value)
        );
};

//------------------------------------------------------------------------------
//- right combinable
//------------------------------------------------------------------------------

template<class GuideT, class CompanionT>
struct is_interval_set_right_combinable
{
    typedef is_interval_set_right_combinable<GuideT, CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::and_
        <
            is_interval_set<GuideT>
          , mpl::or_
            <
                is_interval_set_derivative<GuideT, CompanionT> 
              , is_concept_compatible<is_interval_set, GuideT, CompanionT>
            >
        >::value)
        );
};

template<class GuideT, class CompanionT>
struct is_interval_map_right_intra_combinable //NOTE equivalent to is_fragment_type_of
{
    typedef is_interval_map_right_intra_combinable<GuideT, CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::and_
        <
            is_interval_map<GuideT>
          , mpl::or_
            <
                is_interval_map_derivative<GuideT, CompanionT> 
              , is_concept_compatible<is_interval_map, GuideT, CompanionT>
            >
        >::value)
        );
};

template<class GuideT, class CompanionT>
struct is_interval_map_right_cross_combinable //NOTE equivalent to key_type_of<Comp, Guide>
{
    typedef is_interval_map_right_cross_combinable<GuideT, CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::and_
        <
            is_interval_map<GuideT>
          , mpl::or_
            <
                is_cross_derivative<GuideT, CompanionT> 
              , is_concept_combinable<is_interval_map, is_interval_set, GuideT, CompanionT>
            >
        >::value)
        );
};

template<class GuideT, class CompanionT>
struct is_interval_map_right_inter_combinable
{
    typedef is_interval_map_right_inter_combinable<GuideT, CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::or_<
            is_interval_map_right_intra_combinable<GuideT, CompanionT> 
          , is_interval_map_right_cross_combinable<GuideT, CompanionT> 
        >::value)
        );
};


template<class GuideT, class CompanionT>
struct is_right_intra_combinable
{
    typedef is_right_intra_combinable<GuideT, CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::or_
        <
            is_interval_set_right_combinable<GuideT, CompanionT> 
          , is_interval_map_right_intra_combinable<GuideT, CompanionT> 
        >::value)
        );
};

template<class GuideT, class CompanionT>
struct is_right_inter_combinable
{
    typedef is_right_inter_combinable<GuideT, CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::or_
        <
            is_interval_set_right_combinable<GuideT, CompanionT> 
          , is_interval_map_right_inter_combinable<GuideT, CompanionT> 
        >::value)
        );
};

template<class GuideT, class IntervalSetT>
struct combines_right_to_interval_set
{
    typedef combines_right_to_interval_set<GuideT, IntervalSetT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (is_concept_combinable<is_interval_container, is_interval_set, 
                               GuideT, IntervalSetT>::value)
        );
};

template<class GuideT, class IntervalMapT>
struct combines_right_to_interval_map
{
    typedef combines_right_to_interval_map<GuideT, IntervalMapT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (is_concept_compatible<is_interval_map, GuideT, IntervalMapT>::value) );
};

template<class GuideT, class IntervalContainerT>
struct combines_right_to_interval_container
{
    typedef combines_right_to_interval_container<GuideT, IntervalContainerT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::or_<combines_right_to_interval_set<GuideT, IntervalContainerT>,
                  combines_right_to_interval_map<GuideT, IntervalContainerT> >::value)
        );
};



//------------------------------------------------------------------------------
//- segmentational_fineness
//------------------------------------------------------------------------------
template<class Type> struct unknown_fineness
{
    typedef unknown_fineness<Type> type;
    static const int value = 0;
};

template<class Type> struct known_fineness
{
    typedef known_fineness<Type> type;
    static const int value = Type::fineness;
};

template<class Type>struct segmentational_fineness
{
    typedef segmentational_fineness<Type> type;
    static const int value = 
        mpl::if_<is_interval_container<Type>, 
                        known_fineness<Type>,
                      unknown_fineness<Type>
                >::type::value;
};


//------------------------------------------------------------------------------
// is_interval_set_companion
//------------------------------------------------------------------------------

// CompanionT is either an interval_set or a derivative of set level: 
// element_type=domain_type, segment_type=interval_type
template<class GuideT, class CompanionT> struct is_interval_set_companion
{ 
    typedef is_interval_set_companion<GuideT,CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::or_
        <
            combines_right_to_interval_set<GuideT,CompanionT>
          , is_interval_set_derivative<GuideT,CompanionT>
        >::value)
        );
};


//------------------------------------------------------------------------------
// is_interval_map_companion
//------------------------------------------------------------------------------

template<class GuideT, class CompanionT> struct is_interval_map_companion
{ 
    typedef is_interval_map_companion<GuideT,CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value = 
        (mpl::or_
        <
            combines_right_to_interval_map<GuideT,CompanionT>
          , is_interval_map_derivative<GuideT,CompanionT>
        >::value)
        );
};


//------------------------------------------------------------------------------
//- is_coarser_interval_{set,map}_companion
//------------------------------------------------------------------------------
template<class GuideT, class CompanionT> 
struct is_coarser_interval_set_companion
{
    typedef is_coarser_interval_set_companion<GuideT, CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::and_
        <
           is_interval_set_companion<GuideT, CompanionT>
         , mpl::bool_<(  segmentational_fineness<GuideT>::value 
                       > segmentational_fineness<CompanionT>::value)>
        >::value)
        );
};

template<class GuideT, class CompanionT> 
struct is_coarser_interval_map_companion
{
    typedef is_coarser_interval_map_companion<GuideT, CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::and_
        <
           is_interval_map_companion<GuideT, CompanionT>
         , mpl::bool_<(  segmentational_fineness<GuideT>::value 
                       > segmentational_fineness<CompanionT>::value)>
        >::value)
        );
};

//------------------------------------------------------------------------------
// is_binary_interval_{set,map}_combinable
//------------------------------------------------------------------------------
template<class GuideT, class CompanionT>
struct is_binary_interval_set_combinable
{ 
    typedef is_binary_interval_set_combinable<GuideT,CompanionT> type;
    static const int value =
        mpl::and_<  is_interval_set<GuideT>
                  , is_coarser_interval_set_companion<GuideT, CompanionT> 
                 >::value;
};

template<class GuideT, class CompanionT>
struct is_binary_interval_map_combinable
{ 
    typedef is_binary_interval_map_combinable<GuideT,CompanionT> type;
    static const int value =
        mpl::and_<  is_interval_map<GuideT>
                  , is_coarser_interval_map_companion<GuideT, CompanionT> 
                 >::value;
};

template<class GuideT, class CompanionT>
struct is_binary_intra_combinable
{ 
    typedef is_binary_intra_combinable<GuideT,CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::or_<is_binary_interval_set_combinable<GuideT, CompanionT>,
                 is_binary_interval_map_combinable<GuideT, CompanionT> 
                >::value)
        );
};

template<class GuideT, class CompanionT>
struct is_binary_cross_combinable
{ 
    typedef is_binary_cross_combinable<GuideT,CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::and_
        <     is_interval_map<GuideT>
            , mpl::or_<  is_coarser_interval_map_companion<GuideT, CompanionT>
                       ,         is_interval_set_companion<GuideT, CompanionT> > 
        >::value)
        );
};

template<class GuideT, class CompanionT>
struct is_binary_inter_combinable
{ 
    typedef is_binary_inter_combinable<GuideT,CompanionT> type;
    BOOST_STATIC_CONSTANT(bool, value =
        (mpl::or_
        <     
            mpl::and_<is_interval_map<GuideT>, 
                      is_binary_cross_combinable<GuideT, CompanionT> >
          , mpl::and_<is_interval_set<GuideT>, 
                      is_binary_intra_combinable<GuideT, CompanionT> >
        >::value)
        );
};


}} // namespace icl boost

#endif


