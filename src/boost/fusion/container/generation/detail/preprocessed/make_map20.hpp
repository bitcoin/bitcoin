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
            typename K0 = void_ , typename K1 = void_ , typename K2 = void_ , typename K3 = void_ , typename K4 = void_ , typename K5 = void_ , typename K6 = void_ , typename K7 = void_ , typename K8 = void_ , typename K9 = void_ , typename K10 = void_ , typename K11 = void_ , typename K12 = void_ , typename K13 = void_ , typename K14 = void_ , typename K15 = void_ , typename K16 = void_ , typename K17 = void_ , typename K18 = void_ , typename K19 = void_
          , typename D0 = void_ , typename D1 = void_ , typename D2 = void_ , typename D3 = void_ , typename D4 = void_ , typename D5 = void_ , typename D6 = void_ , typename D7 = void_ , typename D8 = void_ , typename D9 = void_ , typename D10 = void_ , typename D11 = void_ , typename D12 = void_ , typename D13 = void_ , typename D14 = void_ , typename D15 = void_ , typename D16 = void_ , typename D17 = void_ , typename D18 = void_ , typename D19 = void_
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
        struct make_map<K0, D0 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
        struct make_map<K0 , K1, D0 , D1 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
        struct make_map<K0 , K1 , K2, D0 , D1 , D2 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
        struct make_map<K0 , K1 , K2 , K3, D0 , D1 , D2 , D3 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
        struct make_map<K0 , K1 , K2 , K3 , K4, D0 , D1 , D2 , D3 , D4 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5, D0 , D1 , D2 , D3 , D4 , D5 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6, D0 , D1 , D2 , D3 , D4 , D5 , D6 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
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
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10 , D11 const& arg11)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10) , fusion::make_pair<K11>(arg11));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10 , D11 const& arg11 , D12 const& arg12)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10) , fusion::make_pair<K11>(arg11) , fusion::make_pair<K12>(arg12));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10 , D11 const& arg11 , D12 const& arg12 , D13 const& arg13)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10) , fusion::make_pair<K11>(arg11) , fusion::make_pair<K12>(arg12) , fusion::make_pair<K13>(arg13));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10 , D11 const& arg11 , D12 const& arg12 , D13 const& arg13 , D14 const& arg14)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10) , fusion::make_pair<K11>(arg11) , fusion::make_pair<K12>(arg12) , fusion::make_pair<K13>(arg13) , fusion::make_pair<K14>(arg14));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15 , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10 , D11 const& arg11 , D12 const& arg12 , D13 const& arg13 , D14 const& arg14 , D15 const& arg15)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10) , fusion::make_pair<K11>(arg11) , fusion::make_pair<K12>(arg12) , fusion::make_pair<K13>(arg13) , fusion::make_pair<K14>(arg14) , fusion::make_pair<K15>(arg15));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15 , K16, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15 , D16 , void_ , void_ , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10 , D11 const& arg11 , D12 const& arg12 , D13 const& arg13 , D14 const& arg14 , D15 const& arg15 , D16 const& arg16)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10) , fusion::make_pair<K11>(arg11) , fusion::make_pair<K12>(arg12) , fusion::make_pair<K13>(arg13) , fusion::make_pair<K14>(arg14) , fusion::make_pair<K15>(arg15) , fusion::make_pair<K16>(arg16));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15 , K16 , K17, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15 , D16 , D17 , void_ , void_ , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> , fusion::pair< K17 , typename detail::as_fusion_element<D17>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> , fusion::pair< K17 , typename detail::as_fusion_element<D17>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10 , D11 const& arg11 , D12 const& arg12 , D13 const& arg13 , D14 const& arg14 , D15 const& arg15 , D16 const& arg16 , D17 const& arg17)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> , fusion::pair< K17 , typename detail::as_fusion_element<D17>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10) , fusion::make_pair<K11>(arg11) , fusion::make_pair<K12>(arg12) , fusion::make_pair<K13>(arg13) , fusion::make_pair<K14>(arg14) , fusion::make_pair<K15>(arg15) , fusion::make_pair<K16>(arg16) , fusion::make_pair<K17>(arg17));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17 , typename K18
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17 , typename D18
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15 , K16 , K17 , K18, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15 , D16 , D17 , D18 , void_ , void_ , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> , fusion::pair< K17 , typename detail::as_fusion_element<D17>::type> , fusion::pair< K18 , typename detail::as_fusion_element<D18>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17 , typename K18
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17 , typename D18
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> , fusion::pair< K17 , typename detail::as_fusion_element<D17>::type> , fusion::pair< K18 , typename detail::as_fusion_element<D18>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10 , D11 const& arg11 , D12 const& arg12 , D13 const& arg13 , D14 const& arg14 , D15 const& arg15 , D16 const& arg16 , D17 const& arg17 , D18 const& arg18)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> , fusion::pair< K17 , typename detail::as_fusion_element<D17>::type> , fusion::pair< K18 , typename detail::as_fusion_element<D18>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10) , fusion::make_pair<K11>(arg11) , fusion::make_pair<K12>(arg12) , fusion::make_pair<K13>(arg13) , fusion::make_pair<K14>(arg14) , fusion::make_pair<K15>(arg15) , fusion::make_pair<K16>(arg16) , fusion::make_pair<K17>(arg17) , fusion::make_pair<K18>(arg18));
    }
    namespace result_of
    {
        template <
            typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17 , typename K18 , typename K19
          , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17 , typename D18 , typename D19
        >
        struct make_map<K0 , K1 , K2 , K3 , K4 , K5 , K6 , K7 , K8 , K9 , K10 , K11 , K12 , K13 , K14 , K15 , K16 , K17 , K18 , K19, D0 , D1 , D2 , D3 , D4 , D5 , D6 , D7 , D8 , D9 , D10 , D11 , D12 , D13 , D14 , D15 , D16 , D17 , D18 , D19 , void_>
        {
            typedef map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> , fusion::pair< K17 , typename detail::as_fusion_element<D17>::type> , fusion::pair< K18 , typename detail::as_fusion_element<D18>::type> , fusion::pair< K19 , typename detail::as_fusion_element<D19>::type> > type;
        };
    }
    template <
        typename K0 , typename K1 , typename K2 , typename K3 , typename K4 , typename K5 , typename K6 , typename K7 , typename K8 , typename K9 , typename K10 , typename K11 , typename K12 , typename K13 , typename K14 , typename K15 , typename K16 , typename K17 , typename K18 , typename K19
      , typename D0 , typename D1 , typename D2 , typename D3 , typename D4 , typename D5 , typename D6 , typename D7 , typename D8 , typename D9 , typename D10 , typename D11 , typename D12 , typename D13 , typename D14 , typename D15 , typename D16 , typename D17 , typename D18 , typename D19
    >
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> , fusion::pair< K17 , typename detail::as_fusion_element<D17>::type> , fusion::pair< K18 , typename detail::as_fusion_element<D18>::type> , fusion::pair< K19 , typename detail::as_fusion_element<D19>::type> >
    make_map(D0 const& arg0 , D1 const& arg1 , D2 const& arg2 , D3 const& arg3 , D4 const& arg4 , D5 const& arg5 , D6 const& arg6 , D7 const& arg7 , D8 const& arg8 , D9 const& arg9 , D10 const& arg10 , D11 const& arg11 , D12 const& arg12 , D13 const& arg13 , D14 const& arg14 , D15 const& arg15 , D16 const& arg16 , D17 const& arg17 , D18 const& arg18 , D19 const& arg19)
    {
        return map<fusion::pair< K0 , typename detail::as_fusion_element<D0>::type> , fusion::pair< K1 , typename detail::as_fusion_element<D1>::type> , fusion::pair< K2 , typename detail::as_fusion_element<D2>::type> , fusion::pair< K3 , typename detail::as_fusion_element<D3>::type> , fusion::pair< K4 , typename detail::as_fusion_element<D4>::type> , fusion::pair< K5 , typename detail::as_fusion_element<D5>::type> , fusion::pair< K6 , typename detail::as_fusion_element<D6>::type> , fusion::pair< K7 , typename detail::as_fusion_element<D7>::type> , fusion::pair< K8 , typename detail::as_fusion_element<D8>::type> , fusion::pair< K9 , typename detail::as_fusion_element<D9>::type> , fusion::pair< K10 , typename detail::as_fusion_element<D10>::type> , fusion::pair< K11 , typename detail::as_fusion_element<D11>::type> , fusion::pair< K12 , typename detail::as_fusion_element<D12>::type> , fusion::pair< K13 , typename detail::as_fusion_element<D13>::type> , fusion::pair< K14 , typename detail::as_fusion_element<D14>::type> , fusion::pair< K15 , typename detail::as_fusion_element<D15>::type> , fusion::pair< K16 , typename detail::as_fusion_element<D16>::type> , fusion::pair< K17 , typename detail::as_fusion_element<D17>::type> , fusion::pair< K18 , typename detail::as_fusion_element<D18>::type> , fusion::pair< K19 , typename detail::as_fusion_element<D19>::type> >(
            fusion::make_pair<K0>(arg0) , fusion::make_pair<K1>(arg1) , fusion::make_pair<K2>(arg2) , fusion::make_pair<K3>(arg3) , fusion::make_pair<K4>(arg4) , fusion::make_pair<K5>(arg5) , fusion::make_pair<K6>(arg6) , fusion::make_pair<K7>(arg7) , fusion::make_pair<K8>(arg8) , fusion::make_pair<K9>(arg9) , fusion::make_pair<K10>(arg10) , fusion::make_pair<K11>(arg11) , fusion::make_pair<K12>(arg12) , fusion::make_pair<K13>(arg13) , fusion::make_pair<K14>(arg14) , fusion::make_pair<K15>(arg15) , fusion::make_pair<K16>(arg16) , fusion::make_pair<K17>(arg17) , fusion::make_pair<K18>(arg18) , fusion::make_pair<K19>(arg19));
    }
}}
