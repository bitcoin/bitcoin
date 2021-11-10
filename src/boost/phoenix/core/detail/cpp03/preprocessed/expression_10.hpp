/*=============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Eric Niebler
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
    template <
        template <typename> class Actor
      , typename Tag
      , typename A0 = void , typename A1 = void , typename A2 = void , typename A3 = void , typename A4 = void , typename A5 = void , typename A6 = void , typename A7 = void , typename A8 = void , typename A9 = void
      , typename Dummy = void>
    struct expr_ext;
    template <
        typename Tag
      , typename A0 = void , typename A1 = void , typename A2 = void , typename A3 = void , typename A4 = void , typename A5 = void , typename A6 = void , typename A7 = void , typename A8 = void , typename A9 = void
      , typename Dummy = void
    >
    struct expr : expr_ext<actor, Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9> {};
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0>
    struct expr_ext<Actor, Tag, A0>
        : proto::transform<expr_ext<Actor, Tag, A0>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0;
    };
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0 , typename A1>
    struct expr_ext<Actor, Tag, A0 , A1>
        : proto::transform<expr_ext<Actor, Tag, A0 , A1>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type , typename proto::detail::uncvref<A1>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0 , A1>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0 , typename boost::add_reference<typename boost::add_const<A1>::type>::type a1)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0 , a1)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0; typedef A1 proto_child1;
    };
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0 , typename A1 , typename A2>
    struct expr_ext<Actor, Tag, A0 , A1 , A2>
        : proto::transform<expr_ext<Actor, Tag, A0 , A1 , A2>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type , typename proto::detail::uncvref<A1>::type , typename proto::detail::uncvref<A2>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0 , A1 , A2>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0 , typename boost::add_reference<typename boost::add_const<A1>::type>::type a1 , typename boost::add_reference<typename boost::add_const<A2>::type>::type a2)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0 , a1 , a2)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0; typedef A1 proto_child1; typedef A2 proto_child2;
    };
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0 , typename A1 , typename A2 , typename A3>
    struct expr_ext<Actor, Tag, A0 , A1 , A2 , A3>
        : proto::transform<expr_ext<Actor, Tag, A0 , A1 , A2 , A3>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type , typename proto::detail::uncvref<A1>::type , typename proto::detail::uncvref<A2>::type , typename proto::detail::uncvref<A3>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0 , A1 , A2 , A3>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0 , typename boost::add_reference<typename boost::add_const<A1>::type>::type a1 , typename boost::add_reference<typename boost::add_const<A2>::type>::type a2 , typename boost::add_reference<typename boost::add_const<A3>::type>::type a3)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0 , a1 , a2 , a3)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0; typedef A1 proto_child1; typedef A2 proto_child2; typedef A3 proto_child3;
    };
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4>
        : proto::transform<expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type , typename proto::detail::uncvref<A1>::type , typename proto::detail::uncvref<A2>::type , typename proto::detail::uncvref<A3>::type , typename proto::detail::uncvref<A4>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0 , A1 , A2 , A3 , A4>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0 , typename boost::add_reference<typename boost::add_const<A1>::type>::type a1 , typename boost::add_reference<typename boost::add_const<A2>::type>::type a2 , typename boost::add_reference<typename boost::add_const<A3>::type>::type a3 , typename boost::add_reference<typename boost::add_const<A4>::type>::type a4)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0 , a1 , a2 , a3 , a4)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0; typedef A1 proto_child1; typedef A2 proto_child2; typedef A3 proto_child3; typedef A4 proto_child4;
    };
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5>
        : proto::transform<expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type , typename proto::detail::uncvref<A1>::type , typename proto::detail::uncvref<A2>::type , typename proto::detail::uncvref<A3>::type , typename proto::detail::uncvref<A4>::type , typename proto::detail::uncvref<A5>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0 , A1 , A2 , A3 , A4 , A5>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0 , typename boost::add_reference<typename boost::add_const<A1>::type>::type a1 , typename boost::add_reference<typename boost::add_const<A2>::type>::type a2 , typename boost::add_reference<typename boost::add_const<A3>::type>::type a3 , typename boost::add_reference<typename boost::add_const<A4>::type>::type a4 , typename boost::add_reference<typename boost::add_const<A5>::type>::type a5)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0 , a1 , a2 , a3 , a4 , a5)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0; typedef A1 proto_child1; typedef A2 proto_child2; typedef A3 proto_child3; typedef A4 proto_child4; typedef A5 proto_child5;
    };
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6>
        : proto::transform<expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type , typename proto::detail::uncvref<A1>::type , typename proto::detail::uncvref<A2>::type , typename proto::detail::uncvref<A3>::type , typename proto::detail::uncvref<A4>::type , typename proto::detail::uncvref<A5>::type , typename proto::detail::uncvref<A6>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0 , typename boost::add_reference<typename boost::add_const<A1>::type>::type a1 , typename boost::add_reference<typename boost::add_const<A2>::type>::type a2 , typename boost::add_reference<typename boost::add_const<A3>::type>::type a3 , typename boost::add_reference<typename boost::add_const<A4>::type>::type a4 , typename boost::add_reference<typename boost::add_const<A5>::type>::type a5 , typename boost::add_reference<typename boost::add_const<A6>::type>::type a6)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0 , a1 , a2 , a3 , a4 , a5 , a6)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0; typedef A1 proto_child1; typedef A2 proto_child2; typedef A3 proto_child3; typedef A4 proto_child4; typedef A5 proto_child5; typedef A6 proto_child6;
    };
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7>
        : proto::transform<expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type , typename proto::detail::uncvref<A1>::type , typename proto::detail::uncvref<A2>::type , typename proto::detail::uncvref<A3>::type , typename proto::detail::uncvref<A4>::type , typename proto::detail::uncvref<A5>::type , typename proto::detail::uncvref<A6>::type , typename proto::detail::uncvref<A7>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0 , typename boost::add_reference<typename boost::add_const<A1>::type>::type a1 , typename boost::add_reference<typename boost::add_const<A2>::type>::type a2 , typename boost::add_reference<typename boost::add_const<A3>::type>::type a3 , typename boost::add_reference<typename boost::add_const<A4>::type>::type a4 , typename boost::add_reference<typename boost::add_const<A5>::type>::type a5 , typename boost::add_reference<typename boost::add_const<A6>::type>::type a6 , typename boost::add_reference<typename boost::add_const<A7>::type>::type a7)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0; typedef A1 proto_child1; typedef A2 proto_child2; typedef A3 proto_child3; typedef A4 proto_child4; typedef A5 proto_child5; typedef A6 proto_child6; typedef A7 proto_child7;
    };
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8>
        : proto::transform<expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type , typename proto::detail::uncvref<A1>::type , typename proto::detail::uncvref<A2>::type , typename proto::detail::uncvref<A3>::type , typename proto::detail::uncvref<A4>::type , typename proto::detail::uncvref<A5>::type , typename proto::detail::uncvref<A6>::type , typename proto::detail::uncvref<A7>::type , typename proto::detail::uncvref<A8>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0 , typename boost::add_reference<typename boost::add_const<A1>::type>::type a1 , typename boost::add_reference<typename boost::add_const<A2>::type>::type a2 , typename boost::add_reference<typename boost::add_const<A3>::type>::type a3 , typename boost::add_reference<typename boost::add_const<A4>::type>::type a4 , typename boost::add_reference<typename boost::add_const<A5>::type>::type a5 , typename boost::add_reference<typename boost::add_const<A6>::type>::type a6 , typename boost::add_reference<typename boost::add_const<A7>::type>::type a7 , typename boost::add_reference<typename boost::add_const<A8>::type>::type a8)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0; typedef A1 proto_child1; typedef A2 proto_child2; typedef A3 proto_child3; typedef A4 proto_child4; typedef A5 proto_child5; typedef A6 proto_child6; typedef A7 proto_child7; typedef A8 proto_child8;
    };
    
    
    
    
    
    
    
    template <template <typename> class Actor, typename Tag, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9>
        : proto::transform<expr_ext<Actor, Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9>, int>
    {
        typedef
            typename proto::result_of::make_expr<
                Tag
              , phoenix_default_domain 
              , typename proto::detail::uncvref<A0>::type , typename proto::detail::uncvref<A1>::type , typename proto::detail::uncvref<A2>::type , typename proto::detail::uncvref<A3>::type , typename proto::detail::uncvref<A4>::type , typename proto::detail::uncvref<A5>::type , typename proto::detail::uncvref<A6>::type , typename proto::detail::uncvref<A7>::type , typename proto::detail::uncvref<A8>::type , typename proto::detail::uncvref<A9>::type
            >::type
            base_type;
        typedef Actor<base_type> type;
        typedef
            typename proto::nary_expr<Tag, A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9>::proto_grammar
            proto_grammar;
        static type make(typename boost::add_reference<typename boost::add_const<A0>::type>::type a0 , typename boost::add_reference<typename boost::add_const<A1>::type>::type a1 , typename boost::add_reference<typename boost::add_const<A2>::type>::type a2 , typename boost::add_reference<typename boost::add_const<A3>::type>::type a3 , typename boost::add_reference<typename boost::add_const<A4>::type>::type a4 , typename boost::add_reference<typename boost::add_const<A5>::type>::type a5 , typename boost::add_reference<typename boost::add_const<A6>::type>::type a6 , typename boost::add_reference<typename boost::add_const<A7>::type>::type a7 , typename boost::add_reference<typename boost::add_const<A8>::type>::type a8 , typename boost::add_reference<typename boost::add_const<A9>::type>::type a9)
      { 
        
                actor<base_type> const e =
                {
                    proto::make_expr<
                        Tag
                      , phoenix_default_domain 
                    >(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9)
                };
            return e;
        }
        template<typename Expr, typename State, typename Data>
        struct impl
          : proto::pass_through<expr_ext>::template impl<Expr, State, Data>
        {};
        typedef Tag proto_tag;
        typedef A0 proto_child0; typedef A1 proto_child1; typedef A2 proto_child2; typedef A3 proto_child3; typedef A4 proto_child4; typedef A5 proto_child5; typedef A6 proto_child6; typedef A7 proto_child7; typedef A8 proto_child8; typedef A9 proto_child9;
    };
