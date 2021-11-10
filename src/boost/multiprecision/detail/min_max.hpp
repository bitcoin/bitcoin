///////////////////////////////////////////////////////////////////////////////
//  Copyright 2016 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_MIN_MAX_HPP
#define BOOST_MP_MIN_MAX_HPP

#include <boost/multiprecision/traits/is_backend.hpp>

namespace boost { namespace multiprecision {

//
// Expression template overloads for (min) and (max):
//
// Introduced in response to https://svn.boost.org/trac/boost/ticket/11149
// note that these can not legally be injected into namespace std, and that doing so
// may break future enhancements to the standard.  None the less adding
// namespace std{ using boost::multiprecision::(min); using boost::multiprecision::(max); }
// to your code may get some generic code working that wouldn't work otherwise.
//
// The use of enable_if on the return type is to avoid poisoning std::min/max,
// otherwise attempting to make an explicit call to min<long>(a, b) when these and std
// versions are in scope, will cause the compiler to try to instantiate the signatures
// for our versions as well as the std ones, which in turn instantiates number<long>
// which fails to compile as "long" is not a valid backend type.
//
template <class Backend>
inline typename std::enable_if<boost::multiprecision::detail::is_backend<Backend>::value, const number<Backend, et_on>&>::type(min)(const number<Backend, et_on>& a, const number<Backend, et_on>& b)
{
   return a < b ? a : b;
}
template <class Backend, class tag, class A1, class A2, class A3, class A4>
inline typename std::enable_if<boost::multiprecision::detail::is_backend<Backend>::value, const number<Backend, et_on> >::type(min)(const number<Backend, et_on>& a, const detail::expression<tag, A1, A2, A3, A4>& b)
{
   number<Backend, et_on> t(b);
   if (a < t)
      return a;
   return std::move(t);
}
template <class tag, class A1, class A2, class A3, class A4, class Backend>
inline typename std::enable_if<boost::multiprecision::detail::is_backend<Backend>::value, const number<Backend, et_on> >::type(min)(const detail::expression<tag, A1, A2, A3, A4>& a, const number<Backend, et_on>& b)
{
   number<Backend, et_on> t(a);
   if (t < b)
      return std::move(t);
   return b;
}
template <class tag, class A1, class A2, class A3, class A4, class tagb, class A1b, class A2b, class A3b, class A4b>
inline typename detail::expression<tag, A1, A2, A3, A4>::result_type(min)(const detail::expression<tag, A1, A2, A3, A4>& a, const detail::expression<tagb, A1b, A2b, A3b, A4b>& b)
{
   typename detail::expression<tag, A1, A2, A3, A4>::result_type t1(a), t2(b);
   if (t1 < t2)
      return std::move(t1);
   return std::move(t2);
}
template <class tag, class A1, class A2, class A3, class A4>
inline typename detail::expression<tag, A1, A2, A3, A4>::result_type(min)(const detail::expression<tag, A1, A2, A3, A4>& a, const detail::expression<tag, A1, A2, A3, A4>& b)
{
   typename detail::expression<tag, A1, A2, A3, A4>::result_type t1(a), t2(b);
   if (t1 < t2)
      return std::move(t1);
   return std::move(t2);
}

template <class Backend>
inline typename std::enable_if<boost::multiprecision::detail::is_backend<Backend>::value, const number<Backend, et_on>&>::type(max)(const number<Backend, et_on>& a, const number<Backend, et_on>& b)
{
   return a > b ? a : b;
}
template <class Backend, class tag, class A1, class A2, class A3, class A4>
inline typename std::enable_if<boost::multiprecision::detail::is_backend<Backend>::value, const number<Backend, et_on> >::type(max)(const number<Backend, et_on>& a, const detail::expression<tag, A1, A2, A3, A4>& b)
{
   number<Backend, et_on> t(b);
   if (a > t)
      return a;
   return std::move(t);
}
template <class tag, class A1, class A2, class A3, class A4, class Backend>
inline typename std::enable_if<boost::multiprecision::detail::is_backend<Backend>::value, const number<Backend, et_on> >::type(max)(const detail::expression<tag, A1, A2, A3, A4>& a, const number<Backend, et_on>& b)
{
   number<Backend, et_on> t(a);
   if (t > b)
      return std::move(t);
   return b;
}
template <class tag, class A1, class A2, class A3, class A4, class tagb, class A1b, class A2b, class A3b, class A4b>
inline typename detail::expression<tag, A1, A2, A3, A4>::result_type(max)(const detail::expression<tag, A1, A2, A3, A4>& a, const detail::expression<tagb, A1b, A2b, A3b, A4b>& b)
{
   typename detail::expression<tag, A1, A2, A3, A4>::result_type t1(a), t2(b);
   if (t1 > t2)
      return std::move(t1);
   return std::move(t2);
}
template <class tag, class A1, class A2, class A3, class A4>
inline typename detail::expression<tag, A1, A2, A3, A4>::result_type(max)(const detail::expression<tag, A1, A2, A3, A4>& a, const detail::expression<tag, A1, A2, A3, A4>& b)
{
   typename detail::expression<tag, A1, A2, A3, A4>::result_type t1(a), t2(b);
   if (t1 > t2)
      return std::move(t1);
   return std::move(t2);
}

}} // namespace boost::multiprecision

#endif
