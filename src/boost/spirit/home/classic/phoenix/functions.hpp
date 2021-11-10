/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2002 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_FUNCTIONS_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_FUNCTIONS_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/phoenix/actor.hpp>
#include <boost/spirit/home/classic/phoenix/composite.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  function class
//
//      Lazy functions
//
//      This class provides a mechanism for lazily evaluating functions.
//      Syntactically, a lazy function looks like an ordinary C/C++
//      function. The function call looks the same. However, unlike
//      ordinary functions, the actual function execution is deferred.
//      (see actor.hpp, primitives.hpp and composite.hpp for an
//      overview). For example here are sample factorial function calls:
//
//          factorial(4)
//          factorial(arg1)
//          factorial(arg1 * 6)
//
//      These functions are automatically lazily bound unlike ordinary
//      function pointers or functor objects that need to be explicitly
//      bound through the bind function (see binders.hpp).
//
//      A lazy function works in conjunction with a user defined functor
//      (as usual with a member operator()). Only special forms of
//      functor objects are allowed. This is required to enable true
//      polymorphism (STL style monomorphic functors and function
//      pointers can still be used through the bind facility in
//      binders.hpp).
//
//      This special functor is expected to have a nested template class
//      result<A...TN> (where N is the number of arguments of its
//      member operator()). The nested template class result should have
//      a typedef 'type' that reflects the return type of its member
//      operator(). This is essentially a type computer that answers the
//      metaprogramming question "Given arguments of type A...TN, what
//      will be the operator()'s return type?".
//
//      There is a special case for functors that accept no arguments.
//      Such nullary functors are only required to define a typedef
//      result_type that reflects the return type of its operator().
//
//      Here's an example of a simple functor that computes the
//      factorial of a number:
//
//          struct factorial_impl {
//
//              template <typename Arg>
//              struct result { typedef Arg type; };
//
//              template <typename Arg>
//              Arg operator()(Arg n) const
//              { return (n <= 0) ? 1 : n * this->operator()(n-1); }
//          };
//
//      As can be seen, the functor can be polymorphic. Its arguments
//      and return type are not fixed to a particular type. The example
//      above for example, can handle any type as long as it can carry
//      out the required operations (i.e. <=, * and -).
//
//      We can now declare and instantiate a lazy 'factorial' function:
//
//          function<factorial_impl> factorial;
//
//      Invoking a lazy function 'factorial' does not immediately
//      execute the functor factorial_impl. Instead, a composite (see
//      composite.hpp) object is created and returned to the caller.
//      Example:
//
//          factorial(arg1)
//
//      does nothing more than return a composite. A second function
//      call will invoke the actual factorial function. Example:
//
//          int i = 4;
//          cout << factorial(arg1)(i);
//
//      will print out "24".
//
//      Take note that in certain cases (e.g. for functors with state),
//      an instance may be passed on to the constructor. Example:
//
//          function<factorial_impl> factorial(ftor);
//
//      where ftor is an instance of factorial_impl (this is not
//      necessary in this case since factorial is a simple stateless
//      functor). Take care though when using functors with state
//      because the functors are taken in by value. It is best to keep
//      the data manipulated by a functor outside the functor itself and
//      keep a reference to this data inside the functor. Also, it is
//      best to keep functors as small as possible.
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT>
struct function {

    function() : op() {}
    function(OperationT const& op_) : op(op_) {}

    actor<composite<OperationT> >
    operator()() const;

    template <typename A>
    typename impl::make_composite<OperationT, A>::type
    operator()(A const& a) const;

    template <typename A, typename B>
    typename impl::make_composite<OperationT, A, B>::type
    operator()(A const& a, B const& b) const;

    template <typename A, typename B, typename C>
    typename impl::make_composite<OperationT, A, B, C>::type
    operator()(A const& a, B const& b, C const& c) const;

#if PHOENIX_LIMIT > 3

    template <typename A, typename B, typename C, typename D>
    typename impl::make_composite<OperationT, A, B, C, D>::type
    operator()(A const& a, B const& b, C const& c, D const& d) const;

    template <typename A, typename B, typename C, typename D, typename E>
    typename impl::make_composite<
        OperationT, A, B, C, D, E
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e
    ) const;

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f
    ) const;

#if PHOENIX_LIMIT > 6

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F, G
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g
    ) const;

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F, G, H
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h
    ) const;

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F, G, H, I
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i
    ) const;

#if PHOENIX_LIMIT > 9

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F, G, H, I, J
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j
    ) const;

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F, G, H, I, J, K
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k
    ) const;

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F, G, H, I, J, K, L
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l
    ) const;

#if PHOENIX_LIMIT > 12

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F, G, H, I, J, K, L, M
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m
    ) const;

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F, G, H, I, J, K, L, M, N
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n
    ) const;

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N, typename O
    >
    typename impl::make_composite<
        OperationT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O
    >::type
    operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n, O const& o
    ) const;

#endif
#endif
#endif
#endif

    OperationT op;
};

///////////////////////////////////////////////////////////////////////////////
//
//  function class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename OperationT>
inline actor<composite<OperationT> >
function<OperationT>::operator()() const
{
    return actor<composite<OperationT> >(op);
}

//////////////////////////////////
template <typename OperationT>
template <typename A>
inline typename impl::make_composite<OperationT, A>::type
function<OperationT>::operator()(A const& a) const
{
    typedef typename impl::make_composite<OperationT, A>::composite_type ret_t;
    return ret_t
    (
        op,
        as_actor<A>::convert(a)
    );
}

//////////////////////////////////
template <typename OperationT>
template <typename A, typename B>
inline typename impl::make_composite<OperationT, A, B>::type
function<OperationT>::operator()(A const& a, B const& b) const
{
    typedef 
        typename impl::make_composite<OperationT, A, B>::composite_type 
        ret_t;
        
    return ret_t(
        op,
        as_actor<A>::convert(a),
        as_actor<B>::convert(b)
    );
}

//////////////////////////////////
template <typename OperationT>
template <typename A, typename B, typename C>
inline typename impl::make_composite<OperationT, A, B, C>::type
function<OperationT>::operator()(A const& a, B const& b, C const& c) const
{
    typedef 
        typename impl::make_composite<OperationT, A, B, C>::composite_type
        ret_t;
        
    return ret_t(
        op,
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c)
    );
}

#if PHOENIX_LIMIT > 3
//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D
>
inline typename impl::make_composite<
    OperationT, A, B, C, D
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D
        >::composite_type ret_t;
        
    return ret_t(
        op,
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d)
    );
}

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E
        >::composite_type ret_t;

    return ret_t(
        op,
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e)
    );
}

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F
        >::composite_type ret_t;

    return ret_t(
        op,
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f)
    );
}

#if PHOENIX_LIMIT > 6

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F, G
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F, G
        >::composite_type ret_t;

    return ret_t(
        op,
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g)
    );
}

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F, G, H
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F, G, H
        >::composite_type ret_t;
        
    return ret_t(
        op,
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h)
    );
}

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F, G, H, I
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F, G, H, I
        >::composite_type ret_t;
        
    return ret_t(
        op,
        as_actor<A>::convert(a),
        as_actor<B>::convert(b),
        as_actor<C>::convert(c),
        as_actor<D>::convert(d),
        as_actor<E>::convert(e),
        as_actor<F>::convert(f),
        as_actor<G>::convert(g),
        as_actor<H>::convert(h),
        as_actor<I>::convert(i)
    );
}

#if PHOENIX_LIMIT > 9

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F, G, H, I, J
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F, G, H, I, J
        >::composite_type ret_t;
        
    return ret_t(
        op,
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
    );
}

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F, G, H, I, J, K
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F, G, H, I, J, K
        >::composite_type ret_t;
        
    return ret_t(
        op,
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
    );
}

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F, G, H, I, J, K, L
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F, G, H, I, J, K, L
        >::composite_type ret_t;
        
    return ret_t(
        op,
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
    );
}

#if PHOENIX_LIMIT > 12

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F, G, H, I, J, K, L, M
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l, M const& m
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F, G, H, I, J, K, L, M
        >::composite_type ret_t;
        
    return ret_t(
        op,
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
    );
}

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F, G, H, I, J, K, L, M, N
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l, M const& m, N const& n
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F, G, H, I, J, K, L, M, N
        >::composite_type ret_t;

    return ret_t(
        op,
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
    );
}

//////////////////////////////////
template <typename OperationT>
template <
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N, typename O
>
inline typename impl::make_composite<
    OperationT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O
>::type
function<OperationT>::operator()(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l, M const& m, N const& n, O const& o
) const
{
    typedef typename impl::make_composite<
            OperationT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O
        >::composite_type ret_t;
        
    return ret_t(
        op,
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
    );
}

#endif
#endif
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
}   //  namespace phoenix

#endif
