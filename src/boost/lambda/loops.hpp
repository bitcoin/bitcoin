// Boost Lambda Library -- loops.hpp ----------------------------------------

// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
// Copyright (C) 2000 Gary Powell (powellg@amazon.com)
// Copyright (c) 2001-2002 Joel de Guzman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// --------------------------------------------------------------------------

#if !defined(BOOST_LAMBDA_LOOPS_HPP)
#define BOOST_LAMBDA_LOOPS_HPP

#include "boost/lambda/core.hpp"

namespace boost { 
namespace lambda {

// -- loop control structure actions ----------------------

class forloop_action {};
class forloop_no_body_action {};
class whileloop_action {};
class whileloop_no_body_action {};
class dowhileloop_action {};
class dowhileloop_no_body_action {};


// For loop
template <class Arg1, class Arg2, class Arg3, class Arg4>
inline const 
lambda_functor<
  lambda_functor_base<
    forloop_action, 
    tuple<lambda_functor<Arg1>, lambda_functor<Arg2>, 
          lambda_functor<Arg3>, lambda_functor<Arg4> >
  > 
>
for_loop(const lambda_functor<Arg1>& a1, const lambda_functor<Arg2>& a2, 
         const lambda_functor<Arg3>& a3, const lambda_functor<Arg4>& a4) { 
  return 
      lambda_functor_base<
        forloop_action, 
        tuple<lambda_functor<Arg1>, lambda_functor<Arg2>, 
              lambda_functor<Arg3>, lambda_functor<Arg4> >
      > 
    ( tuple<lambda_functor<Arg1>, lambda_functor<Arg2>, 
            lambda_functor<Arg3>, lambda_functor<Arg4> >(a1, a2, a3, a4)
    );
}

// No body case.
template <class Arg1, class Arg2, class Arg3>
inline const 
lambda_functor<
  lambda_functor_base<
    forloop_no_body_action, 
    tuple<lambda_functor<Arg1>, lambda_functor<Arg2>, lambda_functor<Arg3> >
  > 
>
for_loop(const lambda_functor<Arg1>& a1, const lambda_functor<Arg2>& a2, 
         const lambda_functor<Arg3>& a3) { 
  return 
      lambda_functor_base<
        forloop_no_body_action, 
        tuple<lambda_functor<Arg1>, lambda_functor<Arg2>, 
              lambda_functor<Arg3> >
      > 
      ( tuple<lambda_functor<Arg1>, lambda_functor<Arg2>, 
               lambda_functor<Arg3> >(a1, a2, a3) );
}

// While loop
template <class Arg1, class Arg2>
inline const 
lambda_functor<
  lambda_functor_base<
    whileloop_action, 
    tuple<lambda_functor<Arg1>, lambda_functor<Arg2> >
  > 
>
while_loop(const lambda_functor<Arg1>& a1, const lambda_functor<Arg2>& a2) { 
  return 
      lambda_functor_base<
        whileloop_action, 
        tuple<lambda_functor<Arg1>, lambda_functor<Arg2> >
      > 
      ( tuple<lambda_functor<Arg1>, lambda_functor<Arg2> >(a1, a2));
}

// No body case.
template <class Arg1>
inline const 
lambda_functor<
  lambda_functor_base<
    whileloop_no_body_action, 
    tuple<lambda_functor<Arg1> >
  > 
>
while_loop(const lambda_functor<Arg1>& a1) { 
  return 
      lambda_functor_base<
        whileloop_no_body_action, 
        tuple<lambda_functor<Arg1> >
      > 
      ( tuple<lambda_functor<Arg1> >(a1) );
}


// Do While loop
template <class Arg1, class Arg2>
inline const 
lambda_functor<
  lambda_functor_base<
    dowhileloop_action, 
    tuple<lambda_functor<Arg1>, lambda_functor<Arg2> >
  > 
>
do_while_loop(const lambda_functor<Arg1>& a1, const lambda_functor<Arg2>& a2) {
  return 
      lambda_functor_base<
        dowhileloop_action, 
        tuple<lambda_functor<Arg1>, lambda_functor<Arg2> >
      > 
      ( tuple<lambda_functor<Arg1>, lambda_functor<Arg2> >(a1, a2));
}

// No body case.
template <class Arg1>
inline const 
lambda_functor<
  lambda_functor_base<
    dowhileloop_no_body_action, 
    tuple<lambda_functor<Arg1> >
  > 
>
do_while_loop(const lambda_functor<Arg1>& a1) { 
  return 
      lambda_functor_base<
        dowhileloop_no_body_action, 
        tuple<lambda_functor<Arg1> >
      > 
      ( tuple<lambda_functor<Arg1> >(a1));
}


// Control loop lambda_functor_base specializations.

// Specialization for for_loop.
template<class Args>
class 
lambda_functor_base<forloop_action, Args> {
public:
  Args args;
  template <class T> struct sig { typedef void type; };
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
    for(detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS); 
        detail::select(boost::tuples::get<1>(args), CALL_ACTUAL_ARGS); 
        detail::select(boost::tuples::get<2>(args), CALL_ACTUAL_ARGS))
      
      detail::select(boost::tuples::get<3>(args), CALL_ACTUAL_ARGS);
  }
};

// No body case
template<class Args>
class 
lambda_functor_base<forloop_no_body_action, Args> {
public:
  Args args;
  template <class T> struct sig { typedef void type; };
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
    for(detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS); 
        detail::select(boost::tuples::get<1>(args), CALL_ACTUAL_ARGS); 
        detail::select(boost::tuples::get<2>(args), CALL_ACTUAL_ARGS)) {}
   }
};


// Specialization for while_loop.
template<class Args>
class 
lambda_functor_base<whileloop_action, Args> {
public:
  Args args;
  template <class T> struct sig { typedef void type; };
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
    while(detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS))
      
      detail::select(boost::tuples::get<1>(args), CALL_ACTUAL_ARGS);
  }
};

// No body case
template<class Args> 
class 
lambda_functor_base<whileloop_no_body_action, Args> {
public:
  Args args;
  template <class T> struct sig { typedef void type; };
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
          while(detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS)) {}
  }
};

// Specialization for do_while_loop.
// Note that the first argument is the condition.
template<class Args>
class 
lambda_functor_base<dowhileloop_action, Args> {
public:
  Args args;
  template <class T> struct sig { typedef void type; };
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
    do {
      detail::select(boost::tuples::get<1>(args), CALL_ACTUAL_ARGS);      
    } while (detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS) );
  }
};

// No body case
template<class Args>
class 
lambda_functor_base<dowhileloop_no_body_action, Args> {
public:
  Args args;
  template <class T> struct sig { typedef void type; };
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
          do {} while (detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS) );
  }
};

  // The code below is from Joel de Guzman, some name changes etc. 
  // has been made.

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
//      While the condition (an lambda_functor) evaluates to true, statement
//      (another lambda_functor) is executed. The result type of this is void.
//      Note the trailing underscore after while_.
//
///////////////////////////////////////////////////////////////////////////////
template <typename CondT, typename DoT>
struct while_composite {

    typedef while_composite<CondT, DoT> self_t;

    template <class SigArgs>
    struct sig { typedef void type; };

    while_composite(CondT const& cond_, DoT const& do__)
    :   cond(cond_), do_(do__) {}

    template <class Ret, CALL_TEMPLATE_ARGS>
    Ret call(CALL_FORMAL_ARGS) const
    {
        while (cond.internal_call(CALL_ACTUAL_ARGS))
            do_.internal_call(CALL_ACTUAL_ARGS);
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
    lambda_functor<while_composite<
        typename as_lambda_functor<CondT>::type,
        typename as_lambda_functor<DoT>::type> >
    operator[](DoT const& do_) const
    {
        typedef while_composite<
            typename as_lambda_functor<CondT>::type,
            typename as_lambda_functor<DoT>::type>
        result;

        return result(
            to_lambda_functor(cond),
            to_lambda_functor(do_));
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
//      While the condition (an lambda_functor) evaluates to true, statement
//      (another lambda_functor) is executed. The statement is executed at least
//      once. The result type of this is void. Note the trailing
//      underscore after do_ and the leading dot and the trailing
//      underscore before and after .while_.
//
///////////////////////////////////////////////////////////////////////////////
template <typename DoT, typename CondT>
struct do_composite {

    typedef do_composite<DoT, CondT> self_t;

    template <class SigArgs>
    struct sig { typedef void type; };

    do_composite(DoT const& do__, CondT const& cond_)
    :   do_(do__), cond(cond_) {}

    template <class Ret, CALL_TEMPLATE_ARGS>
    Ret call(CALL_FORMAL_ARGS) const
    {
        do
            do_.internal_call(CALL_ACTUAL_ARGS);
        while (cond.internal_call(CALL_ACTUAL_ARGS));
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
    lambda_functor<do_composite<
        typename as_lambda_functor<DoT>::type,
        typename as_lambda_functor<CondT>::type> >
    while_(CondT const& cond) const
    {
        typedef do_composite<
            typename as_lambda_functor<DoT>::type,
            typename as_lambda_functor<CondT>::type>
        result;

        return result(
            to_lambda_functor(do_),
            to_lambda_functor(cond));
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
//      Where init, condition, step and statement are all lambda_functors. init
//      is executed once before entering the for-loop. The for-loop
//      exits once condition evaluates to false. At each loop iteration,
//      step and statement is called. The result of this statement is
//      void. Note the trailing underscore after for_.
//
///////////////////////////////////////////////////////////////////////////////
template <typename InitT, typename CondT, typename StepT, typename DoT>
struct for_composite {

    template <class SigArgs>
    struct sig { typedef void type; };

    for_composite(
        InitT const& init_,
        CondT const& cond_,
        StepT const& step_,
        DoT const& do__)
    :   init(init_), cond(cond_), step(step_), do_(do__) {}

    template <class Ret, CALL_TEMPLATE_ARGS>
    Ret
    call(CALL_FORMAL_ARGS) const
    {
        for (init.internal_call(CALL_ACTUAL_ARGS); cond.internal_call(CALL_ACTUAL_ARGS); step.internal_call(CALL_ACTUAL_ARGS))
            do_.internal_call(CALL_ACTUAL_ARGS);
    }

    InitT init; CondT cond; StepT step; DoT do_; //  lambda_functors
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
    lambda_functor<for_composite<
        typename as_lambda_functor<InitT>::type,
        typename as_lambda_functor<CondT>::type,
        typename as_lambda_functor<StepT>::type,
        typename as_lambda_functor<DoT>::type> >
    operator[](DoT const& do_) const
    {
        typedef for_composite<
            typename as_lambda_functor<InitT>::type,
            typename as_lambda_functor<CondT>::type,
            typename as_lambda_functor<StepT>::type,
            typename as_lambda_functor<DoT>::type>
        result;

        return result(
            to_lambda_functor(init),
            to_lambda_functor(cond),
            to_lambda_functor(step),
            to_lambda_functor(do_));
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

} // lambda
} // boost

#endif // BOOST_LAMBDA_LOOPS_HPP
