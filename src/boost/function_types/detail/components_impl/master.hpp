
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

// no include guards, this file is intended for multiple inclusion

#if   BOOST_FT_ARITY_LOOP_PREFIX

#   ifndef BOOST_FT_DETAIL_COMPONENTS_IMPL_MASTER_HPP_INCLUDED
#   define BOOST_FT_DETAIL_COMPONENTS_IMPL_MASTER_HPP_INCLUDED
#     include <boost/preprocessor/cat.hpp>
#     include <boost/preprocessor/facilities/empty.hpp>
#     include <boost/preprocessor/facilities/identity.hpp>
#     include <boost/preprocessor/arithmetic/dec.hpp>
#     include <boost/preprocessor/punctuation/comma_if.hpp>
#   endif

#   define BOOST_FT_type_name

#   if !BOOST_FT_mfp

#     define BOOST_FT_types \
          R BOOST_PP_COMMA_IF(BOOST_FT_arity) BOOST_FT_params(BOOST_PP_EMPTY)
#   else

#     define BOOST_FT_types \
          R, typename class_transform<T0 BOOST_FT_cv, L>::type \
          BOOST_PP_COMMA_IF(BOOST_PP_DEC(BOOST_FT_arity)) \
          BOOST_FT_params(BOOST_PP_EMPTY)

#   endif

#elif BOOST_FT_ARITY_LOOP_IS_ITERATING

template< BOOST_FT_tplargs(BOOST_PP_IDENTITY(typename)), typename L>
struct components_impl<BOOST_FT_type, L>
{
  typedef encode_bits<BOOST_FT_flags,BOOST_FT_cc_id> bits;
  typedef constant<BOOST_FT_full_mask> mask;

  typedef function_types::components<BOOST_FT_type, L> type;
  typedef components_mpl_sequence_tag tag;

  typedef mpl::integral_c<std::size_t,BOOST_FT_arity> function_arity;

  typedef BOOST_PP_CAT(mpl::vector,BOOST_FT_n)< BOOST_FT_types > types;
};

#elif BOOST_FT_ARITY_LOOP_SUFFIX

#   undef BOOST_FT_types
#   undef BOOST_FT_type_name

#else
#   error "attempt to use arity loop master file without loop"
#endif

