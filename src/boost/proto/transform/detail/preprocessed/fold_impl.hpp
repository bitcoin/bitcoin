    ///////////////////////////////////////////////////////////////////////////////
    /// \file fold_impl.hpp
    /// Contains definition of fold_impl<> and reverse_fold_impl<> templates.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 1>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1;
        typedef state1 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d );
            return s1;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 1>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state1;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state1 s1 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 2>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >::result_type state2;
        typedef state2 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >()( proto::child_c< 1>(e) , s1 , d );
            return s2;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 2>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state2;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state2 s2 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >()( proto::child_c<1>(e) , s2 , d ); state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 3>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >::result_type state3;
        typedef state3 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >()( proto::child_c< 1>(e) , s1 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >()( proto::child_c< 2>(e) , s2 , d );
            return s3;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 3>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state3;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state3 s3 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >()( proto::child_c<2>(e) , s3 , d ); state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >()( proto::child_c<1>(e) , s2 , d ); state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 4>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >::result_type state4;
        typedef state4 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >()( proto::child_c< 1>(e) , s1 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >()( proto::child_c< 2>(e) , s2 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >()( proto::child_c< 3>(e) , s3 , d );
            return s4;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 4>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state4;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state4 s4 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >()( proto::child_c<3>(e) , s4 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >()( proto::child_c<2>(e) , s3 , d ); state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >()( proto::child_c<1>(e) , s2 , d ); state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 5>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >::result_type state5;
        typedef state5 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >()( proto::child_c< 1>(e) , s1 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >()( proto::child_c< 2>(e) , s2 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >()( proto::child_c< 3>(e) , s3 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >()( proto::child_c< 4>(e) , s4 , d );
            return s5;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 5>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state5;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state5 s5 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >()( proto::child_c<4>(e) , s5 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >()( proto::child_c<3>(e) , s4 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >()( proto::child_c<2>(e) , s3 , d ); state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >()( proto::child_c<1>(e) , s2 , d ); state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 6>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >::result_type state6;
        typedef state6 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >()( proto::child_c< 1>(e) , s1 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >()( proto::child_c< 2>(e) , s2 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >()( proto::child_c< 3>(e) , s3 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >()( proto::child_c< 4>(e) , s4 , d ); state6 s6 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >()( proto::child_c< 5>(e) , s5 , d );
            return s6;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 6>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state6;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state6 s6 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >()( proto::child_c<5>(e) , s6 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >()( proto::child_c<4>(e) , s5 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >()( proto::child_c<3>(e) , s4 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >()( proto::child_c<2>(e) , s3 , d ); state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >()( proto::child_c<1>(e) , s2 , d ); state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 7>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >::result_type state6; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 6>::type , state6 , Data >::result_type state7;
        typedef state7 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >()( proto::child_c< 1>(e) , s1 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >()( proto::child_c< 2>(e) , s2 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >()( proto::child_c< 3>(e) , s3 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >()( proto::child_c< 4>(e) , s4 , d ); state6 s6 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >()( proto::child_c< 5>(e) , s5 , d ); state7 s7 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 6>::type , state6 , Data >()( proto::child_c< 6>(e) , s6 , d );
            return s7;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 7>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state7;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 6 >::type , state7 , Data >::result_type state6; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state7 s7 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state6 s6 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 6 >::type , state7 , Data >()( proto::child_c<6>(e) , s7 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >()( proto::child_c<5>(e) , s6 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >()( proto::child_c<4>(e) , s5 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >()( proto::child_c<3>(e) , s4 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >()( proto::child_c<2>(e) , s3 , d ); state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >()( proto::child_c<1>(e) , s2 , d ); state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 8>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >::result_type state6; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 6>::type , state6 , Data >::result_type state7; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 7>::type , state7 , Data >::result_type state8;
        typedef state8 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >()( proto::child_c< 1>(e) , s1 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >()( proto::child_c< 2>(e) , s2 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >()( proto::child_c< 3>(e) , s3 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >()( proto::child_c< 4>(e) , s4 , d ); state6 s6 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >()( proto::child_c< 5>(e) , s5 , d ); state7 s7 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 6>::type , state6 , Data >()( proto::child_c< 6>(e) , s6 , d ); state8 s8 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 7>::type , state7 , Data >()( proto::child_c< 7>(e) , s7 , d );
            return s8;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 8>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state8;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 7 >::type , state8 , Data >::result_type state7; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 6 >::type , state7 , Data >::result_type state6; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state8 s8 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state7 s7 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 7 >::type , state8 , Data >()( proto::child_c<7>(e) , s8 , d ); state6 s6 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 6 >::type , state7 , Data >()( proto::child_c<6>(e) , s7 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >()( proto::child_c<5>(e) , s6 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >()( proto::child_c<4>(e) , s5 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >()( proto::child_c<3>(e) , s4 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >()( proto::child_c<2>(e) , s3 , d ); state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >()( proto::child_c<1>(e) , s2 , d ); state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 9>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >::result_type state6; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 6>::type , state6 , Data >::result_type state7; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 7>::type , state7 , Data >::result_type state8; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 8>::type , state8 , Data >::result_type state9;
        typedef state9 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >()( proto::child_c< 1>(e) , s1 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >()( proto::child_c< 2>(e) , s2 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >()( proto::child_c< 3>(e) , s3 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >()( proto::child_c< 4>(e) , s4 , d ); state6 s6 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >()( proto::child_c< 5>(e) , s5 , d ); state7 s7 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 6>::type , state6 , Data >()( proto::child_c< 6>(e) , s6 , d ); state8 s8 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 7>::type , state7 , Data >()( proto::child_c< 7>(e) , s7 , d ); state9 s9 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 8>::type , state8 , Data >()( proto::child_c< 8>(e) , s8 , d );
            return s9;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 9>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state9;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 8 >::type , state9 , Data >::result_type state8; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 7 >::type , state8 , Data >::result_type state7; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 6 >::type , state7 , Data >::result_type state6; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state9 s9 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state8 s8 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 8 >::type , state9 , Data >()( proto::child_c<8>(e) , s9 , d ); state7 s7 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 7 >::type , state8 , Data >()( proto::child_c<7>(e) , s8 , d ); state6 s6 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 6 >::type , state7 , Data >()( proto::child_c<6>(e) , s7 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >()( proto::child_c<5>(e) , s6 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >()( proto::child_c<4>(e) , s5 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >()( proto::child_c<3>(e) , s4 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >()( proto::child_c<2>(e) , s3 , d ); state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >()( proto::child_c<1>(e) , s2 , d ); state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct fold_impl<State0, Fun, Expr, State, Data, 10>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state0;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >::result_type state6; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 6>::type , state6 , Data >::result_type state7; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 7>::type , state7 , Data >::result_type state8; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 8>::type , state8 , Data >::result_type state9; typedef typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 9>::type , state9 , Data >::result_type state10;
        typedef state10 result_type;
        result_type operator ()(
            typename fold_impl::expr_param e
          , typename fold_impl::state_param s
          , typename fold_impl::data_param d
        ) const
        {
            state0 s0 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 0>::type , state0 , Data >()( proto::child_c< 0>(e) , s0 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 1>::type , state1 , Data >()( proto::child_c< 1>(e) , s1 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 2>::type , state2 , Data >()( proto::child_c< 2>(e) , s2 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 3>::type , state3 , Data >()( proto::child_c< 3>(e) , s3 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 4>::type , state4 , Data >()( proto::child_c< 4>(e) , s4 , d ); state6 s6 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 5>::type , state5 , Data >()( proto::child_c< 5>(e) , s5 , d ); state7 s7 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 6>::type , state6 , Data >()( proto::child_c< 6>(e) , s6 , d ); state8 s8 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 7>::type , state7 , Data >()( proto::child_c< 7>(e) , s7 , d ); state9 s9 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 8>::type , state8 , Data >()( proto::child_c< 8>(e) , s8 , d ); state10 s10 = typename when<_, Fun>::template impl< typename result_of::child_c<Expr, 9>::type , state9 , Data >()( proto::child_c< 9>(e) , s9 , d );
            return s10;
        }
    };
    template<typename State0, typename Fun, typename Expr, typename State, typename Data>
    struct reverse_fold_impl<State0, Fun, Expr, State, Data, 10>
      : transform_impl<Expr, State, Data>
    {
        typedef typename when<_, State0>::template impl<Expr, State, Data>::result_type state10;
        typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 9 >::type , state10 , Data >::result_type state9; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 8 >::type , state9 , Data >::result_type state8; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 7 >::type , state8 , Data >::result_type state7; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 6 >::type , state7 , Data >::result_type state6; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >::result_type state5; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >::result_type state4; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >::result_type state3; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >::result_type state2; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >::result_type state1; typedef typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >::result_type state0;
        typedef state0 result_type;
        result_type operator ()(
            typename reverse_fold_impl::expr_param e
          , typename reverse_fold_impl::state_param s
          , typename reverse_fold_impl::data_param d
        ) const
        {
            state10 s10 =
                typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d);
            state9 s9 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 9 >::type , state10 , Data >()( proto::child_c<9>(e) , s10 , d ); state8 s8 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 8 >::type , state9 , Data >()( proto::child_c<8>(e) , s9 , d ); state7 s7 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 7 >::type , state8 , Data >()( proto::child_c<7>(e) , s8 , d ); state6 s6 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 6 >::type , state7 , Data >()( proto::child_c<6>(e) , s7 , d ); state5 s5 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 5 >::type , state6 , Data >()( proto::child_c<5>(e) , s6 , d ); state4 s4 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 4 >::type , state5 , Data >()( proto::child_c<4>(e) , s5 , d ); state3 s3 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 3 >::type , state4 , Data >()( proto::child_c<3>(e) , s4 , d ); state2 s2 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 2 >::type , state3 , Data >()( proto::child_c<2>(e) , s3 , d ); state1 s1 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 1 >::type , state2 , Data >()( proto::child_c<1>(e) , s2 , d ); state0 s0 = typename when<_, Fun>::template impl< typename result_of::child_c< Expr , 0 >::type , state1 , Data >()( proto::child_c<0>(e) , s1 , d );
            return s0;
        }
    };
