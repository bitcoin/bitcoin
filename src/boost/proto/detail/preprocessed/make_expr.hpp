    ///////////////////////////////////////////////////////////////////////////////
    /// \file make_expr.hpp
    /// Contains overloads of make_expr() free function.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    
    
    template<typename Tag , typename A0 , typename A1>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            , const A0 , const A1
        >
    >::type const
    make_expr(const A0 &a0 , const A1 &a1)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            , const A0 , const A1
        >()(a0 , a1);
    }
    
    
    template<typename Tag, typename Domain , typename C0 , typename C1>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        , const C0 , const C1
    >::type const
    make_expr(const C0 &c0 , const C1 &c1)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            , const C0 , const C1
        >()(c0 , c1);
    }
    
    
    template<typename Tag , typename A0 , typename A1 , typename A2>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            , const A0 , const A1 , const A2
        >
    >::type const
    make_expr(const A0 &a0 , const A1 &a1 , const A2 &a2)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            , const A0 , const A1 , const A2
        >()(a0 , a1 , a2);
    }
    
    
    template<typename Tag, typename Domain , typename C0 , typename C1 , typename C2>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        , const C0 , const C1 , const C2
    >::type const
    make_expr(const C0 &c0 , const C1 &c1 , const C2 &c2)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            , const C0 , const C1 , const C2
        >()(c0 , c1 , c2);
    }
    
    
    template<typename Tag , typename A0 , typename A1 , typename A2 , typename A3>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            , const A0 , const A1 , const A2 , const A3
        >
    >::type const
    make_expr(const A0 &a0 , const A1 &a1 , const A2 &a2 , const A3 &a3)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            , const A0 , const A1 , const A2 , const A3
        >()(a0 , a1 , a2 , a3);
    }
    
    
    template<typename Tag, typename Domain , typename C0 , typename C1 , typename C2 , typename C3>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        , const C0 , const C1 , const C2 , const C3
    >::type const
    make_expr(const C0 &c0 , const C1 &c1 , const C2 &c2 , const C3 &c3)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            , const C0 , const C1 , const C2 , const C3
        >()(c0 , c1 , c2 , c3);
    }
    
    
    template<typename Tag , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            , const A0 , const A1 , const A2 , const A3 , const A4
        >
    >::type const
    make_expr(const A0 &a0 , const A1 &a1 , const A2 &a2 , const A3 &a3 , const A4 &a4)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            , const A0 , const A1 , const A2 , const A3 , const A4
        >()(a0 , a1 , a2 , a3 , a4);
    }
    
    
    template<typename Tag, typename Domain , typename C0 , typename C1 , typename C2 , typename C3 , typename C4>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        , const C0 , const C1 , const C2 , const C3 , const C4
    >::type const
    make_expr(const C0 &c0 , const C1 &c1 , const C2 &c2 , const C3 &c3 , const C4 &c4)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            , const C0 , const C1 , const C2 , const C3 , const C4
        >()(c0 , c1 , c2 , c3 , c4);
    }
    
    
    template<typename Tag , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >
    >::type const
    make_expr(const A0 &a0 , const A1 &a1 , const A2 &a2 , const A3 &a3 , const A4 &a4 , const A5 &a5)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5
        >()(a0 , a1 , a2 , a3 , a4 , a5);
    }
    
    
    template<typename Tag, typename Domain , typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        , const C0 , const C1 , const C2 , const C3 , const C4 , const C5
    >::type const
    make_expr(const C0 &c0 , const C1 &c1 , const C2 &c2 , const C3 &c3 , const C4 &c4 , const C5 &c5)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            , const C0 , const C1 , const C2 , const C3 , const C4 , const C5
        >()(c0 , c1 , c2 , c3 , c4 , c5);
    }
    
    
    template<typename Tag , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >
    >::type const
    make_expr(const A0 &a0 , const A1 &a1 , const A2 &a2 , const A3 &a3 , const A4 &a4 , const A5 &a5 , const A6 &a6)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6
        >()(a0 , a1 , a2 , a3 , a4 , a5 , a6);
    }
    
    
    template<typename Tag, typename Domain , typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        , const C0 , const C1 , const C2 , const C3 , const C4 , const C5 , const C6
    >::type const
    make_expr(const C0 &c0 , const C1 &c1 , const C2 &c2 , const C3 &c3 , const C4 &c4 , const C5 &c5 , const C6 &c6)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            , const C0 , const C1 , const C2 , const C3 , const C4 , const C5 , const C6
        >()(c0 , c1 , c2 , c3 , c4 , c5 , c6);
    }
    
    
    template<typename Tag , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >
    >::type const
    make_expr(const A0 &a0 , const A1 &a1 , const A2 &a2 , const A3 &a3 , const A4 &a4 , const A5 &a5 , const A6 &a6 , const A7 &a7)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7
        >()(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
    }
    
    
    template<typename Tag, typename Domain , typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6 , typename C7>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        , const C0 , const C1 , const C2 , const C3 , const C4 , const C5 , const C6 , const C7
    >::type const
    make_expr(const C0 &c0 , const C1 &c1 , const C2 &c2 , const C3 &c3 , const C4 &c4 , const C5 &c5 , const C6 &c6 , const C7 &c7)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            , const C0 , const C1 , const C2 , const C3 , const C4 , const C5 , const C6 , const C7
        >()(c0 , c1 , c2 , c3 , c4 , c5 , c6 , c7);
    }
    
    
    template<typename Tag , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >
    >::type const
    make_expr(const A0 &a0 , const A1 &a1 , const A2 &a2 , const A3 &a3 , const A4 &a4 , const A5 &a5 , const A6 &a6 , const A7 &a7 , const A8 &a8)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8
        >()(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
    }
    
    
    template<typename Tag, typename Domain , typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6 , typename C7 , typename C8>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        , const C0 , const C1 , const C2 , const C3 , const C4 , const C5 , const C6 , const C7 , const C8
    >::type const
    make_expr(const C0 &c0 , const C1 &c1 , const C2 &c2 , const C3 &c3 , const C4 &c4 , const C5 &c5 , const C6 &c6 , const C7 &c7 , const C8 &c8)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            , const C0 , const C1 , const C2 , const C3 , const C4 , const C5 , const C6 , const C7 , const C8
        >()(c0 , c1 , c2 , c3 , c4 , c5 , c6 , c7 , c8);
    }
    
    
    template<typename Tag , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    BOOST_FORCEINLINE
    typename lazy_disable_if<
        is_domain<A0>
      , result_of::make_expr<
            Tag
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8 , const A9
        >
    >::type const
    make_expr(const A0 &a0 , const A1 &a1 , const A2 &a2 , const A3 &a3 , const A4 &a4 , const A5 &a5 , const A6 &a6 , const A7 &a7 , const A8 &a8 , const A9 &a9)
    {
        return proto::detail::make_expr_<
            Tag
          , deduce_domain
            , const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8 , const A9
        >()(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9);
    }
    
    
    template<typename Tag, typename Domain , typename C0 , typename C1 , typename C2 , typename C3 , typename C4 , typename C5 , typename C6 , typename C7 , typename C8 , typename C9>
    BOOST_FORCEINLINE
    typename result_of::make_expr<
        Tag
      , Domain
        , const C0 , const C1 , const C2 , const C3 , const C4 , const C5 , const C6 , const C7 , const C8 , const C9
    >::type const
    make_expr(const C0 &c0 , const C1 &c1 , const C2 &c2 , const C3 &c3 , const C4 &c4 , const C5 &c5 , const C6 &c6 , const C7 &c7 , const C8 &c8 , const C9 &c9)
    {
        return proto::detail::make_expr_<
            Tag
          , Domain
            , const C0 , const C1 , const C2 , const C3 , const C4 , const C5 , const C6 , const C7 , const C8 , const C9
        >()(c0 , c1 , c2 , c3 , c4 , c5 , c6 , c7 , c8 , c9);
    }
