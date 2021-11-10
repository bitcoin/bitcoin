/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion { namespace detail
{
BOOST_FUSION_BARRIER_BEGIN
    template <bool is_assoc>
    struct as_map<1, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0;
            typedef map<T0> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            
            return result(gen::D0::call(i0));
        }
    };
    template <bool is_assoc>
    struct as_map<2, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1;
            typedef map<T0 , T1> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0);
            return result(gen::D0::call(i0) , gen::D1::call(i1));
        }
    };
    template <bool is_assoc>
    struct as_map<3, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2;
            typedef map<T0 , T1 , T2> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2));
        }
    };
    template <bool is_assoc>
    struct as_map<4, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3;
            typedef map<T0 , T1 , T2 , T3> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3));
        }
    };
    template <bool is_assoc>
    struct as_map<5, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4;
            typedef map<T0 , T1 , T2 , T3 , T4> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4));
        }
    };
    template <bool is_assoc>
    struct as_map<6, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5));
        }
    };
    template <bool is_assoc>
    struct as_map<7, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6));
        }
    };
    template <bool is_assoc>
    struct as_map<8, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7));
        }
    };
    template <bool is_assoc>
    struct as_map<9, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8));
        }
    };
    template <bool is_assoc>
    struct as_map<10, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9));
        }
    };
    template <bool is_assoc>
    struct as_map<11, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10));
        }
    };
    template <bool is_assoc>
    struct as_map<12, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10; typedef typename fusion::result_of::next<I10>::type I11;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10; typedef pair_from<I11, is_assoc> D11; typedef typename D11::type T11;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9); typename gen::I11 i11 = fusion::next(i10);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10) , gen::D11::call(i11));
        }
    };
    template <bool is_assoc>
    struct as_map<13, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10; typedef typename fusion::result_of::next<I10>::type I11; typedef typename fusion::result_of::next<I11>::type I12;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10; typedef pair_from<I11, is_assoc> D11; typedef typename D11::type T11; typedef pair_from<I12, is_assoc> D12; typedef typename D12::type T12;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9); typename gen::I11 i11 = fusion::next(i10); typename gen::I12 i12 = fusion::next(i11);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10) , gen::D11::call(i11) , gen::D12::call(i12));
        }
    };
    template <bool is_assoc>
    struct as_map<14, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10; typedef typename fusion::result_of::next<I10>::type I11; typedef typename fusion::result_of::next<I11>::type I12; typedef typename fusion::result_of::next<I12>::type I13;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10; typedef pair_from<I11, is_assoc> D11; typedef typename D11::type T11; typedef pair_from<I12, is_assoc> D12; typedef typename D12::type T12; typedef pair_from<I13, is_assoc> D13; typedef typename D13::type T13;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9); typename gen::I11 i11 = fusion::next(i10); typename gen::I12 i12 = fusion::next(i11); typename gen::I13 i13 = fusion::next(i12);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10) , gen::D11::call(i11) , gen::D12::call(i12) , gen::D13::call(i13));
        }
    };
    template <bool is_assoc>
    struct as_map<15, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10; typedef typename fusion::result_of::next<I10>::type I11; typedef typename fusion::result_of::next<I11>::type I12; typedef typename fusion::result_of::next<I12>::type I13; typedef typename fusion::result_of::next<I13>::type I14;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10; typedef pair_from<I11, is_assoc> D11; typedef typename D11::type T11; typedef pair_from<I12, is_assoc> D12; typedef typename D12::type T12; typedef pair_from<I13, is_assoc> D13; typedef typename D13::type T13; typedef pair_from<I14, is_assoc> D14; typedef typename D14::type T14;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9); typename gen::I11 i11 = fusion::next(i10); typename gen::I12 i12 = fusion::next(i11); typename gen::I13 i13 = fusion::next(i12); typename gen::I14 i14 = fusion::next(i13);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10) , gen::D11::call(i11) , gen::D12::call(i12) , gen::D13::call(i13) , gen::D14::call(i14));
        }
    };
    template <bool is_assoc>
    struct as_map<16, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10; typedef typename fusion::result_of::next<I10>::type I11; typedef typename fusion::result_of::next<I11>::type I12; typedef typename fusion::result_of::next<I12>::type I13; typedef typename fusion::result_of::next<I13>::type I14; typedef typename fusion::result_of::next<I14>::type I15;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10; typedef pair_from<I11, is_assoc> D11; typedef typename D11::type T11; typedef pair_from<I12, is_assoc> D12; typedef typename D12::type T12; typedef pair_from<I13, is_assoc> D13; typedef typename D13::type T13; typedef pair_from<I14, is_assoc> D14; typedef typename D14::type T14; typedef pair_from<I15, is_assoc> D15; typedef typename D15::type T15;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9); typename gen::I11 i11 = fusion::next(i10); typename gen::I12 i12 = fusion::next(i11); typename gen::I13 i13 = fusion::next(i12); typename gen::I14 i14 = fusion::next(i13); typename gen::I15 i15 = fusion::next(i14);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10) , gen::D11::call(i11) , gen::D12::call(i12) , gen::D13::call(i13) , gen::D14::call(i14) , gen::D15::call(i15));
        }
    };
    template <bool is_assoc>
    struct as_map<17, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10; typedef typename fusion::result_of::next<I10>::type I11; typedef typename fusion::result_of::next<I11>::type I12; typedef typename fusion::result_of::next<I12>::type I13; typedef typename fusion::result_of::next<I13>::type I14; typedef typename fusion::result_of::next<I14>::type I15; typedef typename fusion::result_of::next<I15>::type I16;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10; typedef pair_from<I11, is_assoc> D11; typedef typename D11::type T11; typedef pair_from<I12, is_assoc> D12; typedef typename D12::type T12; typedef pair_from<I13, is_assoc> D13; typedef typename D13::type T13; typedef pair_from<I14, is_assoc> D14; typedef typename D14::type T14; typedef pair_from<I15, is_assoc> D15; typedef typename D15::type T15; typedef pair_from<I16, is_assoc> D16; typedef typename D16::type T16;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9); typename gen::I11 i11 = fusion::next(i10); typename gen::I12 i12 = fusion::next(i11); typename gen::I13 i13 = fusion::next(i12); typename gen::I14 i14 = fusion::next(i13); typename gen::I15 i15 = fusion::next(i14); typename gen::I16 i16 = fusion::next(i15);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10) , gen::D11::call(i11) , gen::D12::call(i12) , gen::D13::call(i13) , gen::D14::call(i14) , gen::D15::call(i15) , gen::D16::call(i16));
        }
    };
    template <bool is_assoc>
    struct as_map<18, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10; typedef typename fusion::result_of::next<I10>::type I11; typedef typename fusion::result_of::next<I11>::type I12; typedef typename fusion::result_of::next<I12>::type I13; typedef typename fusion::result_of::next<I13>::type I14; typedef typename fusion::result_of::next<I14>::type I15; typedef typename fusion::result_of::next<I15>::type I16; typedef typename fusion::result_of::next<I16>::type I17;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10; typedef pair_from<I11, is_assoc> D11; typedef typename D11::type T11; typedef pair_from<I12, is_assoc> D12; typedef typename D12::type T12; typedef pair_from<I13, is_assoc> D13; typedef typename D13::type T13; typedef pair_from<I14, is_assoc> D14; typedef typename D14::type T14; typedef pair_from<I15, is_assoc> D15; typedef typename D15::type T15; typedef pair_from<I16, is_assoc> D16; typedef typename D16::type T16; typedef pair_from<I17, is_assoc> D17; typedef typename D17::type T17;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9); typename gen::I11 i11 = fusion::next(i10); typename gen::I12 i12 = fusion::next(i11); typename gen::I13 i13 = fusion::next(i12); typename gen::I14 i14 = fusion::next(i13); typename gen::I15 i15 = fusion::next(i14); typename gen::I16 i16 = fusion::next(i15); typename gen::I17 i17 = fusion::next(i16);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10) , gen::D11::call(i11) , gen::D12::call(i12) , gen::D13::call(i13) , gen::D14::call(i14) , gen::D15::call(i15) , gen::D16::call(i16) , gen::D17::call(i17));
        }
    };
    template <bool is_assoc>
    struct as_map<19, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10; typedef typename fusion::result_of::next<I10>::type I11; typedef typename fusion::result_of::next<I11>::type I12; typedef typename fusion::result_of::next<I12>::type I13; typedef typename fusion::result_of::next<I13>::type I14; typedef typename fusion::result_of::next<I14>::type I15; typedef typename fusion::result_of::next<I15>::type I16; typedef typename fusion::result_of::next<I16>::type I17; typedef typename fusion::result_of::next<I17>::type I18;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10; typedef pair_from<I11, is_assoc> D11; typedef typename D11::type T11; typedef pair_from<I12, is_assoc> D12; typedef typename D12::type T12; typedef pair_from<I13, is_assoc> D13; typedef typename D13::type T13; typedef pair_from<I14, is_assoc> D14; typedef typename D14::type T14; typedef pair_from<I15, is_assoc> D15; typedef typename D15::type T15; typedef pair_from<I16, is_assoc> D16; typedef typename D16::type T16; typedef pair_from<I17, is_assoc> D17; typedef typename D17::type T17; typedef pair_from<I18, is_assoc> D18; typedef typename D18::type T18;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9); typename gen::I11 i11 = fusion::next(i10); typename gen::I12 i12 = fusion::next(i11); typename gen::I13 i13 = fusion::next(i12); typename gen::I14 i14 = fusion::next(i13); typename gen::I15 i15 = fusion::next(i14); typename gen::I16 i16 = fusion::next(i15); typename gen::I17 i17 = fusion::next(i16); typename gen::I18 i18 = fusion::next(i17);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10) , gen::D11::call(i11) , gen::D12::call(i12) , gen::D13::call(i13) , gen::D14::call(i14) , gen::D15::call(i15) , gen::D16::call(i16) , gen::D17::call(i17) , gen::D18::call(i18));
        }
    };
    template <bool is_assoc>
    struct as_map<20, is_assoc>
    {
        template <typename I0>
        struct apply
        {
            typedef typename fusion::result_of::next<I0>::type I1; typedef typename fusion::result_of::next<I1>::type I2; typedef typename fusion::result_of::next<I2>::type I3; typedef typename fusion::result_of::next<I3>::type I4; typedef typename fusion::result_of::next<I4>::type I5; typedef typename fusion::result_of::next<I5>::type I6; typedef typename fusion::result_of::next<I6>::type I7; typedef typename fusion::result_of::next<I7>::type I8; typedef typename fusion::result_of::next<I8>::type I9; typedef typename fusion::result_of::next<I9>::type I10; typedef typename fusion::result_of::next<I10>::type I11; typedef typename fusion::result_of::next<I11>::type I12; typedef typename fusion::result_of::next<I12>::type I13; typedef typename fusion::result_of::next<I13>::type I14; typedef typename fusion::result_of::next<I14>::type I15; typedef typename fusion::result_of::next<I15>::type I16; typedef typename fusion::result_of::next<I16>::type I17; typedef typename fusion::result_of::next<I17>::type I18; typedef typename fusion::result_of::next<I18>::type I19;
            typedef pair_from<I0, is_assoc> D0; typedef typename D0::type T0; typedef pair_from<I1, is_assoc> D1; typedef typename D1::type T1; typedef pair_from<I2, is_assoc> D2; typedef typename D2::type T2; typedef pair_from<I3, is_assoc> D3; typedef typename D3::type T3; typedef pair_from<I4, is_assoc> D4; typedef typename D4::type T4; typedef pair_from<I5, is_assoc> D5; typedef typename D5::type T5; typedef pair_from<I6, is_assoc> D6; typedef typename D6::type T6; typedef pair_from<I7, is_assoc> D7; typedef typename D7::type T7; typedef pair_from<I8, is_assoc> D8; typedef typename D8::type T8; typedef pair_from<I9, is_assoc> D9; typedef typename D9::type T9; typedef pair_from<I10, is_assoc> D10; typedef typename D10::type T10; typedef pair_from<I11, is_assoc> D11; typedef typename D11::type T11; typedef pair_from<I12, is_assoc> D12; typedef typename D12::type T12; typedef pair_from<I13, is_assoc> D13; typedef typename D13::type T13; typedef pair_from<I14, is_assoc> D14; typedef typename D14::type T14; typedef pair_from<I15, is_assoc> D15; typedef typename D15::type T15; typedef pair_from<I16, is_assoc> D16; typedef typename D16::type T16; typedef pair_from<I17, is_assoc> D17; typedef typename D17::type T17; typedef pair_from<I18, is_assoc> D18; typedef typename D18::type T18; typedef pair_from<I19, is_assoc> D19; typedef typename D19::type T19;
            typedef map<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19> type;
        };
        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static typename apply<Iterator>::type
        call(Iterator const& i0)
        {
            typedef apply<Iterator> gen;
            typedef typename gen::type result;
            typename gen::I1 i1 = fusion::next(i0); typename gen::I2 i2 = fusion::next(i1); typename gen::I3 i3 = fusion::next(i2); typename gen::I4 i4 = fusion::next(i3); typename gen::I5 i5 = fusion::next(i4); typename gen::I6 i6 = fusion::next(i5); typename gen::I7 i7 = fusion::next(i6); typename gen::I8 i8 = fusion::next(i7); typename gen::I9 i9 = fusion::next(i8); typename gen::I10 i10 = fusion::next(i9); typename gen::I11 i11 = fusion::next(i10); typename gen::I12 i12 = fusion::next(i11); typename gen::I13 i13 = fusion::next(i12); typename gen::I14 i14 = fusion::next(i13); typename gen::I15 i15 = fusion::next(i14); typename gen::I16 i16 = fusion::next(i15); typename gen::I17 i17 = fusion::next(i16); typename gen::I18 i18 = fusion::next(i17); typename gen::I19 i19 = fusion::next(i18);
            return result(gen::D0::call(i0) , gen::D1::call(i1) , gen::D2::call(i2) , gen::D3::call(i3) , gen::D4::call(i4) , gen::D5::call(i5) , gen::D6::call(i6) , gen::D7::call(i7) , gen::D8::call(i8) , gen::D9::call(i9) , gen::D10::call(i10) , gen::D11::call(i11) , gen::D12::call(i12) , gen::D13::call(i13) , gen::D14::call(i14) , gen::D15::call(i15) , gen::D16::call(i16) , gen::D17::call(i17) , gen::D18::call(i18) , gen::D19::call(i19));
        }
    };
BOOST_FUSION_BARRIER_END
}}}
