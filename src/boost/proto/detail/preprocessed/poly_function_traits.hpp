    ///////////////////////////////////////////////////////////////////////////////
    // poly_function_traits.hpp
    // Contains specializations of poly_function_traits and as_mono_function
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    
    template<typename PolyFun , typename A0>
    struct poly_function_traits<PolyFun, PolyFun(A0), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0>
    struct as_mono_function_impl<PolyFun(A0), true>
    {
        typedef typename PolyFun::template impl<const A0> type;
    };
    
    template<typename PolyFun , typename A0>
    struct as_mono_function_impl<PolyFun(A0), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0>
    struct as_mono_function<PolyFun(A0)>
      : as_mono_function_impl<PolyFun(A0), is_poly_function<PolyFun>::value>
    {};
    
    template<typename PolyFun , typename A0 , typename A1>
    struct poly_function_traits<PolyFun, PolyFun(A0 , A1), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0 , const A1> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0 , typename A1>
    struct as_mono_function_impl<PolyFun(A0 , A1), true>
    {
        typedef typename PolyFun::template impl<const A0 , const A1> type;
    };
    
    template<typename PolyFun , typename A0 , typename A1>
    struct as_mono_function_impl<PolyFun(A0 , A1), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0 , typename A1>
    struct as_mono_function<PolyFun(A0 , A1)>
      : as_mono_function_impl<PolyFun(A0 , A1), is_poly_function<PolyFun>::value>
    {};
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2>
    struct poly_function_traits<PolyFun, PolyFun(A0 , A1 , A2), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2), true>
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2> type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2>
    struct as_mono_function<PolyFun(A0 , A1 , A2)>
      : as_mono_function_impl<PolyFun(A0 , A1 , A2), is_poly_function<PolyFun>::value>
    {};
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3>
    struct poly_function_traits<PolyFun, PolyFun(A0 , A1 , A2 , A3), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3), true>
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3> type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3>
    struct as_mono_function<PolyFun(A0 , A1 , A2 , A3)>
      : as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3), is_poly_function<PolyFun>::value>
    {};
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct poly_function_traits<PolyFun, PolyFun(A0 , A1 , A2 , A3 , A4), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4), true>
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4> type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct as_mono_function<PolyFun(A0 , A1 , A2 , A3 , A4)>
      : as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4), is_poly_function<PolyFun>::value>
    {};
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct poly_function_traits<PolyFun, PolyFun(A0 , A1 , A2 , A3 , A4 , A5), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5), true>
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5> type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct as_mono_function<PolyFun(A0 , A1 , A2 , A3 , A4 , A5)>
      : as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5), is_poly_function<PolyFun>::value>
    {};
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct poly_function_traits<PolyFun, PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6), true>
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6> type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct as_mono_function<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
      : as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6), is_poly_function<PolyFun>::value>
    {};
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct poly_function_traits<PolyFun, PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7), true>
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7> type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct as_mono_function<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
      : as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7), is_poly_function<PolyFun>::value>
    {};
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct poly_function_traits<PolyFun, PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8), true>
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8> type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct as_mono_function<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
      : as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8), is_poly_function<PolyFun>::value>
    {};
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct poly_function_traits<PolyFun, PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9), mpl::size_t<sizeof(poly_function_t)> >
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8 , const A9> function_type;
        typedef typename function_type::result_type result_type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9), true>
    {
        typedef typename PolyFun::template impl<const A0 , const A1 , const A2 , const A3 , const A4 , const A5 , const A6 , const A7 , const A8 , const A9> type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9), false>
    {
        typedef PolyFun type;
    };
    
    template<typename PolyFun , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct as_mono_function<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
      : as_mono_function_impl<PolyFun(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9), is_poly_function<PolyFun>::value>
    {};
