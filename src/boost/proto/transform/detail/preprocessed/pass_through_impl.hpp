    ///////////////////////////////////////////////////////////////////////////////
    /// \file pass_through_impl.hpp
    ///
    /// Specializations of pass_through_impl, used in the implementation of the
    /// pass_through transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 1>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list1<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 2>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list2<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d ) , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >()( e.proto_base().child1, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 3>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list3<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >::result_type , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d ) , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >()( e.proto_base().child1, s, d ) , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >()( e.proto_base().child2, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 4>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list4<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >::result_type , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >::result_type , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d ) , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >()( e.proto_base().child1, s, d ) , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >()( e.proto_base().child2, s, d ) , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >()( e.proto_base().child3, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 5>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list5<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >::result_type , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >::result_type , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >::result_type , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d ) , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >()( e.proto_base().child1, s, d ) , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >()( e.proto_base().child2, s, d ) , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >()( e.proto_base().child3, s, d ) , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >()( e.proto_base().child4, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 6>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list6<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >::result_type , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >::result_type , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >::result_type , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >::result_type , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d ) , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >()( e.proto_base().child1, s, d ) , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >()( e.proto_base().child2, s, d ) , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >()( e.proto_base().child3, s, d ) , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >()( e.proto_base().child4, s, d ) , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >()( e.proto_base().child5, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 7>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list7<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >::result_type , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >::result_type , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >::result_type , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >::result_type , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >::result_type , typename Grammar::proto_child6::template impl< typename result_of::child_c<Expr, 6>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d ) , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >()( e.proto_base().child1, s, d ) , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >()( e.proto_base().child2, s, d ) , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >()( e.proto_base().child3, s, d ) , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >()( e.proto_base().child4, s, d ) , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >()( e.proto_base().child5, s, d ) , typename Grammar::proto_child6::template impl< typename result_of::child_c<Expr, 6>::type , State , Data >()( e.proto_base().child6, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 8>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list8<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >::result_type , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >::result_type , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >::result_type , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >::result_type , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >::result_type , typename Grammar::proto_child6::template impl< typename result_of::child_c<Expr, 6>::type , State , Data >::result_type , typename Grammar::proto_child7::template impl< typename result_of::child_c<Expr, 7>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d ) , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >()( e.proto_base().child1, s, d ) , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >()( e.proto_base().child2, s, d ) , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >()( e.proto_base().child3, s, d ) , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >()( e.proto_base().child4, s, d ) , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >()( e.proto_base().child5, s, d ) , typename Grammar::proto_child6::template impl< typename result_of::child_c<Expr, 6>::type , State , Data >()( e.proto_base().child6, s, d ) , typename Grammar::proto_child7::template impl< typename result_of::child_c<Expr, 7>::type , State , Data >()( e.proto_base().child7, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 9>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list9<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >::result_type , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >::result_type , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >::result_type , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >::result_type , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >::result_type , typename Grammar::proto_child6::template impl< typename result_of::child_c<Expr, 6>::type , State , Data >::result_type , typename Grammar::proto_child7::template impl< typename result_of::child_c<Expr, 7>::type , State , Data >::result_type , typename Grammar::proto_child8::template impl< typename result_of::child_c<Expr, 8>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d ) , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >()( e.proto_base().child1, s, d ) , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >()( e.proto_base().child2, s, d ) , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >()( e.proto_base().child3, s, d ) , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >()( e.proto_base().child4, s, d ) , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >()( e.proto_base().child5, s, d ) , typename Grammar::proto_child6::template impl< typename result_of::child_c<Expr, 6>::type , State , Data >()( e.proto_base().child6, s, d ) , typename Grammar::proto_child7::template impl< typename result_of::child_c<Expr, 7>::type , State , Data >()( e.proto_base().child7, s, d ) , typename Grammar::proto_child8::template impl< typename result_of::child_c<Expr, 8>::type , State , Data >()( e.proto_base().child8, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
    template<typename Grammar, typename Domain, typename Expr, typename State, typename Data>
    struct pass_through_impl<Grammar, Domain, Expr, State, Data, 10>
      : transform_impl<Expr, State, Data>
    {
        typedef typename pass_through_impl::expr unref_expr;
        typedef
            typename mpl::if_c<
                is_same<Domain, deduce_domain>::value
              , typename unref_expr::proto_domain
              , Domain
            >::type
        result_domain;
        typedef
            typename base_expr<
                result_domain
              , typename unref_expr::proto_tag
              , list10<
                    typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >::result_type , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >::result_type , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >::result_type , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >::result_type , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >::result_type , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >::result_type , typename Grammar::proto_child6::template impl< typename result_of::child_c<Expr, 6>::type , State , Data >::result_type , typename Grammar::proto_child7::template impl< typename result_of::child_c<Expr, 7>::type , State , Data >::result_type , typename Grammar::proto_child8::template impl< typename result_of::child_c<Expr, 8>::type , State , Data >::result_type , typename Grammar::proto_child9::template impl< typename result_of::child_c<Expr, 9>::type , State , Data >::result_type
                >
            >::type
        expr_type;
        typedef typename result_domain::proto_generator proto_generator;
        typedef typename BOOST_PROTO_RESULT_OF<proto_generator(expr_type)>::type result_type;
        BOOST_FORCEINLINE
        BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, result_type const)
        operator ()(
            typename pass_through_impl::expr_param e
          , typename pass_through_impl::state_param s
          , typename pass_through_impl::data_param d
        ) const
        {
            expr_type const that = {
                typename Grammar::proto_child0::template impl< typename result_of::child_c<Expr, 0>::type , State , Data >()( e.proto_base().child0, s, d ) , typename Grammar::proto_child1::template impl< typename result_of::child_c<Expr, 1>::type , State , Data >()( e.proto_base().child1, s, d ) , typename Grammar::proto_child2::template impl< typename result_of::child_c<Expr, 2>::type , State , Data >()( e.proto_base().child2, s, d ) , typename Grammar::proto_child3::template impl< typename result_of::child_c<Expr, 3>::type , State , Data >()( e.proto_base().child3, s, d ) , typename Grammar::proto_child4::template impl< typename result_of::child_c<Expr, 4>::type , State , Data >()( e.proto_base().child4, s, d ) , typename Grammar::proto_child5::template impl< typename result_of::child_c<Expr, 5>::type , State , Data >()( e.proto_base().child5, s, d ) , typename Grammar::proto_child6::template impl< typename result_of::child_c<Expr, 6>::type , State , Data >()( e.proto_base().child6, s, d ) , typename Grammar::proto_child7::template impl< typename result_of::child_c<Expr, 7>::type , State , Data >()( e.proto_base().child7, s, d ) , typename Grammar::proto_child8::template impl< typename result_of::child_c<Expr, 8>::type , State , Data >()( e.proto_base().child8, s, d ) , typename Grammar::proto_child9::template impl< typename result_of::child_c<Expr, 9>::type , State , Data >()( e.proto_base().child9, s, d )
            };
            
            
            
            detail::ignore_unused(&that);
            return proto_generator()(that);
        }
    };
