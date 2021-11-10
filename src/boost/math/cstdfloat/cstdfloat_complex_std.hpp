///////////////////////////////////////////////////////////////////////////////
// Copyright Christopher Kormanyos 2014.
// Copyright John Maddock 2014.
// Copyright Paul Bristow 2014.
// Distributed under the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Implement a specialization of std::complex<> for *anything* that
// is defined as BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE.

#ifndef BOOST_MATH_CSTDFLOAT_COMPLEX_STD_2014_02_15_HPP_
  #define BOOST_MATH_CSTDFLOAT_COMPLEX_STD_2014_02_15_HPP_

  #if defined(__GNUC__)
  #pragma GCC system_header
  #endif

  #include <complex>
  #include <boost/math/constants/constants.hpp>
  #include <boost/math/tools/cxx03_warn.hpp>

  namespace std
  {
    // Forward declarations.
    template<class float_type>
    class complex;

    template<>
    class complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>;

    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE real(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE imag(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE abs (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE arg (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE norm(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> conj (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> proj (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> polar(const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE&,
                                                                      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& = 0);

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> sqrt (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> sin  (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> cos  (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> tan  (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> asin (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> acos (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> atan (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> exp  (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> log  (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> log10(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> pow  (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&,
                                                                      int);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> pow  (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&,
                                                                      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> pow  (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&,
                                                                      const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> pow  (const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE&,
                                                                      const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> sinh (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> cosh (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> tanh (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> asinh(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> acosh(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> atanh(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    template<class char_type, class traits_type>
    inline std::basic_ostream<char_type, traits_type>& operator<<(std::basic_ostream<char_type, traits_type>&, const std::complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    template<class char_type, class traits_type>
    inline std::basic_istream<char_type, traits_type>& operator>>(std::basic_istream<char_type, traits_type>&, std::complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>&);

    // Template specialization of the complex class.
    template<>
    class complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>
    {
    public:
      typedef BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE value_type;

      complex(const complex<float>&);
      complex(const complex<double>&);
      complex(const complex<long double>&);

      #if defined(BOOST_NO_CXX11_CONSTEXPR)
      complex(const value_type& r = value_type(),
              const value_type& i = value_type()) : re(r),
                                                    im(i) { }

      template<typename X>
      explicit complex(const complex<X>& x) : re(x.real()),
                                     im(x.imag()) { }

      const value_type& real() const { return re; }
      const value_type& imag() const { return im; }

      value_type& real() { return re; }
      value_type& imag() { return im; }
      #else
      constexpr complex(const value_type& r = value_type(),
                              const value_type& i = value_type()) : re(r),
                                                                    im(i) { }

      template<typename X>
      explicit constexpr complex(const complex<X>& x) : re(x.real()),
                                                     im(x.imag()) { }

      value_type real() const { return re; }
      value_type imag() const { return im; }
      #endif

      void real(value_type r) { re = r; }
      void imag(value_type i) { im = i; }

      complex<value_type>& operator=(const value_type& v)
      {
        re = v;
        im = value_type(0);
        return *this;
      }

      complex<value_type>& operator+=(const value_type& v)
      {
        re += v;
        return *this;
      }

      complex<value_type>& operator-=(const value_type& v)
      {
        re -= v;
        return *this;
      }

      complex<value_type>& operator*=(const value_type& v)
      {
        re *= v;
        im *= v;
        return *this;
      }

      complex<value_type>& operator/=(const value_type& v)
      {
        re /= v;
        im /= v;
        return *this;
      }

      template<typename X>
      complex<value_type>& operator=(const complex<X>& x)
      {
        re = x.real();
        im = x.imag();
        return *this;
      }

      template<typename X>
      complex<value_type>& operator+=(const complex<X>& x)
      {
        re += x.real();
        im += x.imag();
        return *this;
      }

      template<typename X>
      complex<value_type>& operator-=(const complex<X>& x)
      {
        re -= x.real();
        im -= x.imag();
        return *this;
      }

      template<typename X>
      complex<value_type>& operator*=(const complex<X>& x)
      {
        const value_type tmp_real = (re * x.real()) - (im * x.imag());
        im = (re * x.imag()) + (im * x.real());
        re = tmp_real;
        return *this;
      }

      template<typename X>
      complex<value_type>& operator/=(const complex<X>& x)
      {
        const value_type tmp_real = (re * x.real()) + (im * x.imag());
        const value_type the_norm = std::norm(x);
        im = ((im * x.real()) - (re * x.imag())) / the_norm;
        re = tmp_real / the_norm;
        return *this;
      }

      private:
        value_type re;
        value_type im;
    };

    // Constructors from built-in complex representation of floating-point types.
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::complex(const complex<float>& f)        : re(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE( f.real())), im(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE( f.imag())) { }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::complex(const complex<double>& d)       : re(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE( d.real())), im(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE( d.imag())) { }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::complex(const complex<long double>& ld) : re(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(ld.real())), im(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(ld.imag())) { }
  } // namespace std

  namespace boost { namespace math { namespace cstdfloat { namespace detail {
  template<class float_type> inline std::complex<float_type> multiply_by_i(const std::complex<float_type>& x)
  {
    // Multiply x (in C) by I (the imaginary component), and return the result.
    return std::complex<float_type>(-x.imag(), x.real());
  }
  } } } } // boost::math::cstdfloat::detail

  namespace std
  {
    // ISO/IEC 14882:2011, Section 26.4.7, specific values.
    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE real(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x) { return x.real(); }
    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE imag(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x) { return x.imag(); }

    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE abs (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x) { using std::sqrt;  return sqrt ((real(x) * real(x)) + (imag(x) * imag(x))); }
    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE arg (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x) { using std::atan2; return atan2(x.imag(), x.real()); }
    inline BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE norm(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x) { return (real(x) * real(x)) + (imag(x) * imag(x)); }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> conj (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(x.real(), -x.imag()); }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> proj (const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE m = (std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::max)();
      if ((x.real() > m)
        || (x.real() < -m)
        || (x.imag() > m)
        || (x.imag() < -m))
      {
        // We have an infinity, return a normalized infinity, respecting the sign of the imaginary part:
         return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::infinity(), x.imag() < 0 ? -0 : 0);
      }
      return x;
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> polar(const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& rho,
                                                                      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& theta)
    {
      using std::sin;
      using std::cos;

      return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(rho * cos(theta), rho * sin(theta));
    }

    // Global add, sub, mul, div.
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator+(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& v) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(u.real() + v.real(), u.imag() + v.imag()); }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator-(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& v) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(u.real() - v.real(), u.imag() - v.imag()); }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator*(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& v)
    {
      return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>((u.real() * v.real()) - (u.imag() * v.imag()),
                                                                  (u.real() * v.imag()) + (u.imag() * v.real()));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator/(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& v)
    {
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE the_norm = std::norm(v);

      return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(((u.real() * v.real()) + (u.imag() * v.imag())) / the_norm,
                                                                  ((u.imag() * v.real()) - (u.real() * v.imag())) / the_norm);
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator+(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u, const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& v) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(u.real() + v, u.imag()); }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator-(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u, const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& v) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(u.real() - v, u.imag()); }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator*(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u, const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& v) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(u.real() * v, u.imag() * v); }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator/(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u, const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& v) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(u.real() / v, u.imag() / v); }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator+(const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& u, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& v) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(u + v.real(),     v.imag()); }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator-(const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& u, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& v) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(u - v.real(),    -v.imag()); }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator*(const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& u, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& v) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(u * v.real(), u * v.imag()); }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator/(const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& u, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& v) { const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE v_norm = norm(v); return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>((u * v.real()) / v_norm, (-u * v.imag()) / v_norm); }

    // Unary plus / minus.
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator+(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u) { return u; }
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> operator-(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& u) { return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(-u.real(), -u.imag()); }

    // Equality and inequality.
    inline bool operator==(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& y) { return ((x.real() == y.real()) && (x.imag() == y.imag())); }
    inline bool operator==(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x, const         BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE&  y) { return ((x.real() == y)        && (x.imag() == BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(0))); }
    inline bool operator==(const         BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE&  x, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& y) { return ((x        == y.real()) && (y.imag() == BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(0))); }
    inline bool operator!=(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& y) { return ((x.real() != y.real()) || (x.imag() != y.imag())); }
    inline bool operator!=(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x, const         BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE&  y) { return ((x.real() != y)        || (x.imag() != BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(0))); }
    inline bool operator!=(const         BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE&  x, const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& y) { return ((x        != y.real()) || (y.imag() != BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(0))); }

    // ISO/IEC 14882:2011, Section 26.4.8, transcendentals.
    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> sqrt(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      using std::fabs;
      using std::sqrt;

      // Compute sqrt(x) for x in C:
      // sqrt(x) = (s       , xi / 2s) : for xr > 0,
      //           (|xi| / 2s, +-s)    : for xr < 0,
      //           (sqrt(xi), sqrt(xi) : for xr = 0,
      // where s = sqrt{ [ |xr| + sqrt(xr^2 + xi^2) ] / 2 },
      // and the +- sign is the same as the sign of xi.

      if(x.real() > 0)
      {
        const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE s = sqrt((fabs(x.real()) + std::abs(x)) / 2);

        return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(s, x.imag() / (s * 2));
      }
      else if(x.real() < 0)
      {
        const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE s = sqrt((fabs(x.real()) + std::abs(x)) / 2);

        const bool imag_is_neg = (x.imag() < 0);

        return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(fabs(x.imag()) / (s * 2), (imag_is_neg ? -s : s));
      }
      else
      {
        const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sqrt_xi_half = sqrt(x.imag() / 2);

        return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(sqrt_xi_half, sqrt_xi_half);
      }
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> sin(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      using std::sin;
      using std::cos;
      using std::exp;

      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sin_x  = sin (x.real());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cos_x  = cos (x.real());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_yp = exp (x.imag());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_ym = BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) / exp_yp;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sinh_y = (exp_yp - exp_ym) / 2;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cosh_y = (exp_yp + exp_ym) / 2;

      return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(sin_x * cosh_y, cos_x * sinh_y);
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> cos(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      using std::sin;
      using std::cos;
      using std::exp;

      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sin_x  = sin (x.real());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cos_x  = cos (x.real());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_yp = exp (x.imag());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_ym = BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) / exp_yp;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sinh_y = (exp_yp - exp_ym) / 2;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cosh_y = (exp_yp + exp_ym) / 2;

      return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(cos_x * cosh_y, -(sin_x * sinh_y));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> tan(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      using std::sin;
      using std::cos;
      using std::exp;

      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sin_x  = sin (x.real());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cos_x  = cos (x.real());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_yp = exp (x.imag());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_ym = BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) / exp_yp;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sinh_y = (exp_yp - exp_ym) / 2;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cosh_y = (exp_yp + exp_ym) / 2;

      return (  complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(sin_x * cosh_y,  cos_x * sinh_y)
              / complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(cos_x * cosh_y, -sin_x * sinh_y));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> asin(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      return -boost::math::cstdfloat::detail::multiply_by_i(std::log(boost::math::cstdfloat::detail::multiply_by_i(x) + std::sqrt(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) - (x * x))));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> acos(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      return boost::math::constants::half_pi<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>() - std::asin(x);
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> atan(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> izz = boost::math::cstdfloat::detail::multiply_by_i(x);

      return boost::math::cstdfloat::detail::multiply_by_i(std::log(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) - izz) - std::log(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) + izz)) / 2;
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> exp(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      using std::exp;

      return std::polar(exp(x.real()), x.imag());
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> log(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      using std::atan2;
      using std::log;

      return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(log(std::norm(x)) / 2, atan2(x.imag(), x.real()));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> log10(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      return std::log(x) / boost::math::constants::ln_ten<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>();
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> pow(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x,
                                                                    int p)
    {
      const bool re_isneg  = (x.real() < 0);
      const bool re_isnan  = (x.real() != x.real());
      const bool re_isinf  = ((!re_isneg) ? bool(+x.real() > (std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::max)())
                                          : bool(-x.real() > (std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::max)()));

      const bool im_isneg  = (x.imag() < 0);
      const bool im_isnan  = (x.imag() != x.imag());
      const bool im_isinf  = ((!im_isneg) ? bool(+x.imag() > (std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::max)())
                                          : bool(-x.imag() > (std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::max)()));

      if(re_isnan || im_isnan) { return x; }

      if(re_isinf || im_isinf)
      {
        return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::quiet_NaN(),
                                                                    std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::quiet_NaN());
      }

      if(p < 0)
      {
        if(std::abs(x) < (std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::min)())
        {
          return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::infinity(),
                                                                      std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::infinity());
        }
        else
        {
          return BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) / std::pow(x, -p);
        }
      }

      if(p == 0)
      {
        return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1));
      }
      else
      {
        if(p == 1) { return x; }

        if(std::abs(x) > (std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::max)())
        {
          const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE re = (re_isneg ? -std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::infinity()
                                                                           : +std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::infinity());

          const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE im = (im_isneg ? -std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::infinity()
                                                                           : +std::numeric_limits<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>::infinity());

          return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(re, im);
        }

        if     (p == 2) { return  (x * x); }
        else if(p == 3) { return ((x * x) * x); }
        else if(p == 4) { const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> x2 = (x * x); return (x2 * x2); }
        else
        {
          // The variable xn stores the binary powers of x.
          complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> result(((p % 2) != 0) ? x : complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1)));
          complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> xn    (x);

          int p2 = p;

          while((p2 /= 2) != 0)
          {
            // Square xn for each binary power.
            xn *= xn;

            const bool has_binary_power = ((p2 % 2) != 0);

            if(has_binary_power)
            {
              // Multiply the result with each binary power contained in the exponent.
              result *= xn;
            }
          }

          return result;
        }
      }
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> pow(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x,
                                                                    const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& a)
    {
      return std::exp(a * std::log(x));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> pow(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x,
                                                                    const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& a)
    {
      return std::exp(a * std::log(x));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> pow(const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE& x,
                                                                    const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& a)
    {
      return std::exp(a * std::log(x));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> sinh(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      using std::sin;
      using std::cos;
      using std::exp;

      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sin_y  = sin (x.imag());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cos_y  = cos (x.imag());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_xp = exp (x.real());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_xm = BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) / exp_xp;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sinh_x = (exp_xp - exp_xm) / 2;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cosh_x = (exp_xp + exp_xm) / 2;

      return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(cos_y * sinh_x, cosh_x * sin_y);
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> cosh(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      using std::sin;
      using std::cos;
      using std::exp;

      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sin_y  = sin (x.imag());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cos_y  = cos (x.imag());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_xp = exp (x.real());
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE exp_xm = BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) / exp_xp;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE sinh_x = (exp_xp - exp_xm) / 2;
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE cosh_x = (exp_xp + exp_xm) / 2;

      return complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(cos_y * cosh_x, sin_y * sinh_x);
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> tanh(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> ex_plus  = std::exp(x);
      const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> ex_minus = BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) / ex_plus;

      return (ex_plus - ex_minus) / (ex_plus + ex_minus);
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> asinh(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      return std::log(x + std::sqrt((x * x) + BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1)));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> acosh(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      const BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE my_one(1);

      const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> zp(x.real() + my_one, x.imag());
      const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> zm(x.real() - my_one, x.imag());

      return std::log(x + (zp * std::sqrt(zm / zp)));
    }

    inline complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE> atanh(const complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      return (std::log(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) + x) - std::log(BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE(1) - x)) / 2.0;
    }

    template<class char_type, class traits_type>
    inline std::basic_ostream<char_type, traits_type>& operator<<(std::basic_ostream<char_type, traits_type>& os, const std::complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      std::basic_ostringstream<char_type, traits_type> ostr;

      ostr.flags(os.flags());
      ostr.imbue(os.getloc());
      ostr.precision(os.precision());

      ostr << char_type('(')
           << x.real()
           << char_type(',')
           << x.imag()
           << char_type(')');

      return (os << ostr.str());
    }

    template<class char_type, class traits_type>
    inline std::basic_istream<char_type, traits_type>& operator>>(std::basic_istream<char_type, traits_type>& is, std::complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>& x)
    {
      BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE rx;
      BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE ix;

      char_type the_char;

      static_cast<void>(is >> the_char);

      if(the_char == static_cast<char_type>('('))
      {
        static_cast<void>(is >> rx >> the_char);

        if(the_char == static_cast<char_type>(','))
        {
          static_cast<void>(is >> ix >> the_char);

          if(the_char == static_cast<char_type>(')'))
          {
            x = complex<BOOST_CSTDFLOAT_EXTENDED_COMPLEX_FLOAT_TYPE>(rx, ix);
          }
          else
          {
            is.setstate(ios_base::failbit);
          }
        }
        else if(the_char == static_cast<char_type>(')'))
        {
          x = rx;
        }
        else
        {
          is.setstate(ios_base::failbit);
        }
      }
      else
      {
        static_cast<void>(is.putback(the_char));

        static_cast<void>(is >> rx);

        x = rx;
      }

      return is;
    }
  } // namespace std

#endif // BOOST_MATH_CSTDFLOAT_COMPLEX_STD_2014_02_15_HPP_
