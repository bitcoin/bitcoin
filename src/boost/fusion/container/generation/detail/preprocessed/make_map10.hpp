/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion
{
    struct void_;
    namespace result_of
    {
        template <
            typename K0 = void_ , typename K1 = void_ , typename K2 = void_ , typename K3 = void_ , typename K4 = void_ , typename K5 = void_ , typename K6 = void_ , typename K7 = void_ , typename K8 = void_ , typename K9 = void_
          , typename D0 = void_ , typename D1 = void_ , typename D2 = void_ , typename D3 = void_ , typename D4 = void_ , typename D5 = void_ , typename D6 = void_ , typename D7 = void_ , typename D8 = void_ , typename D9 = void_
          , typename Extra = void_
        >
        struct make_map;
        template <>
        struct make_map<>
        {
            typedef map<> type;
        };
    }
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<>
    make_map()
    {
        return map<>();
    }
    namespace result_of
    {
        template <
            typename K0
          , typename D0
        >
        struct make_map<K0, D0 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> > type;
        };
    }
    template <
        typename K0
      , typename D0
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> >
    make_map(D0 const& arg0)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> >(
            fusion::make_pair<K0>(arg0));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1
          , typename D0 , typename D1
        >
        struct make_map<K0 , K1, D0 , D1 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> > type;
        };
    }
    template <
        typename K0 , typename K1
      , typename D0 , typename D1
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> >
    make_map(D0 const& arg0 , D1 const& arg1)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2
          , typename D0 , typename D1 , typename D2
        >
        struct make_map<K0 , K1 , K2, D0 , D1 , D2 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2
      , typename D0 , typename D1 , typename D2
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3
          , typename D0 , typename D1 , typename D2 , typename D3
        >
        struct make_map<K0 , K1 , K2 , K3, D0 , D1 , D2 , D3 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3
      , typename D0 , typename D1 , typename D2 , typename D3
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4
        >
        struct make_map<K0 , K1 , K2 , K3 , K4, D0 , D1 , D2 , D3 , D4 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5, D0 , D1 , D2 , D3 , D4 , D5 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6, D0 , D1 , D2 , D3 , D4 , D5 , D6 , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9));
    }
}}
