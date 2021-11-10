/*=============================================================================
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#if !defined(BOOST_FUSION_FUNCTIONAL_ADAPTER_UNFUSED_TYPED_HPP_INCLUDED)
#if !defined(BOOST_PP_IS_ITERATING)

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>

#include <boost/config.hpp>

#include <boost/utility/result_of.hpp>

#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/vector/convert.hpp>

#include <boost/fusion/functional/adapter/limits.hpp>
#include <boost/fusion/functional/adapter/detail/access.hpp>

#if defined (BOOST_MSVC)
#  pragma warning(push)
#  pragma warning (disable: 4512) // assignment operator could not be generated.
#endif


namespace boost { namespace fusion
{

    template <class Function, class Sequence> class unfused_typed;

    //----- ---- --- -- - -  -   -

    namespace detail
    {
        template <class Derived, class Function, 
            class Sequence, long Arity>
        struct unfused_typed_impl;
    }

    template <class Function, class Sequence>
    class unfused_typed
        : public detail::unfused_typed_impl
          < unfused_typed<Function,Sequence>, Function, Sequence, 
            result_of::size<Sequence>::value > 
    {
        Function fnc_transformed;

        template <class D, class F, class S, long A>
        friend struct detail::unfused_typed_impl;

        typedef typename detail::call_param<Function>::type func_const_fwd_t;

    public:

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        inline explicit unfused_typed(func_const_fwd_t f = Function())
            : fnc_transformed(f)
        { }
    }; 

    #define  BOOST_PP_FILENAME_1 <boost/fusion/functional/adapter/unfused_typed.hpp>
    #define  BOOST_PP_ITERATION_LIMITS (0,BOOST_FUSION_UNFUSED_TYPED_MAX_ARITY)
    #include BOOST_PP_ITERATE() 

}}

#if defined (BOOST_MSVC)
#  pragma warning(pop)
#endif

namespace boost 
{
#if !defined(BOOST_RESULT_OF_USE_DECLTYPE) || defined(BOOST_NO_CXX11_DECLTYPE)
    template<class F, class Seq>
    struct result_of< boost::fusion::unfused_typed<F,Seq> const () >
        : boost::fusion::unfused_typed<F,Seq>::template result< 
            boost::fusion::unfused_typed<F,Seq> const () >
    { };
    template<class F, class Seq>
    struct result_of< boost::fusion::unfused_typed<F,Seq>() >
        : boost::fusion::unfused_typed<F,Seq>::template result< 
            boost::fusion::unfused_typed<F,Seq> () >
    { };
#endif
    template<class F, class Seq>
    struct tr1_result_of< boost::fusion::unfused_typed<F,Seq> const () >
        : boost::fusion::unfused_typed<F,Seq>::template result< 
            boost::fusion::unfused_typed<F,Seq> const () >
    { };
    template<class F, class Seq>
    struct tr1_result_of< boost::fusion::unfused_typed<F,Seq>() >
        : boost::fusion::unfused_typed<F,Seq>::template result< 
            boost::fusion::unfused_typed<F,Seq> () >
    { };
}


#define BOOST_FUSION_FUNCTIONAL_ADAPTER_UNFUSED_TYPED_HPP_INCLUDED
#else // defined(BOOST_PP_IS_ITERATING)
///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#define N BOOST_PP_ITERATION()

    namespace detail
    {

        template <class Derived, class Function, class Sequence>
        struct unfused_typed_impl<Derived,Function,Sequence,N>
        {
            typedef typename detail::qf_c<Function>::type function_c;
            typedef typename detail::qf<Function>::type function;
            typedef typename result_of::as_vector<Sequence>::type arg_vector_t;

        public:

#define M(z,i,s)                                                                \
    typename call_param<typename result_of::value_at_c<s,i>::type>::type a##i

            BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            inline typename boost::result_of<
                function_c(arg_vector_t &) >::type
            operator()(BOOST_PP_ENUM(N,M,arg_vector_t)) const
            {
#if N > 0
                arg_vector_t arg(BOOST_PP_ENUM_PARAMS(N,a));
#else
                arg_vector_t arg;
#endif
                return static_cast<Derived const *>(this)->fnc_transformed(arg);
            }

            BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            inline typename boost::result_of<
                function(arg_vector_t &) >::type 
            operator()(BOOST_PP_ENUM(N,M,arg_vector_t)) 
            {
#if N > 0
                arg_vector_t arg(BOOST_PP_ENUM_PARAMS(N,a));
#else
                arg_vector_t arg;
#endif
                return static_cast<Derived *>(this)->fnc_transformed(arg);
            }

#undef M

            template <typename Sig> struct result { typedef void type; };

            template <class Self BOOST_PP_ENUM_TRAILING_PARAMS(N,typename T)>
            struct result< Self const (BOOST_PP_ENUM_PARAMS(N,T)) >
                : boost::result_of< function_c(arg_vector_t &) > 
            { };

            template <class Self BOOST_PP_ENUM_TRAILING_PARAMS(N,typename T)>
            struct result< Self (BOOST_PP_ENUM_PARAMS(N,T)) >
                : boost::result_of< function(arg_vector_t &) >
            { };
        };

    } // namespace detail

#undef N
#endif // defined(BOOST_PP_IS_ITERATING)
#endif

