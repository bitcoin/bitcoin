/* Flyweight class. 
 *
 * Copyright 2006-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_FLYWEIGHT_HPP
#define BOOST_FLYWEIGHT_FLYWEIGHT_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/detail/workaround.hpp>
#include <boost/flyweight/detail/default_value_policy.hpp>
#include <boost/flyweight/detail/flyweight_core.hpp>
#include <boost/flyweight/detail/perfect_fwd.hpp>
#include <boost/flyweight/factory_tag.hpp>
#include <boost/flyweight/flyweight_fwd.hpp>
#include <boost/flyweight/locking_tag.hpp>
#include <boost/flyweight/simple_locking_fwd.hpp>
#include <boost/flyweight/static_holder_fwd.hpp>
#include <boost/flyweight/hashed_factory_fwd.hpp>
#include <boost/flyweight/holder_tag.hpp>
#include <boost/flyweight/refcounted_fwd.hpp>
#include <boost/flyweight/tag.hpp>
#include <boost/flyweight/tracking_tag.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>
#include <boost/parameter/binding.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/swap.hpp>

#if !defined(BOOST_NO_SFINAE)&&!defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <initializer_list>
#endif

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1400))
#pragma warning(push)
#pragma warning(disable:4520)  /* multiple default ctors */
#pragma warning(disable:4521)  /* multiple copy ctors */
#endif

namespace boost{
  
namespace flyweights{

namespace detail{

/* Used for the detection of unmatched template args in a
 * flyweight instantiation.
 */

struct unmatched_arg;

/* Boost.Parameter structures for use in flyweight.
 * NB: these types are derived from instead of typedef'd to force their
 * instantiation, which solves http://bugs.sun.com/view_bug.do?bug_id=6782987
 * as found out by Simon Atanasyan.
 */

struct flyweight_signature:
  parameter::parameters<
    parameter::optional<
      parameter::deduced<tag<> >,
      detail::is_tag<boost::mpl::_>
    >,
    parameter::optional<
      parameter::deduced<tracking<> >,
      is_tracking<boost::mpl::_>
    >,
    parameter::optional<
      parameter::deduced<factory<> >,
      is_factory<boost::mpl::_>
    >,
    parameter::optional<
      parameter::deduced<locking<> >,
      is_locking<boost::mpl::_>
    >,
    parameter::optional<
      parameter::deduced<holder<> >,
      is_holder<boost::mpl::_>
    >
  >
{};

struct flyweight_unmatched_signature:
  parameter::parameters<
    parameter::optional<
      parameter::deduced<
        detail::unmatched_arg
      >,
      mpl::not_<
        mpl::or_<
          detail::is_tag<boost::mpl::_>,
          is_tracking<boost::mpl::_>,
          is_factory<boost::mpl::_>,
          is_locking<boost::mpl::_>,
          is_holder<boost::mpl::_>
        >
      >
    >
  >
{};

} /* namespace flyweights::detail */

template<
  typename T,
  typename Arg1,typename Arg2,typename Arg3,typename Arg4,typename Arg5
>
class flyweight
{
private:
  typedef typename mpl::if_<
    detail::is_value<T>,
    T,
    detail::default_value_policy<T>
  >::type                                 value_policy;
  typedef typename detail::
  flyweight_signature::bind<
    Arg1,Arg2,Arg3,Arg4,Arg5
  >::type                                 args;
  typedef typename parameter::binding<
    args,tag<>,mpl::na
  >::type                                 tag_type;
  typedef typename parameter::binding<
    args,tracking<>,refcounted
  >::type                                 tracking_policy;
  typedef typename parameter::binding<
    args,factory<>,hashed_factory<>
  >::type                                 factory_specifier;
  typedef typename parameter::binding<
    args,locking<>,simple_locking
  >::type                                 locking_policy;
  typedef typename parameter::binding<
    args,holder<>,static_holder
  >::type                                 holder_specifier;

  typedef typename detail::
  flyweight_unmatched_signature::bind<
    Arg1,Arg2,Arg3,Arg4,Arg5
  >::type                                 unmatched_args;
  typedef typename parameter::binding<
    unmatched_args,detail::unmatched_arg,
    detail::unmatched_arg
  >::type                                 unmatched_arg_detected;

  /* You have passed a type in the specification of a flyweight type that
   * could not be interpreted as a valid argument.
   */
  BOOST_MPL_ASSERT_MSG(
  (is_same<unmatched_arg_detected,detail::unmatched_arg>::value),
  INVALID_ARGUMENT_TO_FLYWEIGHT,
  (flyweight));

  typedef detail::flyweight_core<
    value_policy,tag_type,tracking_policy,
    factory_specifier,locking_policy,
    holder_specifier
  >                                            core;
  typedef typename core::handle_type           handle_type;

public:
  typedef typename value_policy::key_type      key_type;
  typedef typename value_policy::value_type    value_type;

  /* static data initialization */

  static bool init(){return core::init();}

  class initializer
  {
  public:
    initializer():b(init()){}
  private:
    bool b;
  };

  /* construct/copy/destroy */

  flyweight():h(core::insert()){}
  
#define BOOST_FLYWEIGHT_PERFECT_FWD_CTR_BODY(args) \
  :h(core::insert(BOOST_FLYWEIGHT_FORWARD(args))){}

  BOOST_FLYWEIGHT_PERFECT_FWD_WITH_ARGS(
    explicit flyweight,
    BOOST_FLYWEIGHT_PERFECT_FWD_CTR_BODY)

#undef BOOST_FLYWEIGHT_PERFECT_FWD_CTR_BODY

#if !defined(BOOST_NO_SFINAE)&&!defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  template<typename V>
  flyweight(
    std::initializer_list<V> list,
    typename boost::enable_if<
      boost::is_convertible<std::initializer_list<V>,key_type> >::type* =0):
    h(core::insert(list)){} 
#endif

  flyweight(const flyweight& x):h(x.h){}
  flyweight(flyweight& x):h(x.h){}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
  flyweight(const flyweight&& x):h(x.h){}
  flyweight(flyweight&& x):h(x.h){}
#endif

#if !defined(BOOST_NO_SFINAE)&&!defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  template<typename V>
  typename boost::enable_if<
    boost::is_convertible<std::initializer_list<V>,key_type>,flyweight&>::type
  operator=(std::initializer_list<V> list)
  {
    return operator=(flyweight(list));
  }
#endif

  flyweight& operator=(const flyweight& x){h=x.h;return *this;}
  flyweight& operator=(const value_type& x){return operator=(flyweight(x));}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
  flyweight& operator=(value_type&& x)
  {
    return operator=(flyweight(std::move(x)));
  }
#endif

  /* convertibility to underlying type */
  
  const key_type&   get_key()const{return core::key(h);}
  const value_type& get()const{return core::value(h);}
  operator const    value_type&()const{return get();}
  
  /* exact type equality  */
    
  friend bool operator==(const flyweight& x,const flyweight& y)
  {
    return &x.get()==&y.get();
  }

  /* modifiers */

  void swap(flyweight& x){boost::swap(h,x.h);}
  
private:
  handle_type h;
};

#define BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(n)            \
typename Arg##n##1,typename Arg##n##2,typename Arg##n##3, \
typename Arg##n##4,typename Arg##n##5
#define BOOST_FLYWEIGHT_TEMPL_ARGS(n) \
Arg##n##1,Arg##n##2,Arg##n##3,Arg##n##4,Arg##n##5

/* Comparison. Unlike exact type comparison defined above, intertype
 * comparison just forwards to the underlying objects.
 */

template<
  typename T1,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(1),
  typename T2,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(2)
>
bool operator==(
  const flyweight<T1,BOOST_FLYWEIGHT_TEMPL_ARGS(1)>& x,
  const flyweight<T2,BOOST_FLYWEIGHT_TEMPL_ARGS(2)>& y)
{
  return x.get()==y.get();
}

template<
  typename T1,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(1),
  typename T2,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(2)
>
bool operator<(
  const flyweight<T1,BOOST_FLYWEIGHT_TEMPL_ARGS(1)>& x,
  const flyweight<T2,BOOST_FLYWEIGHT_TEMPL_ARGS(2)>& y)
{
  return x.get()<y.get();
}

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
template<
  typename T1,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(1),
  typename T2
>
bool operator==(
  const flyweight<T1,BOOST_FLYWEIGHT_TEMPL_ARGS(1)>& x,const T2& y)
{
  return x.get()==y;
}

template<
  typename T1,
  typename T2,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(2)
>
bool operator==(
  const T1& x,const flyweight<T2,BOOST_FLYWEIGHT_TEMPL_ARGS(2)>& y)
{
  return x==y.get();
}

template<
  typename T1,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(1),
  typename T2
>
bool operator<(
  const flyweight<T1,BOOST_FLYWEIGHT_TEMPL_ARGS(1)>& x,const T2& y)
{
  return x.get()<y;
}

template<
  typename T1,
  typename T2,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(2)
>
bool operator<(
  const T1& x,const flyweight<T2,BOOST_FLYWEIGHT_TEMPL_ARGS(2)>& y)
{
  return x<y.get();
}
#endif /* !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) */

/* rest of comparison operators */

#define BOOST_FLYWEIGHT_COMPLETE_COMP_OPS(t,a1,a2)                            \
template<t>                                                                   \
inline bool operator!=(const a1& x,const a2& y)                               \
{                                                                             \
  return !(x==y);                                                             \
}                                                                             \
                                                                              \
template<t>                                                                   \
inline bool operator>(const a1& x,const a2& y)                                \
{                                                                             \
  return y<x;                                                                 \
}                                                                             \
                                                                              \
template<t>                                                                   \
inline bool operator>=(const a1& x,const a2& y)                               \
{                                                                             \
  return !(x<y);                                                              \
}                                                                             \
                                                                              \
template<t>                                                                   \
inline bool operator<=(const a1& x,const a2& y)                               \
{                                                                             \
  return !(y<x);                                                              \
}

BOOST_FLYWEIGHT_COMPLETE_COMP_OPS(
  typename T1 BOOST_PP_COMMA()
  BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(1) BOOST_PP_COMMA()
  typename T2 BOOST_PP_COMMA()
  BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(2),
  flyweight<
    T1 BOOST_PP_COMMA() BOOST_FLYWEIGHT_TEMPL_ARGS(1)
  >,
  flyweight<
    T2 BOOST_PP_COMMA() BOOST_FLYWEIGHT_TEMPL_ARGS(2)
  >)

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
BOOST_FLYWEIGHT_COMPLETE_COMP_OPS(
  typename T1 BOOST_PP_COMMA()
  BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(1) BOOST_PP_COMMA()
  typename T2,
  flyweight<
    T1 BOOST_PP_COMMA() BOOST_FLYWEIGHT_TEMPL_ARGS(1)
  >,
  T2)
  
BOOST_FLYWEIGHT_COMPLETE_COMP_OPS(
  typename T1 BOOST_PP_COMMA()
  typename T2 BOOST_PP_COMMA() 
  BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(2),
  T1,
  flyweight<
    T2 BOOST_PP_COMMA() BOOST_FLYWEIGHT_TEMPL_ARGS(2)
  >)
#endif /* !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING) */

/* specialized algorithms */

template<typename T,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(_)>
void swap(
  flyweight<T,BOOST_FLYWEIGHT_TEMPL_ARGS(_)>& x,
  flyweight<T,BOOST_FLYWEIGHT_TEMPL_ARGS(_)>& y)
{
  x.swap(y);
}

template<
  BOOST_TEMPLATED_STREAM_ARGS(ElemType,Traits)
  BOOST_TEMPLATED_STREAM_COMMA
  typename T,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(_)
>
BOOST_TEMPLATED_STREAM(ostream,ElemType,Traits)& operator<<(
  BOOST_TEMPLATED_STREAM(ostream,ElemType,Traits)& out,
  const flyweight<T,BOOST_FLYWEIGHT_TEMPL_ARGS(_)>& x)
{
  return out<<x.get();
}

template<
  BOOST_TEMPLATED_STREAM_ARGS(ElemType,Traits)
  BOOST_TEMPLATED_STREAM_COMMA
  typename T,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(_)
>
BOOST_TEMPLATED_STREAM(istream,ElemType,Traits)& operator>>(
  BOOST_TEMPLATED_STREAM(istream,ElemType,Traits)& in,
  flyweight<T,BOOST_FLYWEIGHT_TEMPL_ARGS(_)>& x)
{
  typedef typename flyweight<
    T,BOOST_FLYWEIGHT_TEMPL_ARGS(_)
  >::value_type                     value_type;

  /* value_type need not be default ctble but must be copy ctble */
  value_type t(x.get());
  in>>t;
  x=t;
  return in;
}

} /* namespace flyweights */

} /* namespace boost */

#if !defined(BOOST_FLYWEIGHT_DISABLE_HASH_SUPPORT)

/* hash support */

#if !defined(BOOST_NO_CXX11_HDR_FUNCTIONAL)
namespace std{

template<typename T,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(_)>
BOOST_FLYWEIGHT_STD_HASH_STRUCT_KEYWORD
hash<boost::flyweight<T,BOOST_FLYWEIGHT_TEMPL_ARGS(_)> >
{
public:
  typedef std::size_t                result_type;
  typedef boost::flyweight<
    T,BOOST_FLYWEIGHT_TEMPL_ARGS(_)> argument_type;

  result_type operator()(const argument_type& x)const
  {
    typedef typename argument_type::value_type value_type;

    std::hash<const value_type*> h;
    return h(&x.get());
  }
};

} /* namespace std */
#endif /* !defined(BOOST_NO_CXX11_HDR_FUNCTIONAL) */

namespace boost{
#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace flyweights{
#endif

template<typename T,BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS(_)>
std::size_t hash_value(const flyweight<T,BOOST_FLYWEIGHT_TEMPL_ARGS(_)>& x)
{
  typedef typename flyweight<
    T,BOOST_FLYWEIGHT_TEMPL_ARGS(_)
  >::value_type                     value_type;

  boost::hash<const value_type*> h;
  return h(&x.get());
}

#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
} /* namespace flyweights */
#endif
} /* namespace boost */
#endif /* !defined(BOOST_FLYWEIGHT_DISABLE_HASH_SUPPORT) */

#undef BOOST_FLYWEIGHT_COMPLETE_COMP_OPS
#undef BOOST_FLYWEIGHT_TEMPL_ARGS
#undef BOOST_FLYWEIGHT_TYPENAME_TEMPL_ARGS

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1400))
#pragma warning(pop)
#endif

#endif
