/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion
{
    struct void_;
    struct fusion_sequence_tag;
    template <typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9 , typename T10 , typename T11 , typename T12 , typename T13 , typename T14 , typename T15 , typename T16 , typename T17 , typename T18 , typename T19 , typename T20 , typename T21 , typename T22 , typename T23 , typename T24 , typename T25 , typename T26 , typename T27 , typename T28 , typename T29 , typename T30 , typename T31 , typename T32 , typename T33 , typename T34 , typename T35 , typename T36 , typename T37 , typename T38 , typename T39 , typename T40 , typename T41 , typename T42 , typename T43 , typename T44 , typename T45 , typename T46 , typename T47 , typename T48 , typename T49>
    struct vector
        : sequence_base<vector<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34 , T35 , T36 , T37 , T38 , T39 , T40 , T41 , T42 , T43 , T44 , T45 , T46 , T47 , T48 , T49> >
    {
    private:
        typedef typename detail::vector_n_chooser<
            T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9 , T10 , T11 , T12 , T13 , T14 , T15 , T16 , T17 , T18 , T19 , T20 , T21 , T22 , T23 , T24 , T25 , T26 , T27 , T28 , T29 , T30 , T31 , T32 , T33 , T34 , T35 , T36 , T37 , T38 , T39 , T40 , T41 , T42 , T43 , T44 , T45 , T46 , T47 , T48 , T49>::type
        vector_n;
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43 , typename U44 , typename U45 , typename U46 , typename U47 , typename U48 , typename U49>
        friend struct vector;
    public:
        typedef typename vector_n::types types;
        typedef typename vector_n::fusion_tag fusion_tag;
        typedef typename vector_n::tag tag;
        typedef typename vector_n::size size;
        typedef typename vector_n::category category;
        typedef typename vector_n::is_view is_view;
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector()
            : vec() {}
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43 , typename U44 , typename U45 , typename U46 , typename U47 , typename U48 , typename U49>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector(vector<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26 , U27 , U28 , U29 , U30 , U31 , U32 , U33 , U34 , U35 , U36 , U37 , U38 , U39 , U40 , U41 , U42 , U43 , U44 , U45 , U46 , U47 , U48 , U49> const& rhs)
            : vec(rhs.vec) {}
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector(vector const& rhs)
            : vec(rhs.vec) {}
        template <typename Sequence>
        BOOST_FUSION_GPU_ENABLED
        vector(Sequence const& rhs,
            typename enable_if<traits::is_sequence<Sequence>, detail::enabler_>::type = detail::enabler)
            : vec(BOOST_FUSION_VECTOR_COPY_INIT()) {}
        
        
        
        
        
        
        
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    explicit
    vector(typename detail::call_param<T0 >::type arg0)
        : vec(arg0) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    explicit
    vector(U0 && arg0
        , typename boost::disable_if_c<boost::is_same<vector, typename boost::remove_cv_ref<U0>::type>::value, detail::enabler_>::type = detail::enabler
        )
        : vec(std::forward<U0>( arg0)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1)
        : vec(arg0 , arg1) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2)
        : vec(arg0 , arg1 , arg2) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3)
        : vec(arg0 , arg1 , arg2 , arg3) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40 , typename detail::call_param<T41 >::type arg41)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40 , arg41) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40 , U41 && arg41
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40) , std::forward<U41>( arg41)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40 , typename detail::call_param<T41 >::type arg41 , typename detail::call_param<T42 >::type arg42)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40 , arg41 , arg42) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40 , U41 && arg41 , U42 && arg42
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40) , std::forward<U41>( arg41) , std::forward<U42>( arg42)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40 , typename detail::call_param<T41 >::type arg41 , typename detail::call_param<T42 >::type arg42 , typename detail::call_param<T43 >::type arg43)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40 , arg41 , arg42 , arg43) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40 , U41 && arg41 , U42 && arg42 , U43 && arg43
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40) , std::forward<U41>( arg41) , std::forward<U42>( arg42) , std::forward<U43>( arg43)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40 , typename detail::call_param<T41 >::type arg41 , typename detail::call_param<T42 >::type arg42 , typename detail::call_param<T43 >::type arg43 , typename detail::call_param<T44 >::type arg44)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40 , arg41 , arg42 , arg43 , arg44) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43 , typename U44>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40 , U41 && arg41 , U42 && arg42 , U43 && arg43 , U44 && arg44
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40) , std::forward<U41>( arg41) , std::forward<U42>( arg42) , std::forward<U43>( arg43) , std::forward<U44>( arg44)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40 , typename detail::call_param<T41 >::type arg41 , typename detail::call_param<T42 >::type arg42 , typename detail::call_param<T43 >::type arg43 , typename detail::call_param<T44 >::type arg44 , typename detail::call_param<T45 >::type arg45)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40 , arg41 , arg42 , arg43 , arg44 , arg45) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43 , typename U44 , typename U45>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40 , U41 && arg41 , U42 && arg42 , U43 && arg43 , U44 && arg44 , U45 && arg45
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40) , std::forward<U41>( arg41) , std::forward<U42>( arg42) , std::forward<U43>( arg43) , std::forward<U44>( arg44) , std::forward<U45>( arg45)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40 , typename detail::call_param<T41 >::type arg41 , typename detail::call_param<T42 >::type arg42 , typename detail::call_param<T43 >::type arg43 , typename detail::call_param<T44 >::type arg44 , typename detail::call_param<T45 >::type arg45 , typename detail::call_param<T46 >::type arg46)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40 , arg41 , arg42 , arg43 , arg44 , arg45 , arg46) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43 , typename U44 , typename U45 , typename U46>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40 , U41 && arg41 , U42 && arg42 , U43 && arg43 , U44 && arg44 , U45 && arg45 , U46 && arg46
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40) , std::forward<U41>( arg41) , std::forward<U42>( arg42) , std::forward<U43>( arg43) , std::forward<U44>( arg44) , std::forward<U45>( arg45) , std::forward<U46>( arg46)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40 , typename detail::call_param<T41 >::type arg41 , typename detail::call_param<T42 >::type arg42 , typename detail::call_param<T43 >::type arg43 , typename detail::call_param<T44 >::type arg44 , typename detail::call_param<T45 >::type arg45 , typename detail::call_param<T46 >::type arg46 , typename detail::call_param<T47 >::type arg47)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40 , arg41 , arg42 , arg43 , arg44 , arg45 , arg46 , arg47) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43 , typename U44 , typename U45 , typename U46 , typename U47>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40 , U41 && arg41 , U42 && arg42 , U43 && arg43 , U44 && arg44 , U45 && arg45 , U46 && arg46 , U47 && arg47
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40) , std::forward<U41>( arg41) , std::forward<U42>( arg42) , std::forward<U43>( arg43) , std::forward<U44>( arg44) , std::forward<U45>( arg45) , std::forward<U46>( arg46) , std::forward<U47>( arg47)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40 , typename detail::call_param<T41 >::type arg41 , typename detail::call_param<T42 >::type arg42 , typename detail::call_param<T43 >::type arg43 , typename detail::call_param<T44 >::type arg44 , typename detail::call_param<T45 >::type arg45 , typename detail::call_param<T46 >::type arg46 , typename detail::call_param<T47 >::type arg47 , typename detail::call_param<T48 >::type arg48)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40 , arg41 , arg42 , arg43 , arg44 , arg45 , arg46 , arg47 , arg48) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43 , typename U44 , typename U45 , typename U46 , typename U47 , typename U48>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40 , U41 && arg41 , U42 && arg42 , U43 && arg43 , U44 && arg44 , U45 && arg45 , U46 && arg46 , U47 && arg47 , U48 && arg48
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40) , std::forward<U41>( arg41) , std::forward<U42>( arg42) , std::forward<U43>( arg43) , std::forward<U44>( arg44) , std::forward<U45>( arg45) , std::forward<U46>( arg46) , std::forward<U47>( arg47) , std::forward<U48>( arg48)) {}
# endif
    
# if !defined(BOOST_CLANG)
    BOOST_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(typename detail::call_param<T0 >::type arg0 , typename detail::call_param<T1 >::type arg1 , typename detail::call_param<T2 >::type arg2 , typename detail::call_param<T3 >::type arg3 , typename detail::call_param<T4 >::type arg4 , typename detail::call_param<T5 >::type arg5 , typename detail::call_param<T6 >::type arg6 , typename detail::call_param<T7 >::type arg7 , typename detail::call_param<T8 >::type arg8 , typename detail::call_param<T9 >::type arg9 , typename detail::call_param<T10 >::type arg10 , typename detail::call_param<T11 >::type arg11 , typename detail::call_param<T12 >::type arg12 , typename detail::call_param<T13 >::type arg13 , typename detail::call_param<T14 >::type arg14 , typename detail::call_param<T15 >::type arg15 , typename detail::call_param<T16 >::type arg16 , typename detail::call_param<T17 >::type arg17 , typename detail::call_param<T18 >::type arg18 , typename detail::call_param<T19 >::type arg19 , typename detail::call_param<T20 >::type arg20 , typename detail::call_param<T21 >::type arg21 , typename detail::call_param<T22 >::type arg22 , typename detail::call_param<T23 >::type arg23 , typename detail::call_param<T24 >::type arg24 , typename detail::call_param<T25 >::type arg25 , typename detail::call_param<T26 >::type arg26 , typename detail::call_param<T27 >::type arg27 , typename detail::call_param<T28 >::type arg28 , typename detail::call_param<T29 >::type arg29 , typename detail::call_param<T30 >::type arg30 , typename detail::call_param<T31 >::type arg31 , typename detail::call_param<T32 >::type arg32 , typename detail::call_param<T33 >::type arg33 , typename detail::call_param<T34 >::type arg34 , typename detail::call_param<T35 >::type arg35 , typename detail::call_param<T36 >::type arg36 , typename detail::call_param<T37 >::type arg37 , typename detail::call_param<T38 >::type arg38 , typename detail::call_param<T39 >::type arg39 , typename detail::call_param<T40 >::type arg40 , typename detail::call_param<T41 >::type arg41 , typename detail::call_param<T42 >::type arg42 , typename detail::call_param<T43 >::type arg43 , typename detail::call_param<T44 >::type arg44 , typename detail::call_param<T45 >::type arg45 , typename detail::call_param<T46 >::type arg46 , typename detail::call_param<T47 >::type arg47 , typename detail::call_param<T48 >::type arg48 , typename detail::call_param<T49 >::type arg49)
        : vec(arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 , arg9 , arg10 , arg11 , arg12 , arg13 , arg14 , arg15 , arg16 , arg17 , arg18 , arg19 , arg20 , arg21 , arg22 , arg23 , arg24 , arg25 , arg26 , arg27 , arg28 , arg29 , arg30 , arg31 , arg32 , arg33 , arg34 , arg35 , arg36 , arg37 , arg38 , arg39 , arg40 , arg41 , arg42 , arg43 , arg44 , arg45 , arg46 , arg47 , arg48 , arg49) {}
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43 , typename U44 , typename U45 , typename U46 , typename U47 , typename U48 , typename U49>
    
# if !defined(BOOST_CLANG)
    BOOST_CXX14_CONSTEXPR
# endif
    BOOST_FUSION_GPU_ENABLED
    vector(U0 && arg0 , U1 && arg1 , U2 && arg2 , U3 && arg3 , U4 && arg4 , U5 && arg5 , U6 && arg6 , U7 && arg7 , U8 && arg8 , U9 && arg9 , U10 && arg10 , U11 && arg11 , U12 && arg12 , U13 && arg13 , U14 && arg14 , U15 && arg15 , U16 && arg16 , U17 && arg17 , U18 && arg18 , U19 && arg19 , U20 && arg20 , U21 && arg21 , U22 && arg22 , U23 && arg23 , U24 && arg24 , U25 && arg25 , U26 && arg26 , U27 && arg27 , U28 && arg28 , U29 && arg29 , U30 && arg30 , U31 && arg31 , U32 && arg32 , U33 && arg33 , U34 && arg34 , U35 && arg35 , U36 && arg36 , U37 && arg37 , U38 && arg38 , U39 && arg39 , U40 && arg40 , U41 && arg41 , U42 && arg42 , U43 && arg43 , U44 && arg44 , U45 && arg45 , U46 && arg46 , U47 && arg47 , U48 && arg48 , U49 && arg49
        )
        : vec(std::forward<U0>( arg0) , std::forward<U1>( arg1) , std::forward<U2>( arg2) , std::forward<U3>( arg3) , std::forward<U4>( arg4) , std::forward<U5>( arg5) , std::forward<U6>( arg6) , std::forward<U7>( arg7) , std::forward<U8>( arg8) , std::forward<U9>( arg9) , std::forward<U10>( arg10) , std::forward<U11>( arg11) , std::forward<U12>( arg12) , std::forward<U13>( arg13) , std::forward<U14>( arg14) , std::forward<U15>( arg15) , std::forward<U16>( arg16) , std::forward<U17>( arg17) , std::forward<U18>( arg18) , std::forward<U19>( arg19) , std::forward<U20>( arg20) , std::forward<U21>( arg21) , std::forward<U22>( arg22) , std::forward<U23>( arg23) , std::forward<U24>( arg24) , std::forward<U25>( arg25) , std::forward<U26>( arg26) , std::forward<U27>( arg27) , std::forward<U28>( arg28) , std::forward<U29>( arg29) , std::forward<U30>( arg30) , std::forward<U31>( arg31) , std::forward<U32>( arg32) , std::forward<U33>( arg33) , std::forward<U34>( arg34) , std::forward<U35>( arg35) , std::forward<U36>( arg36) , std::forward<U37>( arg37) , std::forward<U38>( arg38) , std::forward<U39>( arg39) , std::forward<U40>( arg40) , std::forward<U41>( arg41) , std::forward<U42>( arg42) , std::forward<U43>( arg43) , std::forward<U44>( arg44) , std::forward<U45>( arg45) , std::forward<U46>( arg46) , std::forward<U47>( arg47) , std::forward<U48>( arg48) , std::forward<U49>( arg49)) {}
# endif
        template <typename U0 , typename U1 , typename U2 , typename U3 , typename U4 , typename U5 , typename U6 , typename U7 , typename U8 , typename U9 , typename U10 , typename U11 , typename U12 , typename U13 , typename U14 , typename U15 , typename U16 , typename U17 , typename U18 , typename U19 , typename U20 , typename U21 , typename U22 , typename U23 , typename U24 , typename U25 , typename U26 , typename U27 , typename U28 , typename U29 , typename U30 , typename U31 , typename U32 , typename U33 , typename U34 , typename U35 , typename U36 , typename U37 , typename U38 , typename U39 , typename U40 , typename U41 , typename U42 , typename U43 , typename U44 , typename U45 , typename U46 , typename U47 , typename U48 , typename U49>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector&
        operator=(vector<U0 , U1 , U2 , U3 , U4 , U5 , U6 , U7 , U8 , U9 , U10 , U11 , U12 , U13 , U14 , U15 , U16 , U17 , U18 , U19 , U20 , U21 , U22 , U23 , U24 , U25 , U26 , U27 , U28 , U29 , U30 , U31 , U32 , U33 , U34 , U35 , U36 , U37 , U38 , U39 , U40 , U41 , U42 , U43 , U44 , U45 , U46 , U47 , U48 , U49> const& rhs)
        {
            vec = rhs.vec;
            return *this;
        }
        template <typename T>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector&
        operator=(T const& rhs)
        {
            vec = rhs;
            return *this;
        }
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector&
        operator=(vector const& rhs)
        {
            vec = rhs.vec;
            return *this;
        }
# if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector(vector&& rhs)
            : vec(std::forward<vector_n>(rhs.vec)) {}
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector&
        operator=(vector&& rhs)
        {
            vec = std::forward<vector_n>(rhs.vec);
            return *this;
        }
        template <typename T>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::disable_if_c<
            boost::is_same<vector, typename boost::remove_cv_ref<T>::type>::value
          , vector&
        >::type
        operator=(T&& rhs)
        {
            vec = std::forward<T>( rhs);
            return *this;
        }
# endif
        template <int N>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<
            typename mpl::at_c<types, N>::type
        >::type
        at_impl(mpl::int_<N> index)
        {
            return vec.at_impl(index);
        }
        template <int N>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<
            typename add_const<
                typename mpl::at_c<types, N>::type
            >::type
        >::type
        at_impl(mpl::int_<N> index) const
        {
            return vec.at_impl(index);
        }
        template <typename I>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<
            typename mpl::at<types, I>::type
        >::type
        at_impl(I )
        {
            return vec.at_impl(mpl::int_<I::value>());
        }
        template<typename I>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename add_reference<
            typename add_const<
                typename mpl::at<types, I>::type
            >::type
        >::type
        at_impl(I ) const
        {
            return vec.at_impl(mpl::int_<I::value>());
        }
    private:
        BOOST_FUSION_VECTOR_CTOR_HELPER()
        vector_n vec;
    };
}}
