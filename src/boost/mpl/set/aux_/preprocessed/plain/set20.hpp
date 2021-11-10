
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

// Preprocessed version of "boost/mpl/set/set20.hpp" header
// -- DO NOT modify by hand!

namespace boost { namespace mpl {

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10
    >
struct set11
    : s_item<
          T10
        , typename set10< T0,T1,T2,T3,T4,T5,T6,T7,T8,T9 >::item_
        >
{
    typedef set11 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11
    >
struct set12
    : s_item<
          T11
        , typename set11< T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10 >::item_
        >
{
    typedef set12 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12
    >
struct set13
    : s_item<
          T12
        , typename set12< T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10
        , T11 >::item_
        >
{
    typedef set13 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13
    >
struct set14
    : s_item<
          T13
        , typename set13< T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11
        , T12 >::item_
        >
{
    typedef set14 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    >
struct set15
    : s_item<
          T14
        , typename set14< T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11
        , T12, T13 >::item_
        >
{
    typedef set15 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15
    >
struct set16
    : s_item<
          T15
        , typename set15< T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11
        , T12, T13, T14 >::item_
        >
{
    typedef set16 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16
    >
struct set17
    : s_item<
          T16
        , typename set16< T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11
        , T12, T13, T14, T15 >::item_
        >
{
    typedef set17 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17
    >
struct set18
    : s_item<
          T17
        , typename set17< T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11
        , T12, T13, T14, T15, T16 >::item_
        >
{
    typedef set18 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18
    >
struct set19
    : s_item<
          T18
        , typename set18< T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11
        , T12, T13, T14, T15, T16, T17 >::item_
        >
{
    typedef set19 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    >
struct set20
    : s_item<
          T19
        , typename set19< T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11
        , T12, T13, T14, T15, T16, T17, T18 >::item_
        >
{
    typedef set20 type;
};

}}
