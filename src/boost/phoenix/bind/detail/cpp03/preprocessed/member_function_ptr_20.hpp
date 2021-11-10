/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<1>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0>
            RT operator()(Class& obj, A0 & a0) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0);
            }
            template <typename Class, typename A0>
            RT operator()(Class* obj, A0 & a0) const
            {
                return (obj->*fp)(a0);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<2>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1>
            RT operator()(Class& obj, A0 & a0 , A1 & a1) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1);
            }
            template <typename Class, typename A0 , typename A1>
            RT operator()(Class* obj, A0 & a0 , A1 & a1) const
            {
                return (obj->*fp)(a0 , a1);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<3>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2);
            }
            template <typename Class, typename A0 , typename A1 , typename A2>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2) const
            {
                return (obj->*fp)(a0 , a1 , a2);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<4>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<5>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<6>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<7>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<8>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<9>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<10>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<11>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<12>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<13>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<14>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<15>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<16>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<17>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<18>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<19>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18);
            }
            FP fp;
        };
    };
    
    
    
    
    
    
    
    template <>
    struct member_function_ptr_impl<20>
    {
        template <typename RT, typename FP>
        struct impl
        {
            typedef RT result_type;
            impl(FP fp_)
                : fp(fp_) {}
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19>
            RT operator()(Class& obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19);
            }
            template <typename Class, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19>
            RT operator()(Class* obj, A0 & a0 , A1 & a1 , A2 & a2 , A3 & a3 , A4 & a4 , A5 & a5 , A6 & a6 , A7 & a7 , A8 & a8 , A9 & a9 , A10 & a10 , A11 & a11 , A12 & a12 , A13 & a13 , A14 & a14 , A15 & a15 , A16 & a16 , A17 & a17 , A18 & a18 , A19 & a19) const
            {
                return (obj->*fp)(a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19);
            }
            FP fp;
        };
    };
