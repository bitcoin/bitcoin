/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2002 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_STATEMENTS_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_STATEMENTS_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/phoenix/composite.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  sequential_composite
//
//      Two or more actors separated by the comma generates a
//      sequential_composite which is a composite actor. Example:
//
//          actor,
//          actor,
//          actor
//
//      The actors are evaluated sequentially. The result type of this
//      is void. Note that the last actor should not have a trailing
//      comma.
//
///////////////////////////////////////////////////////////////////////////////
template <typename A0, typename A1>
struct sequential_composite {

    typedef sequential_composite<A0, A1> self_t;

    template <typename TupleT>
    struct result { typedef void type; };

    sequential_composite(A0 const& _0, A1 const& _1)
    :   a0(_0), a1(_1) {}

    template <typename TupleT>
    void
    eval(TupleT const& args) const
    {
        a0.eval(args);
        a1.eval(args);
    }

    A0 a0; A1 a1; //  actors
};

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline actor<sequential_composite<actor<BaseT0>, actor<BaseT1> > >
operator,(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return sequential_composite<actor<BaseT0>, actor<BaseT1> >(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  if_then_else_composite
//
//      This composite has two (2) forms:
//
//          if_(condition)
//          [
//              statement
//          ]
//
//      and
//
//          if_(condition)
//          [
//              true_statement
//          ]
//          .else_
//          [
//              false_statement
//          ]
//
//      where condition is an actor that evaluates to bool. If condition
//      is true, the true_statement (again an actor) is executed
//      otherwise, the false_statement (another actor) is executed. The
//      result type of this is void. Note the trailing underscore after
//      if_ and the leading dot and the trailing underscore before
//      and after .else_.
//
///////////////////////////////////////////////////////////////////////////////
template <typename CondT, typename ThenT, typename ElseT>
struct if_then_else_composite {

    typedef if_then_else_composite<CondT, ThenT, ElseT> self_t;

    template <typename TupleT>
    struct result {

        typedef void type;
    };

    if_then_else_composite(
        CondT const& cond_,
        ThenT const& then_,
        ElseT const& else__)
    :   cond(cond_), then(then_), else_(else__) {}

    template <typename TupleT>
    void eval(TupleT const& args) const
    {
        if (cond.eval(args))
            then.eval(args);
        else
            else_.eval(args);
    }

    CondT cond; ThenT then; ElseT else_; //  actors
};

//////////////////////////////////
template <typename CondT, typename ThenT>
struct else_gen {

    else_gen(CondT const& cond_, ThenT const& then_)
    :   cond(cond_), then(then_) {}

    template <typename ElseT>
    actor<if_then_else_composite<CondT, ThenT,
        typename as_actor<ElseT>::type> >
    operator[](ElseT const& else_)
    {
        typedef if_then_else_composite<CondT, ThenT,
            typename as_actor<ElseT>::type>
        result;

        return result(cond, then, as_actor<ElseT>::convert(else_));
    }

    CondT cond; ThenT then;
};

//////////////////////////////////
template <typename CondT, typename ThenT>
struct if_then_composite {

    typedef if_then_composite<CondT, ThenT> self_t;

    template <typename TupleT>
    struct result { typedef void type; };

    if_then_composite(CondT const& cond_, ThenT const& then_)
    :   cond(cond_), then(then_), else_(cond, then) {}

    template <typename TupleT>
    void eval(TupleT const& args) const
    {
        if (cond.eval(args))
            then.eval(args);
    }

    CondT cond; ThenT then; //  actors
    else_gen<CondT, ThenT> else_;
};

//////////////////////////////////
template <typename CondT>
struct if_gen {

    if_gen(CondT const& cond_)
    :   cond(cond_) {}

    template <typename ThenT>
    actor<if_then_composite<
        typename as_actor<CondT>::type,
        typename as_actor<ThenT>::type> >
    operator[](ThenT const& then) const
    {
        typedef if_then_composite<
            typename as_actor<CondT>::type,
            typename as_actor<ThenT>::type>
        result;

        return result(
            as_actor<CondT>::convert(cond),
            as_actor<ThenT>::convert(then));
    }

    CondT cond;
};

//////////////////////////////////
template <typename CondT>
inline if_gen<CondT>
if_(CondT const& cond)
{
    return if_gen<CondT>(cond);
}

///////////////////////////////////////////////////////////////////////////////
//
//  while_composite
//
//      This composite has the form:
//
//          while_(condition)
//          [
//              statement
//          ]
//
//      While the condition (an actor) evaluates to true, statement
//      (another actor) is executed. The result type of this is void.
//      Note the trailing underscore after while_.
//
///////////////////////////////////////////////////////////////////////////////
template <typename CondT, typename DoT>
struct while_composite {

    typedef while_composite<CondT, DoT> self_t;

    template <typename TupleT>
    struct result { typedef void type; };

    while_composite(CondT const& cond_, DoT const& do__)
    :   cond(cond_), do_(do__) {}

    template <typename TupleT>
    void eval(TupleT const& args) const
    {
        while (cond.eval(args))
            do_.eval(args);
    }

    CondT cond;
    DoT do_;
};

//////////////////////////////////
template <typename CondT>
struct while_gen {

    while_gen(CondT const& cond_)
    :   cond(cond_) {}

    template <typename DoT>
    actor<while_composite<
        typename as_actor<CondT>::type,
        typename as_actor<DoT>::type> >
    operator[](DoT const& do_) const
    {
        typedef while_composite<
            typename as_actor<CondT>::type,
            typename as_actor<DoT>::type>
        result;

        return result(
            as_actor<CondT>::convert(cond),
            as_actor<DoT>::convert(do_));
    }

    CondT cond;
};

//////////////////////////////////
template <typename CondT>
inline while_gen<CondT>
while_(CondT const& cond)
{
    return while_gen<CondT>(cond);
}

///////////////////////////////////////////////////////////////////////////////
//
//  do_composite
//
//      This composite has the form:
//
//          do_
//          [
//              statement
//          ]
//          .while_(condition)
//
//      While the condition (an actor) evaluates to true, statement
//      (another actor) is executed. The statement is executed at least
//      once. The result type of this is void. Note the trailing
//      underscore after do_ and the leading dot and the trailing
//      underscore before and after .while_.
//
///////////////////////////////////////////////////////////////////////////////
template <typename DoT, typename CondT>
struct do_composite {

    typedef do_composite<DoT, CondT> self_t;

    template <typename TupleT>
    struct result { typedef void type; };

    do_composite(DoT const& do__, CondT const& cond_)
    :   do_(do__), cond(cond_) {}

    template <typename TupleT>
    void eval(TupleT const& args) const
    {
        do
            do_.eval(args);
        while (cond.eval(args));
    }

    DoT do_;
    CondT cond;
};

////////////////////////////////////
template <typename DoT>
struct do_gen2 {

    do_gen2(DoT const& do__)
    :   do_(do__) {}

    template <typename CondT>
    actor<do_composite<
        typename as_actor<DoT>::type,
        typename as_actor<CondT>::type> >
    while_(CondT const& cond) const
    {
        typedef do_composite<
            typename as_actor<DoT>::type,
            typename as_actor<CondT>::type>
        result;

        return result(
            as_actor<DoT>::convert(do_),
            as_actor<CondT>::convert(cond));
    }

    DoT do_;
};

////////////////////////////////////
struct do_gen {

    template <typename DoT>
    do_gen2<DoT>
    operator[](DoT const& do_) const
    {
        return do_gen2<DoT>(do_);
    }
};

do_gen const do_ = do_gen();

///////////////////////////////////////////////////////////////////////////////
//
//  for_composite
//
//      This statement has the form:
//
//          for_(init, condition, step)
//          [
//              statement
//          ]
//
//      Where init, condition, step and statement are all actors. init
//      is executed once before entering the for-loop. The for-loop
//      exits once condition evaluates to false. At each loop iteration,
//      step and statement is called. The result of this statement is
//      void. Note the trailing underscore after for_.
//
///////////////////////////////////////////////////////////////////////////////
template <typename InitT, typename CondT, typename StepT, typename DoT>
struct for_composite {

    typedef composite<InitT, CondT, StepT, DoT> self_t;

    template <typename TupleT>
    struct result { typedef void type; };

    for_composite(
        InitT const& init_,
        CondT const& cond_,
        StepT const& step_,
        DoT const& do__)
    :   init(init_), cond(cond_), step(step_), do_(do__) {}

    template <typename TupleT>
    void
    eval(TupleT const& args) const
    {
        for (init.eval(args); cond.eval(args); step.eval(args))
            do_.eval(args);
    }

    InitT init; CondT cond; StepT step; DoT do_; //  actors
};

//////////////////////////////////
template <typename InitT, typename CondT, typename StepT>
struct for_gen {

    for_gen(
        InitT const& init_,
        CondT const& cond_,
        StepT const& step_)
    :   init(init_), cond(cond_), step(step_) {}

    template <typename DoT>
    actor<for_composite<
        typename as_actor<InitT>::type,
        typename as_actor<CondT>::type,
        typename as_actor<StepT>::type,
        typename as_actor<DoT>::type> >
    operator[](DoT const& do_) const
    {
        typedef for_composite<
            typename as_actor<InitT>::type,
            typename as_actor<CondT>::type,
            typename as_actor<StepT>::type,
            typename as_actor<DoT>::type>
        result;

        return result(
            as_actor<InitT>::convert(init),
            as_actor<CondT>::convert(cond),
            as_actor<StepT>::convert(step),
            as_actor<DoT>::convert(do_));
    }

    InitT init; CondT cond; StepT step;
};

//////////////////////////////////
template <typename InitT, typename CondT, typename StepT>
inline for_gen<InitT, CondT, StepT>
for_(InitT const& init, CondT const& cond, StepT const& step)
{
    return for_gen<InitT, CondT, StepT>(init, cond, step);
}

}   //  namespace phoenix

#endif
