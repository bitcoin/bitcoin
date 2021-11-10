    ///////////////////////////////////////////////////////////////////////////////
    /// \file default_function_impl.hpp
    /// Contains definition of the default_function_impl, the implementation of the
    /// _default transform for function-like nodes.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<typename Grammar, typename Expr, typename State, typename Data>
    struct default_function_impl<Grammar, Expr, State, Data, 3>
      : transform_impl<Expr, State, Data>
    {
        typedef typename result_of::child_c< Expr, 0>::type e0; typedef typename Grammar::template impl<e0, State, Data>::result_type r0; typedef typename result_of::child_c< Expr, 1>::type e1; typedef typename Grammar::template impl<e1, State, Data>::result_type r1; typedef typename result_of::child_c< Expr, 2>::type e2; typedef typename Grammar::template impl<e2, State, Data>::result_type r2;
        typedef
            typename proto::detail::result_of_fixup<r0>::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(r1 , r2)
            >::type
        result_type;
        result_type operator ()(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
        ) const
        {
            return this->invoke(e, s, d, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::false_
        ) const
        {
            return typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )(
                typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ) , typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d )
            );
        }
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::true_
        ) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ))) ->* 
                typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )
            )(typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ));
        }
    };
    template<typename Grammar, typename Expr, typename State, typename Data>
    struct default_function_impl<Grammar, Expr, State, Data, 4>
      : transform_impl<Expr, State, Data>
    {
        typedef typename result_of::child_c< Expr, 0>::type e0; typedef typename Grammar::template impl<e0, State, Data>::result_type r0; typedef typename result_of::child_c< Expr, 1>::type e1; typedef typename Grammar::template impl<e1, State, Data>::result_type r1; typedef typename result_of::child_c< Expr, 2>::type e2; typedef typename Grammar::template impl<e2, State, Data>::result_type r2; typedef typename result_of::child_c< Expr, 3>::type e3; typedef typename Grammar::template impl<e3, State, Data>::result_type r3;
        typedef
            typename proto::detail::result_of_fixup<r0>::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(r1 , r2 , r3)
            >::type
        result_type;
        result_type operator ()(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
        ) const
        {
            return this->invoke(e, s, d, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::false_
        ) const
        {
            return typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )(
                typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ) , typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d )
            );
        }
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::true_
        ) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ))) ->* 
                typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )
            )(typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ));
        }
    };
    template<typename Grammar, typename Expr, typename State, typename Data>
    struct default_function_impl<Grammar, Expr, State, Data, 5>
      : transform_impl<Expr, State, Data>
    {
        typedef typename result_of::child_c< Expr, 0>::type e0; typedef typename Grammar::template impl<e0, State, Data>::result_type r0; typedef typename result_of::child_c< Expr, 1>::type e1; typedef typename Grammar::template impl<e1, State, Data>::result_type r1; typedef typename result_of::child_c< Expr, 2>::type e2; typedef typename Grammar::template impl<e2, State, Data>::result_type r2; typedef typename result_of::child_c< Expr, 3>::type e3; typedef typename Grammar::template impl<e3, State, Data>::result_type r3; typedef typename result_of::child_c< Expr, 4>::type e4; typedef typename Grammar::template impl<e4, State, Data>::result_type r4;
        typedef
            typename proto::detail::result_of_fixup<r0>::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(r1 , r2 , r3 , r4)
            >::type
        result_type;
        result_type operator ()(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
        ) const
        {
            return this->invoke(e, s, d, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::false_
        ) const
        {
            return typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )(
                typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ) , typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d )
            );
        }
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::true_
        ) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ))) ->* 
                typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )
            )(typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ));
        }
    };
    template<typename Grammar, typename Expr, typename State, typename Data>
    struct default_function_impl<Grammar, Expr, State, Data, 6>
      : transform_impl<Expr, State, Data>
    {
        typedef typename result_of::child_c< Expr, 0>::type e0; typedef typename Grammar::template impl<e0, State, Data>::result_type r0; typedef typename result_of::child_c< Expr, 1>::type e1; typedef typename Grammar::template impl<e1, State, Data>::result_type r1; typedef typename result_of::child_c< Expr, 2>::type e2; typedef typename Grammar::template impl<e2, State, Data>::result_type r2; typedef typename result_of::child_c< Expr, 3>::type e3; typedef typename Grammar::template impl<e3, State, Data>::result_type r3; typedef typename result_of::child_c< Expr, 4>::type e4; typedef typename Grammar::template impl<e4, State, Data>::result_type r4; typedef typename result_of::child_c< Expr, 5>::type e5; typedef typename Grammar::template impl<e5, State, Data>::result_type r5;
        typedef
            typename proto::detail::result_of_fixup<r0>::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(r1 , r2 , r3 , r4 , r5)
            >::type
        result_type;
        result_type operator ()(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
        ) const
        {
            return this->invoke(e, s, d, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::false_
        ) const
        {
            return typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )(
                typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ) , typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d )
            );
        }
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::true_
        ) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ))) ->* 
                typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )
            )(typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d ));
        }
    };
    template<typename Grammar, typename Expr, typename State, typename Data>
    struct default_function_impl<Grammar, Expr, State, Data, 7>
      : transform_impl<Expr, State, Data>
    {
        typedef typename result_of::child_c< Expr, 0>::type e0; typedef typename Grammar::template impl<e0, State, Data>::result_type r0; typedef typename result_of::child_c< Expr, 1>::type e1; typedef typename Grammar::template impl<e1, State, Data>::result_type r1; typedef typename result_of::child_c< Expr, 2>::type e2; typedef typename Grammar::template impl<e2, State, Data>::result_type r2; typedef typename result_of::child_c< Expr, 3>::type e3; typedef typename Grammar::template impl<e3, State, Data>::result_type r3; typedef typename result_of::child_c< Expr, 4>::type e4; typedef typename Grammar::template impl<e4, State, Data>::result_type r4; typedef typename result_of::child_c< Expr, 5>::type e5; typedef typename Grammar::template impl<e5, State, Data>::result_type r5; typedef typename result_of::child_c< Expr, 6>::type e6; typedef typename Grammar::template impl<e6, State, Data>::result_type r6;
        typedef
            typename proto::detail::result_of_fixup<r0>::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(r1 , r2 , r3 , r4 , r5 , r6)
            >::type
        result_type;
        result_type operator ()(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
        ) const
        {
            return this->invoke(e, s, d, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::false_
        ) const
        {
            return typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )(
                typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ) , typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d ) , typename Grammar::template impl<e6, State, Data>()( proto::child_c< 6>( e), s, d )
            );
        }
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::true_
        ) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ))) ->* 
                typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )
            )(typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d ) , typename Grammar::template impl<e6, State, Data>()( proto::child_c< 6>( e), s, d ));
        }
    };
    template<typename Grammar, typename Expr, typename State, typename Data>
    struct default_function_impl<Grammar, Expr, State, Data, 8>
      : transform_impl<Expr, State, Data>
    {
        typedef typename result_of::child_c< Expr, 0>::type e0; typedef typename Grammar::template impl<e0, State, Data>::result_type r0; typedef typename result_of::child_c< Expr, 1>::type e1; typedef typename Grammar::template impl<e1, State, Data>::result_type r1; typedef typename result_of::child_c< Expr, 2>::type e2; typedef typename Grammar::template impl<e2, State, Data>::result_type r2; typedef typename result_of::child_c< Expr, 3>::type e3; typedef typename Grammar::template impl<e3, State, Data>::result_type r3; typedef typename result_of::child_c< Expr, 4>::type e4; typedef typename Grammar::template impl<e4, State, Data>::result_type r4; typedef typename result_of::child_c< Expr, 5>::type e5; typedef typename Grammar::template impl<e5, State, Data>::result_type r5; typedef typename result_of::child_c< Expr, 6>::type e6; typedef typename Grammar::template impl<e6, State, Data>::result_type r6; typedef typename result_of::child_c< Expr, 7>::type e7; typedef typename Grammar::template impl<e7, State, Data>::result_type r7;
        typedef
            typename proto::detail::result_of_fixup<r0>::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(r1 , r2 , r3 , r4 , r5 , r6 , r7)
            >::type
        result_type;
        result_type operator ()(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
        ) const
        {
            return this->invoke(e, s, d, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::false_
        ) const
        {
            return typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )(
                typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ) , typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d ) , typename Grammar::template impl<e6, State, Data>()( proto::child_c< 6>( e), s, d ) , typename Grammar::template impl<e7, State, Data>()( proto::child_c< 7>( e), s, d )
            );
        }
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::true_
        ) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ))) ->* 
                typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )
            )(typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d ) , typename Grammar::template impl<e6, State, Data>()( proto::child_c< 6>( e), s, d ) , typename Grammar::template impl<e7, State, Data>()( proto::child_c< 7>( e), s, d ));
        }
    };
    template<typename Grammar, typename Expr, typename State, typename Data>
    struct default_function_impl<Grammar, Expr, State, Data, 9>
      : transform_impl<Expr, State, Data>
    {
        typedef typename result_of::child_c< Expr, 0>::type e0; typedef typename Grammar::template impl<e0, State, Data>::result_type r0; typedef typename result_of::child_c< Expr, 1>::type e1; typedef typename Grammar::template impl<e1, State, Data>::result_type r1; typedef typename result_of::child_c< Expr, 2>::type e2; typedef typename Grammar::template impl<e2, State, Data>::result_type r2; typedef typename result_of::child_c< Expr, 3>::type e3; typedef typename Grammar::template impl<e3, State, Data>::result_type r3; typedef typename result_of::child_c< Expr, 4>::type e4; typedef typename Grammar::template impl<e4, State, Data>::result_type r4; typedef typename result_of::child_c< Expr, 5>::type e5; typedef typename Grammar::template impl<e5, State, Data>::result_type r5; typedef typename result_of::child_c< Expr, 6>::type e6; typedef typename Grammar::template impl<e6, State, Data>::result_type r6; typedef typename result_of::child_c< Expr, 7>::type e7; typedef typename Grammar::template impl<e7, State, Data>::result_type r7; typedef typename result_of::child_c< Expr, 8>::type e8; typedef typename Grammar::template impl<e8, State, Data>::result_type r8;
        typedef
            typename proto::detail::result_of_fixup<r0>::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(r1 , r2 , r3 , r4 , r5 , r6 , r7 , r8)
            >::type
        result_type;
        result_type operator ()(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
        ) const
        {
            return this->invoke(e, s, d, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::false_
        ) const
        {
            return typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )(
                typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ) , typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d ) , typename Grammar::template impl<e6, State, Data>()( proto::child_c< 6>( e), s, d ) , typename Grammar::template impl<e7, State, Data>()( proto::child_c< 7>( e), s, d ) , typename Grammar::template impl<e8, State, Data>()( proto::child_c< 8>( e), s, d )
            );
        }
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::true_
        ) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ))) ->* 
                typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )
            )(typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d ) , typename Grammar::template impl<e6, State, Data>()( proto::child_c< 6>( e), s, d ) , typename Grammar::template impl<e7, State, Data>()( proto::child_c< 7>( e), s, d ) , typename Grammar::template impl<e8, State, Data>()( proto::child_c< 8>( e), s, d ));
        }
    };
    template<typename Grammar, typename Expr, typename State, typename Data>
    struct default_function_impl<Grammar, Expr, State, Data, 10>
      : transform_impl<Expr, State, Data>
    {
        typedef typename result_of::child_c< Expr, 0>::type e0; typedef typename Grammar::template impl<e0, State, Data>::result_type r0; typedef typename result_of::child_c< Expr, 1>::type e1; typedef typename Grammar::template impl<e1, State, Data>::result_type r1; typedef typename result_of::child_c< Expr, 2>::type e2; typedef typename Grammar::template impl<e2, State, Data>::result_type r2; typedef typename result_of::child_c< Expr, 3>::type e3; typedef typename Grammar::template impl<e3, State, Data>::result_type r3; typedef typename result_of::child_c< Expr, 4>::type e4; typedef typename Grammar::template impl<e4, State, Data>::result_type r4; typedef typename result_of::child_c< Expr, 5>::type e5; typedef typename Grammar::template impl<e5, State, Data>::result_type r5; typedef typename result_of::child_c< Expr, 6>::type e6; typedef typename Grammar::template impl<e6, State, Data>::result_type r6; typedef typename result_of::child_c< Expr, 7>::type e7; typedef typename Grammar::template impl<e7, State, Data>::result_type r7; typedef typename result_of::child_c< Expr, 8>::type e8; typedef typename Grammar::template impl<e8, State, Data>::result_type r8; typedef typename result_of::child_c< Expr, 9>::type e9; typedef typename Grammar::template impl<e9, State, Data>::result_type r9;
        typedef
            typename proto::detail::result_of_fixup<r0>::type
        function_type;
        typedef
            typename BOOST_PROTO_RESULT_OF<
                function_type(r1 , r2 , r3 , r4 , r5 , r6 , r7 , r8 , r9)
            >::type
        result_type;
        result_type operator ()(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
        ) const
        {
            return this->invoke(e, s, d, is_member_function_pointer<function_type>());
        }
    private:
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::false_
        ) const
        {
            return typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )(
                typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ) , typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d ) , typename Grammar::template impl<e6, State, Data>()( proto::child_c< 6>( e), s, d ) , typename Grammar::template impl<e7, State, Data>()( proto::child_c< 7>( e), s, d ) , typename Grammar::template impl<e8, State, Data>()( proto::child_c< 8>( e), s, d ) , typename Grammar::template impl<e9, State, Data>()( proto::child_c< 9>( e), s, d )
            );
        }
        result_type invoke(
            typename default_function_impl::expr_param e
          , typename default_function_impl::state_param s
          , typename default_function_impl::data_param d
          , mpl::true_
        ) const
        {
            BOOST_PROTO_USE_GET_POINTER();
            typedef typename detail::class_member_traits<function_type>::class_type class_type;
            return (
                BOOST_PROTO_GET_POINTER(class_type, (typename Grammar::template impl<e1, State, Data>()( proto::child_c< 1>( e), s, d ))) ->* 
                typename Grammar::template impl<e0, State, Data>()( proto::child_c< 0>( e), s, d )
            )(typename Grammar::template impl<e2, State, Data>()( proto::child_c< 2>( e), s, d ) , typename Grammar::template impl<e3, State, Data>()( proto::child_c< 3>( e), s, d ) , typename Grammar::template impl<e4, State, Data>()( proto::child_c< 4>( e), s, d ) , typename Grammar::template impl<e5, State, Data>()( proto::child_c< 5>( e), s, d ) , typename Grammar::template impl<e6, State, Data>()( proto::child_c< 6>( e), s, d ) , typename Grammar::template impl<e7, State, Data>()( proto::child_c< 7>( e), s, d ) , typename Grammar::template impl<e8, State, Data>()( proto::child_c< 8>( e), s, d ) , typename Grammar::template impl<e9, State, Data>()( proto::child_c< 9>( e), s, d ));
        }
    };
