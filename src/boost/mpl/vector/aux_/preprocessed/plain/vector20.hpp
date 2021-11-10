
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

// Preprocessed version of "boost/mpl/vector/vector20.hpp" header
// -- DO NOT modify by hand!

namespace boost { namespace mpl {

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10
    >
struct vector11
{
    typedef aux::vector_tag<11> tag;
    typedef vector11 type;
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
    

    typedef void_ item11;
    typedef T10 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,11 > end;
};

template<>
struct push_front_impl< aux::vector_tag<10> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector11<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<11> >
{
    template< typename Vector > struct apply
    {
        typedef vector10<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<10> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector11<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<11> >
{
    template< typename Vector > struct apply
    {
        typedef vector10<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            > type;
    };
};

template< typename V >
struct v_at< V,11 >
{
    typedef typename V::item11 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11
    >
struct vector12
{
    typedef aux::vector_tag<12> tag;
    typedef vector12 type;
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
    

    typedef void_ item12;
    typedef T11 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,12 > end;
};

template<>
struct push_front_impl< aux::vector_tag<11> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector12<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<12> >
{
    template< typename Vector > struct apply
    {
        typedef vector11<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<11> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector12<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<12> >
{
    template< typename Vector > struct apply
    {
        typedef vector11<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10
            > type;
    };
};

template< typename V >
struct v_at< V,12 >
{
    typedef typename V::item12 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12
    >
struct vector13
{
    typedef aux::vector_tag<13> tag;
    typedef vector13 type;
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
    

    typedef void_ item13;
    typedef T12 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,13 > end;
};

template<>
struct push_front_impl< aux::vector_tag<12> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector13<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<13> >
{
    template< typename Vector > struct apply
    {
        typedef vector12<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<12> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector13<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<13> >
{
    template< typename Vector > struct apply
    {
        typedef vector12<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            > type;
    };
};

template< typename V >
struct v_at< V,13 >
{
    typedef typename V::item13 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13
    >
struct vector14
{
    typedef aux::vector_tag<14> tag;
    typedef vector14 type;
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
    

    typedef void_ item14;
    typedef T13 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,14 > end;
};

template<>
struct push_front_impl< aux::vector_tag<13> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector14<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<14> >
{
    template< typename Vector > struct apply
    {
        typedef vector13<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<13> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector14<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<14> >
{
    template< typename Vector > struct apply
    {
        typedef vector13<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12
            > type;
    };
};

template< typename V >
struct v_at< V,14 >
{
    typedef typename V::item14 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    >
struct vector15
{
    typedef aux::vector_tag<15> tag;
    typedef vector15 type;
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
    

    typedef void_ item15;
    typedef T14 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,15 > end;
};

template<>
struct push_front_impl< aux::vector_tag<14> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector15<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<15> >
{
    template< typename Vector > struct apply
    {
        typedef vector14<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<14> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector15<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<15> >
{
    template< typename Vector > struct apply
    {
        typedef vector14<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            > type;
    };
};

template< typename V >
struct v_at< V,15 >
{
    typedef typename V::item15 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15
    >
struct vector16
{
    typedef aux::vector_tag<16> tag;
    typedef vector16 type;
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
    

    typedef void_ item16;
    typedef T15 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,16 > end;
};

template<>
struct push_front_impl< aux::vector_tag<15> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector16<
              T
              ,
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<16> >
{
    template< typename Vector > struct apply
    {
        typedef vector15<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<15> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector16<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<16> >
{
    template< typename Vector > struct apply
    {
        typedef vector15<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14
            > type;
    };
};

template< typename V >
struct v_at< V,16 >
{
    typedef typename V::item16 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16
    >
struct vector17
{
    typedef aux::vector_tag<17> tag;
    typedef vector17 type;
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
    

    typedef void_ item17;
    typedef T16 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,17 > end;
};

template<>
struct push_front_impl< aux::vector_tag<16> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector17<
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
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<17> >
{
    template< typename Vector > struct apply
    {
        typedef vector16<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<16> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector17<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<17> >
{
    template< typename Vector > struct apply
    {
        typedef vector16<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            > type;
    };
};

template< typename V >
struct v_at< V,17 >
{
    typedef typename V::item17 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17
    >
struct vector18
{
    typedef aux::vector_tag<18> tag;
    typedef vector18 type;
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
    

    typedef void_ item18;
    typedef T17 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,18 > end;
};

template<>
struct push_front_impl< aux::vector_tag<17> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector18<
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
            , typename Vector::item16
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<18> >
{
    template< typename Vector > struct apply
    {
        typedef vector17<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<17> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector18<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<18> >
{
    template< typename Vector > struct apply
    {
        typedef vector17<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16
            > type;
    };
};

template< typename V >
struct v_at< V,18 >
{
    typedef typename V::item18 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18
    >
struct vector19
{
    typedef aux::vector_tag<19> tag;
    typedef vector19 type;
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
    

    typedef void_ item19;
    typedef T18 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,19 > end;
};

template<>
struct push_front_impl< aux::vector_tag<18> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector19<
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
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<19> >
{
    template< typename Vector > struct apply
    {
        typedef vector18<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<18> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector19<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<19> >
{
    template< typename Vector > struct apply
    {
        typedef vector18<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            > type;
    };
};

template< typename V >
struct v_at< V,19 >
{
    typedef typename V::item19 type;
};

template<
      typename T0, typename T1, typename T2, typename T3, typename T4
    , typename T5, typename T6, typename T7, typename T8, typename T9
    , typename T10, typename T11, typename T12, typename T13, typename T14
    , typename T15, typename T16, typename T17, typename T18, typename T19
    >
struct vector20
{
    typedef aux::vector_tag<20> tag;
    typedef vector20 type;
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
    

    typedef void_ item20;
    typedef T19 back;
    typedef v_iter< type,0 > begin;
    typedef v_iter< type,20 > end;
};

template<>
struct push_front_impl< aux::vector_tag<19> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector20<
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
            , typename Vector::item18
            > type;
    };
};

template<>
struct pop_front_impl< aux::vector_tag<20> >
{
    template< typename Vector > struct apply
    {
        typedef vector19<
              typename Vector::item1, typename Vector::item2
            , typename Vector::item3, typename Vector::item4
            , typename Vector::item5, typename Vector::item6
            , typename Vector::item7, typename Vector::item8
            , typename Vector::item9, typename Vector::item10
            , typename Vector::item11, typename Vector::item12
            , typename Vector::item13, typename Vector::item14
            , typename Vector::item15, typename Vector::item16
            , typename Vector::item17, typename Vector::item18
            , typename Vector::item19
            > type;
    };
};

template<>
struct push_back_impl< aux::vector_tag<19> >
{
    template< typename Vector, typename T > struct apply
    {
        typedef vector20<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18
              ,
              T
            > type;
    };
};

template<>
struct pop_back_impl< aux::vector_tag<20> >
{
    template< typename Vector > struct apply
    {
        typedef vector19<
              typename Vector::item0, typename Vector::item1
            , typename Vector::item2, typename Vector::item3
            , typename Vector::item4, typename Vector::item5
            , typename Vector::item6, typename Vector::item7
            , typename Vector::item8, typename Vector::item9
            , typename Vector::item10, typename Vector::item11
            , typename Vector::item12, typename Vector::item13
            , typename Vector::item14, typename Vector::item15
            , typename Vector::item16, typename Vector::item17
            , typename Vector::item18
            > type;
    };
};

template< typename V >
struct v_at< V,20 >
{
    typedef typename V::item20 type;
};

}}
