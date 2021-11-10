/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion
{
    struct vector_tag;
    struct fusion_sequence_tag;
    struct random_access_traversal_tag;
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20>
    struct vector_data21
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data21()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data21(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data21(
            vector_data21&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data21(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data21(
            vector_data21 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data21&
        operator=(vector_data21 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data21
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19);
            return vector_data21(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data21
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19);
            return vector_data21(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20>
    struct vector21
      : vector_data21<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20>
      , sequence_base<vector21<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20> >
    {
        typedef vector21<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20> this_type;
        typedef vector_data21<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20> base_type;
        typedef mpl::vector21<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<21> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector21() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector21(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector21(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector21(vector21&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector21(vector21 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector21&
        operator=(vector21 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector21&
        operator=(vector21&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector21(
            vector21<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector21(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector21(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector21&
        operator=(vector21<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21>
    struct vector_data22
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data22()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() , m21() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data22(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) , m21(std::forward<U21>( arg21)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data22(
            vector_data22&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) , m21(std::forward<T21>( other.m21)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data22(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) , m21(arg21) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data22(
            vector_data22 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) , m21(other.m21) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data22&
        operator=(vector_data22 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data22
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20);
            return vector_data22(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data22
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20);
            return vector_data22(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20; T21 m21;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21>
    struct vector22
      : vector_data22<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21>
      , sequence_base<vector22<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21> >
    {
        typedef vector22<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21> this_type;
        typedef vector_data22<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21> base_type;
        typedef mpl::vector22<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<22> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector22() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector22(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector22(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector22(vector22&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector22(vector22 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector22&
        operator=(vector22 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector22&
        operator=(vector22&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20); this->m21 = std::forward< T21>(vec.m21);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector22(
            vector22<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20 , vec.m21) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector22(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector22(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector22&
        operator=(vector22<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20; this->m21 = *i21;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T21>::type at_impl(mpl::int_<21>) { return this->m21; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T21>::type>::type at_impl(mpl::int_<21>) const { return this->m21; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22>
    struct vector_data23
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data23()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() , m21() , m22() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data23(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) , m21(std::forward<U21>( arg21)) , m22(std::forward<U22>( arg22)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data23(
            vector_data23&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) , m21(std::forward<T21>( other.m21)) , m22(std::forward<T22>( other.m22)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data23(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) , m21(arg21) , m22(arg22) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data23(
            vector_data23 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) , m21(other.m21) , m22(other.m22) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data23&
        operator=(vector_data23 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data23
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21);
            return vector_data23(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data23
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21);
            return vector_data23(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20; T21 m21; T22 m22;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22>
    struct vector23
      : vector_data23<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22>
      , sequence_base<vector23<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22> >
    {
        typedef vector23<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22> this_type;
        typedef vector_data23<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22> base_type;
        typedef mpl::vector23<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<23> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector23() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector23(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector23(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector23(vector23&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector23(vector23 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector23&
        operator=(vector23 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector23&
        operator=(vector23&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20); this->m21 = std::forward< T21>(vec.m21); this->m22 = std::forward< T22>(vec.m22);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector23(
            vector23<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20 , vec.m21 , vec.m22) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector23(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector23(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector23&
        operator=(vector23<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20; this->m21 = *i21; this->m22 = *i22;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T21>::type at_impl(mpl::int_<21>) { return this->m21; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T21>::type>::type at_impl(mpl::int_<21>) const { return this->m21; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T22>::type at_impl(mpl::int_<22>) { return this->m22; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T22>::type>::type at_impl(mpl::int_<22>) const { return this->m22; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23>
    struct vector_data24
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data24()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() , m21() , m22() , m23() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data24(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) , m21(std::forward<U21>( arg21)) , m22(std::forward<U22>( arg22)) , m23(std::forward<U23>( arg23)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data24(
            vector_data24&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) , m21(std::forward<T21>( other.m21)) , m22(std::forward<T22>( other.m22)) , m23(std::forward<T23>( other.m23)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data24(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) , m21(arg21) , m22(arg22) , m23(arg23) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data24(
            vector_data24 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) , m21(other.m21) , m22(other.m22) , m23(other.m23) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data24&
        operator=(vector_data24 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data24
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22);
            return vector_data24(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data24
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22);
            return vector_data24(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20; T21 m21; T22 m22; T23 m23;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23>
    struct vector24
      : vector_data24<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23>
      , sequence_base<vector24<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23> >
    {
        typedef vector24<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23> this_type;
        typedef vector_data24<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23> base_type;
        typedef mpl::vector24<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<24> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector24() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector24(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector24(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector24(vector24&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector24(vector24 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector24&
        operator=(vector24 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector24&
        operator=(vector24&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20); this->m21 = std::forward< T21>(vec.m21); this->m22 = std::forward< T22>(vec.m22); this->m23 = std::forward< T23>(vec.m23);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector24(
            vector24<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20 , vec.m21 , vec.m22 , vec.m23) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector24(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector24(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector24&
        operator=(vector24<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20; this->m21 = *i21; this->m22 = *i22; this->m23 = *i23;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T21>::type at_impl(mpl::int_<21>) { return this->m21; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T21>::type>::type at_impl(mpl::int_<21>) const { return this->m21; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T22>::type at_impl(mpl::int_<22>) { return this->m22; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T22>::type>::type at_impl(mpl::int_<22>) const { return this->m22; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T23>::type at_impl(mpl::int_<23>) { return this->m23; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T23>::type>::type at_impl(mpl::int_<23>) const { return this->m23; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24>
    struct vector_data25
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data25()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() , m21() , m22() , m23() , m24() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data25(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) , m21(std::forward<U21>( arg21)) , m22(std::forward<U22>( arg22)) , m23(std::forward<U23>( arg23)) , m24(std::forward<U24>( arg24)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data25(
            vector_data25&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) , m21(std::forward<T21>( other.m21)) , m22(std::forward<T22>( other.m22)) , m23(std::forward<T23>( other.m23)) , m24(std::forward<T24>( other.m24)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data25(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) , m21(arg21) , m22(arg22) , m23(arg23) , m24(arg24) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data25(
            vector_data25 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) , m21(other.m21) , m22(other.m22) , m23(other.m23) , m24(other.m24) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data25&
        operator=(vector_data25 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data25
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23);
            return vector_data25(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data25
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23);
            return vector_data25(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20; T21 m21; T22 m22; T23 m23; T24 m24;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24>
    struct vector25
      : vector_data25<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24>
      , sequence_base<vector25<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24> >
    {
        typedef vector25<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24> this_type;
        typedef vector_data25<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24> base_type;
        typedef mpl::vector25<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<25> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector25() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector25(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector25(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector25(vector25&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector25(vector25 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector25&
        operator=(vector25 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector25&
        operator=(vector25&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20); this->m21 = std::forward< T21>(vec.m21); this->m22 = std::forward< T22>(vec.m22); this->m23 = std::forward< T23>(vec.m23); this->m24 = std::forward< T24>(vec.m24);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector25(
            vector25<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20 , vec.m21 , vec.m22 , vec.m23 , vec.m24) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector25(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector25(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector25&
        operator=(vector25<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20; this->m21 = *i21; this->m22 = *i22; this->m23 = *i23; this->m24 = *i24;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T21>::type at_impl(mpl::int_<21>) { return this->m21; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T21>::type>::type at_impl(mpl::int_<21>) const { return this->m21; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T22>::type at_impl(mpl::int_<22>) { return this->m22; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T22>::type>::type at_impl(mpl::int_<22>) const { return this->m22; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T23>::type at_impl(mpl::int_<23>) { return this->m23; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T23>::type>::type at_impl(mpl::int_<23>) const { return this->m23; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T24>::type at_impl(mpl::int_<24>) { return this->m24; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T24>::type>::type at_impl(mpl::int_<24>) const { return this->m24; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25>
    struct vector_data26
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data26()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() , m21() , m22() , m23() , m24() , m25() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data26(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) , m21(std::forward<U21>( arg21)) , m22(std::forward<U22>( arg22)) , m23(std::forward<U23>( arg23)) , m24(std::forward<U24>( arg24)) , m25(std::forward<U25>( arg25)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data26(
            vector_data26&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) , m21(std::forward<T21>( other.m21)) , m22(std::forward<T22>( other.m22)) , m23(std::forward<T23>( other.m23)) , m24(std::forward<T24>( other.m24)) , m25(std::forward<T25>( other.m25)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data26(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) , m21(arg21) , m22(arg22) , m23(arg23) , m24(arg24) , m25(arg25) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data26(
            vector_data26 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) , m21(other.m21) , m22(other.m22) , m23(other.m23) , m24(other.m24) , m25(other.m25) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data26&
        operator=(vector_data26 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data26
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24);
            return vector_data26(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data26
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24);
            return vector_data26(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20; T21 m21; T22 m22; T23 m23; T24 m24; T25 m25;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25>
    struct vector26
      : vector_data26<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25>
      , sequence_base<vector26<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25> >
    {
        typedef vector26<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25> this_type;
        typedef vector_data26<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25> base_type;
        typedef mpl::vector26<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<26> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector26() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector26(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector26(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector26(vector26&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector26(vector26 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector26&
        operator=(vector26 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector26&
        operator=(vector26&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20); this->m21 = std::forward< T21>(vec.m21); this->m22 = std::forward< T22>(vec.m22); this->m23 = std::forward< T23>(vec.m23); this->m24 = std::forward< T24>(vec.m24); this->m25 = std::forward< T25>(vec.m25);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector26(
            vector26<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20 , vec.m21 , vec.m22 , vec.m23 , vec.m24 , vec.m25) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector26(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector26(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector26&
        operator=(vector26<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20; this->m21 = *i21; this->m22 = *i22; this->m23 = *i23; this->m24 = *i24; this->m25 = *i25;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T21>::type at_impl(mpl::int_<21>) { return this->m21; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T21>::type>::type at_impl(mpl::int_<21>) const { return this->m21; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T22>::type at_impl(mpl::int_<22>) { return this->m22; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T22>::type>::type at_impl(mpl::int_<22>) const { return this->m22; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T23>::type at_impl(mpl::int_<23>) { return this->m23; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T23>::type>::type at_impl(mpl::int_<23>) const { return this->m23; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T24>::type at_impl(mpl::int_<24>) { return this->m24; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T24>::type>::type at_impl(mpl::int_<24>) const { return this->m24; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T25>::type at_impl(mpl::int_<25>) { return this->m25; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T25>::type>::type at_impl(mpl::int_<25>) const { return this->m25; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26>
    struct vector_data27
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data27()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() , m21() , m22() , m23() , m24() , m25() , m26() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data27(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) , m21(std::forward<U21>( arg21)) , m22(std::forward<U22>( arg22)) , m23(std::forward<U23>( arg23)) , m24(std::forward<U24>( arg24)) , m25(std::forward<U25>( arg25)) , m26(std::forward<U26>( arg26)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data27(
            vector_data27&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) , m21(std::forward<T21>( other.m21)) , m22(std::forward<T22>( other.m22)) , m23(std::forward<T23>( other.m23)) , m24(std::forward<T24>( other.m24)) , m25(std::forward<T25>( other.m25)) , m26(std::forward<T26>( other.m26)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data27(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) , m21(arg21) , m22(arg22) , m23(arg23) , m24(arg24) , m25(arg25) , m26(arg26) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data27(
            vector_data27 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) , m21(other.m21) , m22(other.m22) , m23(other.m23) , m24(other.m24) , m25(other.m25) , m26(other.m26) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data27&
        operator=(vector_data27 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25; this->m26 = vec.m26;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data27
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25);
            return vector_data27(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25 , *i26);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data27
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25);
            return vector_data27(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25 , *i26);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20; T21 m21; T22 m22; T23 m23; T24 m24; T25 m25; T26 m26;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26>
    struct vector27
      : vector_data27<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26>
      , sequence_base<vector27<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26> >
    {
        typedef vector27<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26> this_type;
        typedef vector_data27<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26> base_type;
        typedef mpl::vector27<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<27> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector27() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector27(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector27(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector27(vector27&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector27(vector27 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector27&
        operator=(vector27 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector27&
        operator=(vector27&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20); this->m21 = std::forward< T21>(vec.m21); this->m22 = std::forward< T22>(vec.m22); this->m23 = std::forward< T23>(vec.m23); this->m24 = std::forward< T24>(vec.m24); this->m25 = std::forward< T25>(vec.m25); this->m26 = std::forward< T26>(vec.m26);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector27(
            vector27<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20 , vec.m21 , vec.m22 , vec.m23 , vec.m24 , vec.m25 , vec.m26) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector27(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector27(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector27&
        operator=(vector27<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25; this->m26 = vec.m26;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20; this->m21 = *i21; this->m22 = *i22; this->m23 = *i23; this->m24 = *i24; this->m25 = *i25; this->m26 = *i26;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T21>::type at_impl(mpl::int_<21>) { return this->m21; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T21>::type>::type at_impl(mpl::int_<21>) const { return this->m21; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T22>::type at_impl(mpl::int_<22>) { return this->m22; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T22>::type>::type at_impl(mpl::int_<22>) const { return this->m22; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T23>::type at_impl(mpl::int_<23>) { return this->m23; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T23>::type>::type at_impl(mpl::int_<23>) const { return this->m23; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T24>::type at_impl(mpl::int_<24>) { return this->m24; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T24>::type>::type at_impl(mpl::int_<24>) const { return this->m24; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T25>::type at_impl(mpl::int_<25>) { return this->m25; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T25>::type>::type at_impl(mpl::int_<25>) const { return this->m25; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T26>::type at_impl(mpl::int_<26>) { return this->m26; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T26>::type>::type at_impl(mpl::int_<26>) const { return this->m26; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27>
    struct vector_data28
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data28()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() , m21() , m22() , m23() , m24() , m25() , m26() , m27() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data28(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) , m21(std::forward<U21>( arg21)) , m22(std::forward<U22>( arg22)) , m23(std::forward<U23>( arg23)) , m24(std::forward<U24>( arg24)) , m25(std::forward<U25>( arg25)) , m26(std::forward<U26>( arg26)) , m27(std::forward<U27>( arg27)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data28(
            vector_data28&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) , m21(std::forward<T21>( other.m21)) , m22(std::forward<T22>( other.m22)) , m23(std::forward<T23>( other.m23)) , m24(std::forward<T24>( other.m24)) , m25(std::forward<T25>( other.m25)) , m26(std::forward<T26>( other.m26)) , m27(std::forward<T27>( other.m27)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data28(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) , m21(arg21) , m22(arg22) , m23(arg23) , m24(arg24) , m25(arg25) , m26(arg26) , m27(arg27) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data28(
            vector_data28 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) , m21(other.m21) , m22(other.m22) , m23(other.m23) , m24(other.m24) , m25(other.m25) , m26(other.m26) , m27(other.m27) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data28&
        operator=(vector_data28 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25; this->m26 = vec.m26; this->m27 = vec.m27;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data28
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25); typedef typename result_of::next< I26>::type I27; I27 i27 = fusion::next(i26);
            return vector_data28(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25 , *i26 , *i27);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data28
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25); typedef typename result_of::next< I26>::type I27; I27 i27 = fusion::next(i26);
            return vector_data28(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25 , *i26 , *i27);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20; T21 m21; T22 m22; T23 m23; T24 m24; T25 m25; T26 m26; T27 m27;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27>
    struct vector28
      : vector_data28<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27>
      , sequence_base<vector28<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27> >
    {
        typedef vector28<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27> this_type;
        typedef vector_data28<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27> base_type;
        typedef mpl::vector28<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<28> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector28() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector28(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector28(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector28(vector28&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector28(vector28 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector28&
        operator=(vector28 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector28&
        operator=(vector28&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20); this->m21 = std::forward< T21>(vec.m21); this->m22 = std::forward< T22>(vec.m22); this->m23 = std::forward< T23>(vec.m23); this->m24 = std::forward< T24>(vec.m24); this->m25 = std::forward< T25>(vec.m25); this->m26 = std::forward< T26>(vec.m26); this->m27 = std::forward< T27>(vec.m27);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector28(
            vector28<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26 , U27> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20 , vec.m21 , vec.m22 , vec.m23 , vec.m24 , vec.m25 , vec.m26 , vec.m27) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector28(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector28(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector28&
        operator=(vector28<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26 , U27> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25; this->m26 = vec.m26; this->m27 = vec.m27;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25); typedef typename result_of::next< I26>::type I27; I27 i27 = fusion::next(i26);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20; this->m21 = *i21; this->m22 = *i22; this->m23 = *i23; this->m24 = *i24; this->m25 = *i25; this->m26 = *i26; this->m27 = *i27;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T21>::type at_impl(mpl::int_<21>) { return this->m21; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T21>::type>::type at_impl(mpl::int_<21>) const { return this->m21; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T22>::type at_impl(mpl::int_<22>) { return this->m22; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T22>::type>::type at_impl(mpl::int_<22>) const { return this->m22; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T23>::type at_impl(mpl::int_<23>) { return this->m23; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T23>::type>::type at_impl(mpl::int_<23>) const { return this->m23; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T24>::type at_impl(mpl::int_<24>) { return this->m24; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T24>::type>::type at_impl(mpl::int_<24>) const { return this->m24; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T25>::type at_impl(mpl::int_<25>) { return this->m25; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T25>::type>::type at_impl(mpl::int_<25>) const { return this->m25; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T26>::type at_impl(mpl::int_<26>) { return this->m26; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T26>::type>::type at_impl(mpl::int_<26>) const { return this->m26; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T27>::type at_impl(mpl::int_<27>) { return this->m27; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T27>::type>::type at_impl(mpl::int_<27>) const { return this->m27; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27 , typename T28>
    struct vector_data29
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data29()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() , m21() , m22() , m23() , m24() , m25() , m26() , m27() , m28() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data29(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) , m21(std::forward<U21>( arg21)) , m22(std::forward<U22>( arg22)) , m23(std::forward<U23>( arg23)) , m24(std::forward<U24>( arg24)) , m25(std::forward<U25>( arg25)) , m26(std::forward<U26>( arg26)) , m27(std::forward<U27>( arg27)) , m28(std::forward<U28>( arg28)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data29(
            vector_data29&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) , m21(std::forward<T21>( other.m21)) , m22(std::forward<T22>( other.m22)) , m23(std::forward<T23>( other.m23)) , m24(std::forward<T24>( other.m24)) , m25(std::forward<T25>( other.m25)) , m26(std::forward<T26>( other.m26)) , m27(std::forward<T27>( other.m27)) , m28(std::forward<T28>( other.m28)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data29(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) , m21(arg21) , m22(arg22) , m23(arg23) , m24(arg24) , m25(arg25) , m26(arg26) , m27(arg27) , m28(arg28) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data29(
            vector_data29 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) , m21(other.m21) , m22(other.m22) , m23(other.m23) , m24(other.m24) , m25(other.m25) , m26(other.m26) , m27(other.m27) , m28(other.m28) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data29&
        operator=(vector_data29 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25; this->m26 = vec.m26; this->m27 = vec.m27; this->m28 = vec.m28;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data29
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25); typedef typename result_of::next< I26>::type I27; I27 i27 = fusion::next(i26); typedef typename result_of::next< I27>::type I28; I28 i28 = fusion::next(i27);
            return vector_data29(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25 , *i26 , *i27 , *i28);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data29
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25); typedef typename result_of::next< I26>::type I27; I27 i27 = fusion::next(i26); typedef typename result_of::next< I27>::type I28; I28 i28 = fusion::next(i27);
            return vector_data29(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25 , *i26 , *i27 , *i28);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20; T21 m21; T22 m22; T23 m23; T24 m24; T25 m25; T26 m26; T27 m27; T28 m28;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27 , typename T28>
    struct vector29
      : vector_data29<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28>
      , sequence_base<vector29<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28> >
    {
        typedef vector29<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28> this_type;
        typedef vector_data29<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28> base_type;
        typedef mpl::vector29<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<29> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector29() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector29(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector29(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector29(vector29&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector29(vector29 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector29&
        operator=(vector29 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector29&
        operator=(vector29&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20); this->m21 = std::forward< T21>(vec.m21); this->m22 = std::forward< T22>(vec.m22); this->m23 = std::forward< T23>(vec.m23); this->m24 = std::forward< T24>(vec.m24); this->m25 = std::forward< T25>(vec.m25); this->m26 = std::forward< T26>(vec.m26); this->m27 = std::forward< T27>(vec.m27); this->m28 = std::forward< T28>(vec.m28);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector29(
            vector29<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26 , U27 , U28> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20 , vec.m21 , vec.m22 , vec.m23 , vec.m24 , vec.m25 , vec.m26 , vec.m27 , vec.m28) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector29(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector29(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector29&
        operator=(vector29<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26 , U27 , U28> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25; this->m26 = vec.m26; this->m27 = vec.m27; this->m28 = vec.m28;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25); typedef typename result_of::next< I26>::type I27; I27 i27 = fusion::next(i26); typedef typename result_of::next< I27>::type I28; I28 i28 = fusion::next(i27);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20; this->m21 = *i21; this->m22 = *i22; this->m23 = *i23; this->m24 = *i24; this->m25 = *i25; this->m26 = *i26; this->m27 = *i27; this->m28 = *i28;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T21>::type at_impl(mpl::int_<21>) { return this->m21; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T21>::type>::type at_impl(mpl::int_<21>) const { return this->m21; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T22>::type at_impl(mpl::int_<22>) { return this->m22; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T22>::type>::type at_impl(mpl::int_<22>) const { return this->m22; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T23>::type at_impl(mpl::int_<23>) { return this->m23; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T23>::type>::type at_impl(mpl::int_<23>) const { return this->m23; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T24>::type at_impl(mpl::int_<24>) { return this->m24; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T24>::type>::type at_impl(mpl::int_<24>) const { return this->m24; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T25>::type at_impl(mpl::int_<25>) { return this->m25; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T25>::type>::type at_impl(mpl::int_<25>) const { return this->m25; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T26>::type at_impl(mpl::int_<26>) { return this->m26; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T26>::type>::type at_impl(mpl::int_<26>) const { return this->m26; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T27>::type at_impl(mpl::int_<27>) { return this->m27; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T27>::type>::type at_impl(mpl::int_<27>) const { return this->m27; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T28>::type at_impl(mpl::int_<28>) { return this->m28; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T28>::type>::type at_impl(mpl::int_<28>) const { return this->m28; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27 , typename T28 , typename T29>
    struct vector_data30
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data30()
            : m0() , m1() , m2() , m3() , m4() , m5() , m6() , m7() , m8() , m9() , m10() , m11() , m12() , m13() , m14() , m15() , m16() , m17() , m18() , m19() , m20() , m21() , m22() , m23() , m24() , m25() , m26() , m27() , m28() , m29() {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data30(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29
          , typename boost::enable_if<is_convertible<U0, T0> >::type* = 0
        )
            : m0(std::forward<U0>( arg0)) , m1(std::forward<U1>( arg1)) , m2(std::forward<U2>( arg2)) , m3(std::forward<U3>( arg3)) , m4(std::forward<U4>( arg4)) , m5(std::forward<U5>( arg5)) , m6(std::forward<U6>( arg6)) , m7(std::forward<U7>( arg7)) , m8(std::forward<U8>( arg8)) , m9(std::forward<U9>( arg9)) , m10(std::forward<U10>( arg10)) , m11(std::forward<U11>( arg11)) , m12(std::forward<U12>( arg12)) , m13(std::forward<U13>( arg13)) , m14(std::forward<U14>( arg14)) , m15(std::forward<U15>( arg15)) , m16(std::forward<U16>( arg16)) , m17(std::forward<U17>( arg17)) , m18(std::forward<U18>( arg18)) , m19(std::forward<U19>( arg19)) , m20(std::forward<U20>( arg20)) , m21(std::forward<U21>( arg21)) , m22(std::forward<U22>( arg22)) , m23(std::forward<U23>( arg23)) , m24(std::forward<U24>( arg24)) , m25(std::forward<U25>( arg25)) , m26(std::forward<U26>( arg26)) , m27(std::forward<U27>( arg27)) , m28(std::forward<U28>( arg28)) , m29(std::forward<U29>( arg29)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data30(
            vector_data30&& other)
            : m0(std::forward<T0>( other.m0)) , m1(std::forward<T1>( other.m1)) , m2(std::forward<T2>( other.m2)) , m3(std::forward<T3>( other.m3)) , m4(std::forward<T4>( other.m4)) , m5(std::forward<T5>( other.m5)) , m6(std::forward<T6>( other.m6)) , m7(std::forward<T7>( other.m7)) , m8(std::forward<T8>( other.m8)) , m9(std::forward<T9>( other.m9)) , m10(std::forward<T10>( other.m10)) , m11(std::forward<T11>( other.m11)) , m12(std::forward<T12>( other.m12)) , m13(std::forward<T13>( other.m13)) , m14(std::forward<T14>( other.m14)) , m15(std::forward<T15>( other.m15)) , m16(std::forward<T16>( other.m16)) , m17(std::forward<T17>( other.m17)) , m18(std::forward<T18>( other.m18)) , m19(std::forward<T19>( other.m19)) , m20(std::forward<T20>( other.m20)) , m21(std::forward<T21>( other.m21)) , m22(std::forward<T22>( other.m22)) , m23(std::forward<T23>( other.m23)) , m24(std::forward<T24>( other.m24)) , m25(std::forward<T25>( other.m25)) , m26(std::forward<T26>( other.m26)) , m27(std::forward<T27>( other.m27)) , m28(std::forward<T28>( other.m28)) , m29(std::forward<T29>( other.m29)) {}
# endif
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector_data30(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29)
            : m0(arg0) , m1(arg1) , m2(arg2) , m3(arg3) , m4(arg4) , m5(arg5) , m6(arg6) , m7(arg7) , m8(arg8) , m9(arg9) , m10(arg10) , m11(arg11) , m12(arg12) , m13(arg13) , m14(arg14) , m15(arg15) , m16(arg16) , m17(arg17) , m18(arg18) , m19(arg19) , m20(arg20) , m21(arg21) , m22(arg22) , m23(arg23) , m24(arg24) , m25(arg25) , m26(arg26) , m27(arg27) , m28(arg28) , m29(arg29) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data30(
            vector_data30 const& other)
            : m0(other.m0) , m1(other.m1) , m2(other.m2) , m3(other.m3) , m4(other.m4) , m5(other.m5) , m6(other.m6) , m7(other.m7) , m8(other.m8) , m9(other.m9) , m10(other.m10) , m11(other.m11) , m12(other.m12) , m13(other.m13) , m14(other.m14) , m15(other.m15) , m16(other.m16) , m17(other.m17) , m18(other.m18) , m19(other.m19) , m20(other.m20) , m21(other.m21) , m22(other.m22) , m23(other.m23) , m24(other.m24) , m25(other.m25) , m26(other.m26) , m27(other.m27) , m28(other.m28) , m29(other.m29) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector_data30&
        operator=(vector_data30 const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25; this->m26 = vec.m26; this->m27 = vec.m27; this->m28 = vec.m28; this->m29 = vec.m29;
            return *this;
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data30
        init_from_sequence(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25); typedef typename result_of::next< I26>::type I27; I27 i27 = fusion::next(i26); typedef typename result_of::next< I27>::type I28; I28 i28 = fusion::next(i27); typedef typename result_of::next< I28>::type I29; I29 i29 = fusion::next(i28);
            return vector_data30(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25 , *i26 , *i27 , *i28 , *i29);
        }
        template <typename Sequence>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        static vector_data30
        init_from_sequence(Sequence& seq)
        {
            typedef typename result_of::begin<Sequence>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25); typedef typename result_of::next< I26>::type I27; I27 i27 = fusion::next(i26); typedef typename result_of::next< I27>::type I28; I28 i28 = fusion::next(i27); typedef typename result_of::next< I28>::type I29; I29 i29 = fusion::next(i28);
            return vector_data30(*i0 , *i1 , *i2 , *i3 , *i4 , *i5 , *i6 , *i7 , *i8 , *i9 , *i10 , *i11 , *i12 , *i13 , *i14 , *i15 , *i16 , *i17 , *i18 , *i19 , *i20 , *i21 , *i22 , *i23 , *i24 , *i25 , *i26 , *i27 , *i28 , *i29);
        }
        T0 m0; T1 m1; T2 m2; T3 m3; T4 m4; T5 m5; T6 m6; T7 m7; T8 m8; T9 m9; T10 m10; T11 m11; T12 m12; T13 m13; T14 m14; T15 m15; T16 m16; T17 m17; T18 m18; T19 m19; T20 m20; T21 m21; T22 m22; T23 m23; T24 m24; T25 m25; T26 m26; T27 m27; T28 m28; T29 m29;
    };
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27 , typename T28 , typename T29>
    struct vector30
      : vector_data30<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29>
      , sequence_base<vector30<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29> >
    {
        typedef vector30<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29> this_type;
        typedef vector_data30<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29> base_type;
        typedef mpl::vector30<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29> types;
        typedef vector_tag fusion_tag;
        typedef fusion_sequence_tag tag; 
        typedef mpl::false_ is_view;
        typedef random_access_traversal_tag category;
        typedef mpl::int_<30> size;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector30() {}
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector30(
            typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29)
            : base_type(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector30(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29)
            : base_type(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector30(vector30&& rhs)
            : base_type(std::forward<base_type>(rhs)) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector30(vector30 const& rhs)
            : base_type(static_cast<base_type const&>(rhs)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector30&
        operator=(vector30 const& vec)
        {
            base_type::operator=(vec);
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector30&
        operator=(vector30&& vec)
        {
            this->m0 = std::forward< T0>(vec.m0); this->m1 = std::forward< T1>(vec.m1); this->m2 = std::forward< T2>(vec.m2); this->m3 = std::forward< T3>(vec.m3); this->m4 = std::forward< T4>(vec.m4); this->m5 = std::forward< T5>(vec.m5); this->m6 = std::forward< T6>(vec.m6); this->m7 = std::forward< T7>(vec.m7); this->m8 = std::forward< T8>(vec.m8); this->m9 = std::forward< T9>(vec.m9); this->m10 = std::forward< T10>(vec.m10); this->m11 = std::forward< T11>(vec.m11); this->m12 = std::forward< T12>(vec.m12); this->m13 = std::forward< T13>(vec.m13); this->m14 = std::forward< T14>(vec.m14); this->m15 = std::forward< T15>(vec.m15); this->m16 = std::forward< T16>(vec.m16); this->m17 = std::forward< T17>(vec.m17); this->m18 = std::forward< T18>(vec.m18); this->m19 = std::forward< T19>(vec.m19); this->m20 = std::forward< T20>(vec.m20); this->m21 = std::forward< T21>(vec.m21); this->m22 = std::forward< T22>(vec.m22); this->m23 = std::forward< T23>(vec.m23); this->m24 = std::forward< T24>(vec.m24); this->m25 = std::forward< T25>(vec.m25); this->m26 = std::forward< T26>(vec.m26); this->m27 = std::forward< T27>(vec.m27); this->m28 = std::forward< T28>(vec.m28); this->m29 = std::forward< T29>(vec.m29);
            return *this;
        }
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29>
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector30(
            vector30<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26 , U27 , U28 , U29> const& vec)
            : base_type(vec.m0 , vec.m1 , vec.m2 , vec.m3 , vec.m4 , vec.m5 , vec.m6 , vec.m7 , vec.m8 , vec.m9 , vec.m10 , vec.m11 , vec.m12 , vec.m13 , vec.m14 , vec.m15 , vec.m16 , vec.m17 , vec.m18 , vec.m19 , vec.m20 , vec.m21 , vec.m22 , vec.m23 , vec.m24 , vec.m25 , vec.m26 , vec.m27 , vec.m28 , vec.m29) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector30(
            Sequence const& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename Sequence>
# if !defined(BOOST_CLANG)
        BOOST_CXX14_CONSTEXPR
# endif
        BOOST_FUSION_GPU_ENABLED
        vector30(
            Sequence& seq
          , typename boost::enable_if<traits::is_sequence<Sequence> >::type* = 0
            )
            : base_type(base_type::init_from_sequence(seq)) {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector30&
        operator=(vector30<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26 , U27 , U28 , U29> const& vec)
        {
            this->m0 = vec.m0; this->m1 = vec.m1; this->m2 = vec.m2; this->m3 = vec.m3; this->m4 = vec.m4; this->m5 = vec.m5; this->m6 = vec.m6; this->m7 = vec.m7; this->m8 = vec.m8; this->m9 = vec.m9; this->m10 = vec.m10; this->m11 = vec.m11; this->m12 = vec.m12; this->m13 = vec.m13; this->m14 = vec.m14; this->m15 = vec.m15; this->m16 = vec.m16; this->m17 = vec.m17; this->m18 = vec.m18; this->m19 = vec.m19; this->m20 = vec.m20; this->m21 = vec.m21; this->m22 = vec.m22; this->m23 = vec.m23; this->m24 = vec.m24; this->m25 = vec.m25; this->m26 = vec.m26; this->m27 = vec.m27; this->m28 = vec.m28; this->m29 = vec.m29;
            return *this;
        }
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if<is_convertible<Sequence, T0>, this_type&>::type
        operator=(Sequence const& seq)
        {
            typedef typename result_of::begin<Sequence const>::type I0;
            I0 i0 = fusion::begin(seq);
            typedef typename result_of::next< I0>::type I1; I1 i1 = fusion::next(i0); typedef typename result_of::next< I1>::type I2; I2 i2 = fusion::next(i1); typedef typename result_of::next< I2>::type I3; I3 i3 = fusion::next(i2); typedef typename result_of::next< I3>::type I4; I4 i4 = fusion::next(i3); typedef typename result_of::next< I4>::type I5; I5 i5 = fusion::next(i4); typedef typename result_of::next< I5>::type I6; I6 i6 = fusion::next(i5); typedef typename result_of::next< I6>::type I7; I7 i7 = fusion::next(i6); typedef typename result_of::next< I7>::type I8; I8 i8 = fusion::next(i7); typedef typename result_of::next< I8>::type I9; I9 i9 = fusion::next(i8); typedef typename result_of::next< I9>::type I10; I10 i10 = fusion::next(i9); typedef typename result_of::next< I10>::type I11; I11 i11 = fusion::next(i10); typedef typename result_of::next< I11>::type I12; I12 i12 = fusion::next(i11); typedef typename result_of::next< I12>::type I13; I13 i13 = fusion::next(i12); typedef typename result_of::next< I13>::type I14; I14 i14 = fusion::next(i13); typedef typename result_of::next< I14>::type I15; I15 i15 = fusion::next(i14); typedef typename result_of::next< I15>::type I16; I16 i16 = fusion::next(i15); typedef typename result_of::next< I16>::type I17; I17 i17 = fusion::next(i16); typedef typename result_of::next< I17>::type I18; I18 i18 = fusion::next(i17); typedef typename result_of::next< I18>::type I19; I19 i19 = fusion::next(i18); typedef typename result_of::next< I19>::type I20; I20 i20 = fusion::next(i19); typedef typename result_of::next< I20>::type I21; I21 i21 = fusion::next(i20); typedef typename result_of::next< I21>::type I22; I22 i22 = fusion::next(i21); typedef typename result_of::next< I22>::type I23; I23 i23 = fusion::next(i22); typedef typename result_of::next< I23>::type I24; I24 i24 = fusion::next(i23); typedef typename result_of::next< I24>::type I25; I25 i25 = fusion::next(i24); typedef typename result_of::next< I25>::type I26; I26 i26 = fusion::next(i25); typedef typename result_of::next< I26>::type I27; I27 i27 = fusion::next(i26); typedef typename result_of::next< I27>::type I28; I28 i28 = fusion::next(i27); typedef typename result_of::next< I28>::type I29; I29 i29 = fusion::next(i28);
            this->m0 = *i0; this->m1 = *i1; this->m2 = *i2; this->m3 = *i3; this->m4 = *i4; this->m5 = *i5; this->m6 = *i6; this->m7 = *i7; this->m8 = *i8; this->m9 = *i9; this->m10 = *i10; this->m11 = *i11; this->m12 = *i12; this->m13 = *i13; this->m14 = *i14; this->m15 = *i15; this->m16 = *i16; this->m17 = *i17; this->m18 = *i18; this->m19 = *i19; this->m20 = *i20; this->m21 = *i21; this->m22 = *i22; this->m23 = *i23; this->m24 = *i24; this->m25 = *i25; this->m26 = *i26; this->m27 = *i27; this->m28 = *i28; this->m29 = *i29;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T0>::type at_impl(mpl::int_<0>) { return this->m0; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T0>::type>::type at_impl(mpl::int_<0>) const { return this->m0; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T1>::type at_impl(mpl::int_<1>) { return this->m1; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T1>::type>::type at_impl(mpl::int_<1>) const { return this->m1; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T2>::type at_impl(mpl::int_<2>) { return this->m2; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T2>::type>::type at_impl(mpl::int_<2>) const { return this->m2; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T3>::type at_impl(mpl::int_<3>) { return this->m3; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T3>::type>::type at_impl(mpl::int_<3>) const { return this->m3; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T4>::type at_impl(mpl::int_<4>) { return this->m4; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T4>::type>::type at_impl(mpl::int_<4>) const { return this->m4; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T5>::type at_impl(mpl::int_<5>) { return this->m5; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T5>::type>::type at_impl(mpl::int_<5>) const { return this->m5; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T6>::type at_impl(mpl::int_<6>) { return this->m6; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T6>::type>::type at_impl(mpl::int_<6>) const { return this->m6; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T7>::type at_impl(mpl::int_<7>) { return this->m7; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T7>::type>::type at_impl(mpl::int_<7>) const { return this->m7; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T8>::type at_impl(mpl::int_<8>) { return this->m8; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T8>::type>::type at_impl(mpl::int_<8>) const { return this->m8; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T9>::type at_impl(mpl::int_<9>) { return this->m9; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T9>::type>::type at_impl(mpl::int_<9>) const { return this->m9; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T10>::type at_impl(mpl::int_<10>) { return this->m10; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T10>::type>::type at_impl(mpl::int_<10>) const { return this->m10; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T11>::type at_impl(mpl::int_<11>) { return this->m11; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T11>::type>::type at_impl(mpl::int_<11>) const { return this->m11; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T12>::type at_impl(mpl::int_<12>) { return this->m12; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T12>::type>::type at_impl(mpl::int_<12>) const { return this->m12; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T13>::type at_impl(mpl::int_<13>) { return this->m13; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T13>::type>::type at_impl(mpl::int_<13>) const { return this->m13; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T14>::type at_impl(mpl::int_<14>) { return this->m14; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T14>::type>::type at_impl(mpl::int_<14>) const { return this->m14; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T15>::type at_impl(mpl::int_<15>) { return this->m15; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T15>::type>::type at_impl(mpl::int_<15>) const { return this->m15; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T16>::type at_impl(mpl::int_<16>) { return this->m16; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T16>::type>::type at_impl(mpl::int_<16>) const { return this->m16; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T17>::type at_impl(mpl::int_<17>) { return this->m17; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T17>::type>::type at_impl(mpl::int_<17>) const { return this->m17; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T18>::type at_impl(mpl::int_<18>) { return this->m18; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T18>::type>::type at_impl(mpl::int_<18>) const { return this->m18; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T19>::type at_impl(mpl::int_<19>) { return this->m19; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T19>::type>::type at_impl(mpl::int_<19>) const { return this->m19; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T20>::type at_impl(mpl::int_<20>) { return this->m20; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T20>::type>::type at_impl(mpl::int_<20>) const { return this->m20; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T21>::type at_impl(mpl::int_<21>) { return this->m21; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T21>::type>::type at_impl(mpl::int_<21>) const { return this->m21; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T22>::type at_impl(mpl::int_<22>) { return this->m22; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T22>::type>::type at_impl(mpl::int_<22>) const { return this->m22; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T23>::type at_impl(mpl::int_<23>) { return this->m23; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T23>::type>::type at_impl(mpl::int_<23>) const { return this->m23; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T24>::type at_impl(mpl::int_<24>) { return this->m24; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T24>::type>::type at_impl(mpl::int_<24>) const { return this->m24; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T25>::type at_impl(mpl::int_<25>) { return this->m25; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T25>::type>::type at_impl(mpl::int_<25>) const { return this->m25; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T26>::type at_impl(mpl::int_<26>) { return this->m26; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T26>::type>::type at_impl(mpl::int_<26>) const { return this->m26; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T27>::type at_impl(mpl::int_<27>) { return this->m27; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T27>::type>::type at_impl(mpl::int_<27>) const { return this->m27; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T28>::type at_impl(mpl::int_<28>) { return this->m28; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T28>::type>::type at_impl(mpl::int_<28>) const { return this->m28; } BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<T29>::type at_impl(mpl::int_<29>) { return this->m29; } BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED typename add_reference<typename add_const<T29>::type>::type at_impl(mpl::int_<29>) const { return this->m29; }
        template<typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename mpl::at<types, I>::type>::type
        at_impl(I)
        {
            return this->at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<typename add_const<typename mpl::at<types, I>::type>::type>::type
        at_impl(I) const
        {
            return this->at_impl(mpl::int_<I::value>());
        }
    };
}}
