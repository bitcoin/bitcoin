//  (C) Copyright John Maddock 2006.
//  (C) Copyright Jeremy William Murphy 2015.


//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_POLYNOMIAL_HPP
#define BOOST_MATH_TOOLS_POLYNOMIAL_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/assert.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/tools/cxx03_warn.hpp>
#include <boost/math/tools/rational.hpp>
#include <boost/math/tools/real_cast.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/binomial.hpp>
#include <boost/math/tools/detail/is_const_iterable.hpp>

#include <vector>
#include <ostream>
#include <algorithm>
#include <initializer_list>
#include <type_traits>

namespace boost{ namespace math{ namespace tools{

template <class T>
T chebyshev_coefficient(unsigned n, unsigned m)
{
   BOOST_MATH_STD_USING
   if(m > n)
      return 0;
   if((n & 1) != (m & 1))
      return 0;
   if(n == 0)
      return 1;
   T result = T(n) / 2;
   unsigned r = n - m;
   r /= 2;

   BOOST_MATH_ASSERT(n - 2 * r == m);

   if(r & 1)
      result = -result;
   result /= n - r;
   result *= boost::math::binomial_coefficient<T>(n - r, r);
   result *= ldexp(1.0f, m);
   return result;
}

template <class Seq>
Seq polynomial_to_chebyshev(const Seq& s)
{
   // Converts a Polynomial into Chebyshev form:
   typedef typename Seq::value_type value_type;
   typedef typename Seq::difference_type difference_type;
   Seq result(s);
   difference_type order = s.size() - 1;
   difference_type even_order = order & 1 ? order - 1 : order;
   difference_type odd_order = order & 1 ? order : order - 1;

   for(difference_type i = even_order; i >= 0; i -= 2)
   {
      value_type val = s[i];
      for(difference_type k = even_order; k > i; k -= 2)
      {
         val -= result[k] * chebyshev_coefficient<value_type>(static_cast<unsigned>(k), static_cast<unsigned>(i));
      }
      val /= chebyshev_coefficient<value_type>(static_cast<unsigned>(i), static_cast<unsigned>(i));
      result[i] = val;
   }
   result[0] *= 2;

   for(difference_type i = odd_order; i >= 0; i -= 2)
   {
      value_type val = s[i];
      for(difference_type k = odd_order; k > i; k -= 2)
      {
         val -= result[k] * chebyshev_coefficient<value_type>(static_cast<unsigned>(k), static_cast<unsigned>(i));
      }
      val /= chebyshev_coefficient<value_type>(static_cast<unsigned>(i), static_cast<unsigned>(i));
      result[i] = val;
   }
   return result;
}

template <class Seq, class T>
T evaluate_chebyshev(const Seq& a, const T& x)
{
   // Clenshaw's formula:
   typedef typename Seq::difference_type difference_type;
   T yk2 = 0;
   T yk1 = 0;
   T yk = 0;
   for(difference_type i = a.size() - 1; i >= 1; --i)
   {
      yk2 = yk1;
      yk1 = yk;
      yk = 2 * x * yk1 - yk2 + a[i];
   }
   return a[0] / 2 + yk * x - yk1;
}


template <typename T>
class polynomial;

namespace detail {

/**
* Knuth, The Art of Computer Programming: Volume 2, Third edition, 1998
* Chapter 4.6.1, Algorithm D: Division of polynomials over a field.
*
* @tparam  T   Coefficient type, must be not be an integer.
*
* Template-parameter T actually must be a field but we don't currently have that
* subtlety of distinction.
*/
template <typename T, typename N>
typename std::enable_if<!std::numeric_limits<T>::is_integer, void >::type
division_impl(polynomial<T> &q, polynomial<T> &u, const polynomial<T>& v, N n, N k)
{
    q[k] = u[n + k] / v[n];
    for (N j = n + k; j > k;)
    {
        j--;
        u[j] -= q[k] * v[j - k];
    }
}

template <class T, class N>
T integer_power(T t, N n)
{
   switch(n)
   {
   case 0:
      return static_cast<T>(1u);
   case 1:
      return t;
   case 2:
      return t * t;
   case 3:
      return t * t * t;
   }
   T result = integer_power(t, n / 2);
   result *= result;
   if(n & 1)
      result *= t;
   return result;
}


/**
* Knuth, The Art of Computer Programming: Volume 2, Third edition, 1998
* Chapter 4.6.1, Algorithm R: Pseudo-division of polynomials.
*
* @tparam  T   Coefficient type, must be an integer.
*
* Template-parameter T actually must be a unique factorization domain but we
* don't currently have that subtlety of distinction.
*/
template <typename T, typename N>
typename std::enable_if<std::numeric_limits<T>::is_integer, void >::type
division_impl(polynomial<T> &q, polynomial<T> &u, const polynomial<T>& v, N n, N k)
{
    q[k] = u[n + k] * integer_power(v[n], k);
    for (N j = n + k; j > 0;)
    {
        j--;
        u[j] = v[n] * u[j] - (j < k ? T(0) : u[n + k] * v[j - k]);
    }
}


/**
 * Knuth, The Art of Computer Programming: Volume 2, Third edition, 1998
 * Chapter 4.6.1, Algorithm D and R: Main loop.
 *
 * @param   u   Dividend.
 * @param   v   Divisor.
 */
template <typename T>
std::pair< polynomial<T>, polynomial<T> >
division(polynomial<T> u, const polynomial<T>& v)
{
    BOOST_MATH_ASSERT(v.size() <= u.size());
    BOOST_MATH_ASSERT(v);
    BOOST_MATH_ASSERT(u);

    typedef typename polynomial<T>::size_type N;

    N const m = u.size() - 1, n = v.size() - 1;
    N k = m - n;
    polynomial<T> q;
    q.data().resize(m - n + 1);

    do
    {
        division_impl(q, u, v, n, k);
    }
    while (k-- != 0);
    u.data().resize(n);
    u.normalize(); // Occasionally, the remainder is zeroes.
    return std::make_pair(q, u);
}

//
// These structures are the same as the void specializations of the functors of the same name
// in the std lib from C++14 onwards:
//
struct negate
{
   template <class T>
   T operator()(T const &x) const
   {
      return -x;
   }
};

struct plus
{
   template <class T, class U>
   T operator()(T const &x, U const& y) const
   {
      return x + y;
   }
};

struct minus
{
   template <class T, class U>
   T operator()(T const &x, U const& y) const
   {
      return x - y;
   }
};

} // namespace detail

/**
 * Returns the zero element for multiplication of polynomials.
 */
template <class T>
polynomial<T> zero_element(std::multiplies< polynomial<T> >)
{
    return polynomial<T>();
}

template <class T>
polynomial<T> identity_element(std::multiplies< polynomial<T> >)
{
    return polynomial<T>(T(1));
}

/* Calculates a / b and a % b, returning the pair (quotient, remainder) together
 * because the same amount of computation yields both.
 * This function is not defined for division by zero: user beware.
 */
template <typename T>
std::pair< polynomial<T>, polynomial<T> >
quotient_remainder(const polynomial<T>& dividend, const polynomial<T>& divisor)
{
    BOOST_MATH_ASSERT(divisor);
    if (dividend.size() < divisor.size())
        return std::make_pair(polynomial<T>(), dividend);
    return detail::division(dividend, divisor);
}


template <class T>
class polynomial
{
public:
   // typedefs:
   typedef typename std::vector<T>::value_type value_type;
   typedef typename std::vector<T>::size_type size_type;

   // construct:
   polynomial(){}

   template <class U>
   polynomial(const U* data, unsigned order)
      : m_data(data, data + order + 1)
   {
       normalize();
   }

   template <class I>
   polynomial(I first, I last)
   : m_data(first, last)
   {
       normalize();
   }

   polynomial(std::vector<T>&& p) : m_data(std::move(p))
   {
      normalize();
   }

   template <class U, typename std::enable_if<std::is_convertible<U, T>::value, bool>::type = true>
   explicit polynomial(const U& point)
   {
       if (point != U(0))
          m_data.push_back(point);
   }

   // move:
   polynomial(polynomial&& p) noexcept
      : m_data(std::move(p.m_data)) { }

   // copy:
   polynomial(const polynomial& p)
      : m_data(p.m_data) { }

   template <class U>
   polynomial(const polynomial<U>& p)
   {
      m_data.resize(p.size());
      for(unsigned i = 0; i < p.size(); ++i)
      {
         m_data[i] = boost::math::tools::real_cast<T>(p[i]);
      }
   }
#ifdef BOOST_MATH_HAS_IS_CONST_ITERABLE
    template <class Range, typename std::enable_if<boost::math::tools::detail::is_const_iterable<Range>::value, bool>::type = true>
    explicit polynomial(const Range& r) 
       : polynomial(r.begin(), r.end()) 
    {
    }
#endif
    polynomial(std::initializer_list<T> l) : polynomial(std::begin(l), std::end(l))
    {
    }

    polynomial&
    operator=(std::initializer_list<T> l)
    {
        m_data.assign(std::begin(l), std::end(l));
        normalize();
        return *this;
    }


   // access:
   size_type size() const { return m_data.size(); }
   size_type degree() const
   {
       if (size() == 0)
           throw std::logic_error("degree() is undefined for the zero polynomial.");
       return m_data.size() - 1;
   }
   value_type& operator[](size_type i)
   {
      return m_data[i];
   }
   const value_type& operator[](size_type i) const
   {
      return m_data[i];
   }

   T evaluate(T z) const
   {
      return this->operator()(z);
   }

   T operator()(T z) const
   {
      return m_data.size() > 0 ? boost::math::tools::evaluate_polynomial(&m_data[0], z, m_data.size()) : T(0);
   }
   std::vector<T> chebyshev() const
   {
      return polynomial_to_chebyshev(m_data);
   }

   std::vector<T> const& data() const
   {
       return m_data;
   }

   std::vector<T> & data()
   {
       return m_data;
   }

   polynomial<T> prime() const
   {
#ifdef _MSC_VER
      // Disable int->float conversion warning:
#pragma warning(push)
#pragma warning(disable:4244)
#endif
      if (m_data.size() == 0)
      {
        return polynomial<T>({});
      }

      std::vector<T> p_data(m_data.size() - 1);
      for (size_t i = 0; i < p_data.size(); ++i) {
          p_data[i] = m_data[i+1]*static_cast<T>(i+1);
      }
      return polynomial<T>(std::move(p_data));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
   }

   polynomial<T> integrate() const
   {
      std::vector<T> i_data(m_data.size() + 1);
      // Choose integration constant such that P(0) = 0.
      i_data[0] = T(0);
      for (size_t i = 1; i < i_data.size(); ++i)
      {
          i_data[i] = m_data[i-1]/static_cast<T>(i);
      }
      return polynomial<T>(std::move(i_data));
   }

   // operators:
   polynomial& operator =(polynomial&& p) noexcept
   {
       m_data = std::move(p.m_data);
       return *this;
   }

   polynomial& operator =(const polynomial& p)
   {
       m_data = p.m_data;
       return *this;
   }

   template <class U>
   typename std::enable_if<std::is_constructible<T, U>::value, polynomial&>::type operator +=(const U& value)
   {
       addition(value);
       normalize();
       return *this;
   }

   template <class U>
   typename std::enable_if<std::is_constructible<T, U>::value, polynomial&>::type operator -=(const U& value)
   {
       subtraction(value);
       normalize();
       return *this;
   }

   template <class U>
   typename std::enable_if<std::is_constructible<T, U>::value, polynomial&>::type operator *=(const U& value)
   {
      multiplication(value);
      normalize();
      return *this;
   }

   template <class U>
   typename std::enable_if<std::is_constructible<T, U>::value, polynomial&>::type operator /=(const U& value)
   {
       division(value);
       normalize();
       return *this;
   }

   template <class U>
   typename std::enable_if<std::is_constructible<T, U>::value, polynomial&>::type operator %=(const U& /*value*/)
   {
       // We can always divide by a scalar, so there is no remainder:
       this->set_zero();
       return *this;
   }

   template <class U>
   polynomial& operator +=(const polynomial<U>& value)
   {
      addition(value);
      normalize();
      return *this;
   }

   template <class U>
   polynomial& operator -=(const polynomial<U>& value)
   {
       subtraction(value);
       normalize();
       return *this;
   }

   template <typename U, typename V>
   void multiply(const polynomial<U>& a, const polynomial<V>& b) {
       if (!a || !b)
       {
           this->set_zero();
           return;
       }
       std::vector<T> prod(a.size() + b.size() - 1, T(0));
       for (unsigned i = 0; i < a.size(); ++i)
           for (unsigned j = 0; j < b.size(); ++j)
               prod[i+j] += a.m_data[i] * b.m_data[j];
       m_data.swap(prod);
   }

   template <class U>
   polynomial& operator *=(const polynomial<U>& value)
   {
      this->multiply(*this, value);
      return *this;
   }

   template <typename U>
   polynomial& operator /=(const polynomial<U>& value)
   {
       *this = quotient_remainder(*this, value).first;
       return *this;
   }

   template <typename U>
   polynomial& operator %=(const polynomial<U>& value)
   {
       *this = quotient_remainder(*this, value).second;
       return *this;
   }

   template <typename U>
   polynomial& operator >>=(U const &n)
   {
       BOOST_MATH_ASSERT(n <= m_data.size());
       m_data.erase(m_data.begin(), m_data.begin() + n);
       return *this;
   }

   template <typename U>
   polynomial& operator <<=(U const &n)
   {
       m_data.insert(m_data.begin(), n, static_cast<T>(0));
       normalize();
       return *this;
   }

   // Convenient and efficient query for zero.
   bool is_zero() const
   {
       return m_data.empty();
   }

   // Conversion to bool.
   inline explicit operator bool() const
   {
       return !m_data.empty();
   }

   // Fast way to set a polynomial to zero.
   void set_zero()
   {
       m_data.clear();
   }

    /** Remove zero coefficients 'from the top', that is for which there are no
    *        non-zero coefficients of higher degree. */
   void normalize()
   {
      m_data.erase(std::find_if(m_data.rbegin(), m_data.rend(), [](const T& x)->bool { return x != T(0); }).base(), m_data.end());
   }

private:
    template <class U, class R>
    polynomial& addition(const U& value, R op)
    {
        if(m_data.size() == 0)
            m_data.resize(1, 0);
        m_data[0] = op(m_data[0], value);
        return *this;
    }

    template <class U>
    polynomial& addition(const U& value)
    {
        return addition(value, detail::plus());
    }

    template <class U>
    polynomial& subtraction(const U& value)
    {
        return addition(value, detail::minus());
    }

    template <class U, class R>
    polynomial& addition(const polynomial<U>& value, R op)
    {
        if (m_data.size() < value.size())
            m_data.resize(value.size(), 0);
        for(size_type i = 0; i < value.size(); ++i)
            m_data[i] = op(m_data[i], value[i]);
        return *this;
    }

    template <class U>
    polynomial& addition(const polynomial<U>& value)
    {
        return addition(value, detail::plus());
    }

    template <class U>
    polynomial& subtraction(const polynomial<U>& value)
    {
        return addition(value, detail::minus());
    }

    template <class U>
    polynomial& multiplication(const U& value)
    {
       std::transform(m_data.begin(), m_data.end(), m_data.begin(), [&](const T& x)->T { return x * value; });
       return *this;
    }

    template <class U>
    polynomial& division(const U& value)
    {
       std::transform(m_data.begin(), m_data.end(), m_data.begin(), [&](const T& x)->T { return x / value; });
       return *this;
    }

    std::vector<T> m_data;
};


template <class T>
inline polynomial<T> operator + (const polynomial<T>& a, const polynomial<T>& b)
{
   polynomial<T> result(a);
   result += b;
   return result;
}

template <class T>
inline polynomial<T> operator + (polynomial<T>&& a, const polynomial<T>& b)
{
   a += b;
   return a;
}
template <class T>
inline polynomial<T> operator + (const polynomial<T>& a, polynomial<T>&& b)
{
   b += a;
   return b;
}
template <class T>
inline polynomial<T> operator + (polynomial<T>&& a, polynomial<T>&& b)
{
   a += b;
   return a;
}

template <class T>
inline polynomial<T> operator - (const polynomial<T>& a, const polynomial<T>& b)
{
   polynomial<T> result(a);
   result -= b;
   return result;
}

template <class T>
inline polynomial<T> operator - (polynomial<T>&& a, const polynomial<T>& b)
{
   a -= b;
   return a;
}
template <class T>
inline polynomial<T> operator - (const polynomial<T>& a, polynomial<T>&& b)
{
   b -= a;
   return -b;
}
template <class T>
inline polynomial<T> operator - (polynomial<T>&& a, polynomial<T>&& b)
{
   a -= b;
   return a;
}

template <class T>
inline polynomial<T> operator * (const polynomial<T>& a, const polynomial<T>& b)
{
   polynomial<T> result;
   result.multiply(a, b);
   return result;
}

template <class T>
inline polynomial<T> operator / (const polynomial<T>& a, const polynomial<T>& b)
{
   return quotient_remainder(a, b).first;
}

template <class T>
inline polynomial<T> operator % (const polynomial<T>& a, const polynomial<T>& b)
{
   return quotient_remainder(a, b).second;
}

template <class T, class U>
inline typename std::enable_if<std::is_constructible<T, U>::value, polynomial<T> >::type operator + (polynomial<T> a, const U& b)
{
   a += b;
   return a;
}

template <class T, class U>
inline typename std::enable_if<std::is_constructible<T, U>::value, polynomial<T> >::type operator - (polynomial<T> a, const U& b)
{
   a -= b;
   return a;
}

template <class T, class U>
inline typename std::enable_if<std::is_constructible<T, U>::value, polynomial<T> >::type operator * (polynomial<T> a, const U& b)
{
   a *= b;
   return a;
}

template <class T, class U>
inline typename std::enable_if<std::is_constructible<T, U>::value, polynomial<T> >::type operator / (polynomial<T> a, const U& b)
{
   a /= b;
   return a;
}

template <class T, class U>
inline typename std::enable_if<std::is_constructible<T, U>::value, polynomial<T> >::type operator % (const polynomial<T>&, const U&)
{
   // Since we can always divide by a scalar, result is always an empty polynomial:
   return polynomial<T>();
}

template <class U, class T>
inline typename std::enable_if<std::is_constructible<T, U>::value, polynomial<T> >::type operator + (const U& a, polynomial<T> b)
{
   b += a;
   return b;
}

template <class U, class T>
inline typename std::enable_if<std::is_constructible<T, U>::value, polynomial<T> >::type operator - (const U& a, polynomial<T> b)
{
   b -= a;
   return -b;
}

template <class U, class T>
inline typename std::enable_if<std::is_constructible<T, U>::value, polynomial<T> >::type operator * (const U& a, polynomial<T> b)
{
   b *= a;
   return b;
}

template <class T>
bool operator == (const polynomial<T> &a, const polynomial<T> &b)
{
    return a.data() == b.data();
}

template <class T>
bool operator != (const polynomial<T> &a, const polynomial<T> &b)
{
    return a.data() != b.data();
}

template <typename T, typename U>
polynomial<T> operator >> (polynomial<T> a, const U& b)
{
    a >>= b;
    return a;
}

template <typename T, typename U>
polynomial<T> operator << (polynomial<T> a, const U& b)
{
    a <<= b;
    return a;
}

// Unary minus (negate).
template <class T>
polynomial<T> operator - (polynomial<T> a)
{
    std::transform(a.data().begin(), a.data().end(), a.data().begin(), detail::negate());
    return a;
}

template <class T>
bool odd(polynomial<T> const &a)
{
    return a.size() > 0 && a[0] != static_cast<T>(0);
}

template <class T>
bool even(polynomial<T> const &a)
{
    return !odd(a);
}

template <class T>
polynomial<T> pow(polynomial<T> base, int exp)
{
    if (exp < 0)
        return policies::raise_domain_error(
                "boost::math::tools::pow<%1%>",
                "Negative powers are not supported for polynomials.",
                base, policies::policy<>());
        // if the policy is ignore_error or errno_on_error, raise_domain_error
        // will return std::numeric_limits<polynomial<T>>::quiet_NaN(), which
        // defaults to polynomial<T>(), which is the zero polynomial
    polynomial<T> result(T(1));
    if (exp & 1)
        result = base;
    /* "Exponentiation by squaring" */
    while (exp >>= 1)
    {
        base *= base;
        if (exp & 1)
            result *= base;
    }
    return result;
}

template <class charT, class traits, class T>
inline std::basic_ostream<charT, traits>& operator << (std::basic_ostream<charT, traits>& os, const polynomial<T>& poly)
{
   os << "{ ";
   for(unsigned i = 0; i < poly.size(); ++i)
   {
      if(i) os << ", ";
      os << poly[i];
   }
   os << " }";
   return os;
}

} // namespace tools
} // namespace math
} // namespace boost

//
// Polynomial specific overload of gcd algorithm:
//
#include <boost/math/tools/polynomial_gcd.hpp>

#endif // BOOST_MATH_TOOLS_POLYNOMIAL_HPP
