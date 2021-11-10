    ///////////////////////////////////////////////////////////////////////////////
    /// \file and_n.hpp
    /// Definitions of and_N, and_impl
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    
    
    
    template<bool B, typename P0>
    struct and_2
      : mpl::bool_<P0::value>
    {};
    template<typename P0>
    struct and_2<false, P0>
      : mpl::false_
    {};
        template<typename G0 , typename G1, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0 , G1>, Expr, State, Data>
          : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::when<proto::_, G0> ::template impl<Expr, State, Data> Gimpl0; typedef typename proto::when<proto::_, G1> ::template impl<Expr, State, Data> Gimpl1;
            typedef typename Gimpl1::result_type result_type;
            result_type operator()(
                typename _and_impl::expr_param e
              , typename _and_impl::state_param s
              , typename _and_impl::data_param d
            ) const
            {
                
                
                
                
                Gimpl0()(e,s,d);
                return Gimpl1()(e,s,d);
            }
        };
    
    
    
    template<bool B, typename P0 , typename P1>
    struct and_3
      : and_2<
            P0::value ,
            P1
        >
    {};
    template<typename P0 , typename P1>
    struct and_3<false, P0 , P1>
      : mpl::false_
    {};
        template<typename G0 , typename G1 , typename G2, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0 , G1 , G2>, Expr, State, Data>
          : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::when<proto::_, G0> ::template impl<Expr, State, Data> Gimpl0; typedef typename proto::when<proto::_, G1> ::template impl<Expr, State, Data> Gimpl1; typedef typename proto::when<proto::_, G2> ::template impl<Expr, State, Data> Gimpl2;
            typedef typename Gimpl2::result_type result_type;
            result_type operator()(
                typename _and_impl::expr_param e
              , typename _and_impl::state_param s
              , typename _and_impl::data_param d
            ) const
            {
                
                
                
                
                Gimpl0()(e,s,d); Gimpl1()(e,s,d);
                return Gimpl2()(e,s,d);
            }
        };
    
    
    
    template<bool B, typename P0 , typename P1 , typename P2>
    struct and_4
      : and_3<
            P0::value ,
            P1 , P2
        >
    {};
    template<typename P0 , typename P1 , typename P2>
    struct and_4<false, P0 , P1 , P2>
      : mpl::false_
    {};
        template<typename G0 , typename G1 , typename G2 , typename G3, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0 , G1 , G2 , G3>, Expr, State, Data>
          : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::when<proto::_, G0> ::template impl<Expr, State, Data> Gimpl0; typedef typename proto::when<proto::_, G1> ::template impl<Expr, State, Data> Gimpl1; typedef typename proto::when<proto::_, G2> ::template impl<Expr, State, Data> Gimpl2; typedef typename proto::when<proto::_, G3> ::template impl<Expr, State, Data> Gimpl3;
            typedef typename Gimpl3::result_type result_type;
            result_type operator()(
                typename _and_impl::expr_param e
              , typename _and_impl::state_param s
              , typename _and_impl::data_param d
            ) const
            {
                
                
                
                
                Gimpl0()(e,s,d); Gimpl1()(e,s,d); Gimpl2()(e,s,d);
                return Gimpl3()(e,s,d);
            }
        };
    
    
    
    template<bool B, typename P0 , typename P1 , typename P2 , typename P3>
    struct and_5
      : and_4<
            P0::value ,
            P1 , P2 , P3
        >
    {};
    template<typename P0 , typename P1 , typename P2 , typename P3>
    struct and_5<false, P0 , P1 , P2 , P3>
      : mpl::false_
    {};
        template<typename G0 , typename G1 , typename G2 , typename G3 , typename G4, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0 , G1 , G2 , G3 , G4>, Expr, State, Data>
          : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::when<proto::_, G0> ::template impl<Expr, State, Data> Gimpl0; typedef typename proto::when<proto::_, G1> ::template impl<Expr, State, Data> Gimpl1; typedef typename proto::when<proto::_, G2> ::template impl<Expr, State, Data> Gimpl2; typedef typename proto::when<proto::_, G3> ::template impl<Expr, State, Data> Gimpl3; typedef typename proto::when<proto::_, G4> ::template impl<Expr, State, Data> Gimpl4;
            typedef typename Gimpl4::result_type result_type;
            result_type operator()(
                typename _and_impl::expr_param e
              , typename _and_impl::state_param s
              , typename _and_impl::data_param d
            ) const
            {
                
                
                
                
                Gimpl0()(e,s,d); Gimpl1()(e,s,d); Gimpl2()(e,s,d); Gimpl3()(e,s,d);
                return Gimpl4()(e,s,d);
            }
        };
    
    
    
    template<bool B, typename P0 , typename P1 , typename P2 , typename P3 , typename P4>
    struct and_6
      : and_5<
            P0::value ,
            P1 , P2 , P3 , P4
        >
    {};
    template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4>
    struct and_6<false, P0 , P1 , P2 , P3 , P4>
      : mpl::false_
    {};
        template<typename G0 , typename G1 , typename G2 , typename G3 , typename G4 , typename G5, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0 , G1 , G2 , G3 , G4 , G5>, Expr, State, Data>
          : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::when<proto::_, G0> ::template impl<Expr, State, Data> Gimpl0; typedef typename proto::when<proto::_, G1> ::template impl<Expr, State, Data> Gimpl1; typedef typename proto::when<proto::_, G2> ::template impl<Expr, State, Data> Gimpl2; typedef typename proto::when<proto::_, G3> ::template impl<Expr, State, Data> Gimpl3; typedef typename proto::when<proto::_, G4> ::template impl<Expr, State, Data> Gimpl4; typedef typename proto::when<proto::_, G5> ::template impl<Expr, State, Data> Gimpl5;
            typedef typename Gimpl5::result_type result_type;
            result_type operator()(
                typename _and_impl::expr_param e
              , typename _and_impl::state_param s
              , typename _and_impl::data_param d
            ) const
            {
                
                
                
                
                Gimpl0()(e,s,d); Gimpl1()(e,s,d); Gimpl2()(e,s,d); Gimpl3()(e,s,d); Gimpl4()(e,s,d);
                return Gimpl5()(e,s,d);
            }
        };
    
    
    
    template<bool B, typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5>
    struct and_7
      : and_6<
            P0::value ,
            P1 , P2 , P3 , P4 , P5
        >
    {};
    template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5>
    struct and_7<false, P0 , P1 , P2 , P3 , P4 , P5>
      : mpl::false_
    {};
        template<typename G0 , typename G1 , typename G2 , typename G3 , typename G4 , typename G5 , typename G6, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0 , G1 , G2 , G3 , G4 , G5 , G6>, Expr, State, Data>
          : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::when<proto::_, G0> ::template impl<Expr, State, Data> Gimpl0; typedef typename proto::when<proto::_, G1> ::template impl<Expr, State, Data> Gimpl1; typedef typename proto::when<proto::_, G2> ::template impl<Expr, State, Data> Gimpl2; typedef typename proto::when<proto::_, G3> ::template impl<Expr, State, Data> Gimpl3; typedef typename proto::when<proto::_, G4> ::template impl<Expr, State, Data> Gimpl4; typedef typename proto::when<proto::_, G5> ::template impl<Expr, State, Data> Gimpl5; typedef typename proto::when<proto::_, G6> ::template impl<Expr, State, Data> Gimpl6;
            typedef typename Gimpl6::result_type result_type;
            result_type operator()(
                typename _and_impl::expr_param e
              , typename _and_impl::state_param s
              , typename _and_impl::data_param d
            ) const
            {
                
                
                
                
                Gimpl0()(e,s,d); Gimpl1()(e,s,d); Gimpl2()(e,s,d); Gimpl3()(e,s,d); Gimpl4()(e,s,d); Gimpl5()(e,s,d);
                return Gimpl6()(e,s,d);
            }
        };
    
    
    
    template<bool B, typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6>
    struct and_8
      : and_7<
            P0::value ,
            P1 , P2 , P3 , P4 , P5 , P6
        >
    {};
    template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6>
    struct and_8<false, P0 , P1 , P2 , P3 , P4 , P5 , P6>
      : mpl::false_
    {};
        template<typename G0 , typename G1 , typename G2 , typename G3 , typename G4 , typename G5 , typename G6 , typename G7, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0 , G1 , G2 , G3 , G4 , G5 , G6 , G7>, Expr, State, Data>
          : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::when<proto::_, G0> ::template impl<Expr, State, Data> Gimpl0; typedef typename proto::when<proto::_, G1> ::template impl<Expr, State, Data> Gimpl1; typedef typename proto::when<proto::_, G2> ::template impl<Expr, State, Data> Gimpl2; typedef typename proto::when<proto::_, G3> ::template impl<Expr, State, Data> Gimpl3; typedef typename proto::when<proto::_, G4> ::template impl<Expr, State, Data> Gimpl4; typedef typename proto::when<proto::_, G5> ::template impl<Expr, State, Data> Gimpl5; typedef typename proto::when<proto::_, G6> ::template impl<Expr, State, Data> Gimpl6; typedef typename proto::when<proto::_, G7> ::template impl<Expr, State, Data> Gimpl7;
            typedef typename Gimpl7::result_type result_type;
            result_type operator()(
                typename _and_impl::expr_param e
              , typename _and_impl::state_param s
              , typename _and_impl::data_param d
            ) const
            {
                
                
                
                
                Gimpl0()(e,s,d); Gimpl1()(e,s,d); Gimpl2()(e,s,d); Gimpl3()(e,s,d); Gimpl4()(e,s,d); Gimpl5()(e,s,d); Gimpl6()(e,s,d);
                return Gimpl7()(e,s,d);
            }
        };
    
    
    
    template<bool B, typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6 , typename P7>
    struct and_9
      : and_8<
            P0::value ,
            P1 , P2 , P3 , P4 , P5 , P6 , P7
        >
    {};
    template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6 , typename P7>
    struct and_9<false, P0 , P1 , P2 , P3 , P4 , P5 , P6 , P7>
      : mpl::false_
    {};
        template<typename G0 , typename G1 , typename G2 , typename G3 , typename G4 , typename G5 , typename G6 , typename G7 , typename G8, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0 , G1 , G2 , G3 , G4 , G5 , G6 , G7 , G8>, Expr, State, Data>
          : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::when<proto::_, G0> ::template impl<Expr, State, Data> Gimpl0; typedef typename proto::when<proto::_, G1> ::template impl<Expr, State, Data> Gimpl1; typedef typename proto::when<proto::_, G2> ::template impl<Expr, State, Data> Gimpl2; typedef typename proto::when<proto::_, G3> ::template impl<Expr, State, Data> Gimpl3; typedef typename proto::when<proto::_, G4> ::template impl<Expr, State, Data> Gimpl4; typedef typename proto::when<proto::_, G5> ::template impl<Expr, State, Data> Gimpl5; typedef typename proto::when<proto::_, G6> ::template impl<Expr, State, Data> Gimpl6; typedef typename proto::when<proto::_, G7> ::template impl<Expr, State, Data> Gimpl7; typedef typename proto::when<proto::_, G8> ::template impl<Expr, State, Data> Gimpl8;
            typedef typename Gimpl8::result_type result_type;
            result_type operator()(
                typename _and_impl::expr_param e
              , typename _and_impl::state_param s
              , typename _and_impl::data_param d
            ) const
            {
                
                
                
                
                Gimpl0()(e,s,d); Gimpl1()(e,s,d); Gimpl2()(e,s,d); Gimpl3()(e,s,d); Gimpl4()(e,s,d); Gimpl5()(e,s,d); Gimpl6()(e,s,d); Gimpl7()(e,s,d);
                return Gimpl8()(e,s,d);
            }
        };
    
    
    
    template<bool B, typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6 , typename P7 , typename P8>
    struct and_10
      : and_9<
            P0::value ,
            P1 , P2 , P3 , P4 , P5 , P6 , P7 , P8
        >
    {};
    template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6 , typename P7 , typename P8>
    struct and_10<false, P0 , P1 , P2 , P3 , P4 , P5 , P6 , P7 , P8>
      : mpl::false_
    {};
        template<typename G0 , typename G1 , typename G2 , typename G3 , typename G4 , typename G5 , typename G6 , typename G7 , typename G8 , typename G9, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0 , G1 , G2 , G3 , G4 , G5 , G6 , G7 , G8 , G9>, Expr, State, Data>
          : proto::transform_impl<Expr, State, Data>
        {
            typedef typename proto::when<proto::_, G0> ::template impl<Expr, State, Data> Gimpl0; typedef typename proto::when<proto::_, G1> ::template impl<Expr, State, Data> Gimpl1; typedef typename proto::when<proto::_, G2> ::template impl<Expr, State, Data> Gimpl2; typedef typename proto::when<proto::_, G3> ::template impl<Expr, State, Data> Gimpl3; typedef typename proto::when<proto::_, G4> ::template impl<Expr, State, Data> Gimpl4; typedef typename proto::when<proto::_, G5> ::template impl<Expr, State, Data> Gimpl5; typedef typename proto::when<proto::_, G6> ::template impl<Expr, State, Data> Gimpl6; typedef typename proto::when<proto::_, G7> ::template impl<Expr, State, Data> Gimpl7; typedef typename proto::when<proto::_, G8> ::template impl<Expr, State, Data> Gimpl8; typedef typename proto::when<proto::_, G9> ::template impl<Expr, State, Data> Gimpl9;
            typedef typename Gimpl9::result_type result_type;
            result_type operator()(
                typename _and_impl::expr_param e
              , typename _and_impl::state_param s
              , typename _and_impl::data_param d
            ) const
            {
                
                
                
                
                Gimpl0()(e,s,d); Gimpl1()(e,s,d); Gimpl2()(e,s,d); Gimpl3()(e,s,d); Gimpl4()(e,s,d); Gimpl5()(e,s,d); Gimpl6()(e,s,d); Gimpl7()(e,s,d); Gimpl8()(e,s,d);
                return Gimpl9()(e,s,d);
            }
        };
