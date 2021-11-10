
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2014 Anton Bikineev
//  Copyright 2014 Christopher Kormanyos
//  Copyright 2014 John Maddock
//  Copyright 2014 Paul Bristow
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_MATH_DETAIL_HYPERGEOMETRIC_CF_HPP
#define BOOST_MATH_DETAIL_HYPERGEOMETRIC_CF_HPP

  namespace boost { namespace math { namespace detail {

  // primary template for term of continued fraction
  template <class T, unsigned p, unsigned q>
  struct hypergeometric_pFq_cf_term;

  // partial specialization for 0F1
  template <class T>
  struct hypergeometric_pFq_cf_term<T, 0u, 1u>
  {
    typedef std::pair<T,T> result_type;

    hypergeometric_pFq_cf_term(const T& b, const T& z):
      n(1), b(b), z(z),
      term(std::make_pair(T(0), T(1)))
    {
    }

    result_type operator()()
    {
      const result_type result = term;
      ++b; ++n;
      numer = -(z / (b * n));
      term = std::make_pair(numer, 1 - numer);
      return result;
    }

  private:
    unsigned n;
    T b;
    const T z;
    T numer;
    result_type term;
  };

  // partial specialization for 1F0
  template <class T>
  struct hypergeometric_pFq_cf_term<T, 1u, 0u>
  {
    typedef std::pair<T,T> result_type;

    hypergeometric_pFq_cf_term(const T& a, const T& z):
      n(1), a(a), z(z),
      term(std::make_pair(T(0), T(1)))
    {
    }

    result_type operator()()
    {
      const result_type result = term;
      ++a; ++n;
      numer = -((a * z) / n);
      term = std::make_pair(numer, 1 - numer);
      return result;
    }

  private:
    unsigned n;
    T a;
    const T z;
    T numer;
    result_type term;
  };

  // partial specialization for 1F1
  template <class T>
  struct hypergeometric_pFq_cf_term<T, 1u, 1u>
  {
    typedef std::pair<T,T> result_type;

    hypergeometric_pFq_cf_term(const T& a, const T& b, const T& z):
      n(1), a(a), b(b), z(z),
      term(std::make_pair(T(0), T(1)))
    {
    }

    result_type operator()()
    {
      const result_type result = term;
      ++a; ++b; ++n;
      numer = -((a * z) / (b * n));
      term = std::make_pair(numer, 1 - numer);
      return result;
    }

  private:
    unsigned n;
    T a, b;
    const T z;
    T numer;
    result_type term;
  };

  // partial specialization for 1f2
  template <class T>
  struct hypergeometric_pFq_cf_term<T, 1u, 2u>
  {
    typedef std::pair<T,T> result_type;

    hypergeometric_pFq_cf_term(const T& a, const T& b, const T& c, const T& z):
      n(1), a(a), b(b), c(c), z(z),
      term(std::make_pair(T(0), T(1)))
    {
    }

    result_type operator()()
    {
      const result_type result = term;
      ++a; ++b; ++c; ++n;
      numer = -((a * z) / ((b * c) * n));
      term = std::make_pair(numer, 1 - numer);
      return result;
    }

  private:
    unsigned n;
    T a, b, c;
    const T z;
    T numer;
    result_type term;
  };

  // partial specialization for 2f1
  template <class T>
  struct hypergeometric_pFq_cf_term<T, 2u, 1u>
  {
    typedef std::pair<T,T> result_type;

    hypergeometric_pFq_cf_term(const T& a, const T& b, const T& c, const T& z):
      n(1), a(a), b(b), c(c), z(z),
      term(std::make_pair(T(0), T(1)))
    {
    }

    result_type operator()()
    {
      const result_type result = term;
      ++a; ++b; ++c; ++n;
      numer = -(((a * b) * z) / (c * n));
      term = std::make_pair(numer, 1 - numer);
      return result;
    }

  private:
    unsigned n;
    T a, b, c;
    const T z;
    T numer;
    result_type term;
  };

  template <class T, unsigned p, unsigned q, class Policy>
  inline T compute_cf_pFq(detail::hypergeometric_pFq_cf_term<T, p, q>& term, const Policy& pol)
  {
    BOOST_MATH_STD_USING
    std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
    const T result = tools::continued_fraction_b(
      term,
      boost::math::policies::get_epsilon<T, Policy>(),
      max_iter);
    boost::math::policies::check_series_iterations<T>(
      "boost::math::hypergeometric_pFq_cf<%1%>(%1%,%1%,%1%)",
      max_iter,
      pol);
    return result;
  }

  template <class T, class Policy>
  inline T hypergeometric_0F1_cf(const T& b, const T& z, const Policy& pol)
  {
    detail::hypergeometric_pFq_cf_term<T, 0u, 1u> f(b, z);
    T result = detail::compute_cf_pFq(f, pol);
    result = ((z / b) / result) + 1;
    return result;
  }

  template <class T, class Policy>
  inline T hypergeometric_1F0_cf(const T& a, const T& z, const Policy& pol)
  {
    detail::hypergeometric_pFq_cf_term<T, 1u, 0u> f(a, z);
    T result = detail::compute_cf_pFq(f, pol);
    result = ((a * z) / result) + 1;
    return result;
  }

  template <class T, class Policy>
  inline T hypergeometric_1F1_cf(const T& a, const T& b, const T& z, const Policy& pol)
  {
    detail::hypergeometric_pFq_cf_term<T, 1u, 1u> f(a, b, z);
    T result = detail::compute_cf_pFq(f, pol);
    result = (((a * z) / b) / result) + 1;
    return result;
  }

  template <class T, class Policy>
  inline T hypergeometric_1F2_cf(const T& a, const T& b, const T& c, const T& z, const Policy& pol)
  {
    detail::hypergeometric_pFq_cf_term<T, 1u, 2u> f(a, b, c, z);
    T result = detail::compute_cf_pFq(f, pol);
    result = (((a * z) / (b * c)) / result) + 1;
    return result;
  }

  template <class T, class Policy>
  inline T hypergeometric_2F1_cf(const T& a, const T& b, const T& c, const T& z, const Policy& pol)
  {
    detail::hypergeometric_pFq_cf_term<T, 2u, 1u> f(a, b, c, z);
    T result = detail::compute_cf_pFq(f, pol);
    result = ((((a * b) * z) / c) / result) + 1;
    return result;
  }

  } } } // namespaces

#endif // BOOST_MATH_DETAIL_HYPERGEOMETRIC_CF_HPP
