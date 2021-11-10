/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
namespace boost { namespace phoenix
{
    template <typename Dummy = void>
    struct vector0
    {
        typedef mpl::int_<0> size_type;
        static const int size_value = 0;
    };
    template <int> struct vector_chooser;
    template <>
    struct vector_chooser<0>
    {
        template <typename Dummy = void>
        struct apply
        {
            typedef vector0<> type;
        };
    };
}}
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0>
    struct vector1
    {
        typedef A0 member_type0; A0 a0;
        
        typedef mpl::int_<1> size_type;
        static const int size_value = 1;
        typedef
            vector0<>
            args_type;
        args_type args() const
        {
            args_type r = {};
            return r;
        }
    };
    template <>
    struct vector_chooser<1>
    {
        template <typename A0>
        struct apply
        {
            typedef vector1<A0> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0)
  , ( boost::phoenix::vector1 ) (A0)
  , (A0, a0)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1>
    struct vector2
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1;
        
        typedef mpl::int_<2> size_type;
        static const int size_value = 2;
        typedef
            vector1<A1>
            args_type;
        args_type args() const
        {
            args_type r = {a1};
            return r;
        }
    };
    template <>
    struct vector_chooser<2>
    {
        template <typename A0 , typename A1>
        struct apply
        {
            typedef vector2<A0 , A1> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1)
  , ( boost::phoenix::vector2 ) (A0) (A1)
  , (A0, a0) (A1, a1)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2>
    struct vector3
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2;
        
        typedef mpl::int_<3> size_type;
        static const int size_value = 3;
        typedef
            vector2<A1 , A2>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2};
            return r;
        }
    };
    template <>
    struct vector_chooser<3>
    {
        template <typename A0 , typename A1 , typename A2>
        struct apply
        {
            typedef vector3<A0 , A1 , A2> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2)
  , ( boost::phoenix::vector3 ) (A0) (A1) (A2)
  , (A0, a0) (A1, a1) (A2, a2)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3>
    struct vector4
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3;
        
        typedef mpl::int_<4> size_type;
        static const int size_value = 4;
        typedef
            vector3<A1 , A2 , A3>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3};
            return r;
        }
    };
    template <>
    struct vector_chooser<4>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3>
        struct apply
        {
            typedef vector4<A0 , A1 , A2 , A3> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3)
  , ( boost::phoenix::vector4 ) (A0) (A1) (A2) (A3)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct vector5
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4;
        
        typedef mpl::int_<5> size_type;
        static const int size_value = 5;
        typedef
            vector4<A1 , A2 , A3 , A4>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4};
            return r;
        }
    };
    template <>
    struct vector_chooser<5>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        struct apply
        {
            typedef vector5<A0 , A1 , A2 , A3 , A4> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4)
  , ( boost::phoenix::vector5 ) (A0) (A1) (A2) (A3) (A4)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct vector6
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5;
        
        typedef mpl::int_<6> size_type;
        static const int size_value = 6;
        typedef
            vector5<A1 , A2 , A3 , A4 , A5>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5};
            return r;
        }
    };
    template <>
    struct vector_chooser<6>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        struct apply
        {
            typedef vector6<A0 , A1 , A2 , A3 , A4 , A5> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5)
  , ( boost::phoenix::vector6 ) (A0) (A1) (A2) (A3) (A4) (A5)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct vector7
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6;
        
        typedef mpl::int_<7> size_type;
        static const int size_value = 7;
        typedef
            vector6<A1 , A2 , A3 , A4 , A5 , A6>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6};
            return r;
        }
    };
    template <>
    struct vector_chooser<7>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        struct apply
        {
            typedef vector7<A0 , A1 , A2 , A3 , A4 , A5 , A6> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6)
  , ( boost::phoenix::vector7 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct vector8
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7;
        
        typedef mpl::int_<8> size_type;
        static const int size_value = 8;
        typedef
            vector7<A1 , A2 , A3 , A4 , A5 , A6 , A7>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7};
            return r;
        }
    };
    template <>
    struct vector_chooser<8>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        struct apply
        {
            typedef vector8<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7)
  , ( boost::phoenix::vector8 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct vector9
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8;
        
        typedef mpl::int_<9> size_type;
        static const int size_value = 9;
        typedef
            vector8<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8};
            return r;
        }
    };
    template <>
    struct vector_chooser<9>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        struct apply
        {
            typedef vector9<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8)
  , ( boost::phoenix::vector9 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct vector10
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9;
        
        typedef mpl::int_<10> size_type;
        static const int size_value = 10;
        typedef
            vector9<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9};
            return r;
        }
    };
    template <>
    struct vector_chooser<10>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
        struct apply
        {
            typedef vector10<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9)
  , ( boost::phoenix::vector10 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10>
    struct vector11
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10;
        
        typedef mpl::int_<11> size_type;
        static const int size_value = 11;
        typedef
            vector10<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10};
            return r;
        }
    };
    template <>
    struct vector_chooser<11>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10>
        struct apply
        {
            typedef vector11<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10)
  , ( boost::phoenix::vector11 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11>
    struct vector12
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11;
        
        typedef mpl::int_<12> size_type;
        static const int size_value = 12;
        typedef
            vector11<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11};
            return r;
        }
    };
    template <>
    struct vector_chooser<12>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11>
        struct apply
        {
            typedef vector12<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11)
  , ( boost::phoenix::vector12 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12>
    struct vector13
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12;
        
        typedef mpl::int_<13> size_type;
        static const int size_value = 13;
        typedef
            vector12<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12};
            return r;
        }
    };
    template <>
    struct vector_chooser<13>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12>
        struct apply
        {
            typedef vector13<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12)
  , ( boost::phoenix::vector13 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13>
    struct vector14
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13;
        
        typedef mpl::int_<14> size_type;
        static const int size_value = 14;
        typedef
            vector13<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13};
            return r;
        }
    };
    template <>
    struct vector_chooser<14>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13>
        struct apply
        {
            typedef vector14<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13)
  , ( boost::phoenix::vector14 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14>
    struct vector15
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14;
        
        typedef mpl::int_<15> size_type;
        static const int size_value = 15;
        typedef
            vector14<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14};
            return r;
        }
    };
    template <>
    struct vector_chooser<15>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14>
        struct apply
        {
            typedef vector15<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14)
  , ( boost::phoenix::vector15 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15>
    struct vector16
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15;
        
        typedef mpl::int_<16> size_type;
        static const int size_value = 16;
        typedef
            vector15<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15};
            return r;
        }
    };
    template <>
    struct vector_chooser<16>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15>
        struct apply
        {
            typedef vector16<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15)
  , ( boost::phoenix::vector16 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16>
    struct vector17
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16;
        
        typedef mpl::int_<17> size_type;
        static const int size_value = 17;
        typedef
            vector16<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16};
            return r;
        }
    };
    template <>
    struct vector_chooser<17>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16>
        struct apply
        {
            typedef vector17<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16)
  , ( boost::phoenix::vector17 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17>
    struct vector18
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17;
        
        typedef mpl::int_<18> size_type;
        static const int size_value = 18;
        typedef
            vector17<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17};
            return r;
        }
    };
    template <>
    struct vector_chooser<18>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17>
        struct apply
        {
            typedef vector18<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17)
  , ( boost::phoenix::vector18 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18>
    struct vector19
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18;
        
        typedef mpl::int_<19> size_type;
        static const int size_value = 19;
        typedef
            vector18<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18};
            return r;
        }
    };
    template <>
    struct vector_chooser<19>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18>
        struct apply
        {
            typedef vector19<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18)
  , ( boost::phoenix::vector19 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19>
    struct vector20
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19;
        
        typedef mpl::int_<20> size_type;
        static const int size_value = 20;
        typedef
            vector19<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19};
            return r;
        }
    };
    template <>
    struct vector_chooser<20>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19>
        struct apply
        {
            typedef vector20<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19)
  , ( boost::phoenix::vector20 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20>
    struct vector21
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20;
        
        typedef mpl::int_<21> size_type;
        static const int size_value = 21;
        typedef
            vector20<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20};
            return r;
        }
    };
    template <>
    struct vector_chooser<21>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20>
        struct apply
        {
            typedef vector21<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20)
  , ( boost::phoenix::vector21 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21>
    struct vector22
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21;
        
        typedef mpl::int_<22> size_type;
        static const int size_value = 22;
        typedef
            vector21<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21};
            return r;
        }
    };
    template <>
    struct vector_chooser<22>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21>
        struct apply
        {
            typedef vector22<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21)
  , ( boost::phoenix::vector22 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22>
    struct vector23
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22;
        
        typedef mpl::int_<23> size_type;
        static const int size_value = 23;
        typedef
            vector22<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22};
            return r;
        }
    };
    template <>
    struct vector_chooser<23>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22>
        struct apply
        {
            typedef vector23<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22)
  , ( boost::phoenix::vector23 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23>
    struct vector24
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23;
        
        typedef mpl::int_<24> size_type;
        static const int size_value = 24;
        typedef
            vector23<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23};
            return r;
        }
    };
    template <>
    struct vector_chooser<24>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23>
        struct apply
        {
            typedef vector24<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23)
  , ( boost::phoenix::vector24 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24>
    struct vector25
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24;
        
        typedef mpl::int_<25> size_type;
        static const int size_value = 25;
        typedef
            vector24<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24};
            return r;
        }
    };
    template <>
    struct vector_chooser<25>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24>
        struct apply
        {
            typedef vector25<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24)
  , ( boost::phoenix::vector25 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25>
    struct vector26
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25;
        
        typedef mpl::int_<26> size_type;
        static const int size_value = 26;
        typedef
            vector25<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25};
            return r;
        }
    };
    template <>
    struct vector_chooser<26>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25>
        struct apply
        {
            typedef vector26<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25)
  , ( boost::phoenix::vector26 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26>
    struct vector27
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26;
        
        typedef mpl::int_<27> size_type;
        static const int size_value = 27;
        typedef
            vector26<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26};
            return r;
        }
    };
    template <>
    struct vector_chooser<27>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26>
        struct apply
        {
            typedef vector27<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26)
  , ( boost::phoenix::vector27 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27>
    struct vector28
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27;
        
        typedef mpl::int_<28> size_type;
        static const int size_value = 28;
        typedef
            vector27<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27};
            return r;
        }
    };
    template <>
    struct vector_chooser<28>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27>
        struct apply
        {
            typedef vector28<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27)
  , ( boost::phoenix::vector28 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28>
    struct vector29
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28;
        
        typedef mpl::int_<29> size_type;
        static const int size_value = 29;
        typedef
            vector28<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28};
            return r;
        }
    };
    template <>
    struct vector_chooser<29>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28>
        struct apply
        {
            typedef vector29<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28)
  , ( boost::phoenix::vector29 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29>
    struct vector30
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29;
        
        typedef mpl::int_<30> size_type;
        static const int size_value = 30;
        typedef
            vector29<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29};
            return r;
        }
    };
    template <>
    struct vector_chooser<30>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29>
        struct apply
        {
            typedef vector30<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29)
  , ( boost::phoenix::vector30 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30>
    struct vector31
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30;
        
        typedef mpl::int_<31> size_type;
        static const int size_value = 31;
        typedef
            vector30<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30};
            return r;
        }
    };
    template <>
    struct vector_chooser<31>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30>
        struct apply
        {
            typedef vector31<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30)
  , ( boost::phoenix::vector31 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31>
    struct vector32
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31;
        
        typedef mpl::int_<32> size_type;
        static const int size_value = 32;
        typedef
            vector31<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31};
            return r;
        }
    };
    template <>
    struct vector_chooser<32>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31>
        struct apply
        {
            typedef vector32<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31)
  , ( boost::phoenix::vector32 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32>
    struct vector33
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32;
        
        typedef mpl::int_<33> size_type;
        static const int size_value = 33;
        typedef
            vector32<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32};
            return r;
        }
    };
    template <>
    struct vector_chooser<33>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32>
        struct apply
        {
            typedef vector33<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32)
  , ( boost::phoenix::vector33 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33>
    struct vector34
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33;
        
        typedef mpl::int_<34> size_type;
        static const int size_value = 34;
        typedef
            vector33<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33};
            return r;
        }
    };
    template <>
    struct vector_chooser<34>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33>
        struct apply
        {
            typedef vector34<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33)
  , ( boost::phoenix::vector34 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34>
    struct vector35
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34;
        
        typedef mpl::int_<35> size_type;
        static const int size_value = 35;
        typedef
            vector34<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34};
            return r;
        }
    };
    template <>
    struct vector_chooser<35>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34>
        struct apply
        {
            typedef vector35<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34)
  , ( boost::phoenix::vector35 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35>
    struct vector36
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35;
        
        typedef mpl::int_<36> size_type;
        static const int size_value = 36;
        typedef
            vector35<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35};
            return r;
        }
    };
    template <>
    struct vector_chooser<36>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35>
        struct apply
        {
            typedef vector36<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35)
  , ( boost::phoenix::vector36 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36>
    struct vector37
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36;
        
        typedef mpl::int_<37> size_type;
        static const int size_value = 37;
        typedef
            vector36<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36};
            return r;
        }
    };
    template <>
    struct vector_chooser<37>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36>
        struct apply
        {
            typedef vector37<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36)
  , ( boost::phoenix::vector37 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37>
    struct vector38
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37;
        
        typedef mpl::int_<38> size_type;
        static const int size_value = 38;
        typedef
            vector37<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37};
            return r;
        }
    };
    template <>
    struct vector_chooser<38>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37>
        struct apply
        {
            typedef vector38<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37)
  , ( boost::phoenix::vector38 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38>
    struct vector39
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38;
        
        typedef mpl::int_<39> size_type;
        static const int size_value = 39;
        typedef
            vector38<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38};
            return r;
        }
    };
    template <>
    struct vector_chooser<39>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38>
        struct apply
        {
            typedef vector39<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38)
  , ( boost::phoenix::vector39 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39>
    struct vector40
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39;
        
        typedef mpl::int_<40> size_type;
        static const int size_value = 40;
        typedef
            vector39<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39};
            return r;
        }
    };
    template <>
    struct vector_chooser<40>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39>
        struct apply
        {
            typedef vector40<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39)
  , ( boost::phoenix::vector40 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40>
    struct vector41
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40;
        
        typedef mpl::int_<41> size_type;
        static const int size_value = 41;
        typedef
            vector40<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40};
            return r;
        }
    };
    template <>
    struct vector_chooser<41>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40>
        struct apply
        {
            typedef vector41<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40)
  , ( boost::phoenix::vector41 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41>
    struct vector42
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41;
        
        typedef mpl::int_<42> size_type;
        static const int size_value = 42;
        typedef
            vector41<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41};
            return r;
        }
    };
    template <>
    struct vector_chooser<42>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41>
        struct apply
        {
            typedef vector42<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41)
  , ( boost::phoenix::vector42 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42>
    struct vector43
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41; typedef A42 member_type42; A42 a42;
        
        typedef mpl::int_<43> size_type;
        static const int size_value = 43;
        typedef
            vector42<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42};
            return r;
        }
    };
    template <>
    struct vector_chooser<43>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42>
        struct apply
        {
            typedef vector43<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42)
  , ( boost::phoenix::vector43 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41) (A42, a42)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43>
    struct vector44
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41; typedef A42 member_type42; A42 a42; typedef A43 member_type43; A43 a43;
        
        typedef mpl::int_<44> size_type;
        static const int size_value = 44;
        typedef
            vector43<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43};
            return r;
        }
    };
    template <>
    struct vector_chooser<44>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43>
        struct apply
        {
            typedef vector44<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43)
  , ( boost::phoenix::vector44 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41) (A42, a42) (A43, a43)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44>
    struct vector45
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41; typedef A42 member_type42; A42 a42; typedef A43 member_type43; A43 a43; typedef A44 member_type44; A44 a44;
        
        typedef mpl::int_<45> size_type;
        static const int size_value = 45;
        typedef
            vector44<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44};
            return r;
        }
    };
    template <>
    struct vector_chooser<45>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44>
        struct apply
        {
            typedef vector45<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44)
  , ( boost::phoenix::vector45 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41) (A42, a42) (A43, a43) (A44, a44)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45>
    struct vector46
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41; typedef A42 member_type42; A42 a42; typedef A43 member_type43; A43 a43; typedef A44 member_type44; A44 a44; typedef A45 member_type45; A45 a45;
        
        typedef mpl::int_<46> size_type;
        static const int size_value = 46;
        typedef
            vector45<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45};
            return r;
        }
    };
    template <>
    struct vector_chooser<46>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45>
        struct apply
        {
            typedef vector46<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45)
  , ( boost::phoenix::vector46 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41) (A42, a42) (A43, a43) (A44, a44) (A45, a45)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46>
    struct vector47
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41; typedef A42 member_type42; A42 a42; typedef A43 member_type43; A43 a43; typedef A44 member_type44; A44 a44; typedef A45 member_type45; A45 a45; typedef A46 member_type46; A46 a46;
        
        typedef mpl::int_<47> size_type;
        static const int size_value = 47;
        typedef
            vector46<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45 , a46};
            return r;
        }
    };
    template <>
    struct vector_chooser<47>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46>
        struct apply
        {
            typedef vector47<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46)
  , ( boost::phoenix::vector47 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41) (A42, a42) (A43, a43) (A44, a44) (A45, a45) (A46, a46)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47>
    struct vector48
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41; typedef A42 member_type42; A42 a42; typedef A43 member_type43; A43 a43; typedef A44 member_type44; A44 a44; typedef A45 member_type45; A45 a45; typedef A46 member_type46; A46 a46; typedef A47 member_type47; A47 a47;
        
        typedef mpl::int_<48> size_type;
        static const int size_value = 48;
        typedef
            vector47<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46 , A47>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45 , a46 , a47};
            return r;
        }
    };
    template <>
    struct vector_chooser<48>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47>
        struct apply
        {
            typedef vector48<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46 , A47> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46) (A47)
  , ( boost::phoenix::vector48 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46) (A47)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41) (A42, a42) (A43, a43) (A44, a44) (A45, a45) (A46, a46) (A47, a47)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47 , typename A48>
    struct vector49
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41; typedef A42 member_type42; A42 a42; typedef A43 member_type43; A43 a43; typedef A44 member_type44; A44 a44; typedef A45 member_type45; A45 a45; typedef A46 member_type46; A46 a46; typedef A47 member_type47; A47 a47; typedef A48 member_type48; A48 a48;
        
        typedef mpl::int_<49> size_type;
        static const int size_value = 49;
        typedef
            vector48<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46 , A47 , A48>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45 , a46 , a47 , a48};
            return r;
        }
    };
    template <>
    struct vector_chooser<49>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47 , typename A48>
        struct apply
        {
            typedef vector49<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46 , A47 , A48> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46) (A47) (A48)
  , ( boost::phoenix::vector49 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46) (A47) (A48)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41) (A42, a42) (A43, a43) (A44, a44) (A45, a45) (A46, a46) (A47, a47) (A48, a48)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47 , typename A48 , typename A49>
    struct vector50
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41; typedef A42 member_type42; A42 a42; typedef A43 member_type43; A43 a43; typedef A44 member_type44; A44 a44; typedef A45 member_type45; A45 a45; typedef A46 member_type46; A46 a46; typedef A47 member_type47; A47 a47; typedef A48 member_type48; A48 a48; typedef A49 member_type49; A49 a49;
        
        typedef mpl::int_<50> size_type;
        static const int size_value = 50;
        typedef
            vector49<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46 , A47 , A48 , A49>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45 , a46 , a47 , a48 , a49};
            return r;
        }
    };
    template <>
    struct vector_chooser<50>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47 , typename A48 , typename A49>
        struct apply
        {
            typedef vector50<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46 , A47 , A48 , A49> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46) (A47) (A48) (A49)
  , ( boost::phoenix::vector50 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46) (A47) (A48) (A49)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41) (A42, a42) (A43, a43) (A44, a44) (A45, a45) (A46, a46) (A47, a47) (A48, a48) (A49, a49)
)
    
    
    
    
    
    
    
namespace boost { namespace phoenix
{
    template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47 , typename A48 , typename A49 , typename A50>
    struct vector51
    {
        typedef A0 member_type0; A0 a0; typedef A1 member_type1; A1 a1; typedef A2 member_type2; A2 a2; typedef A3 member_type3; A3 a3; typedef A4 member_type4; A4 a4; typedef A5 member_type5; A5 a5; typedef A6 member_type6; A6 a6; typedef A7 member_type7; A7 a7; typedef A8 member_type8; A8 a8; typedef A9 member_type9; A9 a9; typedef A10 member_type10; A10 a10; typedef A11 member_type11; A11 a11; typedef A12 member_type12; A12 a12; typedef A13 member_type13; A13 a13; typedef A14 member_type14; A14 a14; typedef A15 member_type15; A15 a15; typedef A16 member_type16; A16 a16; typedef A17 member_type17; A17 a17; typedef A18 member_type18; A18 a18; typedef A19 member_type19; A19 a19; typedef A20 member_type20; A20 a20; typedef A21 member_type21; A21 a21; typedef A22 member_type22; A22 a22; typedef A23 member_type23; A23 a23; typedef A24 member_type24; A24 a24; typedef A25 member_type25; A25 a25; typedef A26 member_type26; A26 a26; typedef A27 member_type27; A27 a27; typedef A28 member_type28; A28 a28; typedef A29 member_type29; A29 a29; typedef A30 member_type30; A30 a30; typedef A31 member_type31; A31 a31; typedef A32 member_type32; A32 a32; typedef A33 member_type33; A33 a33; typedef A34 member_type34; A34 a34; typedef A35 member_type35; A35 a35; typedef A36 member_type36; A36 a36; typedef A37 member_type37; A37 a37; typedef A38 member_type38; A38 a38; typedef A39 member_type39; A39 a39; typedef A40 member_type40; A40 a40; typedef A41 member_type41; A41 a41; typedef A42 member_type42; A42 a42; typedef A43 member_type43; A43 a43; typedef A44 member_type44; A44 a44; typedef A45 member_type45; A45 a45; typedef A46 member_type46; A46 a46; typedef A47 member_type47; A47 a47; typedef A48 member_type48; A48 a48; typedef A49 member_type49; A49 a49; typedef A50 member_type50; A50 a50;
        
        typedef mpl::int_<51> size_type;
        static const int size_value = 51;
        typedef
            vector50<A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46 , A47 , A48 , A49 , A50>
            args_type;
        args_type args() const
        {
            args_type r = {a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 , a16 , a17 , a18 , a19 , a20 , a21 , a22 , a23 , a24 , a25 , a26 , a27 , a28 , a29 , a30 , a31 , a32 , a33 , a34 , a35 , a36 , a37 , a38 , a39 , a40 , a41 , a42 , a43 , a44 , a45 , a46 , a47 , a48 , a49 , a50};
            return r;
        }
    };
    template <>
    struct vector_chooser<51>
    {
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9 , typename A10 , typename A11 , typename A12 , typename A13 , typename A14 , typename A15 , typename A16 , typename A17 , typename A18 , typename A19 , typename A20 , typename A21 , typename A22 , typename A23 , typename A24 , typename A25 , typename A26 , typename A27 , typename A28 , typename A29 , typename A30 , typename A31 , typename A32 , typename A33 , typename A34 , typename A35 , typename A36 , typename A37 , typename A38 , typename A39 , typename A40 , typename A41 , typename A42 , typename A43 , typename A44 , typename A45 , typename A46 , typename A47 , typename A48 , typename A49 , typename A50>
        struct apply
        {
            typedef vector51<A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9 , A10 , A11 , A12 , A13 , A14 , A15 , A16 , A17 , A18 , A19 , A20 , A21 , A22 , A23 , A24 , A25 , A26 , A27 , A28 , A29 , A30 , A31 , A32 , A33 , A34 , A35 , A36 , A37 , A38 , A39 , A40 , A41 , A42 , A43 , A44 , A45 , A46 , A47 , A48 , A49 , A50> type;
        };
    };
}}
BOOST_FUSION_ADAPT_TPL_STRUCT_NO_PARTIAL(
    (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46) (A47) (A48) (A49) (A50)
  , ( boost::phoenix::vector51 ) (A0) (A1) (A2) (A3) (A4) (A5) (A6) (A7) (A8) (A9) (A10) (A11) (A12) (A13) (A14) (A15) (A16) (A17) (A18) (A19) (A20) (A21) (A22) (A23) (A24) (A25) (A26) (A27) (A28) (A29) (A30) (A31) (A32) (A33) (A34) (A35) (A36) (A37) (A38) (A39) (A40) (A41) (A42) (A43) (A44) (A45) (A46) (A47) (A48) (A49) (A50)
  , (A0, a0) (A1, a1) (A2, a2) (A3, a3) (A4, a4) (A5, a5) (A6, a6) (A7, a7) (A8, a8) (A9, a9) (A10, a10) (A11, a11) (A12, a12) (A13, a13) (A14, a14) (A15, a15) (A16, a16) (A17, a17) (A18, a18) (A19, a19) (A20, a20) (A21, a21) (A22, a22) (A23, a23) (A24, a24) (A25, a25) (A26, a26) (A27, a27) (A28, a28) (A29, a29) (A30, a30) (A31, a31) (A32, a32) (A33, a33) (A34, a34) (A35, a35) (A36, a36) (A37, a37) (A38, a38) (A39, a39) (A40, a40) (A41, a41) (A42, a42) (A43, a43) (A44, a44) (A45, a45) (A46, a46) (A47, a47) (A48, a48) (A49, a49) (A50, a50)
)
