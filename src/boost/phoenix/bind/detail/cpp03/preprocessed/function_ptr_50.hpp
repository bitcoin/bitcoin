/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<2, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1>
            RT operator()(A0 & a0 , A1 & a1) const
            {
                return fp(a0 , a1);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<3, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2) const
            {
                return fp(a0 , a1 , a2);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<4, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3) const
            {
                return fp(a0 , a1 , a2 , a3);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<5, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4) const
            {
                return fp(a0 , a1 , a2 , a3 , a4);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<6, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<7, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<8, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<9, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<10, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<11, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<12, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<13, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<14, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<15, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<16, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<17, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<18, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<19, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<20, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<21, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<22, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<23, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<24, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<25, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<26, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<27, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<28, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<29, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<30, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<31, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<32, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<33, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<34, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<35, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<36, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<37, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<38, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<39, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<40, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<41, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<42, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40 , A41 & a41) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<43, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40 , A41 & a41 , A42 & a42) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<44, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40 , A41 & a41 , A42 & a42 , A43 & a43) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<45, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40 , A41 & a41 , A42 & a42 , A43 & a43 , A44 & a44) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<46, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40 , A41 & a41 , A42 & a42 , A43 & a43 , A44 & a44 , A45 & a45) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<47, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40 , A41 & a41 , A42 & a42 , A43 & a43 , A44 & a44 , A45 & a45 , A46 & a46) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45 , a46);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<48, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40 , A41 & a41 , A42 & a42 , A43 & a43 , A44 & a44 , A45 & a45 , A46 & a46 , A47 & a47) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45 , a46 , a47);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<49, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47 , typename A48>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40 , A41 & a41 , A42 & a42 , A43 & a43 , A44 & a44 , A45 & a45 , A46 & a46 , A47 & a47 , A48 & a48) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45 , a46 , a47 , a48);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <typename Dummy>
    struct function_ptr_impl<50, Dummy>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47 , typename A48 , typename A49>
            RT operator()(A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19 , A20 & a20 , A21 & a21 , A22 & a22 , A23 & a23 , A24 & a24 , A25 & a25 , A26 & a26 , A27 & a27 , A28 & a28 , A29 & a29 , A30 & a30 , A31 & a31 , A32 & a32 , A33 & a33 , A34 & a34 , A35 & a35 , A36 & a36 , A37 & a37 , A38 & a38 , A39 & a39 , A40 & a40 , A41 & a41 , A42 & a42 , A43 & a43 , A44 & a44 , A45 & a45 , A46 & a46 , A47 & a47 , A48 & a48 , A49 & a49) const
            {
                return fp(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45 , a46 , a47 , a48 , a49);
            }
            FP fp;
        };
    };
