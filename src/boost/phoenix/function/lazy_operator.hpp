////////////////////////////////////////////////////////////////////////////
// lazy operator.hpp
//
// Build lazy operations for Phoenix equivalents for FC++
//
// These are equivalents of the Boost FC++ functoids in operator.hpp
//
// Implemented so far:
//
// make_pair
// plus minus multiplies divides modulus
// negate equal not_equal greater less
// greater_equal less_equal positive
// logical_and logical_or
// logical_not min max inc dec
//
// These are not from the FC++ operator.hpp but were made for testing purposes.
//
// identity (renamed id)
// sin
//
// These are now being modified to use boost::phoenix::function
// so that they are available for use as arguments.
// Types are being defined in capitals e.g. Id id;
////////////////////////////////////////////////////////////////////////////
/*=============================================================================
    Copyright (c) 2000-2003 Brian McNamara and Yannis Smaragdakis
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2015 John Fletcher

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/


#ifndef BOOST_PHOENIX_FUNCTION_LAZY_OPERATOR
#define BOOST_PHOENIX_FUNCTION_LAZY_OPERATOR

#include <cmath>
#include <cstdlib>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/function.hpp>

namespace boost {

  namespace phoenix {

//////////////////////////////////////////////////////////////////////
// a_unique_type_for_nil
//////////////////////////////////////////////////////////////////////

// This may need to be moved elsewhere to define reuser.
   struct a_unique_type_for_nil {
     bool operator==( a_unique_type_for_nil ) const { return true; }
     bool operator< ( a_unique_type_for_nil ) const { return false; }
     typedef a_unique_type_for_nil value_type;
   };
    // This maybe put into a namespace.
   a_unique_type_for_nil NIL;

//////////////////////////////////////////////////////////////////////
// lazy_exception - renamed from fcpp_exception.
//////////////////////////////////////////////////////////////////////

#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
   struct lazy_exception : public std::exception {
       const char* s;
       lazy_exception( const char* ss ) : s(ss) {}
       const char* what() const throw() { return s; }
   };
#endif

//////////////////////////////////////////////////////////////////////

   // in ref_count.hpp in BoostFC++
   typedef unsigned int RefCountType;

    namespace impl {

      // Implemented early, moved from lazy_signature.hpp
      template <class T>
      struct remove_RC
      {
          typedef typename boost::remove_reference<T>::type TT;
          typedef typename boost::remove_const<TT>::type type;
      };

      struct XId
      {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
            return a0;
        }

      };


    }

    typedef boost::phoenix::function<impl::XId> Id;
    Id id;

#ifdef BOOST_RESULT_OF_USE_TR1
    // Experiment following examples in
    // phoenix/stl/container/container.hpp

    namespace result_of {

      template <
          typename Arg1
        , typename Arg2
      >
      class make_pair
      {
      public:
        typedef typename impl::remove_RC<Arg1>::type Arg1Type;
        typedef typename impl::remove_RC<Arg2>::type Arg2Type;
        typedef std::pair<Arg1Type,Arg2Type> type;
        typedef std::pair<Arg1Type,Arg2Type> result_type;
      };
    }
#endif

  namespace impl
  {

    struct XMake_pair {


#ifdef BOOST_RESULT_OF_USE_TR1
       template <typename Sig>
       struct result;
       // This fails with -O2 unless refs are removed from A1 and A2.
       template <typename This, typename A0, typename A1>
       struct result<This(A0, A1)>
       {
         typedef typename result_of::make_pair<A0,A1>::type type;
       };
#else
       template <typename Sig>
       struct result;

       template <typename This, typename A0, typename A1>
       struct result<This(A0, A1)>
         : boost::remove_reference<std::pair<A0, A1> >
       {};
      
#endif


       template <typename A0, typename A1>
#ifdef BOOST_RESULT_OF_USE_TR1
       typename result<XMake_pair(A0,A1)>::type
#else
       std::pair<A0, A1>
#endif
       operator()(A0 const & a0, A1 const & a1) const
       {
          return std::make_pair(a0,a1);
       }

    };
  }

  typedef boost::phoenix::function<impl::XMake_pair> Make_pair;
  Make_pair make_pair;

  namespace impl
  {

    // For now I will leave the return type deduction as it is.
    // I want to look at bringing in the sort of type deduction for
    // mixed types which I have in FC++.
    // Also I could look at the case where one of the arguments is
    // another functor or a Phoenix placeholder.
    struct XPlus
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
             : boost::remove_reference<A0>
        {};

        template <typename This, typename A0, typename A1, typename A2>
        struct result<This(A0, A1, A2)>
             : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
          //A0 res = a0 + a1;
          //return res;
          return a0 + a1;
        }

        template <typename A0, typename A1, typename A2>
        A0 operator()(A0 const & a0, A1 const & a1, A2 const & a2) const
        {
            return a0 + a1 + a2;
        }
    };

    struct XMinus
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
           : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 - a1;
        }

    };

    struct XMultiplies
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
           : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 * a1;
        }

    };

    struct XDivides
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
           : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 / a1;
        }

    };

    struct XModulus
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
          : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 % a1;
        }

    };

    struct XNegate
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
            return -a0;
        }
    };

    struct XEqual
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 == a1;
        }
    };

    struct XNot_equal
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 != a1;
        }
    };

    struct XGreater
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 > a1;
        }
    };

    struct XLess
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 < a1;
        }
    };

    struct XGreater_equal
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 >= a1;
        }
    };

    struct XLess_equal
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 <= a1;
        }
    };

    struct XPositive
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
        {
            typedef bool type;
        };

        template <typename A0>
        bool operator()(A0 const & a0) const
        {
          return a0 >= A0(0);
        }
    };

    struct XLogical_and
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 && a1;
        }
    };

    struct XLogical_or
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 || a1;
        }
    };

    struct XLogical_not
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
        {
             typedef bool type;
        };

        template <typename A0>
        bool operator()(A0 const & a0) const
        {
            return !a0;
        }
    };

    struct XMin
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
          : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
           if ( a0 < a1 ) return a0; else return a1;
        }

    };

    struct XMax
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
          : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
           if ( a0 < a1 ) return a1; else return a0;
        }

    };

    struct XInc
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
            return a0 + 1;
        }

    };

    struct XDec
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
            return a0 - 1;
        }

    };

    struct XSin
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
          return std::sin(a0);
        }

    };

    // Example of templated struct.
    // How do I make it callable?
    template <typename Result>
    struct what {

      typedef Result result_type;

      Result operator()(Result const & r) const
      {
        return r;
      }
      // what is not complete - error.
      //static boost::function1<Result,Result> res = what<Result>();
    };

    template <typename Result>
    struct what0 {

      typedef Result result_type;

      Result operator()() const
      {
        return Result(100);
      }

    };


      template <class Result, class F>
      class MonomorphicWrapper0 /* : public c_fun_type<Res> */
      {
          F f;
      public:
          typedef Result result_type;
          MonomorphicWrapper0( const F& g ) : f(g) {}
             Result operator()() const {
             return f();
          }
      };

  }
    /////////////////////////////////////////////////////////
    // Look at this. How to use Phoenix with a templated
    // struct. First adapt with boost::function and then
    // convert that to Phoenix!!
    // I have not found out how to do it directly.
    /////////////////////////////////////////////////////////
boost::function1<int, int > what_int = impl::what<int>();
typedef boost::function1<int,int> fun1_int_int;
typedef boost::function0<int> fun0_int;
boost::function0<int> what0_int = impl::what0<int>();
BOOST_PHOENIX_ADAPT_FUNCTION(int,what,what_int,1)
BOOST_PHOENIX_ADAPT_FUNCTION_NULLARY(int,what0,what0_int)
// And this shows how to make them into argument callable functions.
typedef boost::phoenix::function<fun1_int_int> What_arg;
typedef boost::phoenix::function<fun0_int> What0_arg;
What_arg what_arg(what_int);
What0_arg what0_arg(what0_int);


// To use these as arguments they have to be defined like this.
    typedef boost::phoenix::function<impl::XPlus> Plus;
    typedef boost::phoenix::function<impl::XMinus> Minus;
    typedef boost::phoenix::function<impl::XMultiplies> Multiplies;
    typedef boost::phoenix::function<impl::XDivides>   Divides;
    typedef boost::phoenix::function<impl::XModulus>   Modulus;
    typedef boost::phoenix::function<impl::XNegate>    Negate;
    typedef boost::phoenix::function<impl::XEqual>     Equal;
    typedef boost::phoenix::function<impl::XNot_equal> Not_equal;
    typedef boost::phoenix::function<impl::XGreater>   Greater;
    typedef boost::phoenix::function<impl::XLess>      Less;
    typedef boost::phoenix::function<impl::XGreater_equal> Greater_equal;
    typedef boost::phoenix::function<impl::XLess_equal>    Less_equal;
    typedef boost::phoenix::function<impl::XPositive>    Positive;
    typedef boost::phoenix::function<impl::XLogical_and> Logical_and;
    typedef boost::phoenix::function<impl::XLogical_or>  Logical_or;
    typedef boost::phoenix::function<impl::XLogical_not> Logical_not;
    typedef boost::phoenix::function<impl::XMax> Max;
    typedef boost::phoenix::function<impl::XMin> Min;
    typedef boost::phoenix::function<impl::XInc> Inc;
    typedef boost::phoenix::function<impl::XDec> Dec;
    typedef boost::phoenix::function<impl::XSin> Sin;
    Plus plus;
    Minus minus;
    Multiplies multiplies;
    Divides divides;
    Modulus modulus;
    Negate  negate;
    Equal   equal;
    Not_equal not_equal;
    Greater greater;
    Less    less;
    Greater_equal greater_equal;
    Less_equal    less_equal;
    Positive      positive;
    Logical_and   logical_and;
    Logical_or    logical_or;
    Logical_not   logical_not;
    Max max;
    Min min;
    Inc inc;
    Dec dec;
    Sin sin;
}

}


#endif
