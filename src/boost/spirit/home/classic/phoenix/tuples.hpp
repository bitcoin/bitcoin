/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2002 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_TUPLES_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_TUPLES_HPP

///////////////////////////////////////////////////////////////////////////////
//
//  Phoenix predefined maximum limit. This limit defines the maximum
//  number of elements a tuple can hold. This number defaults to 3. The
//  actual maximum is rounded up in multiples of 3. Thus, if this value
//  is 4, the actual limit is 6. The ultimate maximum limit in this
//  implementation is 15.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef PHOENIX_LIMIT
#define PHOENIX_LIMIT 3
#endif

///////////////////////////////////////////////////////////////////////////////
#include <boost/static_assert.hpp>
#include <boost/call_traits.hpp>
#include <boost/type_traits/remove_reference.hpp>

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
// bogus https://developercommunity.visualstudio.com/t/buggy-warning-c4709/471956
#pragma warning(disable:4709) //comma operator within array index expression
#endif

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  tuple
//
//      Tuples hold heterogeneous types up to a predefined maximum. Only
//      the most basic functionality needed is provided. Unlike other
//      recursive list-like tuple implementations, this tuple
//      implementation uses simple structs similar to std::pair with
//      specialization for 0 to N tuple elements.
//
//          1)  Construction
//              Here are examples on how to construct tuples:
//
//                  typedef tuple<int, char> t1_t;
//                  typedef tuple<int, std::string, double> t2_t;
//
//                  // this tuple has an int and char members
//                  t1_t t1(3, 'c');
//
//                  // this tuple has an int, std::string and double members
//                  t2_t t2(3, "hello", 3.14);
//
//              Tuples can also be constructed from other tuples. The
//              source and destination tuples need not have exactly the
//              same element types. The only requirement is that the
//              source tuple have the same number of elements as the
//              destination and that each element slot in the
//              destination can be copy constructed from the source
//              element. For example:
//
//                  tuple<double, double> t3(t1); // OK. Compatible tuples
//                  tuple<double, double> t4(t2); // Error! Incompatible tuples
//
//          2)  Member access
//                  A member in a tuple can be accessed using the
//                  tuple's [] operator by specifying the Nth
//                  tuple_index. Here are some examples:
//
//                      tuple_index<0> ix0; // 0th index == 1st item
//                      tuple_index<1> ix1; // 1st index == 2nd item
//                      tuple_index<2> ix2; // 2nd index == 3rd item
//
//                      t1[ix0] = 33;  // sets the int member of the tuple t1
//                      t2[ix2] = 6e6; // sets the double member of the tuple t2
//                      t1[ix1] = 'a'; // sets the char member of the tuple t1
//
//                  There are some predefined names are provided in sub-
//                  namespace tuple_index_names:
//
//                      tuple_index<0> _1;
//                      tuple_index<1> _2;
//                      ...
//                      tuple_index<N> _N;
//
//                  These indexes may be used by 'using' namespace
//                  phoenix::tuple_index_names.
//
//                  Access to out of bound indexes returns a nil_t value.
//
//          3)  Member type inquiry
//                  The type of an individual member can be queried.
//                  Example:
//
//                      tuple_element<1, t2_t>::type
//
//                  Refers to the type of the second member (note zero based,
//                  thus 0 = 1st item, 1 = 2nd item) of the tuple.
//
//                  Aside from tuple_element<N, T>::type, there are two
//                  more types that tuple_element provides: rtype and
//                  crtype. While 'type' is the plain underlying type,
//                  'rtype' is the reference type, or type& and 'crtype'
//                  is the constant reference type or type const&. The
//                  latter two are provided to make it easy for the
//                  client in dealing with the possibility of reference
//                  to reference when type is already a reference, which
//                  is illegal in C++.
//
//                  Access to out of bound indexes returns a nil_t type.
//
//          4)  Tuple length
//                  The number of elements in a tuple can be queried.
//                  Example:
//
//                      int n = t1.length;
//
//                  gets the number of elements in tuple t1.
//
//                  length is a static constant. Thus, TupleT::length
//                  also works. Example:
//
//                      int n = t1_t::length;
//
///////////////////////////////////////////////////////////////////////////////
struct nil_t {};
using boost::remove_reference;
using boost::call_traits;

//////////////////////////////////
namespace impl {

    template <typename T>
    struct access {

        typedef const T& ctype;
        typedef T& type;
    };

    template <typename T>
    struct access<T&> {

        typedef T& ctype;
        typedef T& type;
    };
}

///////////////////////////////////////////////////////////////////////////////
//
//  tuple_element
//
//      A query class that gets the Nth element inside a tuple.
//      Examples:
//
//          tuple_element<1, tuple<int, char, void*> >::type    //  plain
//          tuple_element<1, tuple<int, char, void*> >::rtype   //  ref
//          tuple_element<1, tuple<int, char, void*> >::crtype  //  const ref
//
//      Has type char which is the 2nd type in the tuple
//      (note zero based, thus 0 = 1st item, 1 = 2nd item).
//
//          Given a tuple object, the static function tuple_element<N,
//          TupleT>::get(tuple) gets the Nth element in the tuple. The
//          tuple class' tuple::operator[] uses this to get its Nth
//          element.
//
///////////////////////////////////////////////////////////////////////////////
template <int N, typename TupleT>
struct tuple_element
{
    typedef nil_t type;
    typedef nil_t& rtype;
    typedef nil_t const& crtype;

    static nil_t    get(TupleT const&)      { return nil_t(); }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<0, TupleT>
{
    typedef typename TupleT::a_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.a; }
    static crtype   get(TupleT const& t)    { return t.a; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<1, TupleT>
{
    typedef typename TupleT::b_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.b; }
    static crtype   get(TupleT const& t)    { return t.b; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<2, TupleT>
{
    typedef typename TupleT::c_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.c; }
    static crtype   get(TupleT const& t)    { return t.c; }
};

#if PHOENIX_LIMIT > 3
//////////////////////////////////
template <typename TupleT>
struct tuple_element<3, TupleT>
{
    typedef typename TupleT::d_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.d; }
    static crtype   get(TupleT const& t)    { return t.d; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<4, TupleT>
{
    typedef typename TupleT::e_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.e; }
    static crtype   get(TupleT const& t)    { return t.e; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<5, TupleT>
{
    typedef typename TupleT::f_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.f; }
    static crtype   get(TupleT const& t)    { return t.f; }
};

#if PHOENIX_LIMIT > 6
//////////////////////////////////
template <typename TupleT>
struct tuple_element<6, TupleT>
{
    typedef typename TupleT::g_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.g; }
    static crtype   get(TupleT const& t)    { return t.g; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<7, TupleT>
{
    typedef typename TupleT::h_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.h; }
    static crtype   get(TupleT const& t)    { return t.h; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<8, TupleT>
{
    typedef typename TupleT::i_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.i; }
    static crtype   get(TupleT const& t)    { return t.i; }
};

#if PHOENIX_LIMIT > 9
//////////////////////////////////
template <typename TupleT>
struct tuple_element<9, TupleT>
{
    typedef typename TupleT::j_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.j; }
    static crtype   get(TupleT const& t)    { return t.j; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<10, TupleT>
{
    typedef typename TupleT::k_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.k; }
    static crtype   get(TupleT const& t)    { return t.k; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<11, TupleT>
{
    typedef typename TupleT::l_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.l; }
    static crtype   get(TupleT const& t)    { return t.l; }
};

#if PHOENIX_LIMIT > 12
//////////////////////////////////
template <typename TupleT>
struct tuple_element<12, TupleT>
{
    typedef typename TupleT::m_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.m; }
    static crtype   get(TupleT const& t)    { return t.m; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<13, TupleT>
{
    typedef typename TupleT::n_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.n; }
    static crtype   get(TupleT const& t)    { return t.n; }
};

//////////////////////////////////
template <typename TupleT>
struct tuple_element<14, TupleT>
{
    typedef typename TupleT::o_type type;
    typedef typename impl::access<type>::type rtype;
    typedef typename impl::access<type>::ctype crtype;

    static rtype    get(TupleT& t)          { return t.o; }
    static crtype   get(TupleT const& t)    { return t.o; }
};

#endif
#endif
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  tuple forward declaration.
//
///////////////////////////////////////////////////////////////////////////////
template <
        typename A = nil_t
    ,   typename B = nil_t
    ,   typename C = nil_t

#if PHOENIX_LIMIT > 3
    ,   typename D = nil_t
    ,   typename E = nil_t
    ,   typename F = nil_t

#if PHOENIX_LIMIT > 6
    ,   typename G = nil_t
    ,   typename H = nil_t
    ,   typename I = nil_t

#if PHOENIX_LIMIT > 9
    ,   typename J = nil_t
    ,   typename K = nil_t
    ,   typename L = nil_t

#if PHOENIX_LIMIT > 12
    ,   typename M = nil_t
    ,   typename N = nil_t
    ,   typename O = nil_t

#endif
#endif
#endif
#endif

    ,   typename NU = nil_t  // Not used
>
struct tuple;

///////////////////////////////////////////////////////////////////////////////
//
//  tuple_index
//
//      This class wraps an integer in a type to be used for indexing
//      the Nth element in a tuple. See tuple operator[]. Some
//      predefined names are provided in sub-namespace
//      tuple_index_names.
//
///////////////////////////////////////////////////////////////////////////////
template <int N>
struct tuple_index {};

//////////////////////////////////
namespace tuple_index_names {

    tuple_index<0> const _1 = tuple_index<0>();
    tuple_index<1> const _2 = tuple_index<1>();
    tuple_index<2> const _3 = tuple_index<2>();

#if PHOENIX_LIMIT > 3
    tuple_index<3> const _4 = tuple_index<3>();
    tuple_index<4> const _5 = tuple_index<4>();
    tuple_index<5> const _6 = tuple_index<5>();

#if PHOENIX_LIMIT > 6
    tuple_index<6> const _7 = tuple_index<6>();
    tuple_index<7> const _8 = tuple_index<7>();
    tuple_index<8> const _9 = tuple_index<8>();

#if PHOENIX_LIMIT > 9
    tuple_index<9> const _10 = tuple_index<9>();
    tuple_index<10> const _11 = tuple_index<10>();
    tuple_index<11> const _12 = tuple_index<11>();

#if PHOENIX_LIMIT > 12
    tuple_index<12> const _13 = tuple_index<12>();
    tuple_index<13> const _14 = tuple_index<13>();
    tuple_index<14> const _15 = tuple_index<14>();

#endif
#endif
#endif
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
//  tuple_common class
//
///////////////////////////////////////////////////////////////////////////////
template <typename DerivedT>
struct tuple_base {

    typedef nil_t   a_type;
    typedef nil_t   b_type;
    typedef nil_t   c_type;

#if PHOENIX_LIMIT > 3
    typedef nil_t   d_type;
    typedef nil_t   e_type;
    typedef nil_t   f_type;

#if PHOENIX_LIMIT > 6
    typedef nil_t   g_type;
    typedef nil_t   h_type;
    typedef nil_t   i_type;

#if PHOENIX_LIMIT > 9
    typedef nil_t   j_type;
    typedef nil_t   k_type;
    typedef nil_t   l_type;

#if PHOENIX_LIMIT > 12
    typedef nil_t   m_type;
    typedef nil_t   n_type;
    typedef nil_t   o_type;

#endif
#endif
#endif
#endif

    template <int N>
    typename tuple_element<N, DerivedT>::crtype
    operator[](tuple_index<N>) const
    {
        return tuple_element<N, DerivedT>
            ::get(*static_cast<DerivedT const*>(this));
    }

    template <int N>
    typename tuple_element<N, DerivedT>::rtype
    operator[](tuple_index<N>)
    {
        return tuple_element<N, DerivedT>
            ::get(*static_cast<DerivedT*>(this));
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <0 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <>
struct tuple<>
:   public tuple_base<tuple<> > {

    BOOST_STATIC_CONSTANT(int, length = 0);
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <1 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename A>
struct tuple<A, nil_t, nil_t,
#if PHOENIX_LIMIT > 3
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 6
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
#endif
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A> > {

    BOOST_STATIC_CONSTANT(int, length = 1);
    typedef A a_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_
    ):  a(a_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <2 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
struct tuple<A, B, nil_t,
#if PHOENIX_LIMIT > 3
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 6
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
#endif
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B> > {

    BOOST_STATIC_CONSTANT(int, length = 2);
    typedef A a_type; typedef B b_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_
    ):  a(a_), b(b_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <3 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B, typename C>
struct tuple<A, B, C,
#if PHOENIX_LIMIT > 3
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 6
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
#endif
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C> > {

    BOOST_STATIC_CONSTANT(int, length = 3);
    typedef A a_type; typedef B b_type;
    typedef C c_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_
    ):  a(a_), b(b_), c(c_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c;
};

#if PHOENIX_LIMIT > 3
///////////////////////////////////////////////////////////////////////////////
//
//  tuple <4 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B, typename C, typename D>
struct tuple<A, B, C, D, nil_t, nil_t,
#if PHOENIX_LIMIT > 6
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C, D> > {

    BOOST_STATIC_CONSTANT(int, length = 4);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_
    ):  a(a_), b(b_), c(c_), d(d_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <5 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B, typename C, typename D, typename E>
struct tuple<A, B, C, D, E, nil_t,
#if PHOENIX_LIMIT > 6
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C, D, E> > {

    BOOST_STATIC_CONSTANT(int, length = 5);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <6 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F>
struct tuple<A, B, C, D, E, F,
#if PHOENIX_LIMIT > 6
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C, D, E, F> > {

    BOOST_STATIC_CONSTANT(int, length = 6);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f;
};

#if PHOENIX_LIMIT > 6
///////////////////////////////////////////////////////////////////////////////
//
//  tuple <7 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G>
struct tuple<A, B, C, D, E, F, G, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C, D, E, F, G> > {

    BOOST_STATIC_CONSTANT(int, length = 7);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;
    typedef G g_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_,
        typename call_traits<G>::param_type g_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()]),
        g(init[tuple_index<6>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f; G g;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <8 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H>
struct tuple<A, B, C, D, E, F, G, H, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C, D, E, F, G, H> > {

    BOOST_STATIC_CONSTANT(int, length = 8);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;
    typedef G g_type; typedef H h_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_,
        typename call_traits<G>::param_type g_,
        typename call_traits<H>::param_type h_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()]),
        g(init[tuple_index<6>()]), h(init[tuple_index<7>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f; G g; H h;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <9 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I>
struct tuple<A, B, C, D, E, F, G, H, I,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C, D, E, F, G, H, I> > {

    BOOST_STATIC_CONSTANT(int, length = 9);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;
    typedef G g_type; typedef H h_type;
    typedef I i_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_,
        typename call_traits<G>::param_type g_,
        typename call_traits<H>::param_type h_,
        typename call_traits<I>::param_type i_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()]),
        g(init[tuple_index<6>()]), h(init[tuple_index<7>()]),
        i(init[tuple_index<8>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f; G g; H h; I i;
};

#if PHOENIX_LIMIT > 9
///////////////////////////////////////////////////////////////////////////////
//
//  tuple <10 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J>
struct tuple<A, B, C, D, E, F, G, H, I, J, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C, D, E, F, G, H, I, J> > {

    BOOST_STATIC_CONSTANT(int, length = 10);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;
    typedef G g_type; typedef H h_type;
    typedef I i_type; typedef J j_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_,
        typename call_traits<G>::param_type g_,
        typename call_traits<H>::param_type h_,
        typename call_traits<I>::param_type i_,
        typename call_traits<J>::param_type j_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()]),
        g(init[tuple_index<6>()]), h(init[tuple_index<7>()]),
        i(init[tuple_index<8>()]), j(init[tuple_index<9>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f; G g; H h; I i; J j;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <11 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K>
struct tuple<A, B, C, D, E, F, G, H, I, J, K, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C, D, E, F, G, H, I, J, K> > {

    BOOST_STATIC_CONSTANT(int, length = 11);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;
    typedef G g_type; typedef H h_type;
    typedef I i_type; typedef J j_type;
    typedef K k_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_,
        typename call_traits<G>::param_type g_,
        typename call_traits<H>::param_type h_,
        typename call_traits<I>::param_type i_,
        typename call_traits<J>::param_type j_,
        typename call_traits<K>::param_type k_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()]),
        g(init[tuple_index<6>()]), h(init[tuple_index<7>()]),
        i(init[tuple_index<8>()]), j(init[tuple_index<9>()]),
        k(init[tuple_index<10>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f; G g; H h; I i; J j;
    K k;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <12 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L>
struct tuple<A, B, C, D, E, F, G, H, I, J, K, L,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
>
:   public tuple_base<tuple<A, B, C, D, E, F, G, H, I, J, K, L> > {

    BOOST_STATIC_CONSTANT(int, length = 12);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;
    typedef G g_type; typedef H h_type;
    typedef I i_type; typedef J j_type;
    typedef K k_type; typedef L l_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_,
        typename call_traits<G>::param_type g_,
        typename call_traits<H>::param_type h_,
        typename call_traits<I>::param_type i_,
        typename call_traits<J>::param_type j_,
        typename call_traits<K>::param_type k_,
        typename call_traits<L>::param_type l_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_), l(l_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()]),
        g(init[tuple_index<6>()]), h(init[tuple_index<7>()]),
        i(init[tuple_index<8>()]), j(init[tuple_index<9>()]),
        k(init[tuple_index<10>()]), l(init[tuple_index<11>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f; G g; H h; I i; J j;
    K k; L l;
};

#if PHOENIX_LIMIT > 12
///////////////////////////////////////////////////////////////////////////////
//
//  tuple <13 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M>
struct tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, nil_t, nil_t, nil_t>
:   public tuple_base<
        tuple<A, B, C, D, E, F, G, H, I, J, K, L, M> > {

    BOOST_STATIC_CONSTANT(int, length = 13);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;
    typedef G g_type; typedef H h_type;
    typedef I i_type; typedef J j_type;
    typedef K k_type; typedef L l_type;
    typedef M m_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_,
        typename call_traits<G>::param_type g_,
        typename call_traits<H>::param_type h_,
        typename call_traits<I>::param_type i_,
        typename call_traits<J>::param_type j_,
        typename call_traits<K>::param_type k_,
        typename call_traits<L>::param_type l_,
        typename call_traits<M>::param_type m_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_), l(l_), m(m_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()]),
        g(init[tuple_index<6>()]), h(init[tuple_index<7>()]),
        i(init[tuple_index<8>()]), j(init[tuple_index<9>()]),
        k(init[tuple_index<10>()]), l(init[tuple_index<11>()]),
        m(init[tuple_index<12>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f; G g; H h; I i; J j;
    K k; L l; M m;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <14 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N>
struct tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, N, nil_t, nil_t>
:   public tuple_base<
        tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, N> > {

    BOOST_STATIC_CONSTANT(int, length = 14);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;
    typedef G g_type; typedef H h_type;
    typedef I i_type; typedef J j_type;
    typedef K k_type; typedef L l_type;
    typedef M m_type; typedef N n_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_,
        typename call_traits<G>::param_type g_,
        typename call_traits<H>::param_type h_,
        typename call_traits<I>::param_type i_,
        typename call_traits<J>::param_type j_,
        typename call_traits<K>::param_type k_,
        typename call_traits<L>::param_type l_,
        typename call_traits<M>::param_type m_,
        typename call_traits<N>::param_type n_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_), l(l_), m(m_), n(n_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()]),
        g(init[tuple_index<6>()]), h(init[tuple_index<7>()]),
        i(init[tuple_index<8>()]), j(init[tuple_index<9>()]),
        k(init[tuple_index<10>()]), l(init[tuple_index<11>()]),
        m(init[tuple_index<12>()]), n(init[tuple_index<13>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f; G g; H h; I i; J j;
    K k; L l; M m; N n;
};

///////////////////////////////////////////////////////////////////////////////
//
//  tuple <15 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N, typename O>
struct tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, nil_t>
:   public tuple_base<
        tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> > {

    BOOST_STATIC_CONSTANT(int, length = 15);
    typedef A a_type; typedef B b_type;
    typedef C c_type; typedef D d_type;
    typedef E e_type; typedef F f_type;
    typedef G g_type; typedef H h_type;
    typedef I i_type; typedef J j_type;
    typedef K k_type; typedef L l_type;
    typedef M m_type; typedef N n_type;
    typedef O o_type;

    tuple() {}

    tuple(
        typename call_traits<A>::param_type a_,
        typename call_traits<B>::param_type b_,
        typename call_traits<C>::param_type c_,
        typename call_traits<D>::param_type d_,
        typename call_traits<E>::param_type e_,
        typename call_traits<F>::param_type f_,
        typename call_traits<G>::param_type g_,
        typename call_traits<H>::param_type h_,
        typename call_traits<I>::param_type i_,
        typename call_traits<J>::param_type j_,
        typename call_traits<K>::param_type k_,
        typename call_traits<L>::param_type l_,
        typename call_traits<M>::param_type m_,
        typename call_traits<N>::param_type n_,
        typename call_traits<O>::param_type o_
    ):  a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_), l(l_), m(m_), n(n_), o(o_) {}

    template <typename TupleT>
    tuple(TupleT const& init)
    :   a(init[tuple_index<0>()]), b(init[tuple_index<1>()]),
        c(init[tuple_index<2>()]), d(init[tuple_index<3>()]),
        e(init[tuple_index<4>()]), f(init[tuple_index<5>()]),
        g(init[tuple_index<6>()]), h(init[tuple_index<7>()]),
        i(init[tuple_index<8>()]), j(init[tuple_index<9>()]),
        k(init[tuple_index<10>()]), l(init[tuple_index<11>()]),
        m(init[tuple_index<12>()]), n(init[tuple_index<13>()]),
        o(init[tuple_index<14>()])
    { BOOST_STATIC_ASSERT(TupleT::length == length); }

    A a; B b; C c; D d; E e;
    F f; G g; H h; I i; J j;
    K k; L l; M m; N n; O o;
};

#endif
#endif
#endif
#endif

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

///////////////////////////////////////////////////////////////////////////////
}   //  namespace phoenix

#endif
