    ///////////////////////////////////////////////////////////////////////////////
    /// \file pack_impl.hpp
    /// Contains helpers for pseudo-pack expansion.
    //
    //  Copyright 2012 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
        template<typename Fun, typename Cont>
        struct expand_pattern<1, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret >
        struct expand_pattern_rest_0
        {
            template<typename C0 = void , typename C1 = void , typename C2 = void , typename C3 = void , typename C4 = void , typename C5 = void , typename C6 = void , typename C7 = void , typename C8 = void , typename C9 = void , typename C10 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret( C0)> type;
            };
            template<typename C0 , typename C1>
            struct cat<C0 , C1>
            {
                typedef msvc_fun_workaround<Ret( C0 , C1)> type;
            };
            template<typename C0 , typename C1 , typename C2>
            struct cat<C0 , C1 , C2>
            {
                typedef msvc_fun_workaround<Ret( C0 , C1 , C2)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3>
            struct cat<C0 , C1 , C2 , C3>
            {
                typedef msvc_fun_workaround<Ret( C0 , C1 , C2 , C3)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4>
            struct cat<C0 , C1 , C2 , C3 , C4>
            {
                typedef msvc_fun_workaround<Ret( C0 , C1 , C2 , C3 , C4)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5>
            {
                typedef msvc_fun_workaround<Ret( C0 , C1 , C2 , C3 , C4 , C5)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6>
            {
                typedef msvc_fun_workaround<Ret( C0 , C1 , C2 , C3 , C4 , C5 , C6)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6 , typename C7>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7>
            {
                typedef msvc_fun_workaround<Ret( C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6 , typename C7 , typename C8>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7 , C8>
            {
                typedef msvc_fun_workaround<Ret( C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7 , C8)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6 , typename C7 , typename C8 , typename C9>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7 , C8 , C9>
            {
                typedef msvc_fun_workaround<Ret( C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7 , C8 , C9)> type;
            };
        };
        template<typename Fun, typename Cont>
        struct expand_pattern<2, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 1>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret , typename A0>
        struct expand_pattern_rest_1
        {
            template<typename C0 = void , typename C1 = void , typename C2 = void , typename C3 = void , typename C4 = void , typename C5 = void , typename C6 = void , typename C7 = void , typename C8 = void , typename C9 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret(A0 , C0)> type;
            };
            template<typename C0 , typename C1>
            struct cat<C0 , C1>
            {
                typedef msvc_fun_workaround<Ret(A0 , C0 , C1)> type;
            };
            template<typename C0 , typename C1 , typename C2>
            struct cat<C0 , C1 , C2>
            {
                typedef msvc_fun_workaround<Ret(A0 , C0 , C1 , C2)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3>
            struct cat<C0 , C1 , C2 , C3>
            {
                typedef msvc_fun_workaround<Ret(A0 , C0 , C1 , C2 , C3)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4>
            struct cat<C0 , C1 , C2 , C3 , C4>
            {
                typedef msvc_fun_workaround<Ret(A0 , C0 , C1 , C2 , C3 , C4)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5>
            {
                typedef msvc_fun_workaround<Ret(A0 , C0 , C1 , C2 , C3 , C4 , C5)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6>
            {
                typedef msvc_fun_workaround<Ret(A0 , C0 , C1 , C2 , C3 , C4 , C5 , C6)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6 , typename C7>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7>
            {
                typedef msvc_fun_workaround<Ret(A0 , C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6 , typename C7 , typename C8>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7 , C8>
            {
                typedef msvc_fun_workaround<Ret(A0 , C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7 , C8)> type;
            };
        };
        template<typename Fun, typename Cont>
        struct expand_pattern<3, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 1>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 2>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret , typename A0 , typename A1>
        struct expand_pattern_rest_2
        {
            template<typename C0 = void , typename C1 = void , typename C2 = void , typename C3 = void , typename C4 = void , typename C5 = void , typename C6 = void , typename C7 = void , typename C8 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , C0)> type;
            };
            template<typename C0 , typename C1>
            struct cat<C0 , C1>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , C0 , C1)> type;
            };
            template<typename C0 , typename C1 , typename C2>
            struct cat<C0 , C1 , C2>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , C0 , C1 , C2)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3>
            struct cat<C0 , C1 , C2 , C3>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , C0 , C1 , C2 , C3)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4>
            struct cat<C0 , C1 , C2 , C3 , C4>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , C0 , C1 , C2 , C3 , C4)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , C0 , C1 , C2 , C3 , C4 , C5)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , C0 , C1 , C2 , C3 , C4 , C5 , C6)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6 , typename C7>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , C0 , C1 , C2 , C3 , C4 , C5 , C6 , C7)> type;
            };
        };
        template<typename Fun, typename Cont>
        struct expand_pattern<4, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 1>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 2>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 3>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret , typename A0 , typename A1 , typename A2>
        struct expand_pattern_rest_3
        {
            template<typename C0 = void , typename C1 = void , typename C2 = void , typename C3 = void , typename C4 = void , typename C5 = void , typename C6 = void , typename C7 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , C0)> type;
            };
            template<typename C0 , typename C1>
            struct cat<C0 , C1>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , C0 , C1)> type;
            };
            template<typename C0 , typename C1 , typename C2>
            struct cat<C0 , C1 , C2>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , C0 , C1 , C2)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3>
            struct cat<C0 , C1 , C2 , C3>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , C0 , C1 , C2 , C3)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4>
            struct cat<C0 , C1 , C2 , C3 , C4>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , C0 , C1 , C2 , C3 , C4)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , C0 , C1 , C2 , C3 , C4 , C5)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5 , C6>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , C0 , C1 , C2 , C3 , C4 , C5 , C6)> type;
            };
        };
        template<typename Fun, typename Cont>
        struct expand_pattern<5, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 1>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 2>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 3>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 4>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret , typename A0 , typename A1 , typename A2 , typename A3>
        struct expand_pattern_rest_4
        {
            template<typename C0 = void , typename C1 = void , typename C2 = void , typename C3 = void , typename C4 = void , typename C5 = void , typename C6 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , C0)> type;
            };
            template<typename C0 , typename C1>
            struct cat<C0 , C1>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , C0 , C1)> type;
            };
            template<typename C0 , typename C1 , typename C2>
            struct cat<C0 , C1 , C2>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , C0 , C1 , C2)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3>
            struct cat<C0 , C1 , C2 , C3>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , C0 , C1 , C2 , C3)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4>
            struct cat<C0 , C1 , C2 , C3 , C4>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , C0 , C1 , C2 , C3 , C4)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5>
            struct cat<C0 , C1 , C2 , C3 , C4 , C5>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , C0 , C1 , C2 , C3 , C4 , C5)> type;
            };
        };
        template<typename Fun, typename Cont>
        struct expand_pattern<6, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 1>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 2>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 3>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 4>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 5>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        struct expand_pattern_rest_5
        {
            template<typename C0 = void , typename C1 = void , typename C2 = void , typename C3 = void , typename C4 = void , typename C5 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , C0)> type;
            };
            template<typename C0 , typename C1>
            struct cat<C0 , C1>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , C0 , C1)> type;
            };
            template<typename C0 , typename C1 , typename C2>
            struct cat<C0 , C1 , C2>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , C0 , C1 , C2)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3>
            struct cat<C0 , C1 , C2 , C3>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , C0 , C1 , C2 , C3)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3 , typename C4>
            struct cat<C0 , C1 , C2 , C3 , C4>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , C0 , C1 , C2 , C3 , C4)> type;
            };
        };
        template<typename Fun, typename Cont>
        struct expand_pattern<7, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 1>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 2>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 3>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 4>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 5>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 6>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        struct expand_pattern_rest_6
        {
            template<typename C0 = void , typename C1 = void , typename C2 = void , typename C3 = void , typename C4 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , C0)> type;
            };
            template<typename C0 , typename C1>
            struct cat<C0 , C1>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , C0 , C1)> type;
            };
            template<typename C0 , typename C1 , typename C2>
            struct cat<C0 , C1 , C2>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , C0 , C1 , C2)> type;
            };
            template<typename C0 , typename C1 , typename C2 , typename C3>
            struct cat<C0 , C1 , C2 , C3>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , C0 , C1 , C2 , C3)> type;
            };
        };
        template<typename Fun, typename Cont>
        struct expand_pattern<8, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 1>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 2>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 3>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 4>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 5>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 6>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 7>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        struct expand_pattern_rest_7
        {
            template<typename C0 = void , typename C1 = void , typename C2 = void , typename C3 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , A6 , C0)> type;
            };
            template<typename C0 , typename C1>
            struct cat<C0 , C1>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , A6 , C0 , C1)> type;
            };
            template<typename C0 , typename C1 , typename C2>
            struct cat<C0 , C1 , C2>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , A6 , C0 , C1 , C2)> type;
            };
        };
        template<typename Fun, typename Cont>
        struct expand_pattern<9, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 1>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 2>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 3>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 4>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 5>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 6>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 7>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 8>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        struct expand_pattern_rest_8
        {
            template<typename C0 = void , typename C1 = void , typename C2 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , C0)> type;
            };
            template<typename C0 , typename C1>
            struct cat<C0 , C1>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , C0 , C1)> type;
            };
        };
        template<typename Fun, typename Cont>
        struct expand_pattern<10, Fun, Cont>
          : Cont::template cat<typename expand_pattern_helper<proto::_child_c< 0>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 1>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 2>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 3>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 4>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 5>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 6>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 7>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 8>, Fun>::type , typename expand_pattern_helper<proto::_child_c< 9>, Fun>::type>
        {
            BOOST_MPL_ASSERT_MSG(
                (expand_pattern_helper<proto::_child_c<0>, Fun>::applied::value)
              , NO_PACK_EXPRESSION_FOUND_IN_UNPACKING_PATTERN
              , (Fun)
            );
        };
        template<typename Ret , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        struct expand_pattern_rest_9
        {
            template<typename C0 = void , typename C1 = void>
            struct cat;
            template<typename C0>
            struct cat<C0>
            {
                typedef msvc_fun_workaround<Ret(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , C0)> type;
            };
        };
