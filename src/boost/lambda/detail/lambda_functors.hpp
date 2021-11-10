// Boost Lambda Library -  lambda_functors.hpp -------------------------------

// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

// ------------------------------------------------

#ifndef BOOST_LAMBDA_LAMBDA_FUNCTORS_HPP
#define BOOST_LAMBDA_LAMBDA_FUNCTORS_HPP

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/utility/result_of.hpp>

#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)

#include <boost/mpl/or.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_array.hpp>

#define BOOST_LAMBDA_DISABLE_IF_ARRAY1(A1, R1)\
  typename lazy_disable_if<is_array<A1>, typename R1 >::type
#define BOOST_LAMBDA_DISABLE_IF_ARRAY2(A1, A2, R1, R2) \
  typename lazy_disable_if<mpl::or_<is_array<A1>, is_array<A2> >, typename R1, R2 >::type
#define BOOST_LAMBDA_DISABLE_IF_ARRAY3(A1, A2, A3, R1, R2, R3) \
  typename lazy_disable_if<mpl::or_<is_array<A1>, is_array<A2>, is_array<A3> >, typename R1, R2, R3 >::type

#else

#define BOOST_LAMBDA_DISABLE_IF_ARRAY1(A1, R1) typename R1::type
#define BOOST_LAMBDA_DISABLE_IF_ARRAY2(A1, A2, R1, R2) typename R1, R2::type
#define BOOST_LAMBDA_DISABLE_IF_ARRAY3(A1, A2, A3, R1, R2, R3) typename R1, R2, R3::type

#endif

namespace boost { 
namespace lambda {

// -- lambda_functor --------------------------------------------
// --------------------------------------------------------------

//inline const null_type const_null_type() { return null_type(); }

namespace detail {
namespace {

  static const null_type constant_null_type = null_type();

} // unnamed
} // detail

class unused {};

#define cnull_type() detail::constant_null_type

// -- free variables types -------------------------------------------------- 
 
  // helper to work around the case where the nullary return type deduction 
  // is always performed, even though the functor is not nullary  
namespace detail {
  template<int N, class Tuple> struct get_element_or_null_type {
    typedef typename 
      detail::tuple_element_as_reference<N, Tuple>::type type;
  };
  template<int N> struct get_element_or_null_type<N, null_type> {
    typedef null_type type;
  };
}

template <int I> struct placeholder;

template<> struct placeholder<FIRST> {

  template<class SigArgs> struct sig {
    typedef typename detail::get_element_or_null_type<0, SigArgs>::type type;
  };

  template<class RET, CALL_TEMPLATE_ARGS> 
  RET call(CALL_FORMAL_ARGS) const { 
    BOOST_STATIC_ASSERT(boost::is_reference<RET>::value); 
    CALL_USE_ARGS; // does nothing, prevents warnings for unused args
    return a; 
  }
};

template<> struct placeholder<SECOND> {

  template<class SigArgs> struct sig {
    typedef typename detail::get_element_or_null_type<1, SigArgs>::type type;
  };

  template<class RET, CALL_TEMPLATE_ARGS> 
  RET call(CALL_FORMAL_ARGS) const { CALL_USE_ARGS; return b; }
};

template<> struct placeholder<THIRD> {

  template<class SigArgs> struct sig {
    typedef typename detail::get_element_or_null_type<2, SigArgs>::type type;
  };

  template<class RET, CALL_TEMPLATE_ARGS> 
  RET call(CALL_FORMAL_ARGS) const { CALL_USE_ARGS; return c; }
};

template<> struct placeholder<EXCEPTION> {

  template<class SigArgs> struct sig {
    typedef typename detail::get_element_or_null_type<3, SigArgs>::type type;
  };

  template<class RET, CALL_TEMPLATE_ARGS> 
  RET call(CALL_FORMAL_ARGS) const { CALL_USE_ARGS; return env; }
};
   
typedef const lambda_functor<placeholder<FIRST> >  placeholder1_type;
typedef const lambda_functor<placeholder<SECOND> > placeholder2_type;
typedef const lambda_functor<placeholder<THIRD> >  placeholder3_type;
   

///////////////////////////////////////////////////////////////////////////////


// free variables are lambda_functors. This is to allow uniform handling with 
// other lambda_functors.
// -------------------------------------------------------------------

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

// -- lambda_functor NONE ------------------------------------------------
template <class T>
class lambda_functor : public T 
{

BOOST_STATIC_CONSTANT(int, arity_bits = get_arity<T>::value);
 
public:
  typedef T inherited;

  lambda_functor() {}
  lambda_functor(const lambda_functor& l) : inherited(l) {}

  lambda_functor(const T& t) : inherited(t) {}

  template <class SigArgs> struct sig {
    typedef typename inherited::template 
      sig<typename SigArgs::tail_type>::type type;
  };

  // Note that this return type deduction template is instantiated, even 
  // if the nullary 
  // operator() is not called at all. One must make sure that it does not fail.
  typedef typename 
    inherited::template sig<null_type>::type
      nullary_return_type;

  // Support for boost::result_of.
  template <class Sig> struct result;
  template <class F>
  struct result<F()> {
    typedef nullary_return_type type;
  };
  template <class F, class A>
  struct result<F(A)> {
    typedef typename sig<tuple<F, A> >::type type;
  };
  template <class F, class A, class B>
  struct result<F(A, B)> {
    typedef typename sig<tuple<F, A, B> >::type type;
  };
  template <class F, class A, class B, class C>
  struct result<F(A, B, C)> {
    typedef typename sig<tuple<F, A, B, C> >::type type;
  };

  nullary_return_type operator()() const { 
    return inherited::template 
      call<nullary_return_type>
        (cnull_type(), cnull_type(), cnull_type(), cnull_type()); 
  }

  template<class A>
  typename inherited::template sig<tuple<A&> >::type
  operator()(A& a) const { 
    return inherited::template call<
      typename inherited::template sig<tuple<A&> >::type
    >(a, cnull_type(), cnull_type(), cnull_type());
  }

  template<class A>
  BOOST_LAMBDA_DISABLE_IF_ARRAY1(A, inherited::template sig<tuple<A const&> >)
  operator()(A const& a) const { 
    return inherited::template call<
      typename inherited::template sig<tuple<A const&> >::type
    >(a, cnull_type(), cnull_type(), cnull_type());
  }

  template<class A, class B>
  typename inherited::template sig<tuple<A&, B&> >::type
  operator()(A& a, B& b) const { 
    return inherited::template call<
      typename inherited::template sig<tuple<A&, B&> >::type
    >(a, b, cnull_type(), cnull_type()); 
  }

  template<class A, class B>
  BOOST_LAMBDA_DISABLE_IF_ARRAY2(A, B, inherited::template sig<tuple<A const&, B&> >)
  operator()(A const& a, B& b) const { 
    return inherited::template call<
      typename inherited::template sig<tuple<A const&, B&> >::type
    >(a, b, cnull_type(), cnull_type()); 
  }

  template<class A, class B>
  BOOST_LAMBDA_DISABLE_IF_ARRAY2(A, B, inherited::template sig<tuple<A&, B const&> >)
  operator()(A& a, B const& b) const { 
    return inherited::template call<
      typename inherited::template sig<tuple<A&, B const&> >::type
    >(a, b, cnull_type(), cnull_type()); 
  }

  template<class A, class B>
  BOOST_LAMBDA_DISABLE_IF_ARRAY2(A, B, inherited::template sig<tuple<A const&, B const&> >)
  operator()(A const& a, B const& b) const { 
    return inherited::template call<
      typename inherited::template sig<tuple<A const&, B const&> >::type
    >(a, b, cnull_type(), cnull_type()); 
  }

  template<class A, class B, class C>
  typename inherited::template sig<tuple<A&, B&, C&> >::type
  operator()(A& a, B& b, C& c) const
  { 
    return inherited::template call<
      typename inherited::template sig<tuple<A&, B&, C&> >::type
    >(a, b, c, cnull_type()); 
  }

  template<class A, class B, class C>
  BOOST_LAMBDA_DISABLE_IF_ARRAY3(A, B, C, inherited::template sig<tuple<A const&, B const&, C const&> >)
  operator()(A const& a, B const& b, C const& c) const
  { 
    return inherited::template call<
      typename inherited::template sig<tuple<A const&, B const&, C const&> >::type
    >(a, b, c, cnull_type()); 
  }

  // for internal calls with env
  template<CALL_TEMPLATE_ARGS>
  typename inherited::template sig<tuple<CALL_REFERENCE_TYPES> >::type
  internal_call(CALL_FORMAL_ARGS) const { 
     return inherited::template 
       call<typename inherited::template 
         sig<tuple<CALL_REFERENCE_TYPES> >::type>(CALL_ACTUAL_ARGS); 
  }

  template<class A>
  const lambda_functor<lambda_functor_base<
                  other_action<assignment_action>,
                  boost::tuple<lambda_functor,
                  typename const_copy_argument <const A>::type> > >
  operator=(const A& a) const {
    return lambda_functor_base<
                  other_action<assignment_action>,
                  boost::tuple<lambda_functor,
                  typename const_copy_argument <const A>::type> >
     (  boost::tuple<lambda_functor,
             typename const_copy_argument <const A>::type>(*this, a) );
  }

  template<class A> 
  const lambda_functor<lambda_functor_base< 
                  other_action<subscript_action>, 
                  boost::tuple<lambda_functor, 
                        typename const_copy_argument <const A>::type> > > 
  operator[](const A& a) const { 
    return lambda_functor_base< 
                  other_action<subscript_action>, 
                  boost::tuple<lambda_functor, 
                        typename const_copy_argument <const A>::type> >
     ( boost::tuple<lambda_functor, 
             typename const_copy_argument <const A>::type>(*this, a ) ); 
  } 
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace lambda
} // namespace boost

namespace boost {

#if !defined(BOOST_RESULT_OF_USE_DECLTYPE) || defined(BOOST_NO_CXX11_DECLTYPE)

template<class T>
struct result_of<boost::lambda::lambda_functor<T>()>
{
    typedef typename boost::lambda::lambda_functor<T>::nullary_return_type type;
};

template<class T>
struct result_of<const boost::lambda::lambda_functor<T>()>
{
    typedef typename boost::lambda::lambda_functor<T>::nullary_return_type type;
};

#endif

template<class T>
struct tr1_result_of<boost::lambda::lambda_functor<T>()>
{
    typedef typename boost::lambda::lambda_functor<T>::nullary_return_type type;
};

template<class T>
struct tr1_result_of<const boost::lambda::lambda_functor<T>()>
{
    typedef typename boost::lambda::lambda_functor<T>::nullary_return_type type;
};

}

// is_placeholder

#include <boost/is_placeholder.hpp>

namespace boost
{

template<> struct is_placeholder< lambda::lambda_functor< lambda::placeholder<lambda::FIRST> > >
{
    enum _vt { value = 1 };
};

template<> struct is_placeholder< lambda::lambda_functor< lambda::placeholder<lambda::SECOND> > >
{
    enum _vt { value = 2 };
};

template<> struct is_placeholder< lambda::lambda_functor< lambda::placeholder<lambda::THIRD> > >
{
    enum _vt { value = 3 };
};

} // namespace boost

#endif
