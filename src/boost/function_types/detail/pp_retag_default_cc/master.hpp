
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

// no include guards, this file is guarded externally

#ifdef __WAVE__
// this file has been generated from the master.hpp file in the same directory
#   pragma wave option(preserve: 0)
#endif

#if !defined(BOOST_PP_VALUE)
#   include <boost/preprocessor/slot/slot.hpp>
#   include <boost/preprocessor/iteration/self.hpp>

#   include <boost/function_types/detail/encoding/def.hpp>
#   include <boost/function_types/detail/encoding/aliases_def.hpp>

namespace boost { namespace function_types {

namespace detail
{
  template<class Tag, class RefTag> struct selector_bits
  {
#   define  BOOST_PP_VALUE non_member|member|non_variadic|variadic
#   include BOOST_PP_ASSIGN_SLOT(1)

    BOOST_STATIC_CONSTANT(bits_t, value = (
        (::boost::function_types::detail::bits<Tag>::value & BOOST_FT_default_cc) 
      | (::boost::function_types::detail::bits<RefTag>::value & BOOST_PP_SLOT(1))
    ));
  };

  template<bits_t SelectorBits> struct default_cc_tag; 
  
  template<class Tag, class RefTag> struct retag_default_cc
    : detail::compound_tag
      < Tag, detail::default_cc_tag< 
          ::boost::function_types::detail::selector_bits<Tag,RefTag>::value > >
  { };

  template<bits_t SelectorBits> struct default_cc_tag
  {
    typedef null_tag::bits bits;
    typedef null_tag::mask mask;
  };

  class test_class;
  typedef constant<BOOST_FT_cc_mask> cc_mask_constant;

#   define BOOST_FT_self \
      <boost/function_types/detail/pp_retag_default_cc/master.hpp>

#   define  default_cc_ BOOST_FT_default_cc

#   define  BOOST_PP_VALUE default_cc_|non_member|non_variadic
#   define  BOOST_FT_tester void (*tester)()
#   define  BOOST_PP_INDIRECT_SELF BOOST_FT_self
#   include BOOST_PP_INCLUDE_SELF()

#   define  BOOST_PP_VALUE default_cc_|non_member|variadic
#   define  BOOST_FT_tester void (*tester)(...)
#   define  BOOST_PP_INDIRECT_SELF BOOST_FT_self
#   include BOOST_PP_INCLUDE_SELF()

#   define  BOOST_PP_VALUE default_cc_|member|non_variadic
#   define  BOOST_FT_tester void (test_class::*tester)()
#   define  BOOST_PP_INDIRECT_SELF BOOST_FT_self
#   include BOOST_PP_INCLUDE_SELF()

#   define  BOOST_PP_VALUE default_cc_|member|variadic
#   define  BOOST_FT_tester void (test_class::*tester)(...)
#   define  BOOST_PP_INDIRECT_SELF BOOST_FT_self
#   include BOOST_PP_INCLUDE_SELF()

#   undef   default_cc_

#   undef BOOST_FT_self

} } } // namespace ::boost::function_types::detail

#   include <boost/function_types/detail/encoding/aliases_undef.hpp>
#   include <boost/function_types/detail/encoding/undef.hpp>

#else // if defined(BOOST_PP_VALUE)

#   include BOOST_PP_ASSIGN_SLOT(1)

  template<> struct default_cc_tag<BOOST_PP_SLOT(1)> 
  {
    typedef BOOST_FT_tester;
    typedef mpl::bitand_<components<tester>::bits,cc_mask_constant> bits;
    typedef cc_mask_constant mask;
  };

#   undef BOOST_FT_tester

#endif

