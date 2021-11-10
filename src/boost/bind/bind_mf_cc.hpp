//
//  bind/bind_mf_cc.hpp - support for different calling conventions
//
//  Do not include this header directly.
//
//  Copyright (c) 2001 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/bind/bind.html for documentation.
//

// 0

template<class R, class T,
    class A1>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(mf0)<R, T>, typename _bi::list_av_1<A1>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) () BOOST_BIND_MF_NOEXCEPT, A1 a1)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf0)<R, T> F;
    typedef typename _bi::list_av_1<A1>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1));
}

template<class R, class T,
    class A1>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(cmf0)<R, T>, typename _bi::list_av_1<A1>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) () const BOOST_BIND_MF_NOEXCEPT, A1 a1)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf0)<R, T> F;
    typedef typename _bi::list_av_1<A1>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1));
}

template<class Rt2, class R, class T,
    class A1>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(mf0)<R, T>, typename _bi::list_av_1<A1>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) () BOOST_BIND_MF_NOEXCEPT, A1 a1)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf0)<R, T> F;
    typedef typename _bi::list_av_1<A1>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1));
}

template<class Rt2, class R, class T,
    class A1>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(cmf0)<R, T>, typename _bi::list_av_1<A1>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) () const BOOST_BIND_MF_NOEXCEPT, A1 a1)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf0)<R, T> F;
    typedef typename _bi::list_av_1<A1>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1));
}

// 1

template<class R, class T,
    class B1,
    class A1, class A2>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(mf1)<R, T, B1>, typename _bi::list_av_2<A1, A2>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf1)<R, T, B1> F;
    typedef typename _bi::list_av_2<A1, A2>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2));
}

template<class R, class T,
    class B1,
    class A1, class A2>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(cmf1)<R, T, B1>, typename _bi::list_av_2<A1, A2>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf1)<R, T, B1> F;
    typedef typename _bi::list_av_2<A1, A2>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2));
}

template<class Rt2, class R, class T,
    class B1,
    class A1, class A2>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(mf1)<R, T, B1>, typename _bi::list_av_2<A1, A2>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf1)<R, T, B1> F;
    typedef typename _bi::list_av_2<A1, A2>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2));
}

template<class Rt2, class R, class T,
    class B1,
    class A1, class A2>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(cmf1)<R, T, B1>, typename _bi::list_av_2<A1, A2>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf1)<R, T, B1> F;
    typedef typename _bi::list_av_2<A1, A2>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2));
}

// 2

template<class R, class T,
    class B1, class B2,
    class A1, class A2, class A3>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(mf2)<R, T, B1, B2>, typename _bi::list_av_3<A1, A2, A3>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf2)<R, T, B1, B2> F;
    typedef typename _bi::list_av_3<A1, A2, A3>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3));
}

template<class R, class T,
    class B1, class B2,
    class A1, class A2, class A3>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(cmf2)<R, T, B1, B2>, typename _bi::list_av_3<A1, A2, A3>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf2)<R, T, B1, B2> F;
    typedef typename _bi::list_av_3<A1, A2, A3>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3));
}

template<class Rt2, class R, class T,
    class B1, class B2,
    class A1, class A2, class A3>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(mf2)<R, T, B1, B2>, typename _bi::list_av_3<A1, A2, A3>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf2)<R, T, B1, B2> F;
    typedef typename _bi::list_av_3<A1, A2, A3>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3));
}

template<class Rt2, class R, class T,
    class B1, class B2,
    class A1, class A2, class A3>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(cmf2)<R, T, B1, B2>, typename _bi::list_av_3<A1, A2, A3>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf2)<R, T, B1, B2> F;
    typedef typename _bi::list_av_3<A1, A2, A3>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3));
}

// 3

template<class R, class T,
    class B1, class B2, class B3,
    class A1, class A2, class A3, class A4>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(mf3)<R, T, B1, B2, B3>, typename _bi::list_av_4<A1, A2, A3, A4>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf3)<R, T, B1, B2, B3> F;
    typedef typename _bi::list_av_4<A1, A2, A3, A4>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4));
}

template<class R, class T,
    class B1, class B2, class B3,
    class A1, class A2, class A3, class A4>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(cmf3)<R, T, B1, B2, B3>, typename _bi::list_av_4<A1, A2, A3, A4>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf3)<R, T, B1, B2, B3> F;
    typedef typename _bi::list_av_4<A1, A2, A3, A4>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3,
    class A1, class A2, class A3, class A4>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(mf3)<R, T, B1, B2, B3>, typename _bi::list_av_4<A1, A2, A3, A4>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf3)<R, T, B1, B2, B3> F;
    typedef typename _bi::list_av_4<A1, A2, A3, A4>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3,
    class A1, class A2, class A3, class A4>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(cmf3)<R, T, B1, B2, B3>, typename _bi::list_av_4<A1, A2, A3, A4>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf3)<R, T, B1, B2, B3> F;
    typedef typename _bi::list_av_4<A1, A2, A3, A4>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4));
}

// 4

template<class R, class T,
    class B1, class B2, class B3, class B4,
    class A1, class A2, class A3, class A4, class A5>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(mf4)<R, T, B1, B2, B3, B4>, typename _bi::list_av_5<A1, A2, A3, A4, A5>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf4)<R, T, B1, B2, B3, B4> F;
    typedef typename _bi::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5));
}

template<class R, class T,
    class B1, class B2, class B3, class B4,
    class A1, class A2, class A3, class A4, class A5>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(cmf4)<R, T, B1, B2, B3, B4>, typename _bi::list_av_5<A1, A2, A3, A4, A5>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf4)<R, T, B1, B2, B3, B4> F;
    typedef typename _bi::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4,
    class A1, class A2, class A3, class A4, class A5>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(mf4)<R, T, B1, B2, B3, B4>, typename _bi::list_av_5<A1, A2, A3, A4, A5>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf4)<R, T, B1, B2, B3, B4> F;
    typedef typename _bi::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4,
    class A1, class A2, class A3, class A4, class A5>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(cmf4)<R, T, B1, B2, B3, B4>, typename _bi::list_av_5<A1, A2, A3, A4, A5>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf4)<R, T, B1, B2, B3, B4> F;
    typedef typename _bi::list_av_5<A1, A2, A3, A4, A5>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5));
}

// 5

template<class R, class T,
    class B1, class B2, class B3, class B4, class B5,
    class A1, class A2, class A3, class A4, class A5, class A6>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(mf5)<R, T, B1, B2, B3, B4, B5>, typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf5)<R, T, B1, B2, B3, B4, B5> F;
    typedef typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6));
}

template<class R, class T,
    class B1, class B2, class B3, class B4, class B5,
    class A1, class A2, class A3, class A4, class A5, class A6>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(cmf5)<R, T, B1, B2, B3, B4, B5>, typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf5)<R, T, B1, B2, B3, B4, B5> F;
    typedef typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5,
    class A1, class A2, class A3, class A4, class A5, class A6>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(mf5)<R, T, B1, B2, B3, B4, B5>, typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf5)<R, T, B1, B2, B3, B4, B5> F;
    typedef typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5,
    class A1, class A2, class A3, class A4, class A5, class A6>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(cmf5)<R, T, B1, B2, B3, B4, B5>, typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf5)<R, T, B1, B2, B3, B4, B5> F;
    typedef typename _bi::list_av_6<A1, A2, A3, A4, A5, A6>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6));
}

// 6

template<class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(mf6)<R, T, B1, B2, B3, B4, B5, B6>, typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf6)<R, T, B1, B2, B3, B4, B5, B6> F;
    typedef typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7));
}

template<class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(cmf6)<R, T, B1, B2, B3, B4, B5, B6>, typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf6)<R, T, B1, B2, B3, B4, B5, B6> F;
    typedef typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(mf6)<R, T, B1, B2, B3, B4, B5, B6>, typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf6)<R, T, B1, B2, B3, B4, B5, B6> F;
    typedef typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(cmf6)<R, T, B1, B2, B3, B4, B5, B6>, typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf6)<R, T, B1, B2, B3, B4, B5, B6> F;
    typedef typename _bi::list_av_7<A1, A2, A3, A4, A5, A6, A7>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7));
}

// 7

template<class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(mf7)<R, T, B1, B2, B3, B4, B5, B6, B7>, typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6, B7) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf7)<R, T, B1, B2, B3, B4, B5, B6, B7> F;
    typedef typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

template<class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(cmf7)<R, T, B1, B2, B3, B4, B5, B6, B7>, typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6, B7) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf7)<R, T, B1, B2, B3, B4, B5, B6, B7> F;
    typedef typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(mf7)<R, T, B1, B2, B3, B4, B5, B6, B7>, typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6, B7) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf7)<R, T, B1, B2, B3, B4, B5, B6, B7> F;
    typedef typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(cmf7)<R, T, B1, B2, B3, B4, B5, B6, B7>, typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6, B7) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf7)<R, T, B1, B2, B3, B4, B5, B6, B7> F;
    typedef typename _bi::list_av_8<A1, A2, A3, A4, A5, A6, A7, A8>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8));
}

// 8

template<class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(mf8)<R, T, B1, B2, B3, B4, B5, B6, B7, B8>, typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6, B7, B8) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf8)<R, T, B1, B2, B3, B4, B5, B6, B7, B8> F;
    typedef typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}

template<class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    _bi::bind_t<R, _mfi::BOOST_BIND_MF_NAME(cmf8)<R, T, B1, B2, B3, B4, B5, B6, B7, B8>, typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6, B7, B8) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf8)<R, T, B1, B2, B3, B4, B5, B6, B7, B8> F;
    typedef typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return _bi::bind_t<R, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(mf8)<R, T, B1, B2, B3, B4, B5, B6, B7, B8>, typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6, B7, B8) BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef _mfi::BOOST_BIND_MF_NAME(mf8)<R, T, B1, B2, B3, B4, B5, B6, B7, B8> F;
    typedef typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}

template<class Rt2, class R, class T,
    class B1, class B2, class B3, class B4, class B5, class B6, class B7, class B8,
    class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
    typename boost::enable_if_c<!boost::core::is_same<Rt2, R>::value,
    _bi::bind_t<Rt2, _mfi::BOOST_BIND_MF_NAME(cmf8)<R, T, B1, B2, B3, B4, B5, B6, B7, B8>, typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type>
    >::type BOOST_BIND(R (BOOST_BIND_MF_CC T::*f) (B1, B2, B3, B4, B5, B6, B7, B8) const BOOST_BIND_MF_NOEXCEPT, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
    typedef _mfi::BOOST_BIND_MF_NAME(cmf8)<R, T, B1, B2, B3, B4, B5, B6, B7, B8> F;
    typedef typename _bi::list_av_9<A1, A2, A3, A4, A5, A6, A7, A8, A9>::type list_type;
    return _bi::bind_t<Rt2, F, list_type>(F(f), list_type(a1, a2, a3, a4, a5, a6, a7, a8, a9));
}
