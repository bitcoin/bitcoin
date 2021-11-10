/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2001-2003 Hartmut Kaiser
    Copyright (c) 2003 Vaclav Vesely

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_NEW_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_NEW_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/phoenix/actor.hpp>
#include <boost/spirit/home/classic/phoenix/composite.hpp>
#include <boost/static_assert.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  Phoenix predefined maximum new_ limit. This limit defines the maximum
//  number of parameters supported for calles to the set of new_ template
//  functions (lazy object construction, see below). This number defaults to 3.
//  The actual maximum is rounded up in multiples of 3. Thus, if this value
//  is 4, the actual limit is 6. The ultimate maximum limit in this
//  implementation is 15.
//  PHOENIX_CONSTRUCT_LIMIT should NOT be greater than PHOENIX_LIMIT!

#if !defined(PHOENIX_CONSTRUCT_LIMIT)
#define PHOENIX_CONSTRUCT_LIMIT PHOENIX_LIMIT
#endif

// ensure PHOENIX_CONSTRUCT_LIMIT <= PHOENIX_LIMIT
BOOST_STATIC_ASSERT(PHOENIX_CONSTRUCT_LIMIT <= PHOENIX_LIMIT);

// ensure PHOENIX_CONSTRUCT_LIMIT <= 15
BOOST_STATIC_ASSERT(PHOENIX_CONSTRUCT_LIMIT <= 15);

///////////////////////////////////////////////////////////////////////////////
//
//  new_
//
//      Lazy object construction
//
//      The set of new_<> template classes and functions provide a way
//      of lazily constructing certain object from a arbitrary set of
//      actors during parsing.
//      The new_ templates are (syntactically) used very much like
//      the well known C++ casts:
//
//          A *a = new_<A>(...arbitrary list of actors...);
//
//      where the given parameters are submitted as parameters to the
//      constructor of the object of type A. (This certainly implies, that
//      type A has a constructor with a fitting set of parameter types
//      defined.)
//
//      The maximum number of needed parameters is controlled through the
//      preprocessor constant PHOENIX_CONSTRUCT_LIMIT. Note though, that this
//      limit should not be greater than PHOENIX_LIMIT.
//
///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct new_l_0
{
    typedef T* result_type;

    T* operator()() const
    {
        return new T();
    }
};

template <typename T>
struct new_l {

    template <
            typename A
        ,   typename B
        ,   typename C

#if PHOENIX_CONSTRUCT_LIMIT > 3
        ,   typename D
        ,   typename E
        ,   typename F

#if PHOENIX_CONSTRUCT_LIMIT > 6
        ,   typename G
        ,   typename H
        ,   typename I

#if PHOENIX_CONSTRUCT_LIMIT > 9
        ,   typename J
        ,   typename K
        ,   typename L

#if PHOENIX_CONSTRUCT_LIMIT > 12
        ,   typename M
        ,   typename N
        ,   typename O
#endif
#endif
#endif
#endif
    >
    struct result { typedef T* type; };

    T* operator()() const {
        return new T();
    }

    template <typename A>
    T* operator()(A const& a) const {
        return new T(a);
    }

    template <typename A, typename B>
    T* operator()(A const& a, B const& b) const {
        return new T(a, b);
    }

    template <typename A, typename B, typename C>
    T* operator()(A const& a, B const& b, C const& c) const {
        return new T(a, b, c);
    }

#if PHOENIX_CONSTRUCT_LIMIT > 3
    template <
        typename A, typename B, typename C, typename D
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d) const
    {
        return new T(a, b, c, d);
    }

    template <
        typename A, typename B, typename C, typename D, typename E
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e) const
    {
        return new T(a, b, c, d, e);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f) const
    {
        return new T(a, b, c, d, e, f);
    }

#if PHOENIX_CONSTRUCT_LIMIT > 6
    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g) const
    {
        return new T(a, b, c, d, e, f, g);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h) const
    {
        return new T(a, b, c, d, e, f, g, h);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i) const
    {
        return new T(a, b, c, d, e, f, g, h, i);
    }

#if PHOENIX_CONSTRUCT_LIMIT > 9
    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j, k);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j, k, l);
    }

#if PHOENIX_CONSTRUCT_LIMIT > 12
    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j, k, l, m);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N, typename O
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n, O const& o) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
    }

#endif
#endif
#endif
#endif
};

template <typename T>
struct new_1 {

    template <
            typename A
    >
    struct result { typedef T* type; };

    template <typename A>
    T* operator()(A const& a) const {
        return new T(a);
    }

};

template <typename T>
struct new_2 {

    template <
            typename A
        ,   typename B
    >
    struct result { typedef T* type; };

    template <typename A, typename B>
    T* operator()(A const& a, B const& b) const {
        return new T(a, b);
    }

};

template <typename T>
struct new_3 {

    template <
            typename A
        ,   typename B
        ,   typename C
    >
    struct result { typedef T* type; };

    template <typename A, typename B, typename C>
    T* operator()(A const& a, B const& b, C const& c) const {
        return new T(a, b, c);
    }
};

#if PHOENIX_CONSTRUCT_LIMIT > 3
template <typename T>
struct new_4 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
    >
    struct result { typedef T* type; };


    template <
        typename A, typename B, typename C, typename D
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d) const
    {
        return new T(a, b, c, d);
    }
};


template <typename T>
struct new_5 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
    >
    struct result { typedef T* type; };

    template <
        typename A, typename B, typename C, typename D, typename E
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e) const
    {
        return new T(a, b, c, d, e);
    }
};


template <typename T>
struct new_6 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
    >
    struct result { typedef T* type; };

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f) const
    {
        return new T(a, b, c, d, e, f);
    }
};
#endif


#if PHOENIX_CONSTRUCT_LIMIT > 6
template <typename T>
struct new_7 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
    >
    struct result { typedef T* type; };

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g) const
    {
        return new T(a, b, c, d, e, f, g);
    }
};

template <typename T>
struct new_8 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
    >
    struct result { typedef T* type; };

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h) const
    {
        return new T(a, b, c, d, e, f, g, h);
    }
};

template <typename T>
struct new_9 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
    >
    struct result { typedef T* type; };

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i) const
    {
        return new T(a, b, c, d, e, f, g, h, i);
    }
};
#endif


#if PHOENIX_CONSTRUCT_LIMIT > 9
template <typename T>
struct new_10 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
    >
    struct result { typedef T* type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j);
    }
};

template <typename T>
struct new_11 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
    >
    struct result { typedef T* type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j, k);
    }

};

template <typename T>
struct new_12 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
        ,   typename L
    >
    struct result { typedef T* type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l) const
    {
        return new T(a, b, c, d, f, e, g, h, i, j, k, l);
    }
};
#endif

#if PHOENIX_CONSTRUCT_LIMIT > 12
template <typename T>
struct new_13 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
        ,   typename L
        ,   typename M
    >
    struct result { typedef T* type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j, k, l, m);
    }
};

template <typename T>
struct new_14 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
        ,   typename L
        ,   typename M
        ,   typename N
    >
    struct result { typedef T* type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n) const
    {
        return new T(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
    }

};

template <typename T>
struct new_15 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
        ,   typename L
        ,   typename M
        ,   typename N
        ,   typename O
    >
    struct result { typedef T* type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N, typename O
    >
    T* operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n, O const& o) const
    {
        return new T(a, b, c, d, f, e, g, h, i, j, k, l, m, n, o);
    }

};
#endif


#if defined(BOOST_BORLANDC) || (defined(__MWERKS__) && (__MWERKS__ <= 0x3002))

///////////////////////////////////////////////////////////////////////////////
//
//  The following specializations are needed because Borland and CodeWarrior
//  does not accept default template arguments in nested template classes in
//  classes (i.e new_l::result)
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename TupleT>
struct composite0_result<new_l_0<T>, TupleT> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A>
struct composite1_result<new_l<T>, TupleT, A> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B>
struct composite2_result<new_l<T>, TupleT, A, B> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C>
struct composite3_result<new_l<T>, TupleT, A, B, C> {

    typedef T* type;
};

#if PHOENIX_LIMIT > 3
//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D>
struct composite4_result<new_l<T>, TupleT,
    A, B, C, D> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E>
struct composite5_result<new_l<T>, TupleT,
    A, B, C, D, E> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F>
struct composite6_result<new_l<T>, TupleT,
    A, B, C, D, E, F> {

    typedef T* type;
};

#if PHOENIX_LIMIT > 6
//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G>
struct composite7_result<new_l<T>, TupleT,
    A, B, C, D, E, F, G> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H>
struct composite8_result<new_l<T>, TupleT,
    A, B, C, D, E, F, G, H> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I>
struct composite9_result<new_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I> {

    typedef T* type;
};

#if PHOENIX_LIMIT > 9
//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J>
struct composite10_result<new_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K>
struct composite11_result<new_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L>
struct composite12_result<new_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L> {

    typedef T* type;
};

#if PHOENIX_LIMIT > 12
//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M>
struct composite13_result<new_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L, M> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N>
struct composite14_result<new_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N> {

    typedef T* type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N, typename O>
struct composite15_result<new_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> {

    typedef T* type;
};

#endif
#endif
#endif
#endif
#endif

//////////////////////////////////
template <typename T>
inline typename impl::make_composite<new_l_0<T> >::type
new_()
{
    typedef impl::make_composite<new_l_0<T> > make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_l_0<T>()));
}

//////////////////////////////////
template <typename T, typename A>
inline typename impl::make_composite<new_1<T>, A>::type
new_(A const& a)
{
    typedef impl::make_composite<new_1<T>, A> make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_1<T>(),
        as_actor<A>::convert(a)
    ));
}

//////////////////////////////////
template <typename T, typename A, typename B>
inline typename impl::make_composite<new_2<T>, A, B>::type
new_(A const& a, B const& b)
{
    typedef impl::make_composite<new_2<T>, A, B> make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_2<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b)
    ));
}

//////////////////////////////////
template <typename T, typename A, typename B, typename C>
inline typename impl::make_composite<new_3<T>, A, B, C>::type
new_(A const& a, B const& b, C const& c)
{
    typedef impl::make_composite<new_3<T>, A, B, C> make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_3<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c)
    ));
}

#if PHOENIX_CONSTRUCT_LIMIT > 3
//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D
>
inline typename impl::make_composite<new_4<T>, A, B, C, D>::type
new_(
    A const& a, B const& b, C const& c, D const& d)
{
    typedef
        impl::make_composite<new_4<T>, A, B, C, D>
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_4<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d)
    ));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E
>
inline typename impl::make_composite<new_5<T>, A, B, C, D, E>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e)
{
    typedef
        impl::make_composite<new_5<T>, A, B, C, D, E>
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_5<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e)
    ));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F
>
inline typename impl::make_composite<new_6<T>, A, B, C, D, E, F>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f)
{
    typedef
        impl::make_composite<new_6<T>, A, B, C, D, E, F>
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_6<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f)
    ));
}

#if PHOENIX_CONSTRUCT_LIMIT > 6
//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G
>
inline typename impl::make_composite<new_7<T>, A, B, C, D, E, F, G>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g)
{
    typedef
        impl::make_composite<new_7<T>, A, B, C, D, E, F, G>
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_7<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g)
    ));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H
>
inline typename impl::make_composite<new_8<T>, A, B, C, D, E, F, G, H>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h)
{
    typedef
        impl::make_composite<new_8<T>, A, B, C, D, E, F, G, H>
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_8<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h)
    ));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I
>
inline typename impl::make_composite<new_9<T>, A, B, C, D, E, F, G, H, I>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i)
{
    typedef
        impl::make_composite<new_9<T>, A, B, C, D, E, F, G, H, I>
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_9<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h),
        as_actor<I>::convert(i)
    ));
}

#if PHOENIX_CONSTRUCT_LIMIT > 9
//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J
>
inline typename impl::make_composite<
    new_10<T>, A, B, C, D, E, F, G, H, I, J>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j)
{
    typedef
        impl::make_composite<
            new_10<T>, A, B, C, D, E, F, G, H, I, J
        >
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_10<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h),
        as_actor<I>::convert(i),
        as_actor<J>::convert(j)
    ));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K
>
inline typename impl::make_composite<
    new_11<T>, A, B, C, D, E, F, G, H, I, J, K>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k)
{
    typedef
        impl::make_composite<
            new_11<T>, A, B, C, D, E, F, G, H, I, J, K
        >
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_11<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h),
        as_actor<I>::convert(i),
        as_actor<J>::convert(j),
        as_actor<K>::convert(k)
    ));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K,
    typename L
>
inline typename impl::make_composite<
    new_12<T>, A, B, C, D, E, F, G, H, I, J, K, L>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l)
{
    typedef
        impl::make_composite<
            new_12<T>, A, B, C, D, E, F, G, H, I, J, K, L
        >
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_12<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h),
        as_actor<I>::convert(i),
        as_actor<J>::convert(j),
        as_actor<K>::convert(k),
        as_actor<L>::convert(l)
    ));
}

#if PHOENIX_CONSTRUCT_LIMIT > 12
//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K,
    typename L, typename M
>
inline typename impl::make_composite<
    new_13<T>, A, B, C, D, E, F, G, H, I, J, K, L, M>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l, M const& m)
{
    typedef
        impl::make_composite<
            new_13<T>, A, B, C, D, E, F, G, H, I, J, K, L, M
        >
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_13<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h),
        as_actor<I>::convert(i),
        as_actor<J>::convert(j),
        as_actor<K>::convert(k),
        as_actor<L>::convert(l),
        as_actor<M>::convert(m)
    ));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K,
    typename L, typename M, typename N
>
inline typename impl::make_composite<
    new_14<T>, A, B, C, D, E, F, G, H, I, J, K, L, M>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l, M const& m, N const& n)
{
    typedef
        impl::make_composite<
            new_14<T>, A, B, C, D, E, F, G, H, I, J, K, L, M, N
        >
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_14<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h),
        as_actor<I>::convert(i),
        as_actor<J>::convert(j),
        as_actor<K>::convert(k),
        as_actor<L>::convert(l),
        as_actor<M>::convert(m),
        as_actor<N>::convert(n)
    ));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K,
    typename L, typename M, typename N, typename O
>
inline typename impl::make_composite<
    new_15<T>, A, B, C, D, E, F, G, H, I, J, K, L, M, O>::type
new_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l, M const& m, N const& n, O const& o)
{
    typedef
        impl::make_composite<
            new_15<T>, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O
        >
        make_composite_t;
    typedef typename make_composite_t::type type_t;
    typedef typename make_composite_t::composite_type composite_type_t;

    return type_t(composite_type_t(new_15<T>(),
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h),
        as_actor<I>::convert(i),
        as_actor<J>::convert(j),
        as_actor<K>::convert(k),
        as_actor<L>::convert(l),
        as_actor<M>::convert(m),
        as_actor<N>::convert(n),
        as_actor<O>::convert(o)
    ));
}

#endif
#endif
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
}   //  namespace phoenix

#endif
