        ///////////////////////////////////////////////////////////////////////////////
        /// \file expr.hpp
        /// Contains definition of expr\<\> class template.
        //
        //  Copyright 2008 Eric Niebler. Distributed under the Boost
        //  Software License, Version 1.0. (See accompanying file
        //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
        
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag, typename Arg0>
    struct expr<Tag, term<Arg0>, 0>
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 0;
        typedef mpl::long_<0 > proto_arity;
        typedef expr proto_base_expr;
        typedef term<Arg0> proto_args;
        typedef basic_expr<Tag, proto_args, 0 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0;
        typedef void proto_child1; typedef void proto_child2; typedef void proto_child3; typedef void proto_child4; typedef void proto_child5; typedef void proto_child6; typedef void proto_child7; typedef void proto_child8; typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        static expr const make(A0 &a0)
        {
            return detail::make_terminal(a0, static_cast<expr *>(0), static_cast<proto_args *>(0));
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0)
        {
            return detail::make_terminal(a0, static_cast<expr *>(0), static_cast<proto_args *>(0));
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a)
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a)
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr &>, 1> const
        operator ()()
        {
            proto::expr<proto::tag::function, list1<expr &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0)
        {
            return result_of::funop1<
                expr
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1)
        {
            return result_of::funop2<
                expr
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2)
        {
            return result_of::funop3<
                expr
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3)
        {
            return result_of::funop4<
                expr
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4)
        {
            return result_of::funop5<
                expr
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5)
        {
            return result_of::funop6<
                expr
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6)
        {
            return result_of::funop7<
                expr
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7)
        {
            return result_of::funop8<
                expr
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8)
        {
            return result_of::funop9<
                expr
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0>
    struct expr<Tag, list1<Arg0>, 1 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 1;
        typedef mpl::long_<1 > proto_arity;
        typedef expr proto_base_expr;
        typedef list1<Arg0> proto_args;
        typedef basic_expr<Tag, proto_args, 1 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0;
        typedef void proto_child1; typedef void proto_child2; typedef void proto_child3; typedef void proto_child4; typedef void proto_child5; typedef void proto_child6; typedef void proto_child7; typedef void proto_child8; typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0)
        {
            expr that = {a0};
            return that;
        }
        
        
        
        typedef typename detail::address_of_hack<Tag, proto_child0>::type address_of_hack_type_;
        
        
        
        
        
        
        
        BOOST_FORCEINLINE
        operator address_of_hack_type_() const
        {
            return boost::addressof(this->child0);
        }
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0 , typename Arg1>
    struct expr<Tag, list2<Arg0 , Arg1>, 2 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 2;
        typedef mpl::long_<2 > proto_arity;
        typedef expr proto_base_expr;
        typedef list2<Arg0 , Arg1> proto_args;
        typedef basic_expr<Tag, proto_args, 2 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0; typedef Arg1 proto_child1; proto_child1 child1;
        typedef void proto_child2; typedef void proto_child3; typedef void proto_child4; typedef void proto_child5; typedef void proto_child6; typedef void proto_child7; typedef void proto_child8; typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0 , A1 const &a1)
        {
            expr that = {a0 , a1};
            return that;
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0 , typename Arg1 , typename Arg2>
    struct expr<Tag, list3<Arg0 , Arg1 , Arg2>, 3 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 3;
        typedef mpl::long_<3 > proto_arity;
        typedef expr proto_base_expr;
        typedef list3<Arg0 , Arg1 , Arg2> proto_args;
        typedef basic_expr<Tag, proto_args, 3 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0; typedef Arg1 proto_child1; proto_child1 child1; typedef Arg2 proto_child2; proto_child2 child2;
        typedef void proto_child3; typedef void proto_child4; typedef void proto_child5; typedef void proto_child6; typedef void proto_child7; typedef void proto_child8; typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0 , A1 const &a1 , A2 const &a2)
        {
            expr that = {a0 , a1 , a2};
            return that;
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0 , typename Arg1 , typename Arg2 , typename Arg3>
    struct expr<Tag, list4<Arg0 , Arg1 , Arg2 , Arg3>, 4 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 4;
        typedef mpl::long_<4 > proto_arity;
        typedef expr proto_base_expr;
        typedef list4<Arg0 , Arg1 , Arg2 , Arg3> proto_args;
        typedef basic_expr<Tag, proto_args, 4 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0; typedef Arg1 proto_child1; proto_child1 child1; typedef Arg2 proto_child2; proto_child2 child2; typedef Arg3 proto_child3; proto_child3 child3;
        typedef void proto_child4; typedef void proto_child5; typedef void proto_child6; typedef void proto_child7; typedef void proto_child8; typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3)
        {
            expr that = {a0 , a1 , a2 , a3};
            return that;
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0 , typename Arg1 , typename Arg2 , typename Arg3 , typename Arg4>
    struct expr<Tag, list5<Arg0 , Arg1 , Arg2 , Arg3 , Arg4>, 5 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 5;
        typedef mpl::long_<5 > proto_arity;
        typedef expr proto_base_expr;
        typedef list5<Arg0 , Arg1 , Arg2 , Arg3 , Arg4> proto_args;
        typedef basic_expr<Tag, proto_args, 5 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0; typedef Arg1 proto_child1; proto_child1 child1; typedef Arg2 proto_child2; proto_child2 child2; typedef Arg3 proto_child3; proto_child3 child3; typedef Arg4 proto_child4; proto_child4 child4;
        typedef void proto_child5; typedef void proto_child6; typedef void proto_child7; typedef void proto_child8; typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4)
        {
            expr that = {a0 , a1 , a2 , a3 , a4};
            return that;
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0 , typename Arg1 , typename Arg2 , typename Arg3 , typename Arg4 , typename Arg5>
    struct expr<Tag, list6<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5>, 6 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 6;
        typedef mpl::long_<6 > proto_arity;
        typedef expr proto_base_expr;
        typedef list6<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5> proto_args;
        typedef basic_expr<Tag, proto_args, 6 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0; typedef Arg1 proto_child1; proto_child1 child1; typedef Arg2 proto_child2; proto_child2 child2; typedef Arg3 proto_child3; proto_child3 child3; typedef Arg4 proto_child4; proto_child4 child4; typedef Arg5 proto_child5; proto_child5 child5;
        typedef void proto_child6; typedef void proto_child7; typedef void proto_child8; typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5)
        {
            expr that = {a0 , a1 , a2 , a3 , a4 , a5};
            return that;
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0 , typename Arg1 , typename Arg2 , typename Arg3 , typename Arg4 , typename Arg5 , typename Arg6>
    struct expr<Tag, list7<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5 , Arg6>, 7 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 7;
        typedef mpl::long_<7 > proto_arity;
        typedef expr proto_base_expr;
        typedef list7<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5 , Arg6> proto_args;
        typedef basic_expr<Tag, proto_args, 7 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0; typedef Arg1 proto_child1; proto_child1 child1; typedef Arg2 proto_child2; proto_child2 child2; typedef Arg3 proto_child3; proto_child3 child3; typedef Arg4 proto_child4; proto_child4 child4; typedef Arg5 proto_child5; proto_child5 child5; typedef Arg6 proto_child6; proto_child6 child6;
        typedef void proto_child7; typedef void proto_child8; typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6)
        {
            expr that = {a0 , a1 , a2 , a3 , a4 , a5 , a6};
            return that;
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0 , typename Arg1 , typename Arg2 , typename Arg3 , typename Arg4 , typename Arg5 , typename Arg6 , typename Arg7>
    struct expr<Tag, list8<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5 , Arg6 , Arg7>, 8 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 8;
        typedef mpl::long_<8 > proto_arity;
        typedef expr proto_base_expr;
        typedef list8<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5 , Arg6 , Arg7> proto_args;
        typedef basic_expr<Tag, proto_args, 8 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0; typedef Arg1 proto_child1; proto_child1 child1; typedef Arg2 proto_child2; proto_child2 child2; typedef Arg3 proto_child3; proto_child3 child3; typedef Arg4 proto_child4; proto_child4 child4; typedef Arg5 proto_child5; proto_child5 child5; typedef Arg6 proto_child6; proto_child6 child6; typedef Arg7 proto_child7; proto_child7 child7;
        typedef void proto_child8; typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7)
        {
            expr that = {a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7};
            return that;
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0 , typename Arg1 , typename Arg2 , typename Arg3 , typename Arg4 , typename Arg5 , typename Arg6 , typename Arg7 , typename Arg8>
    struct expr<Tag, list9<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5 , Arg6 , Arg7 , Arg8>, 9 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 9;
        typedef mpl::long_<9 > proto_arity;
        typedef expr proto_base_expr;
        typedef list9<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5 , Arg6 , Arg7 , Arg8> proto_args;
        typedef basic_expr<Tag, proto_args, 9 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0; typedef Arg1 proto_child1; proto_child1 child1; typedef Arg2 proto_child2; proto_child2 child2; typedef Arg3 proto_child3; proto_child3 child3; typedef Arg4 proto_child4; proto_child4 child4; typedef Arg5 proto_child5; proto_child5 child5; typedef Arg6 proto_child6; proto_child6 child6; typedef Arg7 proto_child7; proto_child7 child7; typedef Arg8 proto_child8; proto_child8 child8;
        typedef void proto_child9;
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8)
        {
            expr that = {a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8};
            return that;
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Tag , typename Arg0 , typename Arg1 , typename Arg2 , typename Arg3 , typename Arg4 , typename Arg5 , typename Arg6 , typename Arg7 , typename Arg8 , typename Arg9>
    struct expr<Tag, list10<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5 , Arg6 , Arg7 , Arg8 , Arg9>, 10 >
    {
        typedef Tag proto_tag;
        static const long proto_arity_c = 10;
        typedef mpl::long_<10 > proto_arity;
        typedef expr proto_base_expr;
        typedef list10<Arg0 , Arg1 , Arg2 , Arg3 , Arg4 , Arg5 , Arg6 , Arg7 , Arg8 , Arg9> proto_args;
        typedef basic_expr<Tag, proto_args, 10 > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr<Tag, proto_domain> fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; 
        typedef Arg0 proto_child0; proto_child0 child0; typedef Arg1 proto_child1; proto_child1 child1; typedef Arg2 proto_child2; proto_child2 child2; typedef Arg3 proto_child3; proto_child3 child3; typedef Arg4 proto_child4; proto_child4 child4; typedef Arg5 proto_child5; proto_child5 child5; typedef Arg6 proto_child6; proto_child6 child6; typedef Arg7 proto_child7; proto_child7 child7; typedef Arg8 proto_child8; proto_child8 child8; typedef Arg9 proto_child9; proto_child9 child9;
        
        
        
        BOOST_FORCEINLINE
        expr const &proto_base() const
        {
            return *this;
        }
        
        
        BOOST_FORCEINLINE
        expr &proto_base()
        {
            return *this;
        }
        
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
        BOOST_FORCEINLINE
        static expr const make(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8 , A9 const &a9)
        {
            expr that = {a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9};
            return that;
        }
        
        
        typedef detail::not_a_valid_type address_of_hack_type_;
        
        
        
        
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename A>
        BOOST_FORCEINLINE
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
        
        
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };
        
        
        
        BOOST_FORCEINLINE
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }
        
        
        template<typename A0>
        BOOST_FORCEINLINE
        typename result_of::funop1<
            expr const
          , default_domain , const A0
        >::type const
        operator ()(A0 const &a0) const
        {
            return result_of::funop1<
                expr const
              , default_domain , const A0
            >::call(*this , a0);
        }
        
        
        template<typename A0 , typename A1>
        BOOST_FORCEINLINE
        typename result_of::funop2<
            expr const
          , default_domain , const A0 , const A1
        >::type const
        operator ()(A0 const &a0 , A1 const &a1) const
        {
            return result_of::funop2<
                expr const
              , default_domain , const A0 , const A1
            >::call(*this , a0 , a1);
        }
        
        
        template<typename A0 , typename A1 , typename A2>
        BOOST_FORCEINLINE
        typename result_of::funop3<
            expr const
          , default_domain , const A0 , const A1 , const A2
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2) const
        {
            return result_of::funop3<
                expr const
              , default_domain , const A0 , const A1 , const A2
            >::call(*this , a0 , a1 , a2);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3>
        BOOST_FORCEINLINE
        typename result_of::funop4<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3) const
        {
            return result_of::funop4<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3
            >::call(*this , a0 , a1 , a2 , a3);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        BOOST_FORCEINLINE
        typename result_of::funop5<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4) const
        {
            return result_of::funop5<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4
            >::call(*this , a0 , a1 , a2 , a3 , a4);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        BOOST_FORCEINLINE
        typename result_of::funop6<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5) const
        {
            return result_of::funop6<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        BOOST_FORCEINLINE
        typename result_of::funop7<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6) const
        {
            return result_of::funop7<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        BOOST_FORCEINLINE
        typename result_of::funop8<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7) const
        {
            return result_of::funop8<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
        }
        
        
        template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        BOOST_FORCEINLINE
        typename result_of::funop9<
            expr const
          , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >::type const
        operator ()(A0 const &a0 , A1 const &a1 , A2 const &a2 , A3 const &a3 , A4 const &a4 , A5 const &a5 , A6 const &a6 , A7 const &a7 , A8 const &a8) const
        {
            return result_of::funop9<
                expr const
              , default_domain , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
            >::call(*this , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
        }
    };
