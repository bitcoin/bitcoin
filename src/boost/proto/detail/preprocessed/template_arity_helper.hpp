    ///////////////////////////////////////////////////////////////////////////////
    // template_arity_helper.hpp
    // Overloads of template_arity_helper, used by the template_arity\<\> class template
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<
        template<typename P0> class F
      , typename T0
    >
    sized_type<2>::type
    template_arity_helper(F<T0> **, mpl::int_<1> *);
    template<
        template<typename P0 , typename P1> class F
      , typename T0 , typename T1
    >
    sized_type<3>::type
    template_arity_helper(F<T0 , T1> **, mpl::int_<2> *);
    template<
        template<typename P0 , typename P1 , typename P2> class F
      , typename T0 , typename T1 , typename T2
    >
    sized_type<4>::type
    template_arity_helper(F<T0 , T1 , T2> **, mpl::int_<3> *);
    template<
        template<typename P0 , typename P1 , typename P2 , typename P3> class F
      , typename T0 , typename T1 , typename T2 , typename T3
    >
    sized_type<5>::type
    template_arity_helper(F<T0 , T1 , T2 , T3> **, mpl::int_<4> *);
    template<
        template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4> class F
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4
    >
    sized_type<6>::type
    template_arity_helper(F<T0 , T1 , T2 , T3 , T4> **, mpl::int_<5> *);
    template<
        template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5> class F
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5
    >
    sized_type<7>::type
    template_arity_helper(F<T0 , T1 , T2 , T3 , T4 , T5> **, mpl::int_<6> *);
    template<
        template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6> class F
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6
    >
    sized_type<8>::type
    template_arity_helper(F<T0 , T1 , T2 , T3 , T4 , T5 , T6> **, mpl::int_<7> *);
    template<
        template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6 , typename P7> class F
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7
    >
    sized_type<9>::type
    template_arity_helper(F<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7> **, mpl::int_<8> *);
    template<
        template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6 , typename P7 , typename P8> class F
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8
    >
    sized_type<10>::type
    template_arity_helper(F<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8> **, mpl::int_<9> *);
    template<
        template<typename P0 , typename P1 , typename P2 , typename P3 , typename P4 , typename P5 , typename P6 , typename P7 , typename P8 , typename P9> class F
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9
    >
    sized_type<11>::type
    template_arity_helper(F<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9> **, mpl::int_<10> *);
