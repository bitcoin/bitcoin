    ///////////////////////////////////////////////////////////////////////////////
    /// \file deep_copy.hpp
    /// Replace all nodes stored by reference by nodes stored by value.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<typename Expr>
    struct deep_copy_impl<Expr, 1>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list1<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0)
            };
            return proto_generator()(that);
        }
    };
    template<typename Expr>
    struct deep_copy_impl<Expr, 2>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list2<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child1 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0) , proto::deep_copy(e.proto_base().child1)
            };
            return proto_generator()(that);
        }
    };
    template<typename Expr>
    struct deep_copy_impl<Expr, 3>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list3<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child1 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child2 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0) , proto::deep_copy(e.proto_base().child1) , proto::deep_copy(e.proto_base().child2)
            };
            return proto_generator()(that);
        }
    };
    template<typename Expr>
    struct deep_copy_impl<Expr, 4>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list4<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child1 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child2 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child3 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0) , proto::deep_copy(e.proto_base().child1) , proto::deep_copy(e.proto_base().child2) , proto::deep_copy(e.proto_base().child3)
            };
            return proto_generator()(that);
        }
    };
    template<typename Expr>
    struct deep_copy_impl<Expr, 5>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list5<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child1 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child2 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child3 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child4 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0) , proto::deep_copy(e.proto_base().child1) , proto::deep_copy(e.proto_base().child2) , proto::deep_copy(e.proto_base().child3) , proto::deep_copy(e.proto_base().child4)
            };
            return proto_generator()(that);
        }
    };
    template<typename Expr>
    struct deep_copy_impl<Expr, 6>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list6<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child1 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child2 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child3 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child4 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child5 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0) , proto::deep_copy(e.proto_base().child1) , proto::deep_copy(e.proto_base().child2) , proto::deep_copy(e.proto_base().child3) , proto::deep_copy(e.proto_base().child4) , proto::deep_copy(e.proto_base().child5)
            };
            return proto_generator()(that);
        }
    };
    template<typename Expr>
    struct deep_copy_impl<Expr, 7>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list7<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child1 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child2 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child3 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child4 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child5 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child6 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0) , proto::deep_copy(e.proto_base().child1) , proto::deep_copy(e.proto_base().child2) , proto::deep_copy(e.proto_base().child3) , proto::deep_copy(e.proto_base().child4) , proto::deep_copy(e.proto_base().child5) , proto::deep_copy(e.proto_base().child6)
            };
            return proto_generator()(that);
        }
    };
    template<typename Expr>
    struct deep_copy_impl<Expr, 8>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list8<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child1 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child2 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child3 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child4 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child5 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child6 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child7 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0) , proto::deep_copy(e.proto_base().child1) , proto::deep_copy(e.proto_base().child2) , proto::deep_copy(e.proto_base().child3) , proto::deep_copy(e.proto_base().child4) , proto::deep_copy(e.proto_base().child5) , proto::deep_copy(e.proto_base().child6) , proto::deep_copy(e.proto_base().child7)
            };
            return proto_generator()(that);
        }
    };
    template<typename Expr>
    struct deep_copy_impl<Expr, 9>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list9<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child1 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child2 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child3 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child4 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child5 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child6 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child7 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child8 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0) , proto::deep_copy(e.proto_base().child1) , proto::deep_copy(e.proto_base().child2) , proto::deep_copy(e.proto_base().child3) , proto::deep_copy(e.proto_base().child4) , proto::deep_copy(e.proto_base().child5) , proto::deep_copy(e.proto_base().child6) , proto::deep_copy(e.proto_base().child7) , proto::deep_copy(e.proto_base().child8)
            };
            return proto_generator()(that);
        }
    };
    template<typename Expr>
    struct deep_copy_impl<Expr, 10>
    {
        typedef
            typename base_expr<
                typename Expr::proto_domain
              , typename Expr::proto_tag
              , list10<
                    typename deep_copy_impl< typename remove_reference< typename Expr::proto_child0 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child1 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child2 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child3 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child4 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child5 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child6 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child7 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child8 >::type::proto_derived_expr >::result_type , typename deep_copy_impl< typename remove_reference< typename Expr::proto_child9 >::type::proto_derived_expr >::result_type
                >
            >::type
        expr_type;
        typedef typename Expr::proto_generator proto_generator;
        typedef typename proto_generator::template result<proto_generator(expr_type)>::type result_type;
        template<typename Expr2, typename S, typename D>
        result_type operator()(Expr2 const &e, S const &, D const &) const
        {
            expr_type const that = {
                proto::deep_copy(e.proto_base().child0) , proto::deep_copy(e.proto_base().child1) , proto::deep_copy(e.proto_base().child2) , proto::deep_copy(e.proto_base().child3) , proto::deep_copy(e.proto_base().child4) , proto::deep_copy(e.proto_base().child5) , proto::deep_copy(e.proto_base().child6) , proto::deep_copy(e.proto_base().child7) , proto::deep_copy(e.proto_base().child8) , proto::deep_copy(e.proto_base().child9)
            };
            return proto_generator()(that);
        }
    };
