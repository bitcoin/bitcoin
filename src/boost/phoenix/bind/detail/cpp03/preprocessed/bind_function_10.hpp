/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
    
    
    
    
    
    
    
    template <
        typename RT
      , typename T0 , typename T1
      , typename A0 , typename A1
    >
    inline
    typename detail::expression::function_eval<
        detail::function_ptr<
            2
          , RT
          , RT(*)(T0 , T1)
        >
      , A0 , A1
    >::type const
    bind(
        RT(*f)(T0 , T1)
      , A0 const& a0 , A1 const& a1
    )
    {
        typedef detail::function_ptr<
            2
          , RT
          , RT(*)(T0 , T1)
        > fp_type;
        return
            detail::expression::function_eval<
                fp_type
              , A0 , A1
            >::make(
                fp_type(f)
              , a0 , a1
            );
    }
    
    
    
    
    
    
    
    template <
        typename RT
      , typename T0 , typename T1 , typename T2
      , typename A0 , typename A1 , typename A2
    >
    inline
    typename detail::expression::function_eval<
        detail::function_ptr<
            3
          , RT
          , RT(*)(T0 , T1 , T2)
        >
      , A0 , A1 , A2
    >::type const
    bind(
        RT(*f)(T0 , T1 , T2)
      , A0 const& a0 , A1 const& a1 , A2 const& a2
    )
    {
        typedef detail::function_ptr<
            3
          , RT
          , RT(*)(T0 , T1 , T2)
        > fp_type;
        return
            detail::expression::function_eval<
                fp_type
              , A0 , A1 , A2
            >::make(
                fp_type(f)
              , a0 , a1 , a2
            );
    }
    
    
    
    
    
    
    
    template <
        typename RT
      , typename T0 , typename T1 , typename T2 , typename T3
      , typename A0 , typename A1 , typename A2 , typename A3
    >
    inline
    typename detail::expression::function_eval<
        detail::function_ptr<
            4
          , RT
          , RT(*)(T0 , T1 , T2 , T3)
        >
      , A0 , A1 , A2 , A3
    >::type const
    bind(
        RT(*f)(T0 , T1 , T2 , T3)
      , A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3
    )
    {
        typedef detail::function_ptr<
            4
          , RT
          , RT(*)(T0 , T1 , T2 , T3)
        > fp_type;
        return
            detail::expression::function_eval<
                fp_type
              , A0 , A1 , A2 , A3
            >::make(
                fp_type(f)
              , a0 , a1 , a2 , a3
            );
    }
    
    
    
    
    
    
    
    template <
        typename RT
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4
      , typename A0 , typename A1 , typename A2 , typename A3 , typename A4
    >
    inline
    typename detail::expression::function_eval<
        detail::function_ptr<
            5
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4)
        >
      , A0 , A1 , A2 , A3 , A4
    >::type const
    bind(
        RT(*f)(T0 , T1 , T2 , T3 , T4)
      , A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4
    )
    {
        typedef detail::function_ptr<
            5
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4)
        > fp_type;
        return
            detail::expression::function_eval<
                fp_type
              , A0 , A1 , A2 , A3 , A4
            >::make(
                fp_type(f)
              , a0 , a1 , a2 , a3 , a4
            );
    }
    
    
    
    
    
    
    
    template <
        typename RT
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5
      , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5
    >
    inline
    typename detail::expression::function_eval<
        detail::function_ptr<
            6
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4 , T5)
        >
      , A0 , A1 , A2 , A3 , A4 , A5
    >::type const
    bind(
        RT(*f)(T0 , T1 , T2 , T3 , T4 , T5)
      , A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5
    )
    {
        typedef detail::function_ptr<
            6
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4 , T5)
        > fp_type;
        return
            detail::expression::function_eval<
                fp_type
              , A0 , A1 , A2 , A3 , A4 , A5
            >::make(
                fp_type(f)
              , a0 , a1 , a2 , a3 , a4 , a5
            );
    }
    
    
    
    
    
    
    
    template <
        typename RT
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6
      , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6
    >
    inline
    typename detail::expression::function_eval<
        detail::function_ptr<
            7
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4 , T5 , T6)
        >
      , A0 , A1 , A2 , A3 , A4 , A5 , A6
    >::type const
    bind(
        RT(*f)(T0 , T1 , T2 , T3 , T4 , T5 , T6)
      , A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6
    )
    {
        typedef detail::function_ptr<
            7
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4 , T5 , T6)
        > fp_type;
        return
            detail::expression::function_eval<
                fp_type
              , A0 , A1 , A2 , A3 , A4 , A5 , A6
            >::make(
                fp_type(f)
              , a0 , a1 , a2 , a3 , a4 , a5 , a6
            );
    }
    
    
    
    
    
    
    
    template <
        typename RT
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7
      , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7
    >
    inline
    typename detail::expression::function_eval<
        detail::function_ptr<
            8
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7)
        >
      , A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7
    >::type const
    bind(
        RT(*f)(T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7)
      , A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7
    )
    {
        typedef detail::function_ptr<
            8
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7)
        > fp_type;
        return
            detail::expression::function_eval<
                fp_type
              , A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7
            >::make(
                fp_type(f)
              , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7
            );
    }
    
    
    
    
    
    
    
    template <
        typename RT
      , typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8
      , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8
    >
    inline
    typename detail::expression::function_eval<
        detail::function_ptr<
            9
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8)
        >
      , A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8
    >::type const
    bind(
        RT(*f)(T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8)
      , A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7 , A8 const& a8
    )
    {
        typedef detail::function_ptr<
            9
          , RT
          , RT(*)(T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8)
        > fp_type;
        return
            detail::expression::function_eval<
                fp_type
              , A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8
            >::make(
                fp_type(f)
              , a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8
            );
    }
