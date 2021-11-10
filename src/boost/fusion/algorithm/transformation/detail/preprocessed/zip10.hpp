/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion
{
    struct void_;
    namespace result_of
    {
        template<typename T0 = fusion::void_ , typename T1 = fusion::void_ , typename T2 = fusion::void_ , typename T3 = fusion::void_ , typename T4 = fusion::void_ , typename T5 = fusion::void_ , typename T6 = fusion::void_ , typename T7 = fusion::void_ , typename T8 = fusion::void_ , typename T9 = fusion::void_ , typename T10 = fusion::void_>
        struct zip;
    }
    namespace result_of
    {
        template< typename T0 , typename T1 >
        struct zip< T0 , T1
                    , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_
        >
        {
            typedef mpl::vector< T0 , T1 > sequences;
            typedef typename mpl::transform<sequences, add_reference<mpl::_> >::type ref_params;
            typedef zip_view<typename result_of::as_vector<ref_params>::type> type;
        };
    }
    template<typename T0 , typename T1>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::zip<const T0 , const T1>::type
    zip(T0 const& t0 , T1 const& t1)
    {
        fusion::vector<const T0& , const T1&> seqs(
            t0 , t1);
        return typename result_of::zip<const T0 , const T1>::type(
            seqs);
    }
    namespace result_of
    {
        template< typename T0 , typename T1 , typename T2 >
        struct zip< T0 , T1 , T2
                    , void_ , void_ , void_ , void_ , void_ , void_ , void_ , void_
        >
        {
            typedef mpl::vector< T0 , T1 , T2 > sequences;
            typedef typename mpl::transform<sequences, add_reference<mpl::_> >::type ref_params;
            typedef zip_view<typename result_of::as_vector<ref_params>::type> type;
        };
    }
    template<typename T0 , typename T1 , typename T2>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::zip<const T0 , const T1 , const T2>::type
    zip(T0 const& t0 , T1 const& t1 , T2 const& t2)
    {
        fusion::vector<const T0& , const T1& , const T2&> seqs(
            t0 , t1 , t2);
        return typename result_of::zip<const T0 , const T1 , const T2>::type(
            seqs);
    }
    namespace result_of
    {
        template< typename T0 , typename T1 , typename T2 , typename T3 >
        struct zip< T0 , T1 , T2 , T3
                    , void_ , void_ , void_ , void_ , void_ , void_ , void_
        >
        {
            typedef mpl::vector< T0 , T1 , T2 , T3 > sequences;
            typedef typename mpl::transform<sequences, add_reference<mpl::_> >::type ref_params;
            typedef zip_view<typename result_of::as_vector<ref_params>::type> type;
        };
    }
    template<typename T0 , typename T1 , typename T2 , typename T3>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::zip<const T0 , const T1 , const T2 , const T3>::type
    zip(T0 const& t0 , T1 const& t1 , T2 const& t2 , T3 const& t3)
    {
        fusion::vector<const T0& , const T1& , const T2& , const T3&> seqs(
            t0 , t1 , t2 , t3);
        return typename result_of::zip<const T0 , const T1 , const T2 , const T3>::type(
            seqs);
    }
    namespace result_of
    {
        template< typename T0 , typename T1 , typename T2 , typename T3 , typename T4 >
        struct zip< T0 , T1 , T2 , T3 , T4
                    , void_ , void_ , void_ , void_ , void_ , void_
        >
        {
            typedef mpl::vector< T0 , T1 , T2 , T3 , T4 > sequences;
            typedef typename mpl::transform<sequences, add_reference<mpl::_> >::type ref_params;
            typedef zip_view<typename result_of::as_vector<ref_params>::type> type;
        };
    }
    template<typename T0 , typename T1 , typename T2 , typename T3 , typename T4>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4>::type
    zip(T0 const& t0 , T1 const& t1 , T2 const& t2 , T3 const& t3 , T4 const& t4)
    {
        fusion::vector<const T0& , const T1& , const T2& , const T3& , const T4&> seqs(
            t0 , t1 , t2 , t3 , t4);
        return typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4>::type(
            seqs);
    }
    namespace result_of
    {
        template< typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 >
        struct zip< T0 , T1 , T2 , T3 , T4 , T5
                    , void_ , void_ , void_ , void_ , void_
        >
        {
            typedef mpl::vector< T0 , T1 , T2 , T3 , T4 , T5 > sequences;
            typedef typename mpl::transform<sequences, add_reference<mpl::_> >::type ref_params;
            typedef zip_view<typename result_of::as_vector<ref_params>::type> type;
        };
    }
    template<typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5>::type
    zip(T0 const& t0 , T1 const& t1 , T2 const& t2 , T3 const& t3 , T4 const& t4 , T5 const& t5)
    {
        fusion::vector<const T0& , const T1& , const T2& , const T3& , const T4& , const T5&> seqs(
            t0 , t1 , t2 , t3 , t4 , t5);
        return typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5>::type(
            seqs);
    }
    namespace result_of
    {
        template< typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 >
        struct zip< T0 , T1 , T2 , T3 , T4 , T5 , T6
                    , void_ , void_ , void_ , void_
        >
        {
            typedef mpl::vector< T0 , T1 , T2 , T3 , T4 , T5 , T6 > sequences;
            typedef typename mpl::transform<sequences, add_reference<mpl::_> >::type ref_params;
            typedef zip_view<typename result_of::as_vector<ref_params>::type> type;
        };
    }
    template<typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5 , const T6>::type
    zip(T0 const& t0 , T1 const& t1 , T2 const& t2 , T3 const& t3 , T4 const& t4 , T5 const& t5 , T6 const& t6)
    {
        fusion::vector<const T0& , const T1& , const T2& , const T3& , const T4& , const T5& , const T6&> seqs(
            t0 , t1 , t2 , t3 , t4 , t5 , t6);
        return typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5 , const T6>::type(
            seqs);
    }
    namespace result_of
    {
        template< typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 >
        struct zip< T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7
                    , void_ , void_ , void_
        >
        {
            typedef mpl::vector< T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 > sequences;
            typedef typename mpl::transform<sequences, add_reference<mpl::_> >::type ref_params;
            typedef zip_view<typename result_of::as_vector<ref_params>::type> type;
        };
    }
    template<typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5 , const T6 , const T7>::type
    zip(T0 const& t0 , T1 const& t1 , T2 const& t2 , T3 const& t3 , T4 const& t4 , T5 const& t5 , T6 const& t6 , T7 const& t7)
    {
        fusion::vector<const T0& , const T1& , const T2& , const T3& , const T4& , const T5& , const T6& , const T7&> seqs(
            t0 , t1 , t2 , t3 , t4 , t5 , t6 , t7);
        return typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5 , const T6 , const T7>::type(
            seqs);
    }
    namespace result_of
    {
        template< typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 >
        struct zip< T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8
                    , void_ , void_
        >
        {
            typedef mpl::vector< T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 > sequences;
            typedef typename mpl::transform<sequences, add_reference<mpl::_> >::type ref_params;
            typedef zip_view<typename result_of::as_vector<ref_params>::type> type;
        };
    }
    template<typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5 , const T6 , const T7 , const T8>::type
    zip(T0 const& t0 , T1 const& t1 , T2 const& t2 , T3 const& t3 , T4 const& t4 , T5 const& t5 , T6 const& t6 , T7 const& t7 , T8 const& t8)
    {
        fusion::vector<const T0& , const T1& , const T2& , const T3& , const T4& , const T5& , const T6& , const T7& , const T8&> seqs(
            t0 , t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8);
        return typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5 , const T6 , const T7 , const T8>::type(
            seqs);
    }
    namespace result_of
    {
        template< typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 >
        struct zip< T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9
                    , void_
        >
        {
            typedef mpl::vector< T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 > sequences;
            typedef typename mpl::transform<sequences, add_reference<mpl::_> >::type ref_params;
            typedef zip_view<typename result_of::as_vector<ref_params>::type> type;
        };
    }
    template<typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5 , const T6 , const T7 , const T8 , const T9>::type
    zip(T0 const& t0 , T1 const& t1 , T2 const& t2 , T3 const& t3 , T4 const& t4 , T5 const& t5 , T6 const& t6 , T7 const& t7 , T8 const& t8 , T9 const& t9)
    {
        fusion::vector<const T0& , const T1& , const T2& , const T3& , const T4& , const T5& , const T6& , const T7& , const T8& , const T9&> seqs(
            t0 , t1 , t2 , t3 , t4 , t5 , t6 , t7 , t8 , t9);
        return typename result_of::zip<const T0 , const T1 , const T2 , const T3 , const T4 , const T5 , const T6 , const T7 , const T8 , const T9>::type(
            seqs);
    }
}}
