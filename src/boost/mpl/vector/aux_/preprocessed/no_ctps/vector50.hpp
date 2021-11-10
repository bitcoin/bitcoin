
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

// Preprocessed version of "boost/mpl/vector/vector50.hpp" header
// -- DO NOT modify by hand!

namespace boost { namespace mpl {

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40
    >
struct vector41
{
    typedef aux::vector_tag<41> tag;
    typedef vector41 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    

    typedef void_ item41;
    typedef T40 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,41 > end;
};

template<>
struct push_front_impl< aux::vector_tag<40> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector41<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<41> >
{
    template< typename Vector > struct apply
    {
        typedef vector40<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<40> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector41<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<41> >
{
    template< typename Vector > struct apply
    {
        typedef vector40<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<41>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item41 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<41> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<41> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<41> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<41> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<41> >
{
    template< typename Vector > struct apply
        : long_<41>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<41> >
    : size_impl< aux::vector_tag<41> >
{
};

template<>
struct clear_impl< aux::vector_tag<41> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40, typename T41
    >
struct vector42
{
    typedef aux::vector_tag<42> tag;
    typedef vector42 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    typedef T41 item41;
    

    typedef void_ item42;
    typedef T41 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,42 > end;
};

template<>
struct push_front_impl< aux::vector_tag<41> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector42<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<42> >
{
    template< typename Vector > struct apply
    {
        typedef vector41<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            , typename Vector::item41
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<41> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector42<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<42> >
{
    template< typename Vector > struct apply
    {
        typedef vector41<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<42>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item42 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<42> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<42> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<42> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<42> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<42> >
{
    template< typename Vector > struct apply
        : long_<42>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<42> >
    : size_impl< aux::vector_tag<42> >
{
};

template<>
struct clear_impl< aux::vector_tag<42> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40, typename T41, typename T42
    >
struct vector43
{
    typedef aux::vector_tag<43> tag;
    typedef vector43 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    typedef T41 item41;
    typedef T42 item42;
    

    typedef void_ item43;
    typedef T42 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,43 > end;
};

template<>
struct push_front_impl< aux::vector_tag<42> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector43<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<43> >
{
    template< typename Vector > struct apply
    {
        typedef vector42<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            , typename Vector::item41, typename Vector::item42
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<42> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector43<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<43> >
{
    template< typename Vector > struct apply
    {
        typedef vector42<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<43>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item43 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<43> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<43> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<43> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<43> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<43> >
{
    template< typename Vector > struct apply
        : long_<43>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<43> >
    : size_impl< aux::vector_tag<43> >
{
};

template<>
struct clear_impl< aux::vector_tag<43> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40, typename T41, typename T42, typename T43
    >
struct vector44
{
    typedef aux::vector_tag<44> tag;
    typedef vector44 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    typedef T41 item41;
    typedef T42 item42;
    typedef T43 item43;
    

    typedef void_ item44;
    typedef T43 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,44 > end;
};

template<>
struct push_front_impl< aux::vector_tag<43> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector44<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<44> >
{
    template< typename Vector > struct apply
    {
        typedef vector43<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            , typename Vector::item41, typename Vector::item42
            , typename Vector::item43
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<43> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector44<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<44> >
{
    template< typename Vector > struct apply
    {
        typedef vector43<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<44>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item44 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<44> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<44> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<44> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<44> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<44> >
{
    template< typename Vector > struct apply
        : long_<44>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<44> >
    : size_impl< aux::vector_tag<44> >
{
};

template<>
struct clear_impl< aux::vector_tag<44> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40, typename T41, typename T42, typename T43, typename T44
    >
struct vector45
{
    typedef aux::vector_tag<45> tag;
    typedef vector45 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    typedef T41 item41;
    typedef T42 item42;
    typedef T43 item43;
    typedef T44 item44;
    

    typedef void_ item45;
    typedef T44 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,45 > end;
};

template<>
struct push_front_impl< aux::vector_tag<44> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector45<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<45> >
{
    template< typename Vector > struct apply
    {
        typedef vector44<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            , typename Vector::item41, typename Vector::item42
            , typename Vector::item43, typename Vector::item44
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<44> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector45<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<45> >
{
    template< typename Vector > struct apply
    {
        typedef vector44<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<45>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item45 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<45> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<45> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<45> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<45> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<45> >
{
    template< typename Vector > struct apply
        : long_<45>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<45> >
    : size_impl< aux::vector_tag<45> >
{
};

template<>
struct clear_impl< aux::vector_tag<45> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40, typename T41, typename T42, typename T43, typename T44
    , typename T45
    >
struct vector46
{
    typedef aux::vector_tag<46> tag;
    typedef vector46 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    typedef T41 item41;
    typedef T42 item42;
    typedef T43 item43;
    typedef T44 item44;
    typedef T45 item45;
    

    typedef void_ item46;
    typedef T45 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,46 > end;
};

template<>
struct push_front_impl< aux::vector_tag<45> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector46<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<46> >
{
    template< typename Vector > struct apply
    {
        typedef vector45<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            , typename Vector::item41, typename Vector::item42
            , typename Vector::item43, typename Vector::item44
            , typename Vector::item45
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<45> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector46<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<46> >
{
    template< typename Vector > struct apply
    {
        typedef vector45<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<46>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item46 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<46> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<46> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<46> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<46> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<46> >
{
    template< typename Vector > struct apply
        : long_<46>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<46> >
    : size_impl< aux::vector_tag<46> >
{
};

template<>
struct clear_impl< aux::vector_tag<46> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40, typename T41, typename T42, typename T43, typename T44
    , typename T45, typename T46
    >
struct vector47
{
    typedef aux::vector_tag<47> tag;
    typedef vector47 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    typedef T41 item41;
    typedef T42 item42;
    typedef T43 item43;
    typedef T44 item44;
    typedef T45 item45;
    typedef T46 item46;
    

    typedef void_ item47;
    typedef T46 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,47 > end;
};

template<>
struct push_front_impl< aux::vector_tag<46> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector47<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<47> >
{
    template< typename Vector > struct apply
    {
        typedef vector46<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            , typename Vector::item41, typename Vector::item42
            , typename Vector::item43, typename Vector::item44
            , typename Vector::item45, typename Vector::item46
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<46> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector47<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<47> >
{
    template< typename Vector > struct apply
    {
        typedef vector46<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<47>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item47 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<47> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<47> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<47> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<47> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<47> >
{
    template< typename Vector > struct apply
        : long_<47>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<47> >
    : size_impl< aux::vector_tag<47> >
{
};

template<>
struct clear_impl< aux::vector_tag<47> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40, typename T41, typename T42, typename T43, typename T44
    , typename T45, typename T46, typename T47
    >
struct vector48
{
    typedef aux::vector_tag<48> tag;
    typedef vector48 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    typedef T41 item41;
    typedef T42 item42;
    typedef T43 item43;
    typedef T44 item44;
    typedef T45 item45;
    typedef T46 item46;
    typedef T47 item47;
    

    typedef void_ item48;
    typedef T47 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,48 > end;
};

template<>
struct push_front_impl< aux::vector_tag<47> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector48<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            , typename Vector::item46
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<48> >
{
    template< typename Vector > struct apply
    {
        typedef vector47<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            , typename Vector::item41, typename Vector::item42
            , typename Vector::item43, typename Vector::item44
            , typename Vector::item45, typename Vector::item46
            , typename Vector::item47
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<47> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector48<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            , typename Vector::item46
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<48> >
{
    template< typename Vector > struct apply
    {
        typedef vector47<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            , typename Vector::item46
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<48>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item48 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<48> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<48> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<48> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<48> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<48> >
{
    template< typename Vector > struct apply
        : long_<48>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<48> >
    : size_impl< aux::vector_tag<48> >
{
};

template<>
struct clear_impl< aux::vector_tag<48> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40, typename T41, typename T42, typename T43, typename T44
    , typename T45, typename T46, typename T47, typename T48
    >
struct vector49
{
    typedef aux::vector_tag<49> tag;
    typedef vector49 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    typedef T41 item41;
    typedef T42 item42;
    typedef T43 item43;
    typedef T44 item44;
    typedef T45 item45;
    typedef T46 item46;
    typedef T47 item47;
    typedef T48 item48;
    

    typedef void_ item49;
    typedef T48 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,49 > end;
};

template<>
struct push_front_impl< aux::vector_tag<48> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector49<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            , typename Vector::item46, typename Vector::item47
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<49> >
{
    template< typename Vector > struct apply
    {
        typedef vector48<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            , typename Vector::item41, typename Vector::item42
            , typename Vector::item43, typename Vector::item44
            , typename Vector::item45, typename Vector::item46
            , typename Vector::item47, typename Vector::item48
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<48> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector49<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            , typename Vector::item46, typename Vector::item47
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<49> >
{
    template< typename Vector > struct apply
    {
        typedef vector48<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            , typename Vector::item46, typename Vector::item47
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<49>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item49 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<49> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<49> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<49> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<49> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<49> >
{
    template< typename Vector > struct apply
        : long_<49>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<49> >
    : size_impl< aux::vector_tag<49> >
{
};

template<>
struct clear_impl< aux::vector_tag<49> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    , typename T20, typename T21, typename T22, typename T23, typename T24
    , typename T25, typename T26, typename T27, typename T28, typename T29
    , typename T30, typename T31, typename T32, typename T33, typename T34
    , typename T35, typename T36, typename T37, typename T38, typename T39
    , typename T40, typename T41, typename T42, typename T43, typename T44
    , typename T45, typename T46, typename T47, typename T48, typename T49
    >
struct vector50
{
    typedef aux::vector_tag<50> tag;
    typedef vector50 type;
    typedef T0 item0;
    typedef T1 item1;
    typedef T2 item2;
    typedef T3 item3;
    typedef T4 item4;
    typedef T5 item5;
    typedef T6 item6;
    typedef T7 item7;
    typedef T8 item8;
    typedef T9 item9;
    typedef T10 item10;
    typedef T11 item11;
    typedef T12 item12;
    typedef T13 item13;
    typedef T14 item14;
    typedef T15 item15;
    typedef T16 item16;
    typedef T17 item17;
    typedef T18 item18;
    typedef T19 item19;
    typedef T20 item20;
    typedef T21 item21;
    typedef T22 item22;
    typedef T23 item23;
    typedef T24 item24;
    typedef T25 item25;
    typedef T26 item26;
    typedef T27 item27;
    typedef T28 item28;
    typedef T29 item29;
    typedef T30 item30;
    typedef T31 item31;
    typedef T32 item32;
    typedef T33 item33;
    typedef T34 item34;
    typedef T35 item35;
    typedef T36 item36;
    typedef T37 item37;
    typedef T38 item38;
    typedef T39 item39;
    typedef T40 item40;
    typedef T41 item41;
    typedef T42 item42;
    typedef T43 item43;
    typedef T44 item44;
    typedef T45 item45;
    typedef T46 item46;
    typedef T47 item47;
    typedef T48 item48;
    typedef T49 item49;
    

    typedef void_ item50;
    typedef T49 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,50 > end;
};

template<>
struct push_front_impl< aux::vector_tag<49> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector50<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            , typename Vector::item46, typename Vector::item47
            , typename Vector::item48
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<50> >
{
    template< typename Vector > struct apply
    {
        typedef vector49<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19, typename Vector::item20
            , typename Vector::item21, typename Vector::item22
            , typename Vector::item23, typename Vector::item24
            , typename Vector::item25, typename Vector::item26
            , typename Vector::item27, typename Vector::item28
            , typename Vector::item29, typename Vector::item30
            , typename Vector::item31, typename Vector::item32
            , typename Vector::item33, typename Vector::item34
            , typename Vector::item35, typename Vector::item36
            , typename Vector::item37, typename Vector::item38
            , typename Vector::item39, typename Vector::item40
            , typename Vector::item41, typename Vector::item42
            , typename Vector::item43, typename Vector::item44
            , typename Vector::item45, typename Vector::item46
            , typename Vector::item47, typename Vector::item48
            , typename Vector::item49
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<49> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector50<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            , typename Vector::item46, typename Vector::item47
            , typename Vector::item48
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<50> >
{
    template< typename Vector > struct apply
    {
        typedef vector49<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18, typename Vector::item19
            , typename Vector::item20, typename Vector::item21
            , typename Vector::item22, typename Vector::item23
            , typename Vector::item24, typename Vector::item25
            , typename Vector::item26, typename Vector::item27
            , typename Vector::item28, typename Vector::item29
            , typename Vector::item30, typename Vector::item31
            , typename Vector::item32, typename Vector::item33
            , typename Vector::item34, typename Vector::item35
            , typename Vector::item36, typename Vector::item37
            , typename Vector::item38, typename Vector::item39
            , typename Vector::item40, typename Vector::item41
            , typename Vector::item42, typename Vector::item43
            , typename Vector::item44, typename Vector::item45
            , typename Vector::item46, typename Vector::item47
            , typename Vector::item48
            > type;
    };
};

namespace aux {
template<> struct v_at_impl<50>
{
    template< typename V_ > struct result_
    {
        typedef typename V_::item50 type;
    };
};

}

template<>
struct at_impl< aux::vector_tag<50> >
{
    template< typename V_, typename N > struct apply
    {
        typedef typename aux::v_at_impl<BOOST_MPL_AUX_VALUE_WKND(N)::value>
            ::template result_<V_>::type type;
    };
};

template<>
struct front_impl< aux::vector_tag<50> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::item0 type;
    };
};

template<>
struct back_impl< aux::vector_tag<50> >
{
    template< typename Vector > struct apply
    {
        typedef typename Vector::back type;
    };
};

template<>
struct empty_impl< aux::vector_tag<50> >
{
    template< typename Vector > struct apply
        : false_
    {
    };
};

template<>
struct size_impl< aux::vector_tag<50> >
{
    template< typename Vector > struct apply
        : long_<50>
    {
    };
};

template<>
struct O1_size_impl< aux::vector_tag<50> >
    : size_impl< aux::vector_tag<50> >
{
};

template<>
struct clear_impl< aux::vector_tag<50> >
{
    template< typename Vector > struct apply
    {
        typedef vector0<> type;
    };
};

}}
