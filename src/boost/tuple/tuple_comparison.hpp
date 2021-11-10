// tuple_comparison.hpp -----------------------------------------------------
//
// Copyright (C) 2001 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
// Copyright (C) 2001 Gary Powell (gary.powell@sierra.com)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
//
// (The idea and first impl. of comparison operators was from Doug Gregor)

// -----------------------------------------------------------------

#ifndef BOOST_TUPLE_COMPARISON_HPP
#define BOOST_TUPLE_COMPARISON_HPP

#include <boost/tuple/tuple.hpp>

// -------------------------------------------------------------
// equality and comparison operators
//
// == and != compare tuples elementwise
// <, >, <= and >= use lexicographical ordering
//
// Any operator between tuples of different length fails at compile time
// No dependencies between operators are assumed
// (i.e. !(a<b)  does not imply a>=b, a!=b does not imply a==b etc.
// so any weirdnesses of elementary operators are respected).
//
// -------------------------------------------------------------


namespace boost {
namespace tuples {

inline bool operator==(const null_type&, const null_type&) { return true; }
inline bool operator>=(const null_type&, const null_type&) { return true; }
inline bool operator<=(const null_type&, const null_type&) { return true; }
inline bool operator!=(const null_type&, const null_type&) { return false; }
inline bool operator<(const null_type&, const null_type&) { return false; }
inline bool operator>(const null_type&, const null_type&) { return false; }


namespace detail {
  // comparison operators check statically the length of its operands and
  // delegate the comparing task to the following functions. Hence
  // the static check is only made once (should help the compiler).
  // These functions assume tuples to be of the same length.


template<class T1, class T2>
inline bool eq(const T1& lhs, const T2& rhs) {
  return lhs.get_head() == rhs.get_head() &&
         eq(lhs.get_tail(), rhs.get_tail());
}
template<>
inline bool eq<null_type,null_type>(const null_type&, const null_type&) { return true; }

template<class T1, class T2>
inline bool neq(const T1& lhs, const T2& rhs) {
  return lhs.get_head() != rhs.get_head()  ||
         neq(lhs.get_tail(), rhs.get_tail());
}
template<>
inline bool neq<null_type,null_type>(const null_type&, const null_type&) { return false; }

template<class T1, class T2>
inline bool lt(const T1& lhs, const T2& rhs) {
  return lhs.get_head() < rhs.get_head()  ||
          ( !(rhs.get_head() < lhs.get_head()) &&
            lt(lhs.get_tail(), rhs.get_tail()));
}
template<>
inline bool lt<null_type,null_type>(const null_type&, const null_type&) { return false; }

template<class T1, class T2>
inline bool gt(const T1& lhs, const T2& rhs) {
  return lhs.get_head() > rhs.get_head()  ||
          ( !(rhs.get_head() > lhs.get_head()) &&
            gt(lhs.get_tail(), rhs.get_tail()));
}
template<>
inline bool gt<null_type,null_type>(const null_type&, const null_type&) { return false; }

template<class T1, class T2>
inline bool lte(const T1& lhs, const T2& rhs) {
  return lhs.get_head() <= rhs.get_head()  &&
          ( !(rhs.get_head() <= lhs.get_head()) ||
            lte(lhs.get_tail(), rhs.get_tail()));
}
template<>
inline bool lte<null_type,null_type>(const null_type&, const null_type&) { return true; }

template<class T1, class T2>
inline bool gte(const T1& lhs, const T2& rhs) {
  return lhs.get_head() >= rhs.get_head()  &&
          ( !(rhs.get_head() >= lhs.get_head()) ||
            gte(lhs.get_tail(), rhs.get_tail()));
}
template<>
inline bool gte<null_type,null_type>(const null_type&, const null_type&) { return true; }

} // end of namespace detail


// equal ----

template<class T1, class T2, class S1, class S2>
inline bool operator==(const cons<T1, T2>& lhs, const cons<S1, S2>& rhs)
{
  // check that tuple lengths are equal
  BOOST_STATIC_ASSERT(length<T2>::value == length<S2>::value);

  return  detail::eq(lhs, rhs);
}

// not equal -----

template<class T1, class T2, class S1, class S2>
inline bool operator!=(const cons<T1, T2>& lhs, const cons<S1, S2>& rhs)
{

  // check that tuple lengths are equal
  BOOST_STATIC_ASSERT(length<T2>::value == length<S2>::value);

  return detail::neq(lhs, rhs);
}

// <
template<class T1, class T2, class S1, class S2>
inline bool operator<(const cons<T1, T2>& lhs, const cons<S1, S2>& rhs)
{
  // check that tuple lengths are equal
  BOOST_STATIC_ASSERT(length<T2>::value == length<S2>::value);

  return detail::lt(lhs, rhs);
}

// >
template<class T1, class T2, class S1, class S2>
inline bool operator>(const cons<T1, T2>& lhs, const cons<S1, S2>& rhs)
{
  // check that tuple lengths are equal
  BOOST_STATIC_ASSERT(length<T2>::value == length<S2>::value);

  return detail::gt(lhs, rhs);
}

// <=
template<class T1, class T2, class S1, class S2>
inline bool operator<=(const cons<T1, T2>& lhs, const cons<S1, S2>& rhs)
{
  // check that tuple lengths are equal
  BOOST_STATIC_ASSERT(length<T2>::value == length<S2>::value);

  return detail::lte(lhs, rhs);
}

// >=
template<class T1, class T2, class S1, class S2>
inline bool operator>=(const cons<T1, T2>& lhs, const cons<S1, S2>& rhs)
{
  // check that tuple lengths are equal
  BOOST_STATIC_ASSERT(length<T2>::value == length<S2>::value);

  return detail::gte(lhs, rhs);
}

} // end of namespace tuples
} // end of namespace boost


#endif // BOOST_TUPLE_COMPARISON_HPP
