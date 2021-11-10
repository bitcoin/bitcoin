
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

// Preprocessed version of "boost/mpl/map/map50.hpp" header
// -- DO NOT modify by hand!

namespace boost { namespace mpl {

template<>
struct m_at_impl<40>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item40 type;
    };
};

template<>
struct m_item_impl<41>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item40;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40
    >
struct map41
    : m_item<
          41
        , typename P40::first
        , typename P40::second
        , map40< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39 >
        >
{
    typedef map41 type;
};

template<>
struct m_at_impl<41>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item41 type;
    };
};

template<>
struct m_item_impl<42>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item41;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40, typename P41
    >
struct map42
    : m_item<
          42
        , typename P41::first
        , typename P41::second
        , map41< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39,P40 >
        >
{
    typedef map42 type;
};

template<>
struct m_at_impl<42>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item42 type;
    };
};

template<>
struct m_item_impl<43>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item42;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40, typename P41, typename P42
    >
struct map43
    : m_item<
          43
        , typename P42::first
        , typename P42::second
        , map42< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39,P40,P41 >
        >
{
    typedef map43 type;
};

template<>
struct m_at_impl<43>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item43 type;
    };
};

template<>
struct m_item_impl<44>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item43;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40, typename P41, typename P42, typename P43
    >
struct map44
    : m_item<
          44
        , typename P43::first
        , typename P43::second
        , map43< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39,P40,P41,P42 >
        >
{
    typedef map44 type;
};

template<>
struct m_at_impl<44>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item44 type;
    };
};

template<>
struct m_item_impl<45>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item44;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40, typename P41, typename P42, typename P43, typename P44
    >
struct map45
    : m_item<
          45
        , typename P44::first
        , typename P44::second
        , map44< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39,P40,P41,P42,P43 >
        >
{
    typedef map45 type;
};

template<>
struct m_at_impl<45>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item45 type;
    };
};

template<>
struct m_item_impl<46>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item45;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40, typename P41, typename P42, typename P43, typename P44
    , typename P45
    >
struct map46
    : m_item<
          46
        , typename P45::first
        , typename P45::second
        , map45< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39,P40,P41,P42,P43,P44 >
        >
{
    typedef map46 type;
};

template<>
struct m_at_impl<46>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item46 type;
    };
};

template<>
struct m_item_impl<47>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item46;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40, typename P41, typename P42, typename P43, typename P44
    , typename P45, typename P46
    >
struct map47
    : m_item<
          47
        , typename P46::first
        , typename P46::second
        , map46< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39,P40,P41,P42,P43,P44,P45 >
        >
{
    typedef map47 type;
};

template<>
struct m_at_impl<47>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item47 type;
    };
};

template<>
struct m_item_impl<48>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item47;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40, typename P41, typename P42, typename P43, typename P44
    , typename P45, typename P46, typename P47
    >
struct map48
    : m_item<
          48
        , typename P47::first
        , typename P47::second
        , map47< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39,P40,P41,P42,P43,P44,P45,P46 >
        >
{
    typedef map48 type;
};

template<>
struct m_at_impl<48>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item48 type;
    };
};

template<>
struct m_item_impl<49>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item48;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40, typename P41, typename P42, typename P43, typename P44
    , typename P45, typename P46, typename P47, typename P48
    >
struct map49
    : m_item<
          49
        , typename P48::first
        , typename P48::second
        , map48< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39,P40,P41,P42,P43,P44,P45,P46,P47 >
        >
{
    typedef map49 type;
};

template<>
struct m_at_impl<49>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item49 type;
    };
};

template<>
struct m_item_impl<50>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item49;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    , typename P30, typename P31, typename P32, typename P33, typename P34
    , typename P35, typename P36, typename P37, typename P38, typename P39
    , typename P40, typename P41, typename P42, typename P43, typename P44
    , typename P45, typename P46, typename P47, typename P48, typename P49
    >
struct map50
    : m_item<
          50
        , typename P49::first
        , typename P49::second
        , map49< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28,P29,P30,P31,P32,P33,P34,P35,P36,P37,P38,P39,P40,P41,P42,P43,P44,P45,P46,P47,P48 >
        >
{
    typedef map50 type;
};

}}
