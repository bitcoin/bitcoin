    ///////////////////////////////////////////////////////////////////////////////
    /// \file call.hpp
    /// Contains definition of the call<> transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    
    
    
    template<typename Fun , typename A0>
    struct call<Fun(A0...)> : transform<call<Fun(A0...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A0
                  , detail::expand_pattern_rest_0<
                        Fun
                        
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    template<typename Fun , typename A0 , typename A1>
    struct call<Fun(A0 , A1...)> : transform<call<Fun(A0 , A1...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A1
                  , detail::expand_pattern_rest_1<
                        Fun
                        , A0
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2>
    struct call<Fun(A0 , A1 , A2...)> : transform<call<Fun(A0 , A1 , A2...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A2
                  , detail::expand_pattern_rest_2<
                        Fun
                        , A0 , A1
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3>
    struct call<Fun(A0 , A1 , A2 , A3)> : transform<call<Fun(A0 , A1 , A2 , A3)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data> a0; typedef typename a0::result_type b0; typedef typename when<_, A1>::template impl<Expr, State, Data> a1; typedef typename a1::result_type b1; typedef typename when<_, A2>::template impl<Expr, State, Data> a2; typedef typename a2::result_type b2; typedef typename when<_, A3>::template impl<Expr, State, Data> a3; typedef typename a3::result_type b3;
            typedef detail::poly_function_traits<Fun, Fun(b0 , b1 , b2 , b3)> function_traits;
            typedef typename function_traits::result_type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                typedef typename function_traits::function_type function_type;
                return function_type()(detail::as_lvalue(a0()(e, s, d)) , detail::as_lvalue(a1()(e, s, d)) , detail::as_lvalue(a2()(e, s, d)) , detail::as_lvalue(a3()(e, s, d)));
            }
        };
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3>
    struct call<Fun(A0 , A1 , A2 , A3...)> : transform<call<Fun(A0 , A1 , A2 , A3...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A3
                  , detail::expand_pattern_rest_3<
                        Fun
                        , A0 , A1 , A2
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct call<Fun(A0 , A1 , A2 , A3 , A4)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data> a0; typedef typename a0::result_type b0; typedef typename when<_, A1>::template impl<Expr, State, Data> a1; typedef typename a1::result_type b1; typedef typename when<_, A2>::template impl<Expr, State, Data> a2; typedef typename a2::result_type b2; typedef typename when<_, A3>::template impl<Expr, State, Data> a3; typedef typename a3::result_type b3; typedef typename when<_, A4>::template impl<Expr, State, Data> a4; typedef typename a4::result_type b4;
            typedef detail::poly_function_traits<Fun, Fun(b0 , b1 , b2 , b3 , b4)> function_traits;
            typedef typename function_traits::result_type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                typedef typename function_traits::function_type function_type;
                return function_type()(detail::as_lvalue(a0()(e, s, d)) , detail::as_lvalue(a1()(e, s, d)) , detail::as_lvalue(a2()(e, s, d)) , detail::as_lvalue(a3()(e, s, d)) , detail::as_lvalue(a4()(e, s, d)));
            }
        };
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct call<Fun(A0 , A1 , A2 , A3 , A4...)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A4
                  , detail::expand_pattern_rest_4<
                        Fun
                        , A0 , A1 , A2 , A3
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data> a0; typedef typename a0::result_type b0; typedef typename when<_, A1>::template impl<Expr, State, Data> a1; typedef typename a1::result_type b1; typedef typename when<_, A2>::template impl<Expr, State, Data> a2; typedef typename a2::result_type b2; typedef typename when<_, A3>::template impl<Expr, State, Data> a3; typedef typename a3::result_type b3; typedef typename when<_, A4>::template impl<Expr, State, Data> a4; typedef typename a4::result_type b4; typedef typename when<_, A5>::template impl<Expr, State, Data> a5; typedef typename a5::result_type b5;
            typedef detail::poly_function_traits<Fun, Fun(b0 , b1 , b2 , b3 , b4 , b5)> function_traits;
            typedef typename function_traits::result_type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                typedef typename function_traits::function_type function_type;
                return function_type()(detail::as_lvalue(a0()(e, s, d)) , detail::as_lvalue(a1()(e, s, d)) , detail::as_lvalue(a2()(e, s, d)) , detail::as_lvalue(a3()(e, s, d)) , detail::as_lvalue(a4()(e, s, d)) , detail::as_lvalue(a5()(e, s, d)));
            }
        };
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5...)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A5
                  , detail::expand_pattern_rest_5<
                        Fun
                        , A0 , A1 , A2 , A3 , A4
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data> a0; typedef typename a0::result_type b0; typedef typename when<_, A1>::template impl<Expr, State, Data> a1; typedef typename a1::result_type b1; typedef typename when<_, A2>::template impl<Expr, State, Data> a2; typedef typename a2::result_type b2; typedef typename when<_, A3>::template impl<Expr, State, Data> a3; typedef typename a3::result_type b3; typedef typename when<_, A4>::template impl<Expr, State, Data> a4; typedef typename a4::result_type b4; typedef typename when<_, A5>::template impl<Expr, State, Data> a5; typedef typename a5::result_type b5; typedef typename when<_, A6>::template impl<Expr, State, Data> a6; typedef typename a6::result_type b6;
            typedef detail::poly_function_traits<Fun, Fun(b0 , b1 , b2 , b3 , b4 , b5 , b6)> function_traits;
            typedef typename function_traits::result_type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                typedef typename function_traits::function_type function_type;
                return function_type()(detail::as_lvalue(a0()(e, s, d)) , detail::as_lvalue(a1()(e, s, d)) , detail::as_lvalue(a2()(e, s, d)) , detail::as_lvalue(a3()(e, s, d)) , detail::as_lvalue(a4()(e, s, d)) , detail::as_lvalue(a5()(e, s, d)) , detail::as_lvalue(a6()(e, s, d)));
            }
        };
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6...)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A6
                  , detail::expand_pattern_rest_6<
                        Fun
                        , A0 , A1 , A2 , A3 , A4 , A5
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data> a0; typedef typename a0::result_type b0; typedef typename when<_, A1>::template impl<Expr, State, Data> a1; typedef typename a1::result_type b1; typedef typename when<_, A2>::template impl<Expr, State, Data> a2; typedef typename a2::result_type b2; typedef typename when<_, A3>::template impl<Expr, State, Data> a3; typedef typename a3::result_type b3; typedef typename when<_, A4>::template impl<Expr, State, Data> a4; typedef typename a4::result_type b4; typedef typename when<_, A5>::template impl<Expr, State, Data> a5; typedef typename a5::result_type b5; typedef typename when<_, A6>::template impl<Expr, State, Data> a6; typedef typename a6::result_type b6; typedef typename when<_, A7>::template impl<Expr, State, Data> a7; typedef typename a7::result_type b7;
            typedef detail::poly_function_traits<Fun, Fun(b0 , b1 , b2 , b3 , b4 , b5 , b6 , b7)> function_traits;
            typedef typename function_traits::result_type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                typedef typename function_traits::function_type function_type;
                return function_type()(detail::as_lvalue(a0()(e, s, d)) , detail::as_lvalue(a1()(e, s, d)) , detail::as_lvalue(a2()(e, s, d)) , detail::as_lvalue(a3()(e, s, d)) , detail::as_lvalue(a4()(e, s, d)) , detail::as_lvalue(a5()(e, s, d)) , detail::as_lvalue(a6()(e, s, d)) , detail::as_lvalue(a7()(e, s, d)));
            }
        };
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7...)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A7
                  , detail::expand_pattern_rest_7<
                        Fun
                        , A0 , A1 , A2 , A3 , A4 , A5 , A6
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data> a0; typedef typename a0::result_type b0; typedef typename when<_, A1>::template impl<Expr, State, Data> a1; typedef typename a1::result_type b1; typedef typename when<_, A2>::template impl<Expr, State, Data> a2; typedef typename a2::result_type b2; typedef typename when<_, A3>::template impl<Expr, State, Data> a3; typedef typename a3::result_type b3; typedef typename when<_, A4>::template impl<Expr, State, Data> a4; typedef typename a4::result_type b4; typedef typename when<_, A5>::template impl<Expr, State, Data> a5; typedef typename a5::result_type b5; typedef typename when<_, A6>::template impl<Expr, State, Data> a6; typedef typename a6::result_type b6; typedef typename when<_, A7>::template impl<Expr, State, Data> a7; typedef typename a7::result_type b7; typedef typename when<_, A8>::template impl<Expr, State, Data> a8; typedef typename a8::result_type b8;
            typedef detail::poly_function_traits<Fun, Fun(b0 , b1 , b2 , b3 , b4 , b5 , b6 , b7 , b8)> function_traits;
            typedef typename function_traits::result_type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                typedef typename function_traits::function_type function_type;
                return function_type()(detail::as_lvalue(a0()(e, s, d)) , detail::as_lvalue(a1()(e, s, d)) , detail::as_lvalue(a2()(e, s, d)) , detail::as_lvalue(a3()(e, s, d)) , detail::as_lvalue(a4()(e, s, d)) , detail::as_lvalue(a5()(e, s, d)) , detail::as_lvalue(a6()(e, s, d)) , detail::as_lvalue(a7()(e, s, d)) , detail::as_lvalue(a8()(e, s, d)));
            }
        };
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8...)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A8
                  , detail::expand_pattern_rest_8<
                        Fun
                        , A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data> a0; typedef typename a0::result_type b0; typedef typename when<_, A1>::template impl<Expr, State, Data> a1; typedef typename a1::result_type b1; typedef typename when<_, A2>::template impl<Expr, State, Data> a2; typedef typename a2::result_type b2; typedef typename when<_, A3>::template impl<Expr, State, Data> a3; typedef typename a3::result_type b3; typedef typename when<_, A4>::template impl<Expr, State, Data> a4; typedef typename a4::result_type b4; typedef typename when<_, A5>::template impl<Expr, State, Data> a5; typedef typename a5::result_type b5; typedef typename when<_, A6>::template impl<Expr, State, Data> a6; typedef typename a6::result_type b6; typedef typename when<_, A7>::template impl<Expr, State, Data> a7; typedef typename a7::result_type b7; typedef typename when<_, A8>::template impl<Expr, State, Data> a8; typedef typename a8::result_type b8; typedef typename when<_, A9>::template impl<Expr, State, Data> a9; typedef typename a9::result_type b9;
            typedef detail::poly_function_traits<Fun, Fun(b0 , b1 , b2 , b3 , b4 , b5 , b6 , b7 , b8 , b9)> function_traits;
            typedef typename function_traits::result_type result_type;
            
            
            
            
            
            
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                typedef typename function_traits::function_type function_type;
                return function_type()(detail::as_lvalue(a0()(e, s, d)) , detail::as_lvalue(a1()(e, s, d)) , detail::as_lvalue(a2()(e, s, d)) , detail::as_lvalue(a3()(e, s, d)) , detail::as_lvalue(a4()(e, s, d)) , detail::as_lvalue(a5()(e, s, d)) , detail::as_lvalue(a6()(e, s, d)) , detail::as_lvalue(a7()(e, s, d)) , detail::as_lvalue(a8()(e, s, d)) , detail::as_lvalue(a9()(e, s, d)));
            }
        };
    };
    
    
    
    template<typename Fun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9...)> : transform<call<Fun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9...)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename detail::expand_pattern<
                    proto::arity_of<Expr>::value 
                  , A9
                  , detail::expand_pattern_rest_9<
                        Fun
                        , A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8
                    >
                >::type
            >::template impl<Expr, State, Data>
        {};
    };
