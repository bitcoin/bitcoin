/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2002 Joel de Guzman
    MT code Copyright (c) 2002-2003 Martin Wille

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_CLOSURES_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_CLOSURES_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/phoenix/actor.hpp>
#include <boost/assert.hpp>

#ifdef PHOENIX_THREADSAFE
#include <boost/thread/tss.hpp>
#include <boost/thread/once.hpp>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Adaptable closures
//
//      The framework will not be complete without some form of closures
//      support. Closures encapsulate a stack frame where local
//      variables are created upon entering a function and destructed
//      upon exiting. Closures provide an environment for local
//      variables to reside. Closures can hold heterogeneous types.
//
//      Phoenix closures are true hardware stack based closures. At the
//      very least, closures enable true reentrancy in lambda functions.
//      A closure provides access to a function stack frame where local
//      variables reside. Modeled after Pascal nested stack frames,
//      closures can be nested just like nested functions where code in
//      inner closures may access local variables from in-scope outer
//      closures (accessing inner scopes from outer scopes is an error
//      and will cause a run-time assertion failure).
//
//      There are three (3) interacting classes:
//
//      1) closure:
//
//      At the point of declaration, a closure does not yet create a
//      stack frame nor instantiate any variables. A closure declaration
//      declares the types and names[note] of the local variables. The
//      closure class is meant to be subclassed. It is the
//      responsibility of a closure subclass to supply the names for
//      each of the local variable in the closure. Example:
//
//          struct my_closure : closure<int, string, double> {
//
//              member1 num;        // names the 1st (int) local variable
//              member2 message;    // names the 2nd (string) local variable
//              member3 real;       // names the 3rd (double) local variable
//          };
//
//          my_closure clos;
//
//      Now that we have a closure 'clos', its local variables can be
//      accessed lazily using the dot notation. Each qualified local
//      variable can be used just like any primitive actor (see
//      primitives.hpp). Examples:
//
//          clos.num = 30
//          clos.message = arg1
//          clos.real = clos.num * 1e6
//
//      The examples above are lazily evaluated. As usual, these
//      expressions return composite actors that will be evaluated
//      through a second function call invocation (see operators.hpp).
//      Each of the members (clos.xxx) is an actor. As such, applying
//      the operator() will reveal its identity:
//
//          clos.num() // will return the current value of clos.num
//
//      *** [note] Acknowledgement: Juan Carlos Arevalo-Baeza (JCAB)
//      introduced and initilally implemented the closure member names
//      that uses the dot notation.
//
//      2) closure_member
//
//      The named local variables of closure 'clos' above are actually
//      closure members. The closure_member class is an actor and
//      conforms to its conceptual interface. member1..memberN are
//      predefined typedefs that correspond to each of the listed types
//      in the closure template parameters.
//
//      3) closure_frame
//
//      When a closure member is finally evaluated, it should refer to
//      an actual instance of the variable in the hardware stack.
//      Without doing so, the process is not complete and the evaluated
//      member will result to an assertion failure. Remember that the
//      closure is just a declaration. The local variables that a
//      closure refers to must still be instantiated.
//
//      The closure_frame class does the actual instantiation of the
//      local variables and links these variables with the closure and
//      all its members. There can be multiple instances of
//      closure_frames typically situated in the stack inside a
//      function. Each closure_frame instance initiates a stack frame
//      with a new set of closure local variables. Example:
//
//          void foo()
//          {
//              closure_frame<my_closure> frame(clos);
//              /* do something */
//          }
//
//      where 'clos' is an instance of our closure 'my_closure' above.
//      Take note that the usage above precludes locally declared
//      classes. If my_closure is a locally declared type, we can still
//      use its self_type as a parameter to closure_frame:
//
//          closure_frame<my_closure::self_type> frame(clos);
//
//      Upon instantiation, the closure_frame links the local variables
//      to the closure. The previous link to another closure_frame
//      instance created before is saved. Upon destruction, the
//      closure_frame unlinks itself from the closure and relinks the
//      preceding closure_frame prior to this instance.
//
//      The local variables in the closure 'clos' above is default
//      constructed in the stack inside function 'foo'. Once 'foo' is
//      exited, all of these local variables are destructed. In some
//      cases, default construction is not desirable and we need to
//      initialize the local closure variables with some values. This
//      can be done by passing in the initializers in a compatible
//      tuple. A compatible tuple is one with the same number of
//      elements as the destination and where each element from the
//      destination can be constructed from each corresponding element
//      in the source. Example:
//
//          tuple<int, char const*, int> init(123, "Hello", 1000);
//          closure_frame<my_closure> frame(clos, init);
//
//      Here now, our closure_frame's variables are initialized with
//      int: 123, char const*: "Hello" and int: 1000.
//
///////////////////////////////////////////////////////////////////////////////

namespace impl
{
    ///////////////////////////////////////////////////////////////////////
    // closure_frame_holder is a simple class that encapsulates the
    // storage for a frame pointer. It uses thread specific data in
    // case when multithreading is enabled, an ordinary pointer otherwise
    //
    // it has get() and set() member functions. set() has to be used
    // _after_ get(). get() contains intialisation code in the multi
    // threading case
    //
    // closure_frame_holder is used by the closure<> class to store
    // the pointer to the current frame.
    //
#ifndef PHOENIX_THREADSAFE
    template <typename FrameT>
    struct closure_frame_holder
    {
        typedef FrameT frame_t;
        typedef frame_t *frame_ptr;

        closure_frame_holder() : frame(0) {}

        frame_ptr &get() { return frame; }
        void set(frame_t *f) { frame = f; }

    private:
        frame_ptr frame;

        // no copies, no assignments
        closure_frame_holder(closure_frame_holder const &);
        closure_frame_holder &operator=(closure_frame_holder const &);
    };
#else
    template <typename FrameT>
    struct closure_frame_holder
    {
        typedef FrameT   frame_t;
        typedef frame_t *frame_ptr;

        closure_frame_holder() : tsp_frame() {}

        frame_ptr &get()
        {
            if (!tsp_frame.get())
                tsp_frame.reset(new frame_ptr(0));
            return *tsp_frame;
        }
        void set(frame_ptr f)
        {
            *tsp_frame = f;
        }

    private:
        boost::thread_specific_ptr<frame_ptr> tsp_frame;

        // no copies, no assignments
        closure_frame_holder(closure_frame_holder const &);
        closure_frame_holder &operator=(closure_frame_holder const &);
    };
#endif
} // namespace phoenix::impl

///////////////////////////////////////////////////////////////////////////////
//
//  closure_frame class
//
///////////////////////////////////////////////////////////////////////////////
template <typename ClosureT>
class closure_frame : public ClosureT::tuple_t {

public:

    closure_frame(ClosureT const& clos)
    : ClosureT::tuple_t(), save(clos.frame.get()), frame(clos.frame)
    { clos.frame.set(this); }

    template <typename TupleT>
    closure_frame(ClosureT const& clos, TupleT const& init)
    : ClosureT::tuple_t(init), save(clos.frame.get()), frame(clos.frame)
    { clos.frame.set(this); }

    ~closure_frame()
    { frame.set(save); }

private:

    closure_frame(closure_frame const&);            // no copy
    closure_frame& operator=(closure_frame const&); // no assign

    closure_frame* save;
    impl::closure_frame_holder<closure_frame>& frame;
};

///////////////////////////////////////////////////////////////////////////////
//
//  closure_member class
//
///////////////////////////////////////////////////////////////////////////////
template <int N, typename ClosureT>
class closure_member {

public:

    typedef typename ClosureT::tuple_t tuple_t;

    closure_member()
    : frame(ClosureT::closure_frame_holder_ref()) {}

    template <typename TupleT>
    struct result {

        typedef typename tuple_element<
            N, typename ClosureT::tuple_t
        >::rtype type;
    };

    template <typename TupleT>
    typename tuple_element<N, typename ClosureT::tuple_t>::rtype
    eval(TupleT const& /*args*/) const
    {
        using namespace std;
        BOOST_ASSERT(frame.get() != 0);
        tuple_index<N> const idx;
        return (*frame.get())[idx];
    }

private:
    impl::closure_frame_holder<typename ClosureT::closure_frame_t> &frame;
};

///////////////////////////////////////////////////////////////////////////////
//
//  closure class
//
///////////////////////////////////////////////////////////////////////////////
template <
        typename T0 = nil_t
    ,   typename T1 = nil_t
    ,   typename T2 = nil_t

#if PHOENIX_LIMIT > 3
    ,   typename T3 = nil_t
    ,   typename T4 = nil_t
    ,   typename T5 = nil_t

#if PHOENIX_LIMIT > 6
    ,   typename T6 = nil_t
    ,   typename T7 = nil_t
    ,   typename T8 = nil_t

#if PHOENIX_LIMIT > 9
    ,   typename T9 = nil_t
    ,   typename T10 = nil_t
    ,   typename T11 = nil_t

#if PHOENIX_LIMIT > 12
    ,   typename T12 = nil_t
    ,   typename T13 = nil_t
    ,   typename T14 = nil_t

#endif
#endif
#endif
#endif
>
class closure {

public:

    typedef tuple<
            T0, T1, T2
#if PHOENIX_LIMIT > 3
        ,   T3, T4, T5
#if PHOENIX_LIMIT > 6
        ,   T6, T7, T8
#if PHOENIX_LIMIT > 9
        ,   T9, T10, T11
#if PHOENIX_LIMIT > 12
        ,   T12, T13, T14
#endif
#endif
#endif
#endif
        > tuple_t;

    typedef closure<
            T0, T1, T2
#if PHOENIX_LIMIT > 3
        ,   T3, T4, T5
#if PHOENIX_LIMIT > 6
        ,   T6, T7, T8
#if PHOENIX_LIMIT > 9
        ,   T9, T10, T11
#if PHOENIX_LIMIT > 12
        ,   T12, T13, T14
#endif
#endif
#endif
#endif
        > self_t;

    typedef closure_frame<self_t> closure_frame_t;

                            closure()
                            : frame()       { closure_frame_holder_ref(&frame); }

    typedef actor<closure_member<0, self_t> > member1;
    typedef actor<closure_member<1, self_t> > member2;
    typedef actor<closure_member<2, self_t> > member3;

#if PHOENIX_LIMIT > 3
    typedef actor<closure_member<3, self_t> > member4;
    typedef actor<closure_member<4, self_t> > member5;
    typedef actor<closure_member<5, self_t> > member6;

#if PHOENIX_LIMIT > 6
    typedef actor<closure_member<6, self_t> > member7;
    typedef actor<closure_member<7, self_t> > member8;
    typedef actor<closure_member<8, self_t> > member9;

#if PHOENIX_LIMIT > 9
    typedef actor<closure_member<9, self_t> > member10;
    typedef actor<closure_member<10, self_t> > member11;
    typedef actor<closure_member<11, self_t> > member12;

#if PHOENIX_LIMIT > 12
    typedef actor<closure_member<12, self_t> > member13;
    typedef actor<closure_member<13, self_t> > member14;
    typedef actor<closure_member<14, self_t> > member15;

#endif
#endif
#endif
#endif

#if !defined(__MWERKS__) || (__MWERKS__ > 0x3002)
private:
#endif

    closure(closure const&);            // no copy
    closure& operator=(closure const&); // no assign

#if !defined(__MWERKS__) || (__MWERKS__ > 0x3002)
    template <int N, typename ClosureT>
    friend class closure_member;

    template <typename ClosureT>
    friend class closure_frame;
#endif

    typedef impl::closure_frame_holder<closure_frame_t> holder_t;

#ifdef PHOENIX_THREADSAFE
    static boost::thread_specific_ptr<holder_t*> &
    tsp_frame_instance()
    {
        static boost::thread_specific_ptr<holder_t*> the_instance;
        return the_instance;
    }

    static void
    tsp_frame_instance_init()
    {
        tsp_frame_instance();
    }
#endif

    static holder_t &
    closure_frame_holder_ref(holder_t* holder_ = 0)
    {
#ifdef PHOENIX_THREADSAFE
#ifndef BOOST_THREAD_PROVIDES_ONCE_CXX11
        static boost::once_flag been_here = BOOST_ONCE_INIT;
#else
        static boost::once_flag been_here;
#endif
        boost::call_once(been_here, tsp_frame_instance_init);
        boost::thread_specific_ptr<holder_t*> &tsp_frame = tsp_frame_instance();
        if (!tsp_frame.get())
            tsp_frame.reset(new holder_t *(0));
        holder_t *& holder = *tsp_frame;
#else
        static holder_t* holder = 0;
#endif
        if (holder_ != 0)
            holder = holder_;
        return *holder;
    }

    mutable holder_t frame;
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

}
   //  namespace phoenix

#endif
