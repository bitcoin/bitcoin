
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

// no include guards, this file is intended for multiple inclusion

#if   BOOST_FT_ARITY_LOOP_PREFIX

#   ifndef BOOST_FT_DETAIL_SYNTHESIZE_IMPL_MASTER_HPP_INCLUDED
#   define BOOST_FT_DETAIL_SYNTHESIZE_IMPL_MASTER_HPP_INCLUDED
#     include <boost/preprocessor/cat.hpp>
#     include <boost/preprocessor/arithmetic/dec.hpp>
#     include <boost/preprocessor/iteration/local.hpp>
#     include <boost/preprocessor/facilities/empty.hpp>
#     include <boost/preprocessor/facilities/identity.hpp>
#   endif

#   define BOOST_FT_type_name type

#   ifdef BOOST_FT_flags
#     define BOOST_FT_make_type(flags,cc,arity) BOOST_FT_make_type_impl(flags,cc,arity)
#     define BOOST_FT_make_type_impl(flags,cc,arity) make_type_ ## flags ## _ ## cc ## _ ## arity
#   else
BOOST_PP_EXPAND(#) define BOOST_FT_make_type(flags,cc,arity) BOOST_FT_make_type_impl(flags,cc,arity)
BOOST_PP_EXPAND(#) define BOOST_FT_make_type_impl(flags,cc,arity) make_type_ ## flags ## _ ## cc ## _ ## arity
#   endif

#   define BOOST_FT_iter(i) BOOST_PP_CAT(iter_,i)

#elif BOOST_FT_ARITY_LOOP_IS_ITERATING

template< BOOST_FT_tplargs(BOOST_PP_IDENTITY(typename)) >
struct BOOST_FT_make_type(BOOST_FT_flags,BOOST_FT_cc_id,BOOST_FT_arity)
{
  typedef BOOST_FT_type ;
};

template<> 
struct synthesize_impl_o< BOOST_FT_flags, BOOST_FT_cc_id, BOOST_FT_n > 
{ 
  template<typename S> struct synthesize_impl_i
  {
  private:
    typedef typename mpl::begin<S>::type BOOST_FT_iter(0);
#   if BOOST_FT_n > 1
#     define BOOST_PP_LOCAL_MACRO(i) typedef typename mpl::next< \
          BOOST_FT_iter(BOOST_PP_DEC(i)) >::type BOOST_FT_iter(i);
#     define BOOST_PP_LOCAL_LIMITS (1,BOOST_FT_n-1)
#     include BOOST_PP_LOCAL_ITERATE()
#   endif
  public:
    typedef typename detail::BOOST_FT_make_type(BOOST_FT_flags,BOOST_FT_cc_id,BOOST_FT_arity) 
    < typename mpl::deref< BOOST_FT_iter(0) >::type 
#   if BOOST_FT_mfp
    , typename detail::cv_traits< 
          typename mpl::deref< BOOST_FT_iter(1) >::type >::type
#   endif
#   if BOOST_FT_n > (BOOST_FT_mfp+1)
#     define BOOST_PP_LOCAL_LIMITS (BOOST_FT_mfp+1,BOOST_FT_n-1)
#     define BOOST_PP_LOCAL_MACRO(i) \
        , typename mpl::deref< BOOST_FT_iter(i) >::type
#     include BOOST_PP_LOCAL_ITERATE()
#   endif
    >::type type;
  };
};

#elif BOOST_FT_ARITY_LOOP_SUFFIX

#   ifdef BOOST_FT_flags
#     undef BOOST_FT_make_type
#     undef BOOST_FT_make_type_impl
#   else
BOOST_PP_EXPAND(#) undef BOOST_FT_make_type
BOOST_PP_EXPAND(#) undef BOOST_FT_make_type_impl
#   endif
#   undef BOOST_FT_iter
#   undef BOOST_FT_type_name

#else
#   error "attempt to use arity loop master file without loop"
#endif

