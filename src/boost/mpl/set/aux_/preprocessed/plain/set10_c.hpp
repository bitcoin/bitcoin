
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

// Preprocessed version of "boost/mpl/set/set10_c.hpp" header
// -- DO NOT modify by hand!

namespace boost { namespace mpl {

template<
      typename T
    , T C0
    >
struct set1_c
    : s_item<
          integral_c< T,C0 >
        , set0_c<T>
        >
{
    typedef set1_c type;
};

template<
      typename T
    , T C0, T C1
    >
struct set2_c
    : s_item<
          integral_c< T,C1 >
        , set1_c< T,C0 >
        >
{
    typedef set2_c type;
};

template<
      typename T
    , T C0, T C1, T C2
    >
struct set3_c
    : s_item<
          integral_c< T,C2 >
        , set2_c< T,C0,C1 >
        >
{
    typedef set3_c type;
};

template<
      typename T
    , T C0, T C1, T C2, T C3
    >
struct set4_c
    : s_item<
          integral_c< T,C3 >
        , set3_c< T,C0,C1,C2 >
        >
{
    typedef set4_c type;
};

template<
      typename T
    , T C0, T C1, T C2, T C3, T C4
    >
struct set5_c
    : s_item<
          integral_c< T,C4 >
        , set4_c< T,C0,C1,C2,C3 >
        >
{
    typedef set5_c type;
};

template<
      typename T
    , T C0, T C1, T C2, T C3, T C4, T C5
    >
struct set6_c
    : s_item<
          integral_c< T,C5 >
        , set5_c< T,C0,C1,C2,C3,C4 >
        >
{
    typedef set6_c type;
};

template<
      typename T
    , T C0, T C1, T C2, T C3, T C4, T C5, T C6
    >
struct set7_c
    : s_item<
          integral_c< T,C6 >
        , set6_c< T,C0,C1,C2,C3,C4,C5 >
        >
{
    typedef set7_c type;
};

template<
      typename T
    , T C0, T C1, T C2, T C3, T C4, T C5, T C6, T C7
    >
struct set8_c
    : s_item<
          integral_c< T,C7 >
        , set7_c< T,C0,C1,C2,C3,C4,C5,C6 >
        >
{
    typedef set8_c type;
};

template<
      typename T
    , T C0, T C1, T C2, T C3, T C4, T C5, T C6, T C7, T C8
    >
struct set9_c
    : s_item<
          integral_c< T,C8 >
        , set8_c< T,C0,C1,C2,C3,C4,C5,C6,C7 >
        >
{
    typedef set9_c type;
};

template<
      typename T
    , T C0, T C1, T C2, T C3, T C4, T C5, T C6, T C7, T C8, T C9
    >
struct set10_c
    : s_item<
          integral_c< T,C9 >
        , set9_c< T,C0,C1,C2,C3,C4,C5,C6,C7,C8 >
        >
{
    typedef set10_c type;
};

}}
