    ///////////////////////////////////////////////////////////////////////////////
    /// \file callable_eval.hpp
    /// Contains specializations of the callable_eval\<\> class template.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 1>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 1>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr)
                );
            }
        };
    }
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 2>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_) , proto::child_c< 1>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 2>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0; typedef typename proto::result_of::child_c< Expr const &, 1>::type child1;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0 , child1
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr) , proto::child_c< 1>( expr)
                );
            }
        };
    }
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 3>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_) , proto::child_c< 1>( sexpr_) , proto::child_c< 2>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 3>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0; typedef typename proto::result_of::child_c< Expr const &, 1>::type child1; typedef typename proto::result_of::child_c< Expr const &, 2>::type child2;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0 , child1 , child2
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr) , proto::child_c< 1>( expr) , proto::child_c< 2>( expr)
                );
            }
        };
    }
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 4>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_) , proto::child_c< 1>( sexpr_) , proto::child_c< 2>( sexpr_) , proto::child_c< 3>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 4>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0; typedef typename proto::result_of::child_c< Expr const &, 1>::type child1; typedef typename proto::result_of::child_c< Expr const &, 2>::type child2; typedef typename proto::result_of::child_c< Expr const &, 3>::type child3;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0 , child1 , child2 , child3
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr) , proto::child_c< 1>( expr) , proto::child_c< 2>( expr) , proto::child_c< 3>( expr)
                );
            }
        };
    }
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 5>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_) , proto::child_c< 1>( sexpr_) , proto::child_c< 2>( sexpr_) , proto::child_c< 3>( sexpr_) , proto::child_c< 4>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 5>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0; typedef typename proto::result_of::child_c< Expr const &, 1>::type child1; typedef typename proto::result_of::child_c< Expr const &, 2>::type child2; typedef typename proto::result_of::child_c< Expr const &, 3>::type child3; typedef typename proto::result_of::child_c< Expr const &, 4>::type child4;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0 , child1 , child2 , child3 , child4
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr) , proto::child_c< 1>( expr) , proto::child_c< 2>( expr) , proto::child_c< 3>( expr) , proto::child_c< 4>( expr)
                );
            }
        };
    }
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 6>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_) , proto::child_c< 1>( sexpr_) , proto::child_c< 2>( sexpr_) , proto::child_c< 3>( sexpr_) , proto::child_c< 4>( sexpr_) , proto::child_c< 5>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 6>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0; typedef typename proto::result_of::child_c< Expr const &, 1>::type child1; typedef typename proto::result_of::child_c< Expr const &, 2>::type child2; typedef typename proto::result_of::child_c< Expr const &, 3>::type child3; typedef typename proto::result_of::child_c< Expr const &, 4>::type child4; typedef typename proto::result_of::child_c< Expr const &, 5>::type child5;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0 , child1 , child2 , child3 , child4 , child5
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr) , proto::child_c< 1>( expr) , proto::child_c< 2>( expr) , proto::child_c< 3>( expr) , proto::child_c< 4>( expr) , proto::child_c< 5>( expr)
                );
            }
        };
    }
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 7>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_) , proto::child_c< 1>( sexpr_) , proto::child_c< 2>( sexpr_) , proto::child_c< 3>( sexpr_) , proto::child_c< 4>( sexpr_) , proto::child_c< 5>( sexpr_) , proto::child_c< 6>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 7>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0; typedef typename proto::result_of::child_c< Expr const &, 1>::type child1; typedef typename proto::result_of::child_c< Expr const &, 2>::type child2; typedef typename proto::result_of::child_c< Expr const &, 3>::type child3; typedef typename proto::result_of::child_c< Expr const &, 4>::type child4; typedef typename proto::result_of::child_c< Expr const &, 5>::type child5; typedef typename proto::result_of::child_c< Expr const &, 6>::type child6;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0 , child1 , child2 , child3 , child4 , child5 , child6
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr) , proto::child_c< 1>( expr) , proto::child_c< 2>( expr) , proto::child_c< 3>( expr) , proto::child_c< 4>( expr) , proto::child_c< 5>( expr) , proto::child_c< 6>( expr)
                );
            }
        };
    }
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 8>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_) , proto::child_c< 1>( sexpr_) , proto::child_c< 2>( sexpr_) , proto::child_c< 3>( sexpr_) , proto::child_c< 4>( sexpr_) , proto::child_c< 5>( sexpr_) , proto::child_c< 6>( sexpr_) , proto::child_c< 7>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 8>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0; typedef typename proto::result_of::child_c< Expr const &, 1>::type child1; typedef typename proto::result_of::child_c< Expr const &, 2>::type child2; typedef typename proto::result_of::child_c< Expr const &, 3>::type child3; typedef typename proto::result_of::child_c< Expr const &, 4>::type child4; typedef typename proto::result_of::child_c< Expr const &, 5>::type child5; typedef typename proto::result_of::child_c< Expr const &, 6>::type child6; typedef typename proto::result_of::child_c< Expr const &, 7>::type child7;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0 , child1 , child2 , child3 , child4 , child5 , child6 , child7
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr) , proto::child_c< 1>( expr) , proto::child_c< 2>( expr) , proto::child_c< 3>( expr) , proto::child_c< 4>( expr) , proto::child_c< 5>( expr) , proto::child_c< 6>( expr) , proto::child_c< 7>( expr)
                );
            }
        };
    }
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 9>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_) , proto::child_c< 1>( sexpr_) , proto::child_c< 2>( sexpr_) , proto::child_c< 3>( sexpr_) , proto::child_c< 4>( sexpr_) , proto::child_c< 5>( sexpr_) , proto::child_c< 6>( sexpr_) , proto::child_c< 7>( sexpr_) , proto::child_c< 8>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 9>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0; typedef typename proto::result_of::child_c< Expr const &, 1>::type child1; typedef typename proto::result_of::child_c< Expr const &, 2>::type child2; typedef typename proto::result_of::child_c< Expr const &, 3>::type child3; typedef typename proto::result_of::child_c< Expr const &, 4>::type child4; typedef typename proto::result_of::child_c< Expr const &, 5>::type child5; typedef typename proto::result_of::child_c< Expr const &, 6>::type child6; typedef typename proto::result_of::child_c< Expr const &, 7>::type child7; typedef typename proto::result_of::child_c< Expr const &, 8>::type child8;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0 , child1 , child2 , child3 , child4 , child5 , child6 , child7 , child8
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr) , proto::child_c< 1>( expr) , proto::child_c< 2>( expr) , proto::child_c< 3>( expr) , proto::child_c< 4>( expr) , proto::child_c< 5>( expr) , proto::child_c< 6>( expr) , proto::child_c< 7>( expr) , proto::child_c< 8>( expr)
                );
            }
        };
    }
    namespace detail
    {
        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 10>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;
            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(
                            stag_
                            , proto::child_c< 0>( sexpr_) , proto::child_c< 1>( sexpr_) , proto::child_c< 2>( sexpr_) , proto::child_c< 3>( sexpr_) , proto::child_c< 4>( sexpr_) , proto::child_c< 5>( sexpr_) , proto::child_c< 6>( sexpr_) , proto::child_c< 7>( sexpr_) , proto::child_c< 8>( sexpr_) , proto::child_c< 9>( sexpr_)
                        ), 0)
                    )
                );
            typedef mpl::bool_<value> type;
        };
    }
    namespace context
    {
        
        
        
        
        
        
        
        
        
        
        
        
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 10>
        {
            typedef typename proto::result_of::child_c< Expr const &, 0>::type child0; typedef typename proto::result_of::child_c< Expr const &, 1>::type child1; typedef typename proto::result_of::child_c< Expr const &, 2>::type child2; typedef typename proto::result_of::child_c< Expr const &, 3>::type child3; typedef typename proto::result_of::child_c< Expr const &, 4>::type child4; typedef typename proto::result_of::child_c< Expr const &, 5>::type child5; typedef typename proto::result_of::child_c< Expr const &, 6>::type child6; typedef typename proto::result_of::child_c< Expr const &, 7>::type child7; typedef typename proto::result_of::child_c< Expr const &, 8>::type child8; typedef typename proto::result_of::child_c< Expr const &, 9>::type child9;
            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(
                        typename Expr::proto_tag
                        , child0 , child1 , child2 , child3 , child4 , child5 , child6 , child7 , child8 , child9
                    )
                >::type
            result_type;
            
            
            
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(
                    typename Expr::proto_tag()
                    , proto::child_c< 0>( expr) , proto::child_c< 1>( expr) , proto::child_c< 2>( expr) , proto::child_c< 3>( expr) , proto::child_c< 4>( expr) , proto::child_c< 5>( expr) , proto::child_c< 6>( expr) , proto::child_c< 7>( expr) , proto::child_c< 8>( expr) , proto::child_c< 9>( expr)
                );
            }
        };
    }
