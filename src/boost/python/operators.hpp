// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef OPERATORS_DWA2002530_HPP
# define OPERATORS_DWA2002530_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/def_visitor.hpp>
# include <boost/python/converter/arg_to_python.hpp>
# include <boost/python/detail/operator_id.hpp>
# include <boost/python/detail/not_specified.hpp>
# include <boost/python/back_reference.hpp>
# include <boost/mpl/if.hpp>
# include <boost/mpl/eval_if.hpp>
# include <boost/python/self.hpp>
# include <boost/python/other.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/python/refcount.hpp>
# include <boost/python/detail/unwrap_wrapper.hpp>
# include <string>
# include <complex>

namespace boost { namespace python { 

namespace detail
{
  // This is essentially the old v1 to_python(). It will be eliminated
  // once the public interface for to_python is settled on.
  template <class T>
  PyObject* convert_result(T const& x)
  {
      return converter::arg_to_python<T>(x).release();
  }

  // Operator implementation template declarations. The nested apply
  // declaration here keeps MSVC6 happy.
  template <operator_id> struct operator_l
  {
      template <class L, class R> struct apply;
  };
  
  template <operator_id> struct operator_r
  {
      template <class L, class R> struct apply;
  };

  template <operator_id> struct operator_1
  {
      template <class T> struct apply;
  };

  // MSVC6 doesn't want us to do this sort of inheritance on a nested
  // class template, so we use this layer of indirection to avoid
  // ::template<...> on the nested apply functions below
  template <operator_id id, class L, class R>
  struct operator_l_inner
      : operator_l<id>::template apply<L,R>
  {};
      
  template <operator_id id, class L, class R>
  struct operator_r_inner
      : operator_r<id>::template apply<L,R>
  {};

  template <operator_id id, class T>
  struct operator_1_inner
      : operator_1<id>::template apply<T>
  {};
      
  // Define three different binary_op templates which take care of
  // these cases:
  //    self op self
  //    self op R
  //    L op self
  // 
  // The inner apply metafunction is used to adjust the operator to
  // the class type being defined. Inheritance of the outer class is
  // simply used to provide convenient access to the operation's
  // name().

  // self op self
  template <operator_id id>
  struct binary_op : operator_l<id>
  {
      template <class T>
      struct apply : operator_l_inner<id,T,T>
      {
      };
  };

  // self op R
  template <operator_id id, class R>
  struct binary_op_l : operator_l<id>
  {
      template <class T>
      struct apply : operator_l_inner<id,T,R>
      {
      };
  };

  // L op self
  template <operator_id id, class L>
  struct binary_op_r : operator_r<id>
  {
      template <class T>
      struct apply : operator_r_inner<id,L,T>
      {
      };
  };

  template <operator_id id>
  struct unary_op : operator_1<id>
  {
      template <class T>
      struct apply : operator_1_inner<id,T>
      {
      };
  };

  // This type is what actually gets returned from operators used on
  // self_t
  template <operator_id id, class L = not_specified, class R = not_specified>
  struct operator_
    : def_visitor<operator_<id,L,R> >
  {
   private:
      template <class ClassT>
      void visit(ClassT& cl) const
      {
          typedef typename mpl::eval_if<
              is_same<L,self_t>
            , mpl::if_<
                  is_same<R,self_t>
                , binary_op<id>
                , binary_op_l<
                      id
                    , BOOST_DEDUCED_TYPENAME unwrap_other<R>::type
                  >
              >
            , mpl::if_<
                  is_same<L,not_specified>
                , unary_op<id>
                , binary_op_r<
                      id
                    , BOOST_DEDUCED_TYPENAME unwrap_other<L>::type
                  >
              >
          >::type generator;
      
          cl.def(
              generator::name()
            , &generator::template apply<
                 BOOST_DEDUCED_TYPENAME ClassT::wrapped_type
              >::execute
          );
      }
    
      friend class python::def_visitor_access;
  };
}

# define BOOST_PYTHON_BINARY_OPERATION(id, rid, expr)       \
namespace detail                                            \
{                                                           \
  template <>                                               \
  struct operator_l<op_##id>                                \
  {                                                         \
      template <class L, class R>                           \
      struct apply                                          \
      {                                                     \
          typedef typename unwrap_wrapper_<L>::type lhs;    \
          typedef typename unwrap_wrapper_<R>::type rhs;    \
          static PyObject* execute(lhs& l, rhs const& r)    \
          {                                                 \
              return detail::convert_result(expr);          \
          }                                                 \
      };                                                    \
      static char const* name() { return "__" #id "__"; }   \
  };                                                        \
                                                            \
  template <>                                               \
  struct operator_r<op_##id>                                \
  {                                                         \
      template <class L, class R>                           \
      struct apply                                          \
      {                                                     \
          typedef typename unwrap_wrapper_<L>::type lhs;    \
          typedef typename unwrap_wrapper_<R>::type rhs;    \
          static PyObject* execute(rhs& r, lhs const& l)    \
          {                                                 \
              return detail::convert_result(expr);          \
          }                                                 \
      };                                                    \
      static char const* name() { return "__" #rid "__"; }  \
  };                                                        \
} 

# define BOOST_PYTHON_BINARY_OPERATOR(id, rid, op)      \
BOOST_PYTHON_BINARY_OPERATION(id, rid, l op r)          \
namespace self_ns                                       \
{                                                       \
  template <class L, class R>                           \
  inline detail::operator_<detail::op_##id,L,R>         \
  operator op(L const&, R const&)                       \
  {                                                     \
      return detail::operator_<detail::op_##id,L,R>();  \
  }                                                     \
}
  
BOOST_PYTHON_BINARY_OPERATOR(add, radd, +)
BOOST_PYTHON_BINARY_OPERATOR(sub, rsub, -)
BOOST_PYTHON_BINARY_OPERATOR(mul, rmul, *)
#if PY_VERSION_HEX >= 0x03000000
    BOOST_PYTHON_BINARY_OPERATOR(truediv, rtruediv, /)
#else
    BOOST_PYTHON_BINARY_OPERATOR(div, rdiv, /)
#endif
BOOST_PYTHON_BINARY_OPERATOR(mod, rmod, %)
BOOST_PYTHON_BINARY_OPERATOR(lshift, rlshift, <<)
BOOST_PYTHON_BINARY_OPERATOR(rshift, rrshift, >>)
BOOST_PYTHON_BINARY_OPERATOR(and, rand, &)
BOOST_PYTHON_BINARY_OPERATOR(xor, rxor, ^)
BOOST_PYTHON_BINARY_OPERATOR(or, ror, |)
BOOST_PYTHON_BINARY_OPERATOR(gt, lt, >)
BOOST_PYTHON_BINARY_OPERATOR(ge, le, >=)
BOOST_PYTHON_BINARY_OPERATOR(lt, gt, <)
BOOST_PYTHON_BINARY_OPERATOR(le, ge, <=)
BOOST_PYTHON_BINARY_OPERATOR(eq, eq, ==)
BOOST_PYTHON_BINARY_OPERATOR(ne, ne, !=)
# undef BOOST_PYTHON_BINARY_OPERATOR
    
// pow isn't an operator in C++; handle it specially.
BOOST_PYTHON_BINARY_OPERATION(pow, rpow, pow(l,r))
# undef BOOST_PYTHON_BINARY_OPERATION
    
namespace self_ns
{
# ifndef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
  template <class L, class R>
  inline detail::operator_<detail::op_pow,L,R>
  pow(L const&, R const&)
  {
      return detail::operator_<detail::op_pow,L,R>();
  }
# else
  // When there's no argument-dependent lookup, we need these
  // overloads to handle the case when everything is imported into the
  // global namespace. Note that the plain overload below does /not/
  // take const& arguments. This is needed by MSVC6 at least, or it
  // complains of ambiguities, since there's no partial ordering.
  inline detail::operator_<detail::op_pow,self_t,self_t>
  pow(self_t, self_t)
  {
      return detail::operator_<detail::op_pow,self_t,self_t>();
  }
  template <class R>
  inline detail::operator_<detail::op_pow,self_t,R>
  pow(self_t const&, R const&)
  {
      return detail::operator_<detail::op_pow,self_t,R>();
  }
  template <class L>
  inline detail::operator_<detail::op_pow,L,self_t>
  pow(L const&, self_t const&)
  {
      return detail::operator_<detail::op_pow,L,self_t>();
  }
# endif 
}


# define BOOST_PYTHON_INPLACE_OPERATOR(id, op)                  \
namespace detail                                                \
{                                                               \
  template <>                                                   \
  struct operator_l<op_##id>                                    \
  {                                                             \
      template <class L, class R>                               \
      struct apply                                              \
      {                                                         \
          typedef typename unwrap_wrapper_<L>::type lhs;        \
          typedef typename unwrap_wrapper_<R>::type rhs;        \
          static PyObject*                                      \
          execute(back_reference<lhs&> l, rhs const& r)         \
          {                                                     \
              l.get() op r;                                     \
              return python::incref(l.source().ptr());          \
          }                                                     \
      };                                                        \
      static char const* name() { return "__" #id "__"; }       \
  };                                                            \
}                                                               \
namespace self_ns                                               \
{                                                               \
  template <class R>                                            \
  inline detail::operator_<detail::op_##id,self_t,R>            \
  operator op(self_t const&, R const&)                          \
  {                                                             \
      return detail::operator_<detail::op_##id,self_t,R>();     \
  }                                                             \
}

BOOST_PYTHON_INPLACE_OPERATOR(iadd,+=)
BOOST_PYTHON_INPLACE_OPERATOR(isub,-=)
BOOST_PYTHON_INPLACE_OPERATOR(imul,*=)
BOOST_PYTHON_INPLACE_OPERATOR(idiv,/=)
BOOST_PYTHON_INPLACE_OPERATOR(imod,%=)
BOOST_PYTHON_INPLACE_OPERATOR(ilshift,<<=)
BOOST_PYTHON_INPLACE_OPERATOR(irshift,>>=)
BOOST_PYTHON_INPLACE_OPERATOR(iand,&=)
BOOST_PYTHON_INPLACE_OPERATOR(ixor,^=)
BOOST_PYTHON_INPLACE_OPERATOR(ior,|=)
    
# define BOOST_PYTHON_UNARY_OPERATOR(id, op, func_name)         \
namespace detail                                                \
{                                                               \
  template <>                                                   \
  struct operator_1<op_##id>                                    \
  {                                                             \
      template <class T>                                        \
      struct apply                                              \
      {                                                         \
          typedef typename unwrap_wrapper_<T>::type self_t;     \
          static PyObject* execute(self_t& x)                   \
          {                                                     \
              return detail::convert_result(op(x));             \
          }                                                     \
      };                                                        \
      static char const* name() { return "__" #id "__"; }       \
  };                                                            \
}                                                               \
namespace self_ns                                               \
{                                                               \
  inline detail::operator_<detail::op_##id>                     \
  func_name(self_t const&)                                      \
  {                                                             \
      return detail::operator_<detail::op_##id>();              \
  }                                                             \
}
# undef BOOST_PYTHON_INPLACE_OPERATOR

BOOST_PYTHON_UNARY_OPERATOR(neg, -, operator-)
BOOST_PYTHON_UNARY_OPERATOR(pos, +, operator+)
BOOST_PYTHON_UNARY_OPERATOR(abs, abs, abs)
BOOST_PYTHON_UNARY_OPERATOR(invert, ~, operator~)
#if PY_VERSION_HEX >= 0x03000000
BOOST_PYTHON_UNARY_OPERATOR(bool, !!, operator!)
#else
BOOST_PYTHON_UNARY_OPERATOR(nonzero, !!, operator!)
#endif
BOOST_PYTHON_UNARY_OPERATOR(int, long, int_)
BOOST_PYTHON_UNARY_OPERATOR(long, PyLong_FromLong, long_)
BOOST_PYTHON_UNARY_OPERATOR(float, double, float_)
BOOST_PYTHON_UNARY_OPERATOR(complex, std::complex<double>, complex_)
BOOST_PYTHON_UNARY_OPERATOR(str, lexical_cast<std::string>, str)
BOOST_PYTHON_UNARY_OPERATOR(repr, lexical_cast<std::string>, repr)
# undef BOOST_PYTHON_UNARY_OPERATOR

}} // namespace boost::python

# ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
using boost::python::self_ns::abs;
using boost::python::self_ns::int_;
using boost::python::self_ns::long_;
using boost::python::self_ns::float_;
using boost::python::self_ns::complex_;
using boost::python::self_ns::str;
using boost::python::self_ns::repr;
using boost::python::self_ns::pow;
# endif

#endif // OPERATORS_DWA2002530_HPP
