
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#ifndef BOOST_FT_DETAIL_TAGS_HPP_INCLUDED
#define BOOST_FT_DETAIL_TAGS_HPP_INCLUDED

#include <cstddef>

#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/bitxor.hpp>


namespace boost { namespace function_types { 

namespace detail 
{
  typedef long bits_t;

  template<bits_t Value> struct constant 
    : boost::integral_constant<bits_t,Value> 
  { };

  template<bits_t Bits, bits_t Mask> struct property_tag 
  {
    typedef constant<Bits> bits;
    typedef constant<Mask> mask;
  };

  template<typename T> struct bits : T::bits { };
  template<typename T> struct mask : T::mask { };

  // forward declaration, defined in pp_tags
  template<bits_t Bits, bits_t CCID> struct encode_bits_impl; 

  // forward declaration, defined in pp_tags
  template<bits_t LHS_bits, bits_t LHS_mask, 
           bits_t RHS_bits, bits_t RHS_mask> 
  struct tag_ice;
 
  // forward declaration, defined in retag_default_cc 
  template<class Tag, class RegTag = Tag> struct retag_default_cc; 
 
  template<bits_t Bits, bits_t CCID> struct encode_bits
    : constant< 
        ::boost::function_types::detail::encode_bits_impl<Bits,CCID>::value 
      >
  { };

  template<class LHS, class RHS> struct compound_tag
  {
    typedef constant<
      ::boost::function_types::detail::tag_ice
        < ::boost::function_types::detail::bits<LHS>::value
        , ::boost::function_types::detail::mask<LHS>::value
        , ::boost::function_types::detail::bits<RHS>::value
        , ::boost::function_types::detail::mask<RHS>::value
        >::combined_bits 
    > bits;

    typedef constant< 
      ::boost::function_types::detail::tag_ice
        < ::boost::function_types::detail::bits<LHS>::value
        , ::boost::function_types::detail::mask<LHS>::value
        , ::boost::function_types::detail::bits<RHS>::value
        , ::boost::function_types::detail::mask<RHS>::value
        >::combined_mask 
    > mask; 
  };

  template <class Base, class PropOld, class PropNew>
  struct changed_tag
    : Base
  {
    typedef mpl::bitxor_
        <typename Base::bits, typename PropOld::bits, typename PropNew::bits>
    bits;
  };

  template<class Tag, class QueryTag> struct represents_impl
    : boost::integral_constant<bool,
        ::boost::function_types::detail::tag_ice
          < ::boost::function_types::detail::bits<Tag>::value
          , ::boost::function_types::detail::mask<Tag>::value
          , ::boost::function_types::detail::bits<QueryTag>::value
          , ::boost::function_types::detail::mask<QueryTag>::value
          >::match
      >
  { };

} // namespace detail

typedef detail::property_tag<0,0> null_tag;

template<class Tag1, class Tag2, class Tag3 = null_tag, class Tag4 = null_tag>
struct tag
  : detail::compound_tag< detail::compound_tag<Tag1,Tag2>, 
        detail::compound_tag<Tag3,Tag4> >
{ };

template<class Tag1, class Tag2, class Tag3> struct tag<Tag1,Tag2,Tag3,null_tag>
  : detail::compound_tag<detail::compound_tag<Tag1,Tag2>,Tag3>
{ };
template<class Tag1, class Tag2> struct tag<Tag1,Tag2,null_tag,null_tag>
  : detail::compound_tag<Tag1,Tag2>
{ };
template<class Tag1> struct tag<Tag1,null_tag,null_tag,null_tag>
  : Tag1
{ };


template<class Tag, class QueryTag> struct represents
  : detail::represents_impl<Tag, detail::retag_default_cc<QueryTag,Tag> >
{ };


template<class Tag, class QueryTag> struct extract
{ 
  typedef detail::constant<
    ::boost::function_types::detail::tag_ice
      < ::boost::function_types::detail::bits<Tag>::value
      , ::boost::function_types::detail::mask<Tag>::value
      , ::boost::function_types::detail::bits<QueryTag>::value
      , ::boost::function_types::detail::mask<QueryTag>::value
      >::extracted_bits 
  > bits;

  typedef detail::constant< 
    ::boost::function_types::detail::mask<QueryTag>::value
  > mask; 
};

/*

  The following is a metafunction which checks whether a
  property tag is in a possibly compounded tag type.
  
  Here both the possibly compounded tag type and a property tag
  is given.
  
*/

template<class Tag, class PropertyTag> struct has_property_tag
  : detail::represents_impl<Tag,  PropertyTag>
{ };

} } // namespace ::boost::function_types

#include <boost/function_types/detail/pp_tags/preprocessed.hpp>

namespace boost { namespace function_types {
#define BOOST_FT_cc_file <boost/function_types/detail/pp_tags/cc_tag.hpp>
#include <boost/function_types/detail/pp_loop.hpp>

/*

  The following are metafunctions which check whether the
  specific property tag is in a possibly compounded tag type.
  Here only the possibly compounded tag type is given.
  
*/

template<class Tag> struct has_property_tag<Tag,null_tag>
  : ::boost::is_same<Tag, null_tag>
{ };

template<class Tag> struct has_variadic_property_tag
  : has_property_tag<Tag,  variadic>
{ };

template<class Tag> struct has_default_cc_property_tag
  : has_property_tag<Tag,  default_cc>
{ };

template<class Tag> struct has_const_property_tag
  : has_property_tag<Tag,  const_qualified>
{ };

template<class Tag> struct has_volatile_property_tag
  : has_property_tag<Tag,  volatile_qualified>
{ };

template<class Tag> struct has_cv_property_tag
  : has_property_tag<Tag,  cv_qualified>
{ };

template<class Tag> struct has_null_property_tag
  : has_property_tag<Tag,  null_tag>
{ };

} } // namespace boost::function_types

#endif

