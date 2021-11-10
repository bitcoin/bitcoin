
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

// Preprocessed version of "boost/mpl/set/set10.hpp" header
// -- DO NOT modify by hand!

namespace boost { namespace mpl {

template<
      typename T0
    >
struct set1
    : s_item<
          T0
        , typename set0<  >::item_
        >
{
    typedef set1 type;
};

template<
      typename T0, typename T1
    >
struct set2
    : s_item<
          T1
        , typename set1<T0>::item_
        >
{
    typedef set2 type;
};

template<
      typename T0, typename T1, typename T2
    >
struct set3
    : s_item<
          T2
        , typename set2< T0,T1 >::item_
        >
{
    typedef set3 type;
};

template<
      typename T0, typename T1, typename T2, typename T3
    >
struct set4
    : s_item<
          T3
        , typename set3< T0,T1,T2 >::item_
        >
{
    typedef set4 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    >
struct set5
    : s_item<
          T4
        , typename set4< T0,T1,T2,T3 >::item_
        >
{
    typedef set5 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5
    >
struct set6
    : s_item<
          T5
        , typename set5< T0,T1,T2,T3,T4 >::item_
        >
{
    typedef set6 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6
    >
struct set7
    : s_item<
          T6
        , typename set6< T0,T1,T2,T3,T4,T5 >::item_
        >
{
    typedef set7 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7
    >
struct set8
    : s_item<
          T7
        , typename set7< T0,T1,T2,T3,T4,T5,T6 >::item_
        >
{
    typedef set8 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8
    >
struct set9
    : s_item<
          T8
        , typename set8< T0,T1,T2,T3,T4,T5,T6,T7 >::item_
        >
{
    typedef set9 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    >
struct set10
    : s_item<
          T9
        , typename set9< T0,T1,T2,T3,T4,T5,T6,T7,T8 >::item_
        >
{
    typedef set10 type;
};

}}
