/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2002 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_PRIMITIVES_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_PRIMITIVES_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/phoenix/actor.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  argument class
//
//      Lazy arguments
//
//      An actor base class that extracts and returns the Nth argument
//      from the argument list passed in the 'args' tuple in the eval
//      member function (see actor.hpp). There are some predefined
//      argument constants that can be used as actors (arg1..argN).
//
//      The argument actor is a place-holder for the actual arguments
//      passed by the client. For example, wherever arg1 is seen placed
//      in a lazy function (see functions.hpp) or lazy operator (see
//      operators.hpp), this will be replaced by the actual first
//      argument in the actual function evaluation. Argument actors are
//      essentially lazy arguments. A lazy argument is a full actor in
//      its own right and can be evaluated through the actor's operator().
//
//      Example:
//
//          char        c = 'A';
//          int         i = 123;
//          const char* s = "Hello World";
//
//          cout << arg1(c) << ' ';
//          cout << arg1(i, s) << ' ';
//          cout << arg2(i, s) << ' ';
//
//       will print out "A 123 Hello World"
//
///////////////////////////////////////////////////////////////////////////////
template <int N>
struct argument {

    template <typename TupleT>
    struct result { typedef typename tuple_element<N, TupleT>::type type; };

    template <typename TupleT>
    typename tuple_element<N, TupleT>::type
    eval(TupleT const& args) const
    {
        tuple_index<N> const idx;
        return args[idx];
    }
};

//////////////////////////////////
actor<argument<0> > const arg1 = argument<0>();
actor<argument<1> > const arg2 = argument<1>();
actor<argument<2> > const arg3 = argument<2>();

#if PHOENIX_LIMIT > 3
actor<argument<3> > const arg4 = argument<3>();
actor<argument<4> > const arg5 = argument<4>();
actor<argument<5> > const arg6 = argument<5>();

#if PHOENIX_LIMIT > 6
actor<argument<6> > const arg7 = argument<6>();
actor<argument<7> > const arg8 = argument<7>();
actor<argument<8> > const arg9 = argument<8>();

#if PHOENIX_LIMIT > 9
actor<argument<9> > const arg10 = argument<9>();
actor<argument<10> > const arg11 = argument<10>();
actor<argument<11> > const arg12 = argument<11>();

#if PHOENIX_LIMIT > 12
actor<argument<12> > const arg13 = argument<12>();
actor<argument<13> > const arg14 = argument<13>();
actor<argument<14> > const arg15 = argument<14>();

#endif
#endif
#endif
#endif
///////////////////////////////////////////////////////////////////////////////
//
//  value class
//
//      Lazy values
//
//      A bound actual parameter is kept in a value class for deferred
//      access later when needed. A value object is immutable. Value
//      objects are typically created through the val(x) free function
//      which returns a value<T> with T deduced from the type of x. x is
//      held in the value<T> object by value.
//
//      Lazy values are actors. As such, lazy values can be evaluated
//      through the actor's operator(). Such invocation gives the value's
//      identity. Example:
//
//          cout << val(3)() << val("Hello World")();
//
//      prints out "3 Hello World"
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct value {

    typedef typename boost::remove_reference<T>::type plain_t;

    template <typename TupleT>
    struct result { typedef plain_t const type; };

    value(plain_t val_)
    :   val(val_) {}

    template <typename TupleT>
    plain_t const
    eval(TupleT const& /*args*/) const
    {
        return val;
    }

    plain_t val;
};

//////////////////////////////////
template <typename T>
inline actor<value<T> > const
val(T v)
{
    return value<T>(v);
}

//////////////////////////////////
template <typename BaseT>
void
val(actor<BaseT> const& v);     //  This is undefined and not allowed.

///////////////////////////////////////////////////////////////////////////
//
//  Arbitrary types T are typically converted to a actor<value<T> >
//  (see as_actor<T> in actor.hpp). A specialization is also provided
//  for arrays. T[N] arrays are converted to actor<value<T const*> >.
//
///////////////////////////////////////////////////////////////////////////
template <typename T>
struct as_actor {

    typedef actor<value<T> > type;
    static type convert(T const& x)
    { return value<T>(x); }
};

//////////////////////////////////
template <typename T, int N>
struct as_actor<T[N]> {

    typedef actor<value<T const*> > type;
    static type convert(T const x[N])
    { return value<T const*>(x); }
};

///////////////////////////////////////////////////////////////////////////////
//
//  variable class
//
//      Lazy variables
//
//      A bound actual parameter may also be held by non-const reference
//      in a variable class for deferred access later when needed. A
//      variable object is mutable, i.e. its referenced variable can be
//      modified. Variable objects are typically created through the
//      var(x) free function which returns a variable<T> with T deduced
//      from the type of x. x is held in the value<T> object by
//      reference.
//
//      Lazy variables are actors. As such, lazy variables can be
//      evaluated through the actor's operator(). Such invocation gives
//      the variables's identity. Example:
//
//          int i = 3;
//          char const* s = "Hello World";
//          cout << var(i)() << var(s)();
//
//      prints out "3 Hello World"
//
//      Another free function const_(x) may also be used. const_(x) creates
//      a variable<T const&> object using a constant reference.
//
///////////////////////////////////////////////////////////////////////////////
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

template <typename T>
struct variable {

    template <typename TupleT>
    struct result { typedef T& type; };

    variable(T& var_)
    :   var(var_) {}

    template <typename TupleT>
    T&
    eval(TupleT const& /*args*/) const
    {
        return var;
    }

    T& var;
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

//////////////////////////////////
template <typename T>
inline actor<variable<T> > const
var(T& v)
{
    return variable<T>(v);
}

//////////////////////////////////
template <typename T>
inline actor<variable<T const> > const
const_(T const& v)
{
    return variable<T const>(v);
}

//////////////////////////////////
template <typename BaseT>
void
var(actor<BaseT> const& v);     //  This is undefined and not allowed.

//////////////////////////////////
template <typename BaseT>
void
const_(actor<BaseT> const& v);  //  This is undefined and not allowed.

///////////////////////////////////////////////////////////////////////////////
}   //  namespace phoenix

#endif
