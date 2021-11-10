    ///////////////////////////////////////////////////////////////////////////////
    /// \file lambda_matches.hpp
    /// Specializations of the lambda_matches template
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<
        template<typename , typename> class T
        , typename Expr0 , typename Expr1
        , typename Grammar0 , typename Grammar1
    >
    struct lambda_matches<
        T<Expr0 , Expr1>
      , T<Grammar0 , Grammar1>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(2)
    >
      : and_2<
            lambda_matches< Expr0 , Grammar0 >::value,
            lambda_matches< Expr1 , Grammar1 >
        >
    {};
    template<
        template<typename , typename , typename> class T
        , typename Expr0 , typename Expr1 , typename Expr2
        , typename Grammar0 , typename Grammar1 , typename Grammar2
    >
    struct lambda_matches<
        T<Expr0 , Expr1 , Expr2>
      , T<Grammar0 , Grammar1 , Grammar2>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(3)
    >
      : and_3<
            lambda_matches< Expr0 , Grammar0 >::value,
            lambda_matches< Expr1 , Grammar1 > , lambda_matches< Expr2 , Grammar2 >
        >
    {};
    template<
        template<typename , typename , typename , typename> class T
        , typename Expr0 , typename Expr1 , typename Expr2 , typename Expr3
        , typename Grammar0 , typename Grammar1 , typename Grammar2 , typename Grammar3
    >
    struct lambda_matches<
        T<Expr0 , Expr1 , Expr2 , Expr3>
      , T<Grammar0 , Grammar1 , Grammar2 , Grammar3>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(4)
    >
      : and_4<
            lambda_matches< Expr0 , Grammar0 >::value,
            lambda_matches< Expr1 , Grammar1 > , lambda_matches< Expr2 , Grammar2 > , lambda_matches< Expr3 , Grammar3 >
        >
    {};
    template<
        template<typename , typename , typename , typename , typename> class T
        , typename Expr0 , typename Expr1 , typename Expr2 , typename Expr3 , typename Expr4
        , typename Grammar0 , typename Grammar1 , typename Grammar2 , typename Grammar3 , typename Grammar4
    >
    struct lambda_matches<
        T<Expr0 , Expr1 , Expr2 , Expr3 , Expr4>
      , T<Grammar0 , Grammar1 , Grammar2 , Grammar3 , Grammar4>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(5)
    >
      : and_5<
            lambda_matches< Expr0 , Grammar0 >::value,
            lambda_matches< Expr1 , Grammar1 > , lambda_matches< Expr2 , Grammar2 > , lambda_matches< Expr3 , Grammar3 > , lambda_matches< Expr4 , Grammar4 >
        >
    {};
    template<
        template<typename , typename , typename , typename , typename , typename> class T
        , typename Expr0 , typename Expr1 , typename Expr2 , typename Expr3 , typename Expr4 , typename Expr5
        , typename Grammar0 , typename Grammar1 , typename Grammar2 , typename Grammar3 , typename Grammar4 , typename Grammar5
    >
    struct lambda_matches<
        T<Expr0 , Expr1 , Expr2 , Expr3 , Expr4 , Expr5>
      , T<Grammar0 , Grammar1 , Grammar2 , Grammar3 , Grammar4 , Grammar5>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(6)
    >
      : and_6<
            lambda_matches< Expr0 , Grammar0 >::value,
            lambda_matches< Expr1 , Grammar1 > , lambda_matches< Expr2 , Grammar2 > , lambda_matches< Expr3 , Grammar3 > , lambda_matches< Expr4 , Grammar4 > , lambda_matches< Expr5 , Grammar5 >
        >
    {};
    template<
        template<typename , typename , typename , typename , typename , typename , typename> class T
        , typename Expr0 , typename Expr1 , typename Expr2 , typename Expr3 , typename Expr4 , typename Expr5 , typename Expr6
        , typename Grammar0 , typename Grammar1 , typename Grammar2 , typename Grammar3 , typename Grammar4 , typename Grammar5 , typename Grammar6
    >
    struct lambda_matches<
        T<Expr0 , Expr1 , Expr2 , Expr3 , Expr4 , Expr5 , Expr6>
      , T<Grammar0 , Grammar1 , Grammar2 , Grammar3 , Grammar4 , Grammar5 , Grammar6>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(7)
    >
      : and_7<
            lambda_matches< Expr0 , Grammar0 >::value,
            lambda_matches< Expr1 , Grammar1 > , lambda_matches< Expr2 , Grammar2 > , lambda_matches< Expr3 , Grammar3 > , lambda_matches< Expr4 , Grammar4 > , lambda_matches< Expr5 , Grammar5 > , lambda_matches< Expr6 , Grammar6 >
        >
    {};
    template<
        template<typename , typename , typename , typename , typename , typename , typename , typename> class T
        , typename Expr0 , typename Expr1 , typename Expr2 , typename Expr3 , typename Expr4 , typename Expr5 , typename Expr6 , typename Expr7
        , typename Grammar0 , typename Grammar1 , typename Grammar2 , typename Grammar3 , typename Grammar4 , typename Grammar5 , typename Grammar6 , typename Grammar7
    >
    struct lambda_matches<
        T<Expr0 , Expr1 , Expr2 , Expr3 , Expr4 , Expr5 , Expr6 , Expr7>
      , T<Grammar0 , Grammar1 , Grammar2 , Grammar3 , Grammar4 , Grammar5 , Grammar6 , Grammar7>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(8)
    >
      : and_8<
            lambda_matches< Expr0 , Grammar0 >::value,
            lambda_matches< Expr1 , Grammar1 > , lambda_matches< Expr2 , Grammar2 > , lambda_matches< Expr3 , Grammar3 > , lambda_matches< Expr4 , Grammar4 > , lambda_matches< Expr5 , Grammar5 > , lambda_matches< Expr6 , Grammar6 > , lambda_matches< Expr7 , Grammar7 >
        >
    {};
    template<
        template<typename , typename , typename , typename , typename , typename , typename , typename , typename> class T
        , typename Expr0 , typename Expr1 , typename Expr2 , typename Expr3 , typename Expr4 , typename Expr5 , typename Expr6 , typename Expr7 , typename Expr8
        , typename Grammar0 , typename Grammar1 , typename Grammar2 , typename Grammar3 , typename Grammar4 , typename Grammar5 , typename Grammar6 , typename Grammar7 , typename Grammar8
    >
    struct lambda_matches<
        T<Expr0 , Expr1 , Expr2 , Expr3 , Expr4 , Expr5 , Expr6 , Expr7 , Expr8>
      , T<Grammar0 , Grammar1 , Grammar2 , Grammar3 , Grammar4 , Grammar5 , Grammar6 , Grammar7 , Grammar8>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(9)
    >
      : and_9<
            lambda_matches< Expr0 , Grammar0 >::value,
            lambda_matches< Expr1 , Grammar1 > , lambda_matches< Expr2 , Grammar2 > , lambda_matches< Expr3 , Grammar3 > , lambda_matches< Expr4 , Grammar4 > , lambda_matches< Expr5 , Grammar5 > , lambda_matches< Expr6 , Grammar6 > , lambda_matches< Expr7 , Grammar7 > , lambda_matches< Expr8 , Grammar8 >
        >
    {};
    template<
        template<typename , typename , typename , typename , typename , typename , typename , typename , typename , typename> class T
        , typename Expr0 , typename Expr1 , typename Expr2 , typename Expr3 , typename Expr4 , typename Expr5 , typename Expr6 , typename Expr7 , typename Expr8 , typename Expr9
        , typename Grammar0 , typename Grammar1 , typename Grammar2 , typename Grammar3 , typename Grammar4 , typename Grammar5 , typename Grammar6 , typename Grammar7 , typename Grammar8 , typename Grammar9
    >
    struct lambda_matches<
        T<Expr0 , Expr1 , Expr2 , Expr3 , Expr4 , Expr5 , Expr6 , Expr7 , Expr8 , Expr9>
      , T<Grammar0 , Grammar1 , Grammar2 , Grammar3 , Grammar4 , Grammar5 , Grammar6 , Grammar7 , Grammar8 , Grammar9>
        BOOST_PROTO_TEMPLATE_ARITY_PARAM(10)
    >
      : and_10<
            lambda_matches< Expr0 , Grammar0 >::value,
            lambda_matches< Expr1 , Grammar1 > , lambda_matches< Expr2 , Grammar2 > , lambda_matches< Expr3 , Grammar3 > , lambda_matches< Expr4 , Grammar4 > , lambda_matches< Expr5 , Grammar5 > , lambda_matches< Expr6 , Grammar6 > , lambda_matches< Expr7 , Grammar7 > , lambda_matches< Expr8 , Grammar8 > , lambda_matches< Expr9 , Grammar9 >
        >
    {};
