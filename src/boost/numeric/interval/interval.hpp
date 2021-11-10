/* Boost interval/interval.hpp header file
 *
 * Copyright 2002-2003 Hervé Brönnimann, Guillaume Melquiond, Sylvain Pion
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_INTERVAL_INTERVAL_HPP
#define BOOST_NUMERIC_INTERVAL_INTERVAL_HPP

#include <stdexcept>
#include <string>
#include <boost/numeric/interval/detail/interval_prototype.hpp>

namespace boost {
namespace numeric {

namespace interval_lib {
    
class comparison_error
  : public std::runtime_error 
{
public:
  comparison_error()
    : std::runtime_error("boost::interval: uncertain comparison")
  { }
};

} // namespace interval_lib

/*
 * interval class
 */

template<class T, class Policies>
class interval
{
private:
  struct interval_holder;
  struct number_holder;
public:
  typedef T base_type;
  typedef Policies traits_type;

  T const &lower() const;
  T const &upper() const;

  interval();
  interval(T const &v);
  template<class T1> interval(T1 const &v);
  interval(T const &l, T const &u);
  template<class T1, class T2> interval(T1 const &l, T2 const &u);
  interval(interval<T, Policies> const &r);
  template<class Policies1> interval(interval<T, Policies1> const &r);
  template<class T1, class Policies1> interval(interval<T1, Policies1> const &r);

  interval &operator=(T const &v);
  template<class T1> interval &operator=(T1 const &v);
  interval &operator=(interval<T, Policies> const &r);
  template<class Policies1> interval &operator=(interval<T, Policies1> const &r);
  template<class T1, class Policies1> interval &operator=(interval<T1, Policies1> const &r);
 
  void assign(const T& l, const T& u);

  static interval empty();
  static interval whole();
  static interval hull(const T& x, const T& y);

  interval& operator+= (const T& r);
  interval& operator+= (const interval& r);
  interval& operator-= (const T& r);
  interval& operator-= (const interval& r);
  interval& operator*= (const T& r);
  interval& operator*= (const interval& r);
  interval& operator/= (const T& r);
  interval& operator/= (const interval& r);

  bool operator< (const interval_holder& r) const;
  bool operator> (const interval_holder& r) const;
  bool operator<= (const interval_holder& r) const;
  bool operator>= (const interval_holder& r) const;
  bool operator== (const interval_holder& r) const;
  bool operator!= (const interval_holder& r) const;

  bool operator< (const number_holder& r) const;
  bool operator> (const number_holder& r) const;
  bool operator<= (const number_holder& r) const;
  bool operator>= (const number_holder& r) const;
  bool operator== (const number_holder& r) const;
  bool operator!= (const number_holder& r) const;

  // the following is for internal use only, it is not a published interface
  // nevertheless, it's public because friends don't always work correctly.
  interval(const T& l, const T& u, bool): low(l), up(u) {}
  void set_empty();
  void set_whole();
  void set(const T& l, const T& u);

private:
  struct interval_holder {
    template<class Policies2>
    interval_holder(const interval<T, Policies2>& r)
      : low(r.lower()), up(r.upper())
    {
      typedef typename Policies2::checking checking2;
      if (checking2::is_empty(low, up))
        throw interval_lib::comparison_error();
    }

    const T& low;
    const T& up;
  };

  struct number_holder {
    number_holder(const T& r) : val(r)
    {
      typedef typename Policies::checking checking;
      if (checking::is_nan(r))
        throw interval_lib::comparison_error();
    }
    
    const T& val;
  };

  typedef typename Policies::checking checking;
  typedef typename Policies::rounding rounding;

  T low;
  T up;
};

template<class T, class Policies> inline
interval<T, Policies>::interval():
  low(static_cast<T>(0)), up(static_cast<T>(0))
{}

template<class T, class Policies> inline
interval<T, Policies>::interval(T const &v): low(v), up(v)
{
  if (checking::is_nan(v)) set_empty();
}

template<class T, class Policies> template<class T1> inline
interval<T, Policies>::interval(T1 const &v)
{
  if (checking::is_nan(v)) set_empty();
  else {
    rounding rnd;
    low = rnd.conv_down(v);
    up  = rnd.conv_up  (v);
  }
}

template<class T, class Policies> template<class T1, class T2> inline
interval<T, Policies>::interval(T1 const &l, T2 const &u)
{
  if (checking::is_nan(l) || checking::is_nan(u) || !(l <= u)) set_empty();
  else {
    rounding rnd;
    low = rnd.conv_down(l);
    up  = rnd.conv_up  (u);
  }
}

template<class T, class Policies> inline
interval<T, Policies>::interval(T const &l, T const &u): low(l), up(u)
{
  if (checking::is_nan(l) || checking::is_nan(u) || !(l <= u))
    set_empty();
}


template<class T, class Policies> inline
interval<T, Policies>::interval(interval<T, Policies> const &r): low(r.lower()), up(r.upper())
{}

template<class T, class Policies> template<class Policies1> inline
interval<T, Policies>::interval(interval<T, Policies1> const &r): low(r.lower()), up(r.upper())
{
  typedef typename Policies1::checking checking1;
  if (checking1::is_empty(r.lower(), r.upper())) set_empty();
}

template<class T, class Policies> template<class T1, class Policies1> inline
interval<T, Policies>::interval(interval<T1, Policies1> const &r)
{
  typedef typename Policies1::checking checking1;
  if (checking1::is_empty(r.lower(), r.upper())) set_empty();
  else {
    rounding rnd;
    low = rnd.conv_down(r.lower());
    up  = rnd.conv_up  (r.upper());
  }
}

template<class T, class Policies> inline
interval<T, Policies> &interval<T, Policies>::operator=(T const &v)
{
  if (checking::is_nan(v)) set_empty();
  else low = up = v;
  return *this;
}

template<class T, class Policies> template<class T1> inline
interval<T, Policies> &interval<T, Policies>::operator=(T1 const &v)
{
  if (checking::is_nan(v)) set_empty();
  else {
    rounding rnd;
    low = rnd.conv_down(v);
    up  = rnd.conv_up  (v);
  }
  return *this;
}

template<class T, class Policies> inline
interval<T, Policies> &interval<T, Policies>::operator=(interval<T, Policies> const &r)
{
  low = r.lower();
  up  = r.upper();
  return *this;
}

template<class T, class Policies> template<class Policies1> inline
interval<T, Policies> &interval<T, Policies>::operator=(interval<T, Policies1> const &r)
{
  typedef typename Policies1::checking checking1;
  if (checking1::is_empty(r.lower(), r.upper())) set_empty();
  else {
    low = r.lower();
    up  = r.upper();
  }
  return *this;
}

template<class T, class Policies> template<class T1, class Policies1> inline
interval<T, Policies> &interval<T, Policies>::operator=(interval<T1, Policies1> const &r)
{
  typedef typename Policies1::checking checking1;
  if (checking1::is_empty(r.lower(), r.upper())) set_empty();
  else {
    rounding rnd;
    low = rnd.conv_down(r.lower());
    up  = rnd.conv_up  (r.upper());
  }
  return *this;
}

template<class T, class Policies> inline
void interval<T, Policies>::assign(const T& l, const T& u)
{
  if (checking::is_nan(l) || checking::is_nan(u) || !(l <= u))
    set_empty();
  else set(l, u);
}

template<class T, class Policies> inline
void interval<T, Policies>::set(const T& l, const T& u)
{
  low = l;
  up  = u;
}

template<class T, class Policies> inline
void interval<T, Policies>::set_empty()
{
  low = checking::empty_lower();
  up  = checking::empty_upper();
}

template<class T, class Policies> inline
void interval<T, Policies>::set_whole()
{
  low = checking::neg_inf();
  up  = checking::pos_inf();
}

template<class T, class Policies> inline
interval<T, Policies> interval<T, Policies>::hull(const T& x, const T& y)
{
  bool bad_x = checking::is_nan(x);
  bool bad_y = checking::is_nan(y);
  if (bad_x)
    if (bad_y) return interval::empty();
    else       return interval(y, y, true);
  else
    if (bad_y) return interval(x, x, true);
  if (x <= y) return interval(x, y, true);
  else        return interval(y, x, true);
}

template<class T, class Policies> inline
interval<T, Policies> interval<T, Policies>::empty()
{
  return interval<T, Policies>(checking::empty_lower(),
                               checking::empty_upper(), true);
}

template<class T, class Policies> inline
interval<T, Policies> interval<T, Policies>::whole()
{
  return interval<T, Policies>(checking::neg_inf(), checking::pos_inf(), true);
}

template<class T, class Policies> inline
const T& interval<T, Policies>::lower() const
{
  return low;
}

template<class T, class Policies> inline
const T& interval<T, Policies>::upper() const
{
  return up;
}

/*
 * interval/interval comparisons
 */

template<class T, class Policies> inline
bool interval<T, Policies>::operator< (const interval_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (up < r.low) return true;
    else if (low >= r.up) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator> (const interval_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (low > r.up) return true;
    else if (up <= r.low) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator<= (const interval_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (up <= r.low) return true;
    else if (low > r.up) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator>= (const interval_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (low >= r.up) return true;
    else if (up < r.low) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator== (const interval_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (up == r.low && low == r.up) return true;
    else if (up < r.low || low > r.up) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator!= (const interval_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (up < r.low || low > r.up) return true;
    else if (up == r.low && low == r.up) return false;
  }
  throw interval_lib::comparison_error();
}

/*
 * interval/number comparisons
 */

template<class T, class Policies> inline
bool interval<T, Policies>::operator< (const number_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (up < r.val) return true;
    else if (low >= r.val) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator> (const number_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (low > r.val) return true;
    else if (up <= r.val) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator<= (const number_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (up <= r.val) return true;
    else if (low > r.val) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator>= (const number_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (low >= r.val) return true;
    else if (up < r.val) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator== (const number_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (up == r.val && low == r.val) return true;
    else if (up < r.val || low > r.val) return false;
  }
  throw interval_lib::comparison_error();
}

template<class T, class Policies> inline
bool interval<T, Policies>::operator!= (const number_holder& r) const
{
  if (!checking::is_empty(low, up)) {
    if (up < r.val || low > r.val) return true;
    else if (up == r.val && low == r.val) return false;
  }
  throw interval_lib::comparison_error();
}

} // namespace numeric
} // namespace boost

#endif // BOOST_NUMERIC_INTERVAL_INTERVAL_HPP
