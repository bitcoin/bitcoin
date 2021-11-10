/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2002 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_COMPOSITE_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_COMPOSITE_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/phoenix/actor.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  composite class
//
//      A composite is an actor base class composed of zero or more
//      actors (see actor.hpp) and an operation. A composite is itself
//      an actor superclass and conforms to its conceptual interface.
//      Its eval member function un-funnels the tupled actual arguments
//      from the tuple by invoking each of the actors' eval member
//      function. The results of each are then passed on as arguments to
//      the operation. Specializations are provided to handle different
//      numbers of actors.
//
//      Schematically:
//
//          actor0.eval(tupled_args) --> arg0 --> |
//          actor1.eval(tupled_args) --> arg1 --> |
//          actor2.eval(tupled_args) --> arg3 --> | --> operation(arg0...argN)
//            ...                                 |
//          actorN.eval(tupled_args) --> argN --> |
//
//      The operation can be any suitable functor that can accept the
//      arguments passed in by the composite. The operation is expected
//      to have a member operator() that carries out the actual
//      operation. There should be a one to one correspondence between
//      actors of the composite and the arguments of the operation's
//      member operator().
//
//      The operation is also expected to have a nested template class
//      result<T0...TN>. The nested template class result should have a
//      typedef 'type' that reflects the return type of its member
//      operator(). This is essentially a type computer that answers the
//      metaprogramming question "Given arguments of type T0...TN, what
//      will be its operator()'s return type?".
//
//      There is a special case for operations that accept no arguments.
//      Such nullary operations are only required to define a typedef
//      result_type that reflects the return type of its operator().
//
//      Here's an example of a simple operation that squares a number:
//
//          struct square {
//
//              template <typename ArgT>
//              struct result { typedef ArgT type; };
//
//              template <typename ArgT>
//              ArgT operator()(ArgT n) const { return n * n; }
//          };
//
//      As can be seen, operations can be polymorphic. Its arguments and
//      return type are not fixed to a particular type. The example
//      above for example, can handle any ArgT type as long as it has a
//      multiplication operator.
//
//      Composites are not created directly. Instead, there are meta-
//      programs provided that indirectly create composites. See
//      operators.hpp, binders.hpp and functions.hpp for examples.
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename OperationT
    ,   typename A = nil_t
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
struct composite;

///////////////////////////////////////////////////////////////////////////////
//
//  composite <0 actor> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT>
struct composite0_result {

    typedef typename OperationT::result_type type;
};

//////////////////////////////////
template <typename OperationT>
struct composite<OperationT,
    nil_t, nil_t, nil_t,
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
> {

    typedef composite<OperationT> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite0_result<
            OperationT, TupleT
        >::type type;
    };

    composite(OperationT const& op_)
    :   op(op_) {}

    template <typename TupleT>
    typename OperationT::result_type
    eval(TupleT const& /*args*/) const
    {
        return op();
    }

    mutable OperationT op; //  operation
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <1 actor> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A>
struct composite1_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A>
struct composite<OperationT,
    A, nil_t, nil_t,
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
> {

    typedef composite<OperationT, A> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite1_result<
            OperationT, TupleT, A
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_)
    :   op(op_), a(a_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        return op(ra);
    }

    mutable OperationT op; //  operation
    A a; //  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <2 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B>
struct composite2_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B>
struct composite<OperationT,
    A, B, nil_t,
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
> {

    typedef composite<OperationT, A, B> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite2_result<
            OperationT, TupleT, A, B
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_)
    :   op(op_), a(a_), b(b_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        return op(ra, rb);
    }

    mutable OperationT op; //  operation
    A a; B b; //  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <3 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C>
struct composite3_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C>
struct composite<OperationT,
    A, B, C,
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
> {

    typedef composite<OperationT, A, B, C> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite3_result<
            OperationT, TupleT, A, B, C
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_)
    :   op(op_), a(a_), b(b_), c(c_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        return op(ra, rb, rc);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; //  actors
};

#if PHOENIX_LIMIT > 3
///////////////////////////////////////////////////////////////////////////////
//
//  composite <4 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D>
struct composite4_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D>
struct composite<OperationT,
    A, B, C, D, nil_t, nil_t,
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
> {

    typedef composite<OperationT, A, B, C, D> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite4_result<
            OperationT, TupleT, A, B, C, D
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        return op(ra, rb, rc, rd);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; //  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <5 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E>
struct composite5_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E>
struct composite<OperationT,
    A, B, C, D, E, nil_t,
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
> {

    typedef composite<OperationT, A, B, C, D, E> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite5_result<
            OperationT, TupleT, A, B, C, D, E
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        return op(ra, rb, rc, rd, re);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; //  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <6 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F>
struct composite6_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F>
struct composite<OperationT,
    A, B, C, D, E, F,
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
> {

    typedef composite<OperationT, A, B, C, D, E, F> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite6_result<
            OperationT, TupleT, A, B, C, D, E, F
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        return op(ra, rb, rc, rd, re, rf);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; //  actors
};

#if PHOENIX_LIMIT > 6
///////////////////////////////////////////////////////////////////////////////
//
//  composite <7 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G>
struct composite7_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type,
        typename actor_result<G, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G>
struct composite<OperationT,
    A, B, C, D, E, F, G, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef composite<OperationT, A, B, C, D, E, F, G> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite7_result<
            OperationT, TupleT, A, B, C, D, E, F, G
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_, G const& g_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        typename actor_result<G, TupleT>::type rg = g.eval(args);
        return op(ra, rb, rc, rd, re, rf, rg);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; G g; //  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <8 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H>
struct composite8_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type,
        typename actor_result<G, TupleT>::plain_type,
        typename actor_result<H, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H>
struct composite<OperationT,
    A, B, C, D, E, F, G, H, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef composite<OperationT, A, B, C, D, E, F, G, H> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite8_result<
            OperationT, TupleT, A, B, C, D, E, F, G, H
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_, G const& g_, H const& h_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        typename actor_result<G, TupleT>::type rg = g.eval(args);
        typename actor_result<H, TupleT>::type rh = h.eval(args);
        return op(ra, rb, rc, rd, re, rf, rg, rh);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; G g; H h; //  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <9 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I>
struct composite9_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type,
        typename actor_result<G, TupleT>::plain_type,
        typename actor_result<H, TupleT>::plain_type,
        typename actor_result<I, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I>
struct composite<OperationT,
    A, B, C, D, E, F, G, H, I,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef composite<OperationT, A, B, C, D, E, F, G, H, I> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite9_result<
            OperationT, TupleT, A, B, C, D, E, F, G, H, I
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_, G const& g_, H const& h_, I const& i_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        typename actor_result<G, TupleT>::type rg = g.eval(args);
        typename actor_result<H, TupleT>::type rh = h.eval(args);
        typename actor_result<I, TupleT>::type ri = i.eval(args);
        return op(ra, rb, rc, rd, re, rf, rg, rh, ri);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; G g; H h; I i; //  actors
};

#if PHOENIX_LIMIT > 9
///////////////////////////////////////////////////////////////////////////////
//
//  composite <10 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J>
struct composite10_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type,
        typename actor_result<G, TupleT>::plain_type,
        typename actor_result<H, TupleT>::plain_type,
        typename actor_result<I, TupleT>::plain_type,
        typename actor_result<J, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J>
struct composite<OperationT,
    A, B, C, D, E, F, G, H, I, J, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef composite<OperationT, A, B, C, D, E, F, G, H, I, J> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite10_result<
            OperationT, TupleT, A, B, C, D, E, F, G, H, I, J
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_, G const& g_, H const& h_, I const& i_, J const& j_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        typename actor_result<G, TupleT>::type rg = g.eval(args);
        typename actor_result<H, TupleT>::type rh = h.eval(args);
        typename actor_result<I, TupleT>::type ri = i.eval(args);
        typename actor_result<J, TupleT>::type rj = j.eval(args);
        return op(ra, rb, rc, rd, re, rf, rg, rh, ri, rj);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; G g; H h; I i; J j; //  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <11 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K>
struct composite11_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type,
        typename actor_result<G, TupleT>::plain_type,
        typename actor_result<H, TupleT>::plain_type,
        typename actor_result<I, TupleT>::plain_type,
        typename actor_result<J, TupleT>::plain_type,
        typename actor_result<K, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K>
struct composite<OperationT,
    A, B, C, D, E, F, G, H, I, J, K, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef composite<OperationT,
        A, B, C, D, E, F, G, H, I, J, K> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite11_result<
            OperationT, TupleT, A, B, C, D, E, F, G, H, I, J, K
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_, G const& g_, H const& h_, I const& i_, J const& j_,
        K const& k_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        typename actor_result<G, TupleT>::type rg = g.eval(args);
        typename actor_result<H, TupleT>::type rh = h.eval(args);
        typename actor_result<I, TupleT>::type ri = i.eval(args);
        typename actor_result<J, TupleT>::type rj = j.eval(args);
        typename actor_result<K, TupleT>::type rk = k.eval(args);
        return op(ra, rb, rc, rd, re, rf, rg, rh, ri, rj, rk);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; G g; H h; I i; J j;
    K k;//  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <12 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L>
struct composite12_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type,
        typename actor_result<G, TupleT>::plain_type,
        typename actor_result<H, TupleT>::plain_type,
        typename actor_result<I, TupleT>::plain_type,
        typename actor_result<J, TupleT>::plain_type,
        typename actor_result<K, TupleT>::plain_type,
        typename actor_result<L, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L>
struct composite<OperationT,
    A, B, C, D, E, F, G, H, I, J, K, L,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef composite<OperationT,
        A, B, C, D, E, F, G, H, I, J, K, L> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite12_result<
            OperationT, TupleT, A, B, C, D, E, F, G, H, I, J, K, L
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_, G const& g_, H const& h_, I const& i_, J const& j_,
        K const& k_, L const& l_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_), l(l_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        typename actor_result<G, TupleT>::type rg = g.eval(args);
        typename actor_result<H, TupleT>::type rh = h.eval(args);
        typename actor_result<I, TupleT>::type ri = i.eval(args);
        typename actor_result<J, TupleT>::type rj = j.eval(args);
        typename actor_result<K, TupleT>::type rk = k.eval(args);
        typename actor_result<L, TupleT>::type rl = l.eval(args);
        return op(ra, rb, rc, rd, re, rf, rg, rh, ri, rj, rk, rl);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; G g; H h; I i; J j;
    K k; L l;//  actors
};

#if PHOENIX_LIMIT > 12
///////////////////////////////////////////////////////////////////////////////
//
//  composite <13 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M>
struct composite13_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type,
        typename actor_result<G, TupleT>::plain_type,
        typename actor_result<H, TupleT>::plain_type,
        typename actor_result<I, TupleT>::plain_type,
        typename actor_result<J, TupleT>::plain_type,
        typename actor_result<K, TupleT>::plain_type,
        typename actor_result<L, TupleT>::plain_type,
        typename actor_result<M, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M>
struct composite<OperationT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, nil_t, nil_t, nil_t
> {

    typedef composite<OperationT,
        A, B, C, D, E, F, G, H, I, J, K, L, M> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite13_result<
            OperationT, TupleT, A, B, C, D, E, F, G, H, I, J, K, L, M
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_, G const& g_, H const& h_, I const& i_, J const& j_,
        K const& k_, L const& l_, M const& m_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_), l(l_), m(m_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        typename actor_result<G, TupleT>::type rg = g.eval(args);
        typename actor_result<H, TupleT>::type rh = h.eval(args);
        typename actor_result<I, TupleT>::type ri = i.eval(args);
        typename actor_result<J, TupleT>::type rj = j.eval(args);
        typename actor_result<K, TupleT>::type rk = k.eval(args);
        typename actor_result<L, TupleT>::type rl = l.eval(args);
        typename actor_result<M, TupleT>::type rm = m.eval(args);
        return op(ra, rb, rc, rd, re, rf, rg, rh, ri, rj, rk, rl, rm);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; G g; H h; I i; J j;
    K k; L l; M m; //  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <14 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N>
struct composite14_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type,
        typename actor_result<G, TupleT>::plain_type,
        typename actor_result<H, TupleT>::plain_type,
        typename actor_result<I, TupleT>::plain_type,
        typename actor_result<J, TupleT>::plain_type,
        typename actor_result<K, TupleT>::plain_type,
        typename actor_result<L, TupleT>::plain_type,
        typename actor_result<M, TupleT>::plain_type,
        typename actor_result<N, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N>
struct composite<OperationT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, nil_t, nil_t
> {

    typedef composite<OperationT,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite14_result<
            OperationT, TupleT, A, B, C, D, E, F, G, H, I, J, K, L, M, N
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_, G const& g_, H const& h_, I const& i_, J const& j_,
        K const& k_, L const& l_, M const& m_, N const& n_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_), l(l_), m(m_), n(n_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        typename actor_result<G, TupleT>::type rg = g.eval(args);
        typename actor_result<H, TupleT>::type rh = h.eval(args);
        typename actor_result<I, TupleT>::type ri = i.eval(args);
        typename actor_result<J, TupleT>::type rj = j.eval(args);
        typename actor_result<K, TupleT>::type rk = k.eval(args);
        typename actor_result<L, TupleT>::type rl = l.eval(args);
        typename actor_result<M, TupleT>::type rm = m.eval(args);
        typename actor_result<N, TupleT>::type rn = n.eval(args);
        return op(ra, rb, rc, rd, re, rf, rg, rh, ri, rj, rk, rl, rm, rn);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; G g; H h; I i; J j;
    K k; L l; M m; N n; //  actors
};

///////////////////////////////////////////////////////////////////////////////
//
//  composite <15 actors> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N, typename O>
struct composite15_result {

    typedef typename OperationT::template result<
        typename actor_result<A, TupleT>::plain_type,
        typename actor_result<B, TupleT>::plain_type,
        typename actor_result<C, TupleT>::plain_type,
        typename actor_result<D, TupleT>::plain_type,
        typename actor_result<E, TupleT>::plain_type,
        typename actor_result<F, TupleT>::plain_type,
        typename actor_result<G, TupleT>::plain_type,
        typename actor_result<H, TupleT>::plain_type,
        typename actor_result<I, TupleT>::plain_type,
        typename actor_result<J, TupleT>::plain_type,
        typename actor_result<K, TupleT>::plain_type,
        typename actor_result<L, TupleT>::plain_type,
        typename actor_result<M, TupleT>::plain_type,
        typename actor_result<N, TupleT>::plain_type,
        typename actor_result<O, TupleT>::plain_type
    >::type type;
};

//////////////////////////////////
template <typename OperationT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N, typename O>
struct composite<OperationT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, nil_t
> {

    typedef composite<OperationT,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> self_t;

    template <typename TupleT>
    struct result {

        typedef typename composite15_result<
            OperationT, TupleT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O
        >::type type;
    };

    composite(OperationT const& op_,
        A const& a_, B const& b_, C const& c_, D const& d_, E const& e_,
        F const& f_, G const& g_, H const& h_, I const& i_, J const& j_,
        K const& k_, L const& l_, M const& m_, N const& n_, O const& o_)
    :   op(op_), a(a_), b(b_), c(c_), d(d_), e(e_),
        f(f_), g(g_), h(h_), i(i_), j(j_),
        k(k_), l(l_), m(m_), n(n_), o(o_) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        typename actor_result<A, TupleT>::type ra = a.eval(args);
        typename actor_result<B, TupleT>::type rb = b.eval(args);
        typename actor_result<C, TupleT>::type rc = c.eval(args);
        typename actor_result<D, TupleT>::type rd = d.eval(args);
        typename actor_result<E, TupleT>::type re = e.eval(args);
        typename actor_result<F, TupleT>::type rf = f.eval(args);
        typename actor_result<G, TupleT>::type rg = g.eval(args);
        typename actor_result<H, TupleT>::type rh = h.eval(args);
        typename actor_result<I, TupleT>::type ri = i.eval(args);
        typename actor_result<J, TupleT>::type rj = j.eval(args);
        typename actor_result<K, TupleT>::type rk = k.eval(args);
        typename actor_result<L, TupleT>::type rl = l.eval(args);
        typename actor_result<M, TupleT>::type rm = m.eval(args);
        typename actor_result<N, TupleT>::type rn = n.eval(args);
        typename actor_result<O, TupleT>::type ro = o.eval(args);
        return op(ra, rb, rc, rd, re, rf, rg, rh, ri, rj, rk, rl, rm, rn, ro);
    }

    mutable OperationT op; //  operation
    A a; B b; C c; D d; E e; F f; G g; H h; I i; J j;
    K k; L l; M m; N n; O o; //  actors
};

#endif
#endif
#endif
#endif

namespace impl {

    ///////////////////////////////////////////////////////////////////////////
    //
    //      make_composite is basically a type computer that answers the
    //      question "Given types T0..TN, what composite type should I
    //      create <composite_type> and if I were to generate an actual
    //      composite, what type <type> should I return?"
    //
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename OperationT
        ,   typename A = nil_t
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
    >
    struct make_composite {

        typedef composite<OperationT
            ,   typename as_actor<A>::type
            ,   typename as_actor<B>::type
            ,   typename as_actor<C>::type

#if PHOENIX_LIMIT > 3
            ,   typename as_actor<D>::type
            ,   typename as_actor<E>::type
            ,   typename as_actor<F>::type

#if PHOENIX_LIMIT > 6
            ,   typename as_actor<G>::type
            ,   typename as_actor<H>::type
            ,   typename as_actor<I>::type

#if PHOENIX_LIMIT > 9
            ,   typename as_actor<J>::type
            ,   typename as_actor<K>::type
            ,   typename as_actor<L>::type

#if PHOENIX_LIMIT > 12
            ,   typename as_actor<M>::type
            ,   typename as_actor<N>::type
            ,   typename as_actor<O>::type

#endif
#endif
#endif
#endif
        > composite_type;

        typedef actor<composite_type> type;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //      make_unary, make_binary, make_binary1, make_binary2 and
    //      make_binary3 utilities are provided here for easy creation of
    //      unary and binary composites.
    //
    ///////////////////////////////////////////////////////////////////////////

    //////////////////////////////////  input is an actor
    template <typename OperationT, typename BaseT>
    struct make_unary {

        typedef typename make_composite
            <OperationT, actor<BaseT> >::type type;

        static type
        construct(actor<BaseT> const& _0)
        {
            typedef typename make_composite
                    <OperationT, actor<BaseT> >::composite_type
                ret_t;

            return ret_t(OperationT(), _0);
        }
    };

    //////////////////////////////////  LHS is an actor, RHS is unknown
    template <typename OperationT, typename BaseT, typename B>
    struct make_binary1 {

        typedef typename make_composite
            <OperationT, actor<BaseT>, B>::type type;

        static type
        construct(actor<BaseT> const& _0_, B const& _1_)
        {
            typedef typename make_composite
                    <OperationT, actor<BaseT>, B>::composite_type
                ret_t;
            
            return ret_t(OperationT(), _0_, as_actor<B>::convert(_1_));
        }
    };

    //////////////////////////////////  LHS is unknown, RHS is an actor
    template <typename OperationT, typename A, typename BaseT>
    struct make_binary2 {

        typedef typename make_composite
            <OperationT, A, actor<BaseT> >::type type;

        static type
        construct(A const& _0_, actor<BaseT> const& _1_)
        {
            typedef typename make_composite
                    <OperationT, A, actor<BaseT> >::composite_type
                ret_t;

            return ret_t(OperationT(), as_actor<A>::convert(_0_), _1_);
        }
    };

    //////////////////////////////////  Both LHS and RHS are actors
    template <typename OperationT, typename BaseA, typename BaseB>
    struct make_binary3 {

        typedef typename make_composite
            <OperationT, actor<BaseA>, actor<BaseB> >::type type;

        static type
        construct(actor<BaseA> const& _0_, actor<BaseB> const& _1_)
        {
            typedef typename make_composite
                    <OperationT, actor<BaseA>, actor<BaseB> >::composite_type
                ret_t;

            return ret_t(OperationT(), _0_, _1_);
        }
    };

}   // namespace impl

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

}   //  namespace phoenix

#endif
