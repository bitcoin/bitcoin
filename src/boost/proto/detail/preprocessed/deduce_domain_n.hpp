    ///////////////////////////////////////////////////////////////////////////////
    // deduce_domain_n.hpp
    // Definitions of common_domain[n] and deduce_domain[n] class templates.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<typename A0 , typename A1 , typename A2>
    struct common_domain3
    {
        typedef A0 common1;
        typedef typename common_domain2<common1, A1>::type common2; typedef typename common_domain2<common2, A2>::type common3;
        typedef common3 type;
        BOOST_PROTO_ASSERT_VALID_DOMAIN(type);
    };
    template<typename E0 , typename E1 , typename E2>
    struct deduce_domain3
      : common_domain3<
            typename domain_of<E0 >::type , typename domain_of<E1 >::type , typename domain_of<E2 >::type
        >
    {};
    template<typename A0 , typename A1 , typename A2 , typename A3>
    struct common_domain4
    {
        typedef A0 common1;
        typedef typename common_domain2<common1, A1>::type common2; typedef typename common_domain2<common2, A2>::type common3; typedef typename common_domain2<common3, A3>::type common4;
        typedef common4 type;
        BOOST_PROTO_ASSERT_VALID_DOMAIN(type);
    };
    template<typename E0 , typename E1 , typename E2 , typename E3>
    struct deduce_domain4
      : common_domain4<
            typename domain_of<E0 >::type , typename domain_of<E1 >::type , typename domain_of<E2 >::type , typename domain_of<E3 >::type
        >
    {};
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct common_domain5
    {
        typedef A0 common1;
        typedef typename common_domain2<common1, A1>::type common2; typedef typename common_domain2<common2, A2>::type common3; typedef typename common_domain2<common3, A3>::type common4; typedef typename common_domain2<common4, A4>::type common5;
        typedef common5 type;
        BOOST_PROTO_ASSERT_VALID_DOMAIN(type);
    };
    template<typename E0 , typename E1 , typename E2 , typename E3 , typename E4>
    struct deduce_domain5
      : common_domain5<
            typename domain_of<E0 >::type , typename domain_of<E1 >::type , typename domain_of<E2 >::type , typename domain_of<E3 >::type , typename domain_of<E4 >::type
        >
    {};
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct common_domain6
    {
        typedef A0 common1;
        typedef typename common_domain2<common1, A1>::type common2; typedef typename common_domain2<common2, A2>::type common3; typedef typename common_domain2<common3, A3>::type common4; typedef typename common_domain2<common4, A4>::type common5; typedef typename common_domain2<common5, A5>::type common6;
        typedef common6 type;
        BOOST_PROTO_ASSERT_VALID_DOMAIN(type);
    };
    template<typename E0 , typename E1 , typename E2 , typename E3 , typename E4 , typename E5>
    struct deduce_domain6
      : common_domain6<
            typename domain_of<E0 >::type , typename domain_of<E1 >::type , typename domain_of<E2 >::type , typename domain_of<E3 >::type , typename domain_of<E4 >::type , typename domain_of<E5 >::type
        >
    {};
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct common_domain7
    {
        typedef A0 common1;
        typedef typename common_domain2<common1, A1>::type common2; typedef typename common_domain2<common2, A2>::type common3; typedef typename common_domain2<common3, A3>::type common4; typedef typename common_domain2<common4, A4>::type common5; typedef typename common_domain2<common5, A5>::type common6; typedef typename common_domain2<common6, A6>::type common7;
        typedef common7 type;
        BOOST_PROTO_ASSERT_VALID_DOMAIN(type);
    };
    template<typename E0 , typename E1 , typename E2 , typename E3 , typename E4 , typename E5 , typename E6>
    struct deduce_domain7
      : common_domain7<
            typename domain_of<E0 >::type , typename domain_of<E1 >::type , typename domain_of<E2 >::type , typename domain_of<E3 >::type , typename domain_of<E4 >::type , typename domain_of<E5 >::type , typename domain_of<E6 >::type
        >
    {};
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct common_domain8
    {
        typedef A0 common1;
        typedef typename common_domain2<common1, A1>::type common2; typedef typename common_domain2<common2, A2>::type common3; typedef typename common_domain2<common3, A3>::type common4; typedef typename common_domain2<common4, A4>::type common5; typedef typename common_domain2<common5, A5>::type common6; typedef typename common_domain2<common6, A6>::type common7; typedef typename common_domain2<common7, A7>::type common8;
        typedef common8 type;
        BOOST_PROTO_ASSERT_VALID_DOMAIN(type);
    };
    template<typename E0 , typename E1 , typename E2 , typename E3 , typename E4 , typename E5 , typename E6 , typename E7>
    struct deduce_domain8
      : common_domain8<
            typename domain_of<E0 >::type , typename domain_of<E1 >::type , typename domain_of<E2 >::type , typename domain_of<E3 >::type , typename domain_of<E4 >::type , typename domain_of<E5 >::type , typename domain_of<E6 >::type , typename domain_of<E7 >::type
        >
    {};
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct common_domain9
    {
        typedef A0 common1;
        typedef typename common_domain2<common1, A1>::type common2; typedef typename common_domain2<common2, A2>::type common3; typedef typename common_domain2<common3, A3>::type common4; typedef typename common_domain2<common4, A4>::type common5; typedef typename common_domain2<common5, A5>::type common6; typedef typename common_domain2<common6, A6>::type common7; typedef typename common_domain2<common7, A7>::type common8; typedef typename common_domain2<common8, A8>::type common9;
        typedef common9 type;
        BOOST_PROTO_ASSERT_VALID_DOMAIN(type);
    };
    template<typename E0 , typename E1 , typename E2 , typename E3 , typename E4 , typename E5 , typename E6 , typename E7 , typename E8>
    struct deduce_domain9
      : common_domain9<
            typename domain_of<E0 >::type , typename domain_of<E1 >::type , typename domain_of<E2 >::type , typename domain_of<E3 >::type , typename domain_of<E4 >::type , typename domain_of<E5 >::type , typename domain_of<E6 >::type , typename domain_of<E7 >::type , typename domain_of<E8 >::type
        >
    {};
    template<typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct common_domain10
    {
        typedef A0 common1;
        typedef typename common_domain2<common1, A1>::type common2; typedef typename common_domain2<common2, A2>::type common3; typedef typename common_domain2<common3, A3>::type common4; typedef typename common_domain2<common4, A4>::type common5; typedef typename common_domain2<common5, A5>::type common6; typedef typename common_domain2<common6, A6>::type common7; typedef typename common_domain2<common7, A7>::type common8; typedef typename common_domain2<common8, A8>::type common9; typedef typename common_domain2<common9, A9>::type common10;
        typedef common10 type;
        BOOST_PROTO_ASSERT_VALID_DOMAIN(type);
    };
    template<typename E0 , typename E1 , typename E2 , typename E3 , typename E4 , typename E5 , typename E6 , typename E7 , typename E8 , typename E9>
    struct deduce_domain10
      : common_domain10<
            typename domain_of<E0 >::type , typename domain_of<E1 >::type , typename domain_of<E2 >::type , typename domain_of<E3 >::type , typename domain_of<E4 >::type , typename domain_of<E5 >::type , typename domain_of<E6 >::type , typename domain_of<E7 >::type , typename domain_of<E8 >::type , typename domain_of<E9 >::type
        >
    {};
