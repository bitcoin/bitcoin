
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

// Preprocessed version of "boost/mpl/map/map30.hpp" header
// -- DO NOT modify by hand!

namespace boost { namespace mpl {

template<>
struct m_at_impl<20>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item20 type;
    };
};

template<>
struct m_item_impl<21>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item20;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20
    >
struct map21
    : m_item<
          21
        , typename P20::first
        , typename P20::second
        , map20< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19 >
        >
{
    typedef map21 type;
};

template<>
struct m_at_impl<21>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item21 type;
    };
};

template<>
struct m_item_impl<22>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item21;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21
    >
struct map22
    : m_item<
          22
        , typename P21::first
        , typename P21::second
        , map21< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20 >
        >
{
    typedef map22 type;
};

template<>
struct m_at_impl<22>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item22 type;
    };
};

template<>
struct m_item_impl<23>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item22;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22
    >
struct map23
    : m_item<
          23
        , typename P22::first
        , typename P22::second
        , map22< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21 >
        >
{
    typedef map23 type;
};

template<>
struct m_at_impl<23>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item23 type;
    };
};

template<>
struct m_item_impl<24>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item23;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23
    >
struct map24
    : m_item<
          24
        , typename P23::first
        , typename P23::second
        , map23< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22 >
        >
{
    typedef map24 type;
};

template<>
struct m_at_impl<24>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item24 type;
    };
};

template<>
struct m_item_impl<25>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item24;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    >
struct map25
    : m_item<
          25
        , typename P24::first
        , typename P24::second
        , map24< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23 >
        >
{
    typedef map25 type;
};

template<>
struct m_at_impl<25>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item25 type;
    };
};

template<>
struct m_item_impl<26>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item25;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25
    >
struct map26
    : m_item<
          26
        , typename P25::first
        , typename P25::second
        , map25< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24 >
        >
{
    typedef map26 type;
};

template<>
struct m_at_impl<26>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item26 type;
    };
};

template<>
struct m_item_impl<27>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item26;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26
    >
struct map27
    : m_item<
          27
        , typename P26::first
        , typename P26::second
        , map26< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25 >
        >
{
    typedef map27 type;
};

template<>
struct m_at_impl<27>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item27 type;
    };
};

template<>
struct m_item_impl<28>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item27;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27
    >
struct map28
    : m_item<
          28
        , typename P27::first
        , typename P27::second
        , map27< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26 >
        >
{
    typedef map28 type;
};

template<>
struct m_at_impl<28>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item28 type;
    };
};

template<>
struct m_item_impl<29>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item28;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28
    >
struct map29
    : m_item<
          29
        , typename P28::first
        , typename P28::second
        , map28< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27 >
        >
{
    typedef map29 type;
};

template<>
struct m_at_impl<29>
{
    template< typename Map > struct result_
    {
        typedef typename Map::item29 type;
    };
};

template<>
struct m_item_impl<30>
{
    template< typename Key, typename T, typename Base > struct result_
        : m_item_< Key,T,Base >
    {
        typedef pair< Key,T > item29;
    };
};

template<
      typename P0, typename P1, typename P2, typename P3, typename P4
    , typename P5, typename P6, typename P7, typename P8, typename P9
    , typename P10, typename P11, typename P12, typename P13, typename P14
    , typename P15, typename P16, typename P17, typename P18, typename P19
    , typename P20, typename P21, typename P22, typename P23, typename P24
    , typename P25, typename P26, typename P27, typename P28, typename P29
    >
struct map30
    : m_item<
          30
        , typename P29::first
        , typename P29::second
        , map29< P0,P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19,P20,P21,P22,P23,P24,P25,P26,P27,P28 >
        >
{
    typedef map30 type;
};

}}
