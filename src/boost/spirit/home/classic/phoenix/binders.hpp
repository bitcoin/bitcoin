/*=============================================================================
    Phoenix v1.2
    Copyright (c) 2001-2002 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_BINDERS_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_BINDERS_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/phoenix/functions.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/mpl/if.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  Binders
//
//      There are times when it is desirable to bind a simple functor,
//      function, member function or member variable for deferred
//      evaluation. This can be done through the binding facilities
//      provided below. There are template classes:
//
//          1) function_ptr           ( function pointer binder )
//          2) functor                ( functor pointer binder )
//          3) member_function_ptr    ( member function pointer binder )
//          4) member_var_ptr         ( member variable pointer binder )
//
//      These template classes are specialized lazy function classes for
//      functors, function pointers, member function pointers and member
//      variable pointers, respectively. These are subclasses of the
//      lazy-function class (see functions.hpp). Each of these has a
//      corresponding overloaded bind(x) function. Each bind(x) function
//      generates a suitable binder object.
//
//      Example, given a function foo:
//
//          void foo_(int n) { std::cout << n << std::endl; }
//
//      Here's how the function foo is bound:
//
//          bind(&foo_)
//
//      This bind expression results to a lazy-function (see
//      functions.hpp) that is lazily evaluated. This bind expression is
//      also equivalent to:
//
//          function_ptr<void, int> foo = &foo_;
//
//      The template parameter of the function_ptr is the return and
//      argument types of actual signature of the function to be bound
//      read from left to right:
//
//          void foo_(int); ---> function_ptr<void, int>
//
//      Either bind(&foo_) and its equivalent foo can now be used in the
//      same way a lazy function (see functions.hpp) is used:
//
//          bind(&foo_)(arg1)
//
//      or
//
//          foo(arg1)
//
//      The latter, of course, being much easier to understand. This is
//      now a full-fledged lazy function that can finally be evaluated
//      by another function call invocation. A second function call will
//      invoke the actual foo function:
//
//          int i = 4;
//          foo(arg1)(i);
//
//      will print out "4".
//
//      Binding functors and member functions can be done similarly.
//      Here's how to bind a functor (e.g. std::plus<int>):
//
//          bind(std::plus<int>())
//
//      or
//
//          functor<std::plus<int> > plus;
//
//      Again, these are full-fledged lazy functions. In this case,
//      unlike the first example, expect 2 arguments (std::plus<int>
//      needs two arguments lhs and rhs). Either or both of which can be
//      lazily bound:
//
//          plus(arg1, arg2)     // arg1 + arg2
//          plus(100, arg1)      // 100 + arg1
//          plus(100, 200)       // 300
//
//      A bound member function takes in a pointer or reference to an
//      object as the first argument. For instance, given:
//
//          struct xyz { void foo(int) const; };
//
//      xyz's foo member function can be bound as:
//
//          bind(&xyz::foo)
//
//      or
//
//          member_function_ptr<void, xyz, int> xyz_foo = &xyz::foo;
//
//      The template parameter of the member_function_ptr is the return,
//      class and argument types of actual signature of the function to
//      be bound read from left to right:
//
//          void xyz::foo_(int); ---> member_function_ptr<void, xyz, int>
//
//      Take note that a member_function_ptr lazy-function expects the
//      first argument to be a pointer or reference to an object. Both
//      the object (reference or pointer) and the arguments can be
//      lazily bound. Examples:
//
//          xyz obj;
//          xyz_foo(arg1, arg2)   // arg1.foo(arg2)
//          xyz_foo(obj, arg1)    // obj.foo(arg1)
//          xyz_foo(obj, 100)     // obj.foo(100)
//
//      Be reminded that var(obj) must be used to call non-const member
//      functions. For example, if xyz was declared as:
//
//          struct xyz { void foo(int); };
//
//      the pointer or reference to the object must also be non-const.
//      Lazily bound arguments are stored as const value by default (see
//      variable class in primitives.hpp).
//
//          xyz_foo(var(obj), 100)    // obj.foo(100)
//
//      Finally, member variables can be bound much like member
//      functions. For instance, given:
//
//          struct xyz { int v; };
//
//      xyz::v can be bound as:
//
//          bind(&xyz::v)
//      or
//
//          member_var_ptr<int, xyz> xyz_v = &xyz::v;
//
//      The template parameter of the member_var_ptr is the type of the
//      variable followed by the class:
//
//          int xyz::v; ---> member_var_ptr<int, xyz>
//
//      Just like the member_function_ptr, member_var_ptr also expects
//      the first argument to be a pointer or reference to an object.
//      Both the object (reference or pointer) and the arguments can be
//      lazily bound. Examples:
//
//          xyz obj;
//          xyz_v(arg1)   // arg1.v
//          xyz_v(obj)    // obj.v
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  Functor binder
//
///////////////////////////////////////////////////////////////////////////////
template <typename FuncT>
struct functor_action : public FuncT {

#if !defined(BOOST_BORLANDC) && (!defined(__MWERKS__) || (__MWERKS__ > 0x3002))

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
    >
    struct result { typedef typename FuncT::result_type type; };
#endif

    functor_action(FuncT fptr_ = FuncT())
    :   FuncT(fptr_) {}
};

#if defined(BOOST_BORLANDC) || (defined(__MWERKS__) && (__MWERKS__ <= 0x3002))

///////////////////////////////////////////////////////////////////////////////
//
//  The following specializations are needed because Borland and CodeWarrior
//  does not accept default template arguments in nested template classes in
//  classes (i.e functor_action::result)
//
///////////////////////////////////////////////////////////////////////////////
template <typename FuncT, typename TupleT>
struct composite0_result<functor_action<FuncT>, TupleT> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A>
struct composite1_result<functor_action<FuncT>, TupleT, A> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B>
struct composite2_result<functor_action<FuncT>, TupleT, A, B> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C>
struct composite3_result<functor_action<FuncT>, TupleT, A, B, C> {

    typedef typename FuncT::result_type type;
};

#if PHOENIX_LIMIT > 3
//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D>
struct composite4_result<functor_action<FuncT>, TupleT,
    A, B, C, D> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E>
struct composite5_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F>
struct composite6_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F> {

    typedef typename FuncT::result_type type;
};

#if PHOENIX_LIMIT > 6
//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G>
struct composite7_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F, G> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H>
struct composite8_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F, G, H> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I>
struct composite9_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F, G, H, I> {

    typedef typename FuncT::result_type type;
};

#if PHOENIX_LIMIT > 9
//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J>
struct composite10_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F, G, H, I, J> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K>
struct composite11_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L>
struct composite12_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L> {

    typedef typename FuncT::result_type type;
};

#if PHOENIX_LIMIT > 12
//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M>
struct composite13_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L, M> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N>
struct composite14_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N> {

    typedef typename FuncT::result_type type;
};

//////////////////////////////////
template <typename FuncT, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N, typename O>
struct composite15_result<functor_action<FuncT>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> {

    typedef typename FuncT::result_type type;
};

#endif
#endif
#endif
#endif
#endif

//////////////////////////////////
template <typename FuncT>
struct functor : public function<functor_action<FuncT> > {

    functor(FuncT func)
    :   function<functor_action<FuncT> >(functor_action<FuncT>(func)) {};
};

//////////////////////////////////
template <typename FuncT>
inline functor<FuncT>
bind(FuncT func)
{
    return functor<FuncT>(func);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member variable pointer binder
//
///////////////////////////////////////////////////////////////////////////////
namespace impl {

    //////////////////////////////////
    template <typename T>
    struct as_ptr {

        typedef T* pointer_type;

        static T* get(T& ref)
        { return &ref; }
    };

    //////////////////////////////////
    template <typename T>
    struct as_ptr<T*> {

        typedef T* pointer_type;

        static T* get(T* ptr)
        { return ptr; }
    };
}

//////////////////////////////////
template <typename ActionT, typename ClassT>
struct member_var_ptr_action_result {

    typedef typename ActionT::template result<ClassT>::type type;
};

//////////////////////////////////
template <typename T, typename ClassT>
struct member_var_ptr_action {

    typedef member_var_ptr_action<T, ClassT> self_t;

    template <typename CT>
    struct result {
        typedef typename boost::mpl::if_<boost::is_const<CT>, T const&, T&
            >::type type;
    };

    typedef T ClassT::*mem_var_ptr_t;

    member_var_ptr_action(mem_var_ptr_t ptr_)
    :   ptr(ptr_) {}

    template <typename CT>
    typename member_var_ptr_action_result<self_t, CT>::type
    operator()(CT& obj) const
    { return impl::as_ptr<CT>::get(obj)->*ptr; }

    mem_var_ptr_t ptr;
};

//////////////////////////////////
template <typename T, typename ClassT>
struct member_var_ptr
:   public function<member_var_ptr_action<T, ClassT> > {

    member_var_ptr(T ClassT::*mp)
    :   function<member_var_ptr_action<T, ClassT> >
        (member_var_ptr_action<T, ClassT>(mp)) {}
};

//////////////////////////////////
template <typename T, typename ClassT>
inline member_var_ptr<T, ClassT>
bind(T ClassT::*mp)
{
    return member_var_ptr<T, ClassT>(mp);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (main class)
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename RT
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
struct function_ptr_action;

//////////////////////////////////
template <
    typename RT
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
struct function_ptr
:   public function<function_ptr_action<RT
    , A, B, C
#if PHOENIX_LIMIT > 3
    , D, E, F
#if PHOENIX_LIMIT > 6
    , G, H, I
#if PHOENIX_LIMIT > 9
    , J, K, L
#if PHOENIX_LIMIT > 12
    , M, N, O
#endif
#endif
#endif
#endif
    > > {

    typedef function_ptr_action<RT
        , A, B, C
#if PHOENIX_LIMIT > 3
        , D, E, F
#if PHOENIX_LIMIT > 6
        , G, H, I
#if PHOENIX_LIMIT > 9
        , J, K, L
#if PHOENIX_LIMIT > 12
        , M, N, O
#endif
#endif
#endif
#endif
    > action_t;

    template <typename FPT>
    function_ptr(FPT fp)
    :   function<action_t>(action_t(fp)) {}
};

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 0 arg)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT>
struct function_ptr_action<RT,
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

    typedef RT result_type;
    typedef RT(*func_ptr_t)();

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()() const
    { return fptr(); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT>
inline function_ptr<RT>
bind(RT(*fptr)())
{
    return function_ptr<RT>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 1 arg)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename A>
struct function_ptr_action<RT,
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

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A);

    template <typename A_>
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(A a) const
    { return fptr(a); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename A>
inline function_ptr<RT, A>
bind(RT(*fptr)(A))
{
    return function_ptr<RT, A>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 2 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename A, typename B>
struct function_ptr_action<RT,
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

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B);

    template <typename A_, typename B_>
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(A a, B b) const
    { return fptr(a, b); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename A, typename B>
inline function_ptr<RT, A, B>
bind(RT(*fptr)(A, B))
{
    return function_ptr<RT, A, B>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 3 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename A, typename B, typename C>
struct function_ptr_action<RT,
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

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C);

    template <typename A_, typename B_, typename C_>
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(A a, B b, C c) const
    { return fptr(a, b, c); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename A, typename B, typename C>
inline function_ptr<RT, A, B, C>
bind(RT(*fptr)(A, B, C))
{
    return function_ptr<RT, A, B, C>(fptr);
}

#if PHOENIX_LIMIT > 3
///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 4 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename A, typename B, typename C, typename D>
struct function_ptr_action<RT,
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

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D);

    template <typename A_, typename B_, typename C_, typename D_>
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(A a, B b, C c, D d) const
    { return fptr(a, b, c, d); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename A, typename B, typename C, typename D>
inline function_ptr<RT, A, B, C, D>
bind(RT(*fptr)(A, B, C, D))
{
    return function_ptr<RT, A, B, C, D>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 5 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E
>
struct function_ptr_action<RT,
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

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e
    ) const
    { return fptr(a, b, c, d, e); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E
>
inline function_ptr<RT, A, B, C, D, E>
bind(RT(*fptr)(A, B, C, D, E))
{
    return function_ptr<RT, A, B, C, D, E>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 6 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F
>
struct function_ptr_action<RT,
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

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f
    ) const
    { return fptr(a, b, c, d, e, f); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F
>
inline function_ptr<RT, A, B, C, D, E, F>
bind(RT(*fptr)(A, B, C, D, E, F))
{
    return function_ptr<RT, A, B, C, D, E, F>(fptr);
}

#if PHOENIX_LIMIT > 6
///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 7 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G
>
struct function_ptr_action<RT,
    A, B, C, D, E, F, G, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F, G);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_, typename G_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f, G g
    ) const
    { return fptr(a, b, c, d, e, f, g); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G
>
inline function_ptr<RT, A, B, C, D, E, F, G>
bind(RT(*fptr)(A, B, C, D, E, F, G))
{
    return function_ptr<RT, A, B, C, D, E, F, G>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 8 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H
>
struct function_ptr_action<RT,
    A, B, C, D, E, F, G, H, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F, G, H);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_, typename G_, typename H_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f, G g, H h
    ) const
    { return fptr(a, b, c, d, e, f, g, h); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H
>
inline function_ptr<RT, A, B, C, D, E, F, G, H>
bind(RT(*fptr)(A, B, C, D, E, F, G, H))
{
    return function_ptr<RT, A, B, C, D, E, F, G, H>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 9 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I
>
struct function_ptr_action<RT,
    A, B, C, D, E, F, G, H, I,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F, G, H, I);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_, typename G_, typename H_, typename I_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f, G g, H h, I i
    ) const
    { return fptr(a, b, c, d, e, f, g, h, i); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I
>
inline function_ptr<RT, A, B, C, D, E, F, G, H, I>
bind(RT(*fptr)(A, B, C, D, E, F, G, H, I))
{
    return function_ptr<RT, A, B, C, D, E, F, G, H, I>(fptr);
}

#if PHOENIX_LIMIT > 9
///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 10 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J
>
struct function_ptr_action<RT,
    A, B, C, D, E, F, G, H, I, J, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F, G, H, I, J);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_, typename G_, typename H_, typename I_, typename J_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f, G g, H h, I i, J j
    ) const
    { return fptr(a, b, c, d, e, f, g, h, i, j); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J
>
inline function_ptr<RT, A, B, C, D, E, F, G, H, I, J>
bind(RT(*fptr)(A, B, C, D, E, F, G, H, I, J))
{
    return function_ptr<RT, A, B, C, D, E, F, G, H, I, J>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 11 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K
>
struct function_ptr_action<RT,
    A, B, C, D, E, F, G, H, I, J, K, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F, G, H, I, J, K);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_, typename G_, typename H_, typename I_, typename J_,
        typename K_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f, G g, H h, I i, J j,
        K k
    ) const
    { return fptr(a, b, c, d, e, f, g, h, i, j, k); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K
>
inline function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K>
bind(RT(*fptr)(A, B, C, D, E, F, G, H, I, J, K))
{
    return function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 12 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L
>
struct function_ptr_action<RT,
    A, B, C, D, E, F, G, H, I, J, K, L,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F, G, H, I, J, K, L);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_, typename G_, typename H_, typename I_, typename J_,
        typename K_, typename L_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f, G g, H h, I i, J j,
        K k, L l
    ) const
    { return fptr(a, b, c, d, e, f, g, h, i, j, k, l); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L
>
inline function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K, L>
bind(RT(*fptr)(A, B, C, D, E, F, G, H, I, J, K, L))
{
    return function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K, L>(fptr);
}

#if PHOENIX_LIMIT > 12
///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 13 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M
>
struct function_ptr_action<RT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, nil_t, nil_t, nil_t> {

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F, G, H, I, J, K, L, M);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_, typename G_, typename H_, typename I_, typename J_,
        typename K_, typename L_, typename M_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f, G g, H h, I i, J j,
        K k, L l, M m
    ) const
    { return fptr(a, b, c, d, e, f, g, h, i, j, k, l, m); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M
>
inline function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K, L, M>
bind(RT(*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M))
{
    return function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K, L, M>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 14 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N
>
struct function_ptr_action<RT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, nil_t, nil_t> {

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F, G, H, I, J, K, L, M, N);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_, typename G_, typename H_, typename I_, typename J_,
        typename K_, typename L_, typename M_, typename N_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f, G g, H h, I i, J j,
        K k, L l, M m, N n
    ) const
    { return fptr(a, b, c, d, e, f, g, h, i, j, k, l, m, n); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N
>
inline function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K, L, M, N>
bind(RT(*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N))
{
    return function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Function pointer binder (specialization for 15 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N, typename O
>
struct function_ptr_action<RT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, nil_t> {

    typedef RT result_type;
    typedef RT(*func_ptr_t)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O);

    template <
        typename A_, typename B_, typename C_, typename D_, typename E_,
        typename F_, typename G_, typename H_, typename I_, typename J_,
        typename K_, typename L_, typename M_, typename N_, typename O_
    >
    struct result { typedef result_type type; };

    function_ptr_action(func_ptr_t fptr_)
    :   fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e,
        F f, G g, H h, I i, J j,
        K k, L l, M m, N n, O o
    ) const
    { return fptr(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o); }

    func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N, typename O
>
inline function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>
bind(RT(*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O))
{
    return function_ptr<RT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(fptr);
}

#endif
#endif
#endif
#endif
///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (main class)
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename RT,
    typename ClassT
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
struct member_function_ptr_action;

//////////////////////////////////
template <
    typename RT,
    typename ClassT
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
struct member_function_ptr
:   public function<member_function_ptr_action<RT, ClassT
    , A, B, C
#if PHOENIX_LIMIT > 3
    , D, E, F
#if PHOENIX_LIMIT > 6
    , G, H, I
#if PHOENIX_LIMIT > 9
    , J, K, L
#if PHOENIX_LIMIT > 12
    , M, N, O
#endif
#endif
#endif
#endif
    > > {

    typedef member_function_ptr_action<RT, ClassT
        , A, B, C
#if PHOENIX_LIMIT > 3
        , D, E, F
#if PHOENIX_LIMIT > 6
        , G, H, I
#if PHOENIX_LIMIT > 9
        , J, K, L
#if PHOENIX_LIMIT > 12
        , M, N, O
#endif
#endif
#endif
#endif
    > action_t;

    template <typename FPT>
    member_function_ptr(FPT fp)
    :   function<action_t>(action_t(fp)) {}
};

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 0 arg)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT>
struct member_function_ptr_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)();
    typedef RT(ClassT::*cmf)() const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT>
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT>
inline member_function_ptr<RT, ClassT>
bind(RT(ClassT::*fptr)())
{
    return member_function_ptr<RT, ClassT>(fptr);
}

template <typename RT, typename ClassT>
inline member_function_ptr<RT, ClassT const>
bind(RT(ClassT::*fptr)() const)
{
    return member_function_ptr<RT, ClassT const>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 1 arg)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT, typename A>
struct member_function_ptr_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A);
    typedef RT(ClassT::*cmf)(A) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT, typename A_>
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj, A a) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(a); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT, typename A>
inline member_function_ptr<RT, ClassT, A>
bind(RT(ClassT::*fptr)(A))
{
    return member_function_ptr<RT, ClassT, A>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT, typename A>
inline member_function_ptr<RT, ClassT const, A>
bind(RT(ClassT::*fptr)(A) const)
{
    return member_function_ptr<RT, ClassT const, A>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 2 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B>
struct member_function_ptr_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B);
    typedef RT(ClassT::*cmf)(A, B) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT, typename A_, typename B_>
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj, A a, B b) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(a, b); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B>
inline member_function_ptr<RT, ClassT, A, B>
bind(RT(ClassT::*fptr)(A, B))
{
    return member_function_ptr<RT, ClassT, A, B>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B>
inline member_function_ptr<RT, ClassT const, A, B>
bind(RT(ClassT::*fptr)(A, B) const)
{
    return member_function_ptr<RT, ClassT const, A, B>(fptr);
}

#if PHOENIX_LIMIT > 3
///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 3 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B, typename C>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, nil_t, nil_t, nil_t,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C);
    typedef RT(ClassT::*cmf)(A, B, C) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT, typename A_, typename B_, typename C_>
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj, A a, B b, C c) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(a, b, c); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B, typename C>
inline member_function_ptr<RT, ClassT, A, B, C>
bind(RT(ClassT::*fptr)(A, B, C))
{
    return member_function_ptr<RT, ClassT, A, B, C>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B, typename C>
inline member_function_ptr<RT, ClassT const, A, B, C>
bind(RT(ClassT::*fptr)(A, B, C) const)
{
    return member_function_ptr<RT, ClassT const, A, B, C>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 4 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D
>
struct member_function_ptr_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D);
    typedef RT(ClassT::*cmf)(A, B, C, D) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d
    ) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(a, b, c, d); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D
>
inline member_function_ptr<RT, ClassT, A, B, C, D>
bind(RT(ClassT::*fptr)(A, B, C, D))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D
>
inline member_function_ptr<RT, ClassT const, A, B, C, D>
bind(RT(ClassT::*fptr)(A, B, C, D) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 5 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E
>
struct member_function_ptr_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E);
    typedef RT(ClassT::*cmf)(A, B, C, D, E) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e
    ) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(a, b, c, d, e); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E>
bind(RT(ClassT::*fptr)(A, B, C, D, E))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E>
bind(RT(ClassT::*fptr)(A, B, C, D, E) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E>(fptr);
}

#if PHOENIX_LIMIT > 6
///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 6 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f
    ) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(a, b, c, d, e, f); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 7 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, G, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f, G g
    ) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(a, b, c, d, e, f, g); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F, G>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F, G>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F, G>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F, G>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 8 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, G, H, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f, G g, H h
    ) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(a, b, c, d, e, f, g, h); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F, G, H>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F, G, H>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F, G, H>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F, G, H>(fptr);
}

#if PHOENIX_LIMIT > 9
///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 9 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f, G g, H h, I i
    ) const
    { return (impl::as_ptr<CT>::get(obj)->*fptr)(a, b, c, d, e, f, g, h, i); }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F, G, H, I>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F, G, H, I>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F, G, H, I>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F, G, H, I>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 10 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j
    ) const
    {
        return (impl::as_ptr<CT>::get(obj)->*fptr)
            (a, b, c, d, e, f, g, h, i, j);
    }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F, G, H, I, J>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F, G, H, I, J>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 11 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k
    ) const
    {
        return (impl::as_ptr<CT>::get(obj)->*fptr)
            (a, b, c, d, e, f, g, h, i, j, k);
    }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K>(fptr);
}

#if PHOENIX_LIMIT > 12
///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 12 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, L, nil_t, nil_t, nil_t, nil_t> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K, L);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K, L) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_, typename L_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l
    ) const
    {
        return (impl::as_ptr<CT>::get(obj)->*fptr)
            (a, b, c, d, e, f, g, h, i, j, k, l);
    }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 13 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, nil_t, nil_t, nil_t> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K, L, M);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K, L, M) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_, typename L_, typename M_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m
    ) const
    {
        return (impl::as_ptr<CT>::get(obj)->*fptr)
            (a, b, c, d, e, f, g, h, i, j, k, l, m);
    }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 14 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, nil_t, nil_t> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K, L, M, N);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K, L, M, N) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_, typename L_, typename M_, typename N_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n
    ) const
    {
        return (impl::as_ptr<CT>::get(obj)->*fptr)
            (a, b, c, d, e, f, g, h, i, j, k, l, m, n);
    }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Member function pointer binder (specialization for 15 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N,
    typename O
>
struct member_function_ptr_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, nil_t> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT,
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_, typename L_, typename M_, typename N_,
        typename O_
    >
    struct result { typedef result_type type; };

    member_function_ptr_action(mem_func_ptr_t fptr_)
    :   fptr(fptr_) {}

    template <typename CT>
    result_type operator()(CT& obj,
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o
    ) const
    {
        return (impl::as_ptr<CT>::get(obj)->*fptr)
            (a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
    }

    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N,
    typename O
>
inline member_function_ptr<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O))
{
    return member_function_ptr<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N,
    typename O
>
inline member_function_ptr<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>
bind(RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O) const)
{
    return member_function_ptr<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(fptr);
}

#endif
#endif
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (main class)
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename RT,
    typename ClassT
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
struct bound_member_action;

//////////////////////////////////
template <
    typename RT,
    typename ClassT
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
struct bound_member
:   public function<bound_member_action<RT, ClassT
    , A, B, C
#if PHOENIX_LIMIT > 3
    , D, E, F
#if PHOENIX_LIMIT > 6
    , G, H, I
#if PHOENIX_LIMIT > 9
    , J, K, L
#if PHOENIX_LIMIT > 12
    , M, N, O
#endif
#endif
#endif
#endif
    > > {

    typedef bound_member_action<RT, ClassT
        , A, B, C
#if PHOENIX_LIMIT > 3
        , D, E, F
#if PHOENIX_LIMIT > 6
        , G, H, I
#if PHOENIX_LIMIT > 9
        , J, K, L
#if PHOENIX_LIMIT > 12
        , M, N, O
#endif
#endif
#endif
#endif
    > action_t;

    template <typename CT, typename FPT>
    bound_member(CT & c, FPT fp)
    :   function<action_t>(action_t(c,fp)) {}

#if !defined(BOOST_BORLANDC)
    template <typename CT, typename FPT>
    bound_member(CT * c, FPT fp)
    :   function<action_t>(action_t(c,fp)) {}
#endif
};

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 0 arg)
//
///////////////////////////////////////////////////////////////////////////////

template <typename RT, typename ClassT>
struct bound_member_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)();
    typedef RT(ClassT::*cmf)() const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename CT>
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()() const
    { return (obj->*fptr)(); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////

template <typename RT, typename ClassT>
inline bound_member<RT,ClassT>
bind(ClassT & obj, RT(ClassT::*fptr)())
{
    return bound_member<RT,ClassT>(obj, fptr);
}

template <typename RT, typename ClassT>
inline bound_member<RT,ClassT>
bind(ClassT * obj, RT(ClassT::*fptr)())
{
#if defined(__MWERKS__) && (__MWERKS__ < 0x3003)
    return bound_member<RT,ClassT>(*obj, fptr);
#else
    return bound_member<RT,ClassT>(obj, fptr);
#endif
}

template <typename RT, typename ClassT>
inline bound_member<RT,ClassT const>
bind(ClassT const& obj, RT(ClassT::*fptr)())
{
    return bound_member<RT,ClassT const>(obj, fptr);
}

template <typename RT, typename ClassT>
inline bound_member<RT,ClassT const>
bind(ClassT  const* obj, RT(ClassT::*fptr)() const)
{
#if defined(__MWERKS__) && (__MWERKS__ < 0x3003)
    return bound_member<RT,ClassT const>(*obj, fptr);
#else
    return bound_member<RT,ClassT const>(obj, fptr);
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 1 arg)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT, typename A>
struct bound_member_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A);
    typedef RT(ClassT::*cmf)(A) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename A_>
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(A a) const
    { return (obj->*fptr)(a); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT, typename A>
inline bound_member<RT, ClassT, A>
bind(ClassT & obj, RT(ClassT::*fptr)(A))
{
    return bound_member<RT, ClassT, A>(obj,fptr);
}

template <typename RT, typename ClassT, typename A>
inline bound_member<RT, ClassT, A>
bind(ClassT * obj, RT(ClassT::*fptr)(A))
{
    return bound_member<RT, ClassT, A>(obj,fptr);
}

//////////////////////////////////
template <typename RT, typename ClassT, typename A>
inline bound_member<RT, ClassT const, A>
bind(ClassT const& obj, RT(ClassT::*fptr)(A) const)
{
    return bound_member<RT, ClassT const, A>(obj,fptr);
}

template <typename RT, typename ClassT, typename A>
inline bound_member<RT, ClassT const, A>
bind(ClassT const* obj, RT(ClassT::*fptr)(A) const)
{
    return bound_member<RT, ClassT const, A>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 2 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B>
struct bound_member_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B);
    typedef RT(ClassT::*cmf)(A, B) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename A_, typename B_>
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(A a, B b) const
    { return (obj->*fptr)(a, b); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B>
inline bound_member<RT, ClassT, A, B>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B))
{
    return bound_member<RT, ClassT, A, B>(obj,fptr);
}

template <typename RT, typename ClassT, typename A, typename B>
inline bound_member<RT, ClassT, A, B>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B))
{
    return bound_member<RT, ClassT, A, B>(obj,fptr);
}

template <typename RT, typename ClassT, typename A, typename B>
inline bound_member<RT, ClassT const, A, B>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B) const)
{
    return bound_member<RT, ClassT const, A, B>(obj,fptr);
}

template <typename RT, typename ClassT, typename A, typename B>
inline bound_member<RT, ClassT const, A, B>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B) const)
{
    return bound_member<RT, ClassT const, A, B>(obj,fptr);
}

#if PHOENIX_LIMIT > 3
///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 3 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B, typename C>
struct bound_member_action<RT, ClassT,
    A, B, C, nil_t, nil_t, nil_t,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C);
    typedef RT(ClassT::*cmf)(A, B, C) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename A_, typename B_, typename C_>
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(A a, B b, C c) const
    { return (obj->*fptr)(a, b, c); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT, typename A, typename B, typename C>
inline bound_member<RT, ClassT, A, B, C>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C))
{
    return bound_member<RT, ClassT, A, B, C>(obj,fptr);
}

template <typename RT, typename ClassT, typename A, typename B, typename C>
inline bound_member<RT, ClassT, A, B, C>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C))
{
    return bound_member<RT, ClassT, A, B, C>(obj,fptr);
}

template <typename RT, typename ClassT, typename A, typename B, typename C>
inline bound_member<RT, ClassT const, A, B, C>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C) const)
{
    return bound_member<RT, ClassT const, A, B, C>(obj,fptr);
}

template <typename RT, typename ClassT, typename A, typename B, typename C>
inline bound_member<RT, ClassT const, A, B, C>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C) const)
{
    return bound_member<RT, ClassT const, A, B, C>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 4 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D
>
struct bound_member_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D);
    typedef RT(ClassT::*cmf)(A, B, C, D) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename A_, typename B_, typename C_, typename D_>
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(A a, B b, C c, D d) const
    { return (obj->*fptr)(a, b, c, d); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D
>
inline bound_member<RT, ClassT, A, B, C, D>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D))
{
    return bound_member<
        RT, ClassT, A, B, C, D>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D
>
inline bound_member<RT, ClassT, A, B, C, D>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D))
{
    return bound_member<
        RT, ClassT, A, B, C, D>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D
>
inline bound_member<RT, ClassT const, A, B, C, D>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D
>
inline bound_member<RT, ClassT const, A, B, C, D>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 5 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E
>
struct bound_member_action<RT, ClassT,
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

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E);
    typedef RT(ClassT::*cmf)(A, B, C, D, E) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <typename A_, typename B_, typename C_, typename D_,
        typename E_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e
    ) const
    { return (obj->*fptr)(a, b, c, d, e); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E
>
inline bound_member<RT, ClassT, A, B, C, D, E>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E
>
inline bound_member<RT, ClassT, A, B, C, D, E>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E
>
inline bound_member<RT, ClassT const, A, B, C, D, E>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E
>
inline bound_member<RT, ClassT const, A, B, C, D, E>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E>(obj,fptr);
}

#if PHOENIX_LIMIT > 6
///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 6 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f
    ) const
    { return (obj->*fptr)(a, b, c, d, e, f); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F
>
inline bound_member<RT, ClassT, A, B, C, D, E, F>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F
>
inline bound_member<RT, ClassT, A, B, C, D, E, F>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 7 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, G, nil_t, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f, G g
    ) const
    { return (obj->*fptr)(a, b, c, d, e, f, g); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 8 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, G, H, nil_t,
#if PHOENIX_LIMIT > 9
    nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f, G g, H h
    ) const
    { return (obj->*fptr)(a, b, c, d, e, f, g, h); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H>(obj,fptr);
}

#if PHOENIX_LIMIT > 9
///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 9 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, nil_t, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f, G g, H h, I i
    ) const
    { return (obj->*fptr)(a, b, c, d, e, f, g, h, i); }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 10 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, nil_t, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j
    ) const
    {
        return (obj->*fptr)(a, b, c, d, e, f, g, h, i, j);
    }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 11 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, nil_t,
#if PHOENIX_LIMIT > 12
    nil_t, nil_t, nil_t,
#endif
    nil_t   //  Unused
> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k
    ) const
    {
        return (obj->*fptr)(a, b, c, d, e, f, g, h, i, j, k);
    }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K>(obj,fptr);
}

#if PHOENIX_LIMIT > 12
///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 12 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, L, nil_t, nil_t, nil_t, nil_t> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K, L);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K, L) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_, typename L_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l
    ) const
    {
        return (obj->*fptr)(a, b, c, d, e, f, g, h, i, j, k, l);
    }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 13 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, nil_t, nil_t, nil_t> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K, L, M);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K, L, M) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_, typename L_, typename M_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m
    ) const
    {
        return (obj->*fptr)(a, b, c, d, e, f, g, h, i, j, k, l, m);
    }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 14 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, nil_t, nil_t> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K, L, M, N);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K, L, M, N) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_, typename L_, typename M_, typename N_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n
    ) const
    {
        return (obj->*fptr)(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
    }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N>(obj,fptr);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Bound member function binder (specialization for 15 args)
//
///////////////////////////////////////////////////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N,
    typename O
>
struct bound_member_action<RT, ClassT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, nil_t> {

    typedef RT result_type;
    typedef RT(ClassT::*mf)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O);
    typedef RT(ClassT::*cmf)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O) const;
    typedef typename boost::mpl::if_<boost::is_const<ClassT>, cmf, mf>::type
        mem_func_ptr_t;

    template <
        typename A_, typename B_, typename C_, typename D_,
        typename E_, typename F_, typename G_, typename H_, typename I_,
        typename J_, typename K_, typename L_, typename M_, typename N_,
        typename O_
    >
    struct result { typedef result_type type; };

    template <typename CT>
    bound_member_action(CT & obj_, mem_func_ptr_t fptr_)
    :   obj(impl::as_ptr<CT>::get(obj_)), fptr(fptr_) {}

    result_type operator()(
        A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o
    ) const
    {
        return (obj->*fptr)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
    }

    typename impl::as_ptr<ClassT>::pointer_type obj;
    mem_func_ptr_t fptr;
};

//////////////////////////////////
template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N,
    typename O
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>
bind(ClassT & obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N,
    typename O
>
inline bound_member<RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>
bind(ClassT * obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O))
{
    return bound_member<
        RT, ClassT, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N,
    typename O
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>
bind(ClassT const& obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(obj,fptr);
}

template <typename RT, typename ClassT,
    typename A, typename B, typename C, typename D,
    typename E, typename F, typename G, typename H, typename I,
    typename J, typename K, typename L, typename M, typename N,
    typename O
>
inline bound_member<RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>
bind(ClassT const* obj,RT(ClassT::*fptr)(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O) const)
{
    return bound_member<
        RT, ClassT const, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O>(obj,fptr);
}

#endif
#endif
#endif
#endif

}   //  namespace phoenix

#endif
