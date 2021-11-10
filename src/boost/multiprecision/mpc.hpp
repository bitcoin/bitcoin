///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MULTIPRECISION_MPC_HPP
#define BOOST_MULTIPRECISION_MPC_HPP

#include <boost/multiprecision/number.hpp>
#include <cstdint>
#include <boost/multiprecision/detail/digits.hpp>
#include <boost/multiprecision/detail/atomic.hpp>
#include <boost/multiprecision/traits/is_variable_precision.hpp>
#include <boost/multiprecision/mpfr.hpp>
#include <boost/multiprecision/detail/hash.hpp>
#include <mpc.h>
#include <cmath>
#include <algorithm>
#include <complex>

#ifndef BOOST_MULTIPRECISION_MPFI_DEFAULT_PRECISION
#define BOOST_MULTIPRECISION_MPFI_DEFAULT_PRECISION 20
#endif

namespace boost {
namespace multiprecision {
namespace backends {

template <unsigned digits10>
struct mpc_complex_backend;

template <class Backend>
struct logged_adaptor;
template <class Backend>
struct debug_adaptor;

} // namespace backends

template <unsigned digits10>
struct number_category<backends::mpc_complex_backend<digits10> > : public std::integral_constant<int, number_kind_complex>
{};

namespace backends {

namespace detail {

inline void mpc_copy_precision(mpc_t dest, const mpc_t src)
{
   mpfr_prec_t p_dest = mpc_get_prec(dest);
   mpfr_prec_t p_src  = mpc_get_prec(src);
   if (p_dest != p_src)
      mpc_set_prec(dest, p_src);
}
inline void mpc_copy_precision(mpc_t dest, const mpc_t src1, const mpc_t src2)
{
   mpfr_prec_t p_dest = mpc_get_prec(dest);
   mpfr_prec_t p_src1 = mpc_get_prec(src1);
   mpfr_prec_t p_src2 = mpc_get_prec(src2);
   if (p_src2 > p_src1)
      p_src1 = p_src2;
   if (p_dest != p_src1)
      mpc_set_prec(dest, p_src1);
}

template <unsigned digits10>
struct mpc_complex_imp
{
#ifdef BOOST_HAS_LONG_LONG
   using signed_types = std::tuple<long, boost::long_long_type>          ;
   using unsigned_types = std::tuple<unsigned long, boost::ulong_long_type>;
#else
   using signed_types = std::tuple<long>         ;
   using unsigned_types = std::tuple<unsigned long>;
#endif
   using float_types = std::tuple<double, long double>;
   using exponent_type = long                          ;

   mpc_complex_imp()
   {
      mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpc_set_ui(m_data, 0u, GMP_RNDN);
   }
   mpc_complex_imp(unsigned digits2)
   {
      mpc_init2(m_data, digits2);
      mpc_set_ui(m_data, 0u, GMP_RNDN);
   }

   mpc_complex_imp(const mpc_complex_imp& o)
   {
      mpc_init2(m_data, preserve_source_precision() ? mpc_get_prec(o.data()) : boost::multiprecision::detail::digits10_2_2(get_default_precision()));
      if (o.m_data[0].re[0]._mpfr_d)
         mpc_set(m_data, o.m_data, GMP_RNDN);
   }
   // rvalue copy
   mpc_complex_imp(mpc_complex_imp&& o) noexcept
   {
      mpfr_prec_t binary_default_precision = boost::multiprecision::detail::digits10_2_2(get_default_precision());
      if ((this->get_default_options() != variable_precision_options::preserve_target_precision) || (mpc_get_prec(o.data()) == binary_default_precision))
      {
         m_data[0] = o.m_data[0];
         o.m_data[0].re[0]._mpfr_d = 0;
      }
      else
      {
         // NOTE: C allocation interface must not throw:
         mpc_init2(m_data, binary_default_precision);
         if (o.m_data[0].re[0]._mpfr_d)
            mpc_set(m_data, o.m_data, GMP_RNDN);
      }
   }
   mpc_complex_imp& operator=(const mpc_complex_imp& o)
   {
      if ((o.m_data[0].re[0]._mpfr_d) && (this != &o))
      {
         if (m_data[0].re[0]._mpfr_d == 0)
            mpc_init2(m_data, preserve_source_precision() ? mpc_get_prec(o.m_data) : boost::multiprecision::detail::digits10_2_2(get_default_precision()));
         else if (preserve_source_precision() && (mpc_get_prec(o.data()) != mpc_get_prec(data())))
         {
            mpc_set_prec(m_data, mpc_get_prec(o.m_data));
         }
         mpc_set(m_data, o.m_data, GMP_RNDN);
      }
      return *this;
   }
   // rvalue assign
   mpc_complex_imp& operator=(mpc_complex_imp&& o) noexcept
   {
      if ((this->get_default_options() != variable_precision_options::preserve_target_precision) || (mpc_get_prec(o.data()) == mpc_get_prec(data())))
         mpc_swap(m_data, o.m_data);
      else
         *this = static_cast<const mpc_complex_imp&>(o);
      return *this;
   }
#ifdef BOOST_HAS_LONG_LONG
#ifdef _MPFR_H_HAVE_INTMAX_T
   mpc_complex_imp& operator=(boost::ulong_long_type i)
   {
      if (m_data[0].re[0]._mpfr_d == 0)
         mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpc_set_uj(data(), i, GMP_RNDD);
      return *this;
   }
   mpc_complex_imp& operator=(boost::long_long_type i)
   {
      if (m_data[0].re[0]._mpfr_d == 0)
         mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpc_set_sj(data(), i, GMP_RNDD);
      return *this;
   }
#else
   mpc_complex_imp& operator=(boost::ulong_long_type i)
   {
      mpfr_float_backend<digits10> f(0uL, mpc_get_prec(m_data));
      f = i;
      mpc_set_fr(this->data(), f.data(), GMP_RNDN);
      return *this;
   }
   mpc_complex_imp& operator=(boost::long_long_type i)
   {
      mpfr_float_backend<digits10> f(0uL, mpc_get_prec(m_data));
      f = i;
      mpc_set_fr(this->data(), f.data(), GMP_RNDN);
      return *this;
   }
#endif
#endif
   mpc_complex_imp& operator=(unsigned long i)
   {
      if (m_data[0].re[0]._mpfr_d == 0)
         mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpc_set_ui(m_data, i, GMP_RNDN);
      return *this;
   }
   mpc_complex_imp& operator=(long i)
   {
      if (m_data[0].re[0]._mpfr_d == 0)
         mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpc_set_si(m_data, i, GMP_RNDN);
      return *this;
   }
   mpc_complex_imp& operator=(double d)
   {
      if (m_data[0].re[0]._mpfr_d == 0)
         mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpc_set_d(m_data, d, GMP_RNDN);
      return *this;
   }
   mpc_complex_imp& operator=(long double d)
   {
      if (m_data[0].re[0]._mpfr_d == 0)
         mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpc_set_ld(m_data, d, GMP_RNDN);
      return *this;
   }
   mpc_complex_imp& operator=(mpz_t i)
   {
      if (m_data[0].re[0]._mpfr_d == 0)
         mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpc_set_z(m_data, i, GMP_RNDN);
      return *this;
   }
   mpc_complex_imp& operator=(gmp_int i)
   {
      if (m_data[0].re[0]._mpfr_d == 0)
         mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpc_set_z(m_data, i.data(), GMP_RNDN);
      return *this;
   }

   mpc_complex_imp& operator=(const char* s)
   {
      using default_ops::eval_fpclassify;

      if (m_data[0].re[0]._mpfr_d == 0)
         mpc_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));

      mpfr_float_backend<digits10> a(0uL, mpc_get_prec(m_data)), b(0uL, mpc_get_prec(m_data));

      if (s && (*s == '('))
      {
         std::string part;
         const char* p = ++s;
         while (*p && (*p != ',') && (*p != ')'))
            ++p;
         part.assign(s, p);
         if (part.size())
            a = part.c_str();
         else
            a = 0uL;
         s = p;
         if (*p && (*p != ')'))
         {
            ++p;
            while (*p && (*p != ')'))
               ++p;
            part.assign(s + 1, p);
         }
         else
            part.erase();
         if (part.size())
            b = part.c_str();
         else
            b = 0uL;
      }
      else
      {
         a = s;
         b = 0uL;
      }

      if (eval_fpclassify(a) == (int)FP_NAN)
      {
         mpc_set_fr(this->data(), a.data(), GMP_RNDN);
      }
      else if (eval_fpclassify(b) == (int)FP_NAN)
      {
         mpc_set_fr(this->data(), b.data(), GMP_RNDN);
      }
      else
      {
         mpc_set_fr_fr(m_data, a.data(), b.data(), GMP_RNDN);
      }
      return *this;
   }
   void swap(mpc_complex_imp& o) noexcept
   {
      mpc_swap(m_data, o.m_data);
   }
   std::string str(std::streamsize digits, std::ios_base::fmtflags f) const
   {
      BOOST_ASSERT(m_data[0].re[0]._mpfr_d);

      mpfr_float_backend<digits10> a(0uL, mpc_get_prec(m_data)), b(0uL, mpc_get_prec(m_data));

      mpc_real(a.data(), m_data, GMP_RNDD);
      mpc_imag(b.data(), m_data, GMP_RNDD);

      if (eval_is_zero(b))
         return a.str(digits, f);

      return "(" + a.str(digits, f) + "," + b.str(digits, f) + ")";
   }
   ~mpc_complex_imp() noexcept
   {
      if (m_data[0].re[0]._mpfr_d)
         mpc_clear(m_data);
   }
   void negate() noexcept
   {
      BOOST_ASSERT(m_data[0].re[0]._mpfr_d);
      mpc_neg(m_data, m_data, GMP_RNDD);
   }
   int compare(const mpc_complex_imp& o) const noexcept
   {
      BOOST_ASSERT(m_data[0].re[0]._mpfr_d && o.m_data[0].re[0]._mpfr_d);
      return mpc_cmp(m_data, o.m_data);
   }
   int compare(const mpc_complex_backend<digits10>& o) const noexcept
   {
      BOOST_ASSERT(m_data[0].re[0]._mpfr_d && o.m_data[0].re[0]._mpfr_d);
      return mpc_cmp(m_data, o.data());
   }
   int compare(long int i) const noexcept
   {
      BOOST_ASSERT(m_data[0].re[0]._mpfr_d);
      return mpc_cmp_si(m_data, i);
   }
   int compare(unsigned long int i) const noexcept
   {
      BOOST_ASSERT(m_data[0].re[0]._mpfr_d);
      constexpr const unsigned long int max_val = (std::numeric_limits<long>::max)();
      if (i > max_val)
      {
         mpc_complex_imp d(mpc_get_prec(m_data));
         d = i;
         return compare(d);
      }
      return mpc_cmp_si(m_data, (long)i);
   }
   template <class V>
   int compare(const V& v) const noexcept
   {
      mpc_complex_imp d(mpc_get_prec(m_data));
      d = v;
      return compare(d);
   }
   mpc_t& data() noexcept
   {
      BOOST_ASSERT(m_data[0].re[0]._mpfr_d);
      return m_data;
   }
   const mpc_t& data() const noexcept
   {
      BOOST_ASSERT(m_data[0].re[0]._mpfr_d);
      return m_data;
   }

 protected:
   mpc_t            m_data;
   static boost::multiprecision::detail::precision_type& get_global_default_precision() noexcept
   {
      static boost::multiprecision::detail::precision_type val(BOOST_MULTIPRECISION_MPFI_DEFAULT_PRECISION);
      return val;
   }
   static unsigned& get_default_precision() noexcept
   {
      static BOOST_MP_THREAD_LOCAL unsigned val(get_global_default_precision());
      return val;
   }
#ifndef BOOST_MT_NO_ATOMIC_INT
   static std::atomic<variable_precision_options>& get_global_default_options() noexcept
#else
   static variable_precision_options& get_global_default_options() noexcept
#endif
   {
#ifndef BOOST_MT_NO_ATOMIC_INT
      static std::atomic<variable_precision_options> val{variable_precision_options::preserve_related_precision};
#else
      static variable_precision_options val{variable_precision_options::preserve_related_precision};
#endif
      return val;
   }
   static variable_precision_options& get_default_options() noexcept
   {
      static BOOST_MP_THREAD_LOCAL variable_precision_options val(get_global_default_options());
      return val;
   }
   static bool preserve_source_precision() noexcept
   {
      return get_default_options() >= variable_precision_options::preserve_source_precision;
   }
   static bool preserve_component_precision() noexcept
   {
      return get_default_options() >= variable_precision_options::preserve_component_precision;
   }
   static bool preserve_related_precision() noexcept
   {
      return get_default_options() >= variable_precision_options::preserve_related_precision;
   }
   static bool preserve_all_precision() noexcept
   {
      return get_default_options() >= variable_precision_options::preserve_all_precision;
   }
};

} // namespace detail

template <unsigned digits10>
struct mpc_complex_backend : public detail::mpc_complex_imp<digits10>
{
   mpc_complex_backend() : detail::mpc_complex_imp<digits10>() {}
   mpc_complex_backend(const mpc_complex_backend& o) : detail::mpc_complex_imp<digits10>(o) {}
   // rvalue copy
   mpc_complex_backend(mpc_complex_backend&& o) : detail::mpc_complex_imp<digits10>(static_cast<detail::mpc_complex_imp<digits10>&&>(o))
   {}
   template <unsigned D>
   mpc_complex_backend(const mpc_complex_backend<D>& val, typename std::enable_if<D <= digits10>::type* = 0)
       : detail::mpc_complex_imp<digits10>()
   {
      mpc_set(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D>
   explicit mpc_complex_backend(const mpc_complex_backend<D>& val, typename std::enable_if<!(D <= digits10)>::type* = 0)
       : detail::mpc_complex_imp<digits10>()
   {
      mpc_set(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D>
   mpc_complex_backend(const mpfr_float_backend<D>& val, typename std::enable_if<D <= digits10>::type* = 0)
       : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_fr(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D>
   explicit mpc_complex_backend(const mpfr_float_backend<D>& val, typename std::enable_if<!(D <= digits10)>::type* = 0)
       : detail::mpc_complex_imp<digits10>()
   {
      mpc_set(this->m_data, val.data(), GMP_RNDN);
   }
   mpc_complex_backend(const mpc_t val)
       : detail::mpc_complex_imp<digits10>()
   {
      mpc_set(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend(const std::complex<float>& val)
       : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_d_d(this->m_data, val.real(), val.imag(), GMP_RNDN);
   }
   mpc_complex_backend(const std::complex<double>& val)
       : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_d_d(this->m_data, val.real(), val.imag(), GMP_RNDN);
   }
   mpc_complex_backend(const std::complex<long double>& val)
       : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_ld_ld(this->m_data, val.real(), val.imag(), GMP_RNDN);
   }
   mpc_complex_backend(mpz_srcptr val) : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_z(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend& operator=(mpz_srcptr val)
   {
      mpc_set_z(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(gmp_int const& val) : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_z(this->m_data, val.data(), GMP_RNDN);
   }
   mpc_complex_backend& operator=(gmp_int const& val)
   {
      mpc_set_z(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(mpf_srcptr val) : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_f(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend& operator=(mpf_srcptr val)
   {
      mpc_set_f(this->m_data, val, GMP_RNDN);
      return *this;
   }
   template <unsigned D10>
   mpc_complex_backend(gmp_float<D10> const& val) : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_f(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D10>
   mpc_complex_backend& operator=(gmp_float<D10> const& val)
   {
      mpc_set_f(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(mpq_srcptr val) : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_q(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend& operator=(mpq_srcptr val)
   {
      mpc_set_q(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(gmp_rational const& val) : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_q(this->m_data, val.data(), GMP_RNDN);
   }
   mpc_complex_backend& operator=(gmp_rational const& val)
   {
      mpc_set_q(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(mpfr_srcptr val) : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_fr(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend& operator=(mpfr_srcptr val)
   {
      mpc_set_fr(this->m_data, val, GMP_RNDN);
      return *this;
   }
   template <unsigned D10, mpfr_allocation_type AllocationType>
   mpc_complex_backend(mpfr_float_backend<D10, AllocationType> const& val) : detail::mpc_complex_imp<digits10>()
   {
      mpc_set_fr(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D10, mpfr_allocation_type AllocationType>
   mpc_complex_backend& operator=(mpfr_float_backend<D10, AllocationType> const& val)
   {
      mpc_set_fr(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend& operator=(const mpc_complex_backend& o)
   {
      *static_cast<detail::mpc_complex_imp<digits10>*>(this) = static_cast<detail::mpc_complex_imp<digits10> const&>(o);
      return *this;
   }
   // rvalue assign
   mpc_complex_backend& operator=(mpc_complex_backend&& o) noexcept
   {
      *static_cast<detail::mpc_complex_imp<digits10>*>(this) = static_cast<detail::mpc_complex_imp<digits10>&&>(o);
      return *this;
   }
   template <class V>
   mpc_complex_backend& operator=(const V& v)
   {
      *static_cast<detail::mpc_complex_imp<digits10>*>(this) = v;
      return *this;
   }
   mpc_complex_backend& operator=(const mpc_t val)
   {
      mpc_set(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpc_complex_backend& operator=(const std::complex<float>& val)
   {
      mpc_set_d_d(this->m_data, val.real(), val.imag(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend& operator=(const std::complex<double>& val)
   {
      mpc_set_d_d(this->m_data, val.real(), val.imag(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend& operator=(const std::complex<long double>& val)
   {
      mpc_set_ld_ld(this->m_data, val.real(), val.imag(), GMP_RNDN);
      return *this;
   }
   // We don't change our precision here, this is a fixed precision type:
   template <unsigned D>
   mpc_complex_backend& operator=(const mpc_complex_backend<D>& val)
   {
      mpc_set(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
};

template <>
struct mpc_complex_backend<0> : public detail::mpc_complex_imp<0>
{
   mpc_complex_backend() : detail::mpc_complex_imp<0>() {}
   mpc_complex_backend(const mpc_t val)
       : detail::mpc_complex_imp<0>(mpc_get_prec(val))
   {
      mpc_set(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend(const mpc_complex_backend& o) : detail::mpc_complex_imp<0>(o) {}
   // rvalue copy
   mpc_complex_backend(mpc_complex_backend&& o) noexcept : detail::mpc_complex_imp<0>(static_cast<detail::mpc_complex_imp<0>&&>(o))
   {}
   mpc_complex_backend(const mpc_complex_backend& o, unsigned digits10)
       : detail::mpc_complex_imp<0>(multiprecision::detail::digits10_2_2(digits10))
   {
      mpc_set(this->m_data, o.data(), GMP_RNDN);
   }
   template <unsigned D>
   mpc_complex_backend(const mpc_complex_backend<D>& val)
       : detail::mpc_complex_imp<0>(preserve_related_precision() ? mpc_get_prec(val.data()) : multiprecision::detail::digits10_2_2(get_default_precision()))
   {
      mpc_set(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D>
   mpc_complex_backend(const mpfr_float_backend<D>& val)
       : detail::mpc_complex_imp<0>((D == 0 ? this->preserve_component_precision() : this->preserve_related_precision()) ? mpfr_get_prec(val.data()) : multiprecision::detail::digits10_2_2(this->get_default_precision()))
   {
      mpc_set_fr(this->m_data, val.data(), GMP_RNDN);
   }
   mpc_complex_backend(mpz_srcptr val) : detail::mpc_complex_imp<0>()
   {
      mpc_set_z(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend& operator=(mpz_srcptr val)
   {
      mpc_set_z(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(gmp_int const& val) : detail::mpc_complex_imp<0>(preserve_all_precision() ? used_gmp_int_bits(val) : boost::multiprecision::detail::digits10_2_2(thread_default_precision()))
   {
      mpc_set_z(this->m_data, val.data(), GMP_RNDN);
   }
   mpc_complex_backend& operator=(gmp_int const& val)
   {
      if (this->m_data[0].im->_mpfr_d == 0)
      {
         unsigned requested_precision = this->thread_default_precision();
         if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
         {
            unsigned d2 = used_gmp_int_bits(val);
            unsigned d10 = 1 + multiprecision::detail::digits2_2_10(d2);
            if (d10 > requested_precision)
               requested_precision = d10;
         }
         mpc_init2(this->m_data, multiprecision::detail::digits10_2_2(requested_precision));
      }
      else if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
      {
         unsigned requested_precision = this->thread_default_precision();
         unsigned d2 = used_gmp_int_bits(val);
         unsigned d10 = 1 + multiprecision::detail::digits2_2_10(d2);
         if (d10 > requested_precision)
            this->precision(d10);
      }
      mpc_set_z(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(mpf_srcptr val) : detail::mpc_complex_imp<0>((unsigned)mpf_get_prec(val))
   {
      mpc_set_f(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend& operator=(mpf_srcptr val)
   {
      if ((mp_bitcnt_t)mpc_get_prec(data()) != mpf_get_prec(val))
      {
         mpc_complex_backend t(val);
         t.swap(*this);
      }
      else
         mpc_set_f(this->m_data, val, GMP_RNDN);
      return *this;
   }
   template <unsigned digits10>
   mpc_complex_backend(gmp_float<digits10> const& val) : detail::mpc_complex_imp<0>(preserve_all_precision() ? (unsigned)mpf_get_prec(val.data()) : multiprecision::detail::digits10_2_2(get_default_precision()))
   {
      mpc_set_f(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned digits10>
   mpc_complex_backend& operator=(gmp_float<digits10> const& val)
   {
      if (preserve_all_precision() && (mpc_get_prec(data()) != (mpfr_prec_t)mpf_get_prec(val.data())))
      {
         mpc_complex_backend t(val);
         t.swap(*this);
      }
      else
         mpc_set_f(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(mpq_srcptr val) : detail::mpc_complex_imp<0>()
   {
      mpc_set_q(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend& operator=(mpq_srcptr val)
   {
      mpc_set_q(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(gmp_rational const& val) : detail::mpc_complex_imp<0>(preserve_all_precision() ? used_gmp_rational_bits(val) : boost::multiprecision::detail::digits10_2_2(thread_default_precision()))
   {
      mpc_set_q(this->m_data, val.data(), GMP_RNDN);
   }
   mpc_complex_backend& operator=(gmp_rational const& val)
   {
      if (this->m_data[0].im->_mpfr_d == 0)
      {
         unsigned requested_precision = this->get_default_precision();
         if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
         {
            unsigned d10 = 1 + multiprecision::detail::digits2_2_10(used_gmp_rational_bits(val));
            if (d10 > requested_precision)
               requested_precision = d10;
         }
         mpc_init2(this->m_data, multiprecision::detail::digits10_2_2(requested_precision));
      }
      else if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
      {
         unsigned requested_precision = this->get_default_precision();
         unsigned d10 = 1 + multiprecision::detail::digits2_2_10(used_gmp_rational_bits(val));
         if (d10 > requested_precision)
            this->precision(d10);
      }
      mpc_set_q(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(mpfr_srcptr val) : detail::mpc_complex_imp<0>(mpfr_get_prec(val))
   {
      mpc_set_fr(this->m_data, val, GMP_RNDN);
   }
   mpc_complex_backend& operator=(mpfr_srcptr val)
   {
      if (mpc_get_prec(data()) != mpfr_get_prec(val))
      {
         mpc_complex_backend t(val);
         t.swap(*this);
      }
      else
         mpc_set_fr(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpc_complex_backend(const std::complex<float>& val)
       : detail::mpc_complex_imp<0>()
   {
      mpc_set_d_d(this->m_data, val.real(), val.imag(), GMP_RNDN);
   }
   mpc_complex_backend(const std::complex<double>& val)
       : detail::mpc_complex_imp<0>()
   {
      mpc_set_d_d(this->m_data, val.real(), val.imag(), GMP_RNDN);
   }
   mpc_complex_backend(const std::complex<long double>& val)
       : detail::mpc_complex_imp<0>()
   {
      mpc_set_ld_ld(this->m_data, val.real(), val.imag(), GMP_RNDN);
   }
   // Construction with precision:
   template <class T, class U>
   mpc_complex_backend(const T& a, const U& b, unsigned digits10)
       : detail::mpc_complex_imp<0>(multiprecision::detail::digits10_2_2(digits10))
   {
      // We can't use assign_components here because it copies the precision of
      // a and b, not digits10....
      boost::multiprecision::detail::scoped_precision_options<mpfr_float> scoped(*this);
      (void)scoped;
      mpfr_float ca(a), cb(b);
      mpc_set_fr_fr(this->data(), ca.backend().data(), cb.backend().data(), GMP_RNDN);
   }
   template <unsigned N>
   mpc_complex_backend(const mpfr_float_backend<N>& a, const mpfr_float_backend<N>& b, unsigned digits10)
       : detail::mpc_complex_imp<0>(multiprecision::detail::digits10_2_2(digits10))
   {
      mpc_set_fr_fr(this->data(), a.data(), b.data(), GMP_RNDN);
   }

   mpc_complex_backend& operator=(const mpc_complex_backend& o) = default;
   // rvalue assign
   mpc_complex_backend& operator=(mpc_complex_backend&& o) noexcept = default;

   template <class V>
   mpc_complex_backend& operator=(const V& v)
   {
      constexpr unsigned d10 = std::is_floating_point<V>::value ?
         std::numeric_limits<V>::digits10 :
         std::numeric_limits<V>::digits10 ? 1 + std::numeric_limits<V>::digits10 :
         1 + boost::multiprecision::detail::digits2_2_10(std::numeric_limits<V>::digits);

      if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
      {
         BOOST_IF_CONSTEXPR(std::is_floating_point<V>::value)
         {
            if (std::numeric_limits<V>::digits > mpc_get_prec(this->data()))
               mpc_set_prec(this->data(), std::numeric_limits<V>::digits);
         }
      else
      {
         if (precision() < d10)
            this->precision(d10);
      }
      }

      *static_cast<detail::mpc_complex_imp<0>*>(this) = v;
      return *this;
   }
   mpc_complex_backend& operator=(const mpc_t val)
   {
      mpc_set_prec(this->m_data, mpc_get_prec(val));
      mpc_set(this->m_data, val, GMP_RNDN);
      return *this;
   }
   template <unsigned D>
   mpc_complex_backend& operator=(const mpc_complex_backend<D>& val)
   {
      mpc_set_prec(this->m_data, mpc_get_prec(val.data()));
      mpc_set(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   template <unsigned D>
   mpc_complex_backend& operator=(const mpfr_float_backend<D>& val)
   {
      if (D == 0 ? this->preserve_component_precision() : this->preserve_related_precision())
         mpc_set_prec(this->m_data, mpfr_get_prec(val.data()));
      mpc_set_fr(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend& operator=(const std::complex<float>& val)
   {
      mpc_set_d_d(this->m_data, val.real(), val.imag(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend& operator=(const std::complex<double>& val)
   {
      mpc_set_d_d(this->m_data, val.real(), val.imag(), GMP_RNDN);
      return *this;
   }
   mpc_complex_backend& operator=(const std::complex<long double>& val)
   {
      mpc_set_ld_ld(this->m_data, val.real(), val.imag(), GMP_RNDN);
      return *this;
   }
   static unsigned default_precision() noexcept
   {
      return get_global_default_precision();
   }
   static void default_precision(unsigned v) noexcept
   {
      get_global_default_precision() = v;
   }
   static unsigned thread_default_precision() noexcept
   {
      return get_default_precision();
   }
   static void thread_default_precision(unsigned v) noexcept
   {
      get_default_precision() = v;
   }
   unsigned precision() const noexcept
   {
      return multiprecision::detail::digits2_2_10(mpc_get_prec(this->m_data));
   }
   void precision(unsigned digits10) noexcept
   {
      mpfr_prec_round(mpc_realref(this->m_data), multiprecision::detail::digits10_2_2((digits10)), GMP_RNDN);
      mpfr_prec_round(mpc_imagref(this->m_data), multiprecision::detail::digits10_2_2((digits10)), GMP_RNDN);
   }
   //
   // Variable precision options:
   //
   static variable_precision_options default_variable_precision_options() noexcept
   {
      return get_global_default_options();
   }
   static variable_precision_options thread_default_variable_precision_options() noexcept
   {
      return get_default_options();
   }
   static void default_variable_precision_options(variable_precision_options opts)
   {
      get_global_default_options() = opts;
   }
   static void thread_default_variable_precision_options(variable_precision_options opts)
   {
      get_default_options() = opts;
   }
};

template <unsigned digits10, class T>
inline typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value, bool>::type eval_eq(const mpc_complex_backend<digits10>& a, const T& b) noexcept
{
   return a.compare(b) == 0;
}
template <unsigned digits10, class T>
inline typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value, bool>::type eval_lt(const mpc_complex_backend<digits10>& a, const T& b) noexcept
{
   return a.compare(b) < 0;
}
template <unsigned digits10, class T>
inline typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value, bool>::type eval_gt(const mpc_complex_backend<digits10>& a, const T& b) noexcept
{
   return a.compare(b) > 0;
}

template <unsigned D1, unsigned D2>
inline void eval_add(mpc_complex_backend<D1>& result, const mpc_complex_backend<D2>& o)
{
   mpc_add(result.data(), result.data(), o.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_add(mpc_complex_backend<D1>& result, const mpfr_float_backend<D2>& o)
{
   mpc_add_fr(result.data(), result.data(), o.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_subtract(mpc_complex_backend<D1>& result, const mpc_complex_backend<D2>& o)
{
   mpc_sub(result.data(), result.data(), o.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_subtract(mpc_complex_backend<D1>& result, const mpfr_float_backend<D2>& o)
{
   mpc_sub_fr(result.data(), result.data(), o.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_multiply(mpc_complex_backend<D1>& result, const mpc_complex_backend<D2>& o)
{
   if ((void*)&result == (void*)&o)
      mpc_sqr(result.data(), o.data(), GMP_RNDN);
   else
      mpc_mul(result.data(), result.data(), o.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2>
inline void eval_multiply(mpc_complex_backend<D1>& result, const mpfr_float_backend<D2>& o)
{
   mpc_mul_fr(result.data(), result.data(), o.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2>
inline void eval_divide(mpc_complex_backend<D1>& result, const mpc_complex_backend<D2>& o)
{
   mpc_div(result.data(), result.data(), o.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_divide(mpc_complex_backend<D1>& result, const mpfr_float_backend<D2>& o)
{
   mpc_div_fr(result.data(), result.data(), o.data(), GMP_RNDD);
}
template <unsigned digits10>
inline void eval_add(mpc_complex_backend<digits10>& result, unsigned long i)
{
   mpc_add_ui(result.data(), result.data(), i, GMP_RNDN);
}
template <unsigned digits10>
inline void eval_subtract(mpc_complex_backend<digits10>& result, unsigned long i)
{
   mpc_sub_ui(result.data(), result.data(), i, GMP_RNDN);
}
template <unsigned digits10>
inline void eval_multiply(mpc_complex_backend<digits10>& result, unsigned long i)
{
   mpc_mul_ui(result.data(), result.data(), i, GMP_RNDN);
}
template <unsigned digits10>
inline void eval_divide(mpc_complex_backend<digits10>& result, unsigned long i)
{
   mpc_div_ui(result.data(), result.data(), i, GMP_RNDN);
}
template <unsigned digits10>
inline void eval_add(mpc_complex_backend<digits10>& result, long i)
{
   if (i > 0)
      mpc_add_ui(result.data(), result.data(), i, GMP_RNDN);
   else
      mpc_sub_ui(result.data(), result.data(), boost::multiprecision::detail::unsigned_abs(i), GMP_RNDN);
}
template <unsigned digits10>
inline void eval_subtract(mpc_complex_backend<digits10>& result, long i)
{
   if (i > 0)
      mpc_sub_ui(result.data(), result.data(), i, GMP_RNDN);
   else
      mpc_add_ui(result.data(), result.data(), boost::multiprecision::detail::unsigned_abs(i), GMP_RNDN);
}
template <unsigned digits10>
inline void eval_multiply(mpc_complex_backend<digits10>& result, long i)
{
   mpc_mul_ui(result.data(), result.data(), boost::multiprecision::detail::unsigned_abs(i), GMP_RNDN);
   if (i < 0)
      mpc_neg(result.data(), result.data(), GMP_RNDN);
}
template <unsigned digits10>
inline void eval_divide(mpc_complex_backend<digits10>& result, long i)
{
   mpc_div_ui(result.data(), result.data(), boost::multiprecision::detail::unsigned_abs(i), GMP_RNDN);
   if (i < 0)
      mpc_neg(result.data(), result.data(), GMP_RNDN);
}
//
// Specialised 3 arg versions of the basic operators:
//
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_add(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, const mpc_complex_backend<D3>& y)
{
   mpc_add(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_add(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, const mpfr_float_backend<D3>& y)
{
   mpc_add_fr(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_add(mpc_complex_backend<D1>& a, const mpfr_float_backend<D2>& x, const mpc_complex_backend<D3>& y)
{
   mpc_add_fr(a.data(), y.data(), x.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_add(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, unsigned long y)
{
   mpc_add_ui(a.data(), x.data(), y, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_add(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, long y)
{
   if (y < 0)
      mpc_sub_ui(a.data(), x.data(), boost::multiprecision::detail::unsigned_abs(y), GMP_RNDD);
   else
      mpc_add_ui(a.data(), x.data(), y, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_add(mpc_complex_backend<D1>& a, unsigned long x, const mpc_complex_backend<D2>& y)
{
   mpc_add_ui(a.data(), y.data(), x, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_add(mpc_complex_backend<D1>& a, long x, const mpc_complex_backend<D2>& y)
{
   if (x < 0)
   {
      mpc_ui_sub(a.data(), boost::multiprecision::detail::unsigned_abs(x), y.data(), GMP_RNDN);
      mpc_neg(a.data(), a.data(), GMP_RNDD);
   }
   else
      mpc_add_ui(a.data(), y.data(), x, GMP_RNDD);
}
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_subtract(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, const mpc_complex_backend<D3>& y)
{
   mpc_sub(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_subtract(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, const mpfr_float_backend<D3>& y)
{
   mpc_sub_fr(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_subtract(mpc_complex_backend<D1>& a, const mpfr_float_backend<D2>& x, const mpc_complex_backend<D3>& y)
{
   mpc_fr_sub(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_subtract(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, unsigned long y)
{
   mpc_sub_ui(a.data(), x.data(), y, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_subtract(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, long y)
{
   if (y < 0)
      mpc_add_ui(a.data(), x.data(), boost::multiprecision::detail::unsigned_abs(y), GMP_RNDD);
   else
      mpc_sub_ui(a.data(), x.data(), y, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_subtract(mpc_complex_backend<D1>& a, unsigned long x, const mpc_complex_backend<D2>& y)
{
   mpc_ui_sub(a.data(), x, y.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2>
inline void eval_subtract(mpc_complex_backend<D1>& a, long x, const mpc_complex_backend<D2>& y)
{
   if (x < 0)
   {
      mpc_add_ui(a.data(), y.data(), boost::multiprecision::detail::unsigned_abs(x), GMP_RNDD);
      mpc_neg(a.data(), a.data(), GMP_RNDD);
   }
   else
      mpc_ui_sub(a.data(), x, y.data(), GMP_RNDN);
}

template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_multiply(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, const mpc_complex_backend<D3>& y)
{
   if ((void*)&x == (void*)&y)
      mpc_sqr(a.data(), x.data(), GMP_RNDD);
   else
      mpc_mul(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_multiply(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, const mpfr_float_backend<D3>& y)
{
   mpc_mul_fr(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_multiply(mpc_complex_backend<D1>& a, const mpfr_float_backend<D2>& x, const mpc_complex_backend<D3>& y)
{
   mpc_mul_fr(a.data(), y.data(), x.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_multiply(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, unsigned long y)
{
   mpc_mul_ui(a.data(), x.data(), y, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_multiply(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, long y)
{
   if (y < 0)
   {
      mpc_mul_ui(a.data(), x.data(), boost::multiprecision::detail::unsigned_abs(y), GMP_RNDD);
      a.negate();
   }
   else
      mpc_mul_ui(a.data(), x.data(), y, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_multiply(mpc_complex_backend<D1>& a, unsigned long x, const mpc_complex_backend<D2>& y)
{
   mpc_mul_ui(a.data(), y.data(), x, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_multiply(mpc_complex_backend<D1>& a, long x, const mpc_complex_backend<D2>& y)
{
   if (x < 0)
   {
      mpc_mul_ui(a.data(), y.data(), boost::multiprecision::detail::unsigned_abs(x), GMP_RNDD);
      mpc_neg(a.data(), a.data(), GMP_RNDD);
   }
   else
      mpc_mul_ui(a.data(), y.data(), x, GMP_RNDD);
}

template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_divide(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, const mpc_complex_backend<D3>& y)
{
   mpc_div(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_divide(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, const mpfr_float_backend<D3>& y)
{
   mpc_div_fr(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2, unsigned D3>
inline void eval_divide(mpc_complex_backend<D1>& a, const mpfr_float_backend<D2>& x, const mpc_complex_backend<D3>& y)
{
   mpc_fr_div(a.data(), x.data(), y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_divide(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, unsigned long y)
{
   mpc_div_ui(a.data(), x.data(), y, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_divide(mpc_complex_backend<D1>& a, const mpc_complex_backend<D2>& x, long y)
{
   if (y < 0)
   {
      mpc_div_ui(a.data(), x.data(), boost::multiprecision::detail::unsigned_abs(y), GMP_RNDD);
      a.negate();
   }
   else
      mpc_div_ui(a.data(), x.data(), y, GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_divide(mpc_complex_backend<D1>& a, unsigned long x, const mpc_complex_backend<D2>& y)
{
   mpc_ui_div(a.data(), x, y.data(), GMP_RNDD);
}
template <unsigned D1, unsigned D2>
inline void eval_divide(mpc_complex_backend<D1>& a, long x, const mpc_complex_backend<D2>& y)
{
   if (x < 0)
   {
      mpc_ui_div(a.data(), boost::multiprecision::detail::unsigned_abs(x), y.data(), GMP_RNDD);
      mpc_neg(a.data(), a.data(), GMP_RNDD);
   }
   else
      mpc_ui_div(a.data(), x, y.data(), GMP_RNDD);
}

template <unsigned digits10>
inline bool eval_is_zero(const mpc_complex_backend<digits10>& val) noexcept
{
   return (0 != mpfr_zero_p(mpc_realref(val.data()))) && (0 != mpfr_zero_p(mpc_imagref(val.data())));
}
template <unsigned digits10>
inline int eval_get_sign(const mpc_complex_backend<digits10>&)
{
   static_assert(digits10 == UINT_MAX, "Complex numbers have no sign bit."); // designed to always fail
   return 0;
}

template <unsigned digits10>
inline void eval_convert_to(unsigned long* result, const mpc_complex_backend<digits10>& val)
{
   if (0 == mpfr_zero_p(mpc_imagref(val.data())))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert imaginary number to scalar."));
   }
   mpfr_float_backend<digits10> t;
   mpc_real(t.data(), val.data(), GMP_RNDN);
   eval_convert_to(result, t);
}
template <unsigned digits10>
inline void eval_convert_to(long* result, const mpc_complex_backend<digits10>& val)
{
   if (0 == mpfr_zero_p(mpc_imagref(val.data())))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert imaginary number to scalar."));
   }
   mpfr_float_backend<digits10> t;
   mpc_real(t.data(), val.data(), GMP_RNDN);
   eval_convert_to(result, t);
}
#ifdef _MPFR_H_HAVE_INTMAX_T
template <unsigned digits10>
inline void eval_convert_to(boost::ulong_long_type* result, const mpc_complex_backend<digits10>& val)
{
   if (0 == mpfr_zero_p(mpc_imagref(val.data())))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert imaginary number to scalar."));
   }
   mpfr_float_backend<digits10> t;
   mpc_real(t.data(), val.data(), GMP_RNDN);
   eval_convert_to(result, t);
}
template <unsigned digits10>
inline void eval_convert_to(boost::long_long_type* result, const mpc_complex_backend<digits10>& val)
{
   if (0 == mpfr_zero_p(mpc_imagref(val.data())))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert imaginary number to scalar."));
   }
   mpfr_float_backend<digits10> t;
   mpc_real(t.data(), val.data(), GMP_RNDN);
   eval_convert_to(result, t);
}
#endif
template <unsigned digits10>
inline void eval_convert_to(double* result, const mpc_complex_backend<digits10>& val) noexcept
{
   if (0 == mpfr_zero_p(mpc_imagref(val.data())))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert imaginary number to scalar."));
   }
   mpfr_float_backend<digits10> t;
   mpc_real(t.data(), val.data(), GMP_RNDN);
   eval_convert_to(result, t);
}
template <unsigned digits10>
inline void eval_convert_to(long double* result, const mpc_complex_backend<digits10>& val) noexcept
{
   if (0 == mpfr_zero_p(mpc_imagref(val.data())))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert imaginary number to scalar."));
   }
   mpfr_float_backend<digits10> t;
   mpc_real(t.data(), val.data(), GMP_RNDN);
   eval_convert_to(result, t);
}

template <mpfr_allocation_type AllocationType>
inline void assign_components_set_precision(mpc_complex_backend<0>& result, const mpfr_float_backend<0, AllocationType>& a, const mpfr_float_backend<0, AllocationType>& b)
{
   if (result.thread_default_variable_precision_options() >= variable_precision_options::preserve_component_precision)
   {
      unsigned long prec = (std::max)(mpfr_get_prec(a.data()), mpfr_get_prec(b.data()));
      mpc_set_prec(result.data(), prec);
   }
}
template <unsigned D2, mpfr_allocation_type AllocationType>
inline void assign_components_set_precision(mpc_complex_backend<0>& result, const mpfr_float_backend<D2, AllocationType>& a, const mpfr_float_backend<D2, AllocationType>& b)
{
   if (result.thread_default_variable_precision_options() >= variable_precision_options::preserve_related_precision)
   {
      unsigned long prec = (std::max)(mpfr_get_prec(a.data()), mpfr_get_prec(b.data()));
      mpc_set_prec(result.data(), prec);
   }
}
template <unsigned D1, unsigned D2, mpfr_allocation_type AllocationType>
inline void assign_components_set_precision(mpc_complex_backend<D1>&, const mpfr_float_backend<D2, AllocationType>&, const mpfr_float_backend<D2, AllocationType>&)
{
}

template <unsigned D1, unsigned D2, mpfr_allocation_type AllocationType>
inline void assign_components(mpc_complex_backend<D1>& result, const mpfr_float_backend<D2, AllocationType>& a, const mpfr_float_backend<D2, AllocationType>& b)
{
   //
   // This is called from class number's constructors, so if we have variable
   // precision, then copy the precision of the source variables.
   //
   assign_components_set_precision(result, a, b);
   using default_ops::eval_fpclassify;
   if (eval_fpclassify(a) == (int)FP_NAN)
   {
      mpc_set_fr(result.data(), a.data(), GMP_RNDN);
   }
   else if (eval_fpclassify(b) == (int)FP_NAN)
   {
      mpc_set_fr(result.data(), b.data(), GMP_RNDN);
   }
   else
   {
      mpc_set_fr_fr(result.data(), a.data(), b.data(), GMP_RNDN);
   }
}

template <unsigned D1, unsigned D2, mpfr_allocation_type AllocationType>
inline void assign_components(mpc_complex_backend<D1>& result, unsigned long a, unsigned long b)
{
   mpc_set_ui_ui(result.data(), a, b, GMP_RNDN);
}

template <unsigned D1, unsigned D2, mpfr_allocation_type AllocationType>
inline void assign_components(mpc_complex_backend<D1>& result, long a, long b)
{
   mpc_set_si_si(result.data(), a, b, GMP_RNDN);
}

#if defined(BOOST_HAS_LONG_LONG) && defined(_MPFR_H_HAVE_INTMAX_T)
template <unsigned D1, unsigned D2, mpfr_allocation_type AllocationType>
inline void assign_components(mpc_complex_backend<D1>& result, unsigned long long a, unsigned long long b)
{
   mpc_set_uj_uj(result.data(), a, b, GMP_RNDN);
}

template <unsigned D1, unsigned D2, mpfr_allocation_type AllocationType>
inline void assign_components(mpc_complex_backend<D1>& result, long long a, long long b)
{
   mpc_set_sj_sj(result.data(), a, b, GMP_RNDN);
}
#endif

template <unsigned D1, unsigned D2, mpfr_allocation_type AllocationType>
inline void assign_components(mpc_complex_backend<D1>& result, double a, double b)
{
   if ((boost::math::isnan)(a))
   {
      mpc_set_d(result.data(), a, GMP_RNDN);
   }
   else if ((boost::math::isnan)(b))
   {
      mpc_set_d(result.data(), b, GMP_RNDN);
   }
   else
   {
      mpc_set_d_d(result.data(), a, b, GMP_RNDN);
   }
}

template <unsigned D1, unsigned D2, mpfr_allocation_type AllocationType>
inline void assign_components(mpc_complex_backend<D1>& result, long double a, long double b)
{
   if ((boost::math::isnan)(a))
   {
      mpc_set_d(result.data(), a, GMP_RNDN);
   }
   else if ((boost::math::isnan)(b))
   {
      mpc_set_d(result.data(), b, GMP_RNDN);
   }
   else
   {
      mpc_set_ld_ld(result.data(), a, b, GMP_RNDN);
   }
}

//
// Native non-member operations:
//
template <unsigned Digits10>
inline void eval_sqrt(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& val)
{
   mpc_sqrt(result.data(), val.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_pow(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& b, const mpc_complex_backend<Digits10>& e)
{
   mpc_pow(result.data(), b.data(), e.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_exp(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_exp(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_log(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_log(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_log10(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_log10(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_sin(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_sin(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_cos(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_cos(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_tan(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_tan(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_asin(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_asin(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_acos(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_acos(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_atan(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_atan(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_sinh(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_sinh(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_cosh(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_cosh(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_tanh(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_tanh(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_asinh(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_asinh(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_acosh(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_acosh(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_atanh(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_atanh(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_conj(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_conj(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_proj(mpc_complex_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpc_proj(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_real(mpfr_float_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpfr_set_prec(result.data(), mpfr_get_prec(mpc_realref(arg.data())));
   mpfr_set(result.data(), mpc_realref(arg.data()), GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_imag(mpfr_float_backend<Digits10>& result, const mpc_complex_backend<Digits10>& arg)
{
   mpfr_set_prec(result.data(), mpfr_get_prec(mpc_imagref(arg.data())));
   mpfr_set(result.data(), mpc_imagref(arg.data()), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const mpfr_float_backend<Digits10>& arg)
{
   mpfr_set(mpc_imagref(result.data()), arg.data(), GMP_RNDN);
}

template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const mpfr_float_backend<Digits10>& arg)
{
   mpfr_set(mpc_realref(result.data()), arg.data(), GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const gmp_int& arg)
{
   mpfr_set_z(mpc_realref(result.data()), arg.data(), GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const gmp_rational& arg)
{
   mpfr_set_q(mpc_realref(result.data()), arg.data(), GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const unsigned& arg)
{
   mpfr_set_ui(mpc_realref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const unsigned long& arg)
{
   mpfr_set_ui(mpc_realref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const int& arg)
{
   mpfr_set_si(mpc_realref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const long& arg)
{
   mpfr_set_si(mpc_realref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const float& arg)
{
   mpfr_set_flt(mpc_realref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const double& arg)
{
   mpfr_set_d(mpc_realref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const long double& arg)
{
   mpfr_set_ld(mpc_realref(result.data()), arg, GMP_RNDN);
}
#if defined(BOOST_HAS_LONG_LONG) && defined(_MPFR_H_HAVE_INTMAX_T)
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const unsigned long long& arg)
{
   mpfr_set_uj(mpc_realref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_real(mpc_complex_backend<Digits10>& result, const long long& arg)
{
   mpfr_set_sj(mpc_realref(result.data()), arg, GMP_RNDN);
}
#endif

template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const gmp_int& arg)
{
   mpfr_set_z(mpc_imagref(result.data()), arg.data(), GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const gmp_rational& arg)
{
   mpfr_set_q(mpc_imagref(result.data()), arg.data(), GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const unsigned& arg)
{
   mpfr_set_ui(mpc_imagref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const unsigned long& arg)
{
   mpfr_set_ui(mpc_imagref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const int& arg)
{
   mpfr_set_si(mpc_imagref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const long& arg)
{
   mpfr_set_si(mpc_imagref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const float& arg)
{
   mpfr_set_flt(mpc_imagref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const double& arg)
{
   mpfr_set_d(mpc_imagref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const long double& arg)
{
   mpfr_set_ld(mpc_imagref(result.data()), arg, GMP_RNDN);
}
#if defined(BOOST_HAS_LONG_LONG) && defined(_MPFR_H_HAVE_INTMAX_T)
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const unsigned long long& arg)
{
   mpfr_set_uj(mpc_imagref(result.data()), arg, GMP_RNDN);
}
template <unsigned Digits10>
inline void eval_set_imag(mpc_complex_backend<Digits10>& result, const long long& arg)
{
   mpfr_set_sj(mpc_imagref(result.data()), arg, GMP_RNDN);
}
#endif

template <unsigned Digits10>
inline std::size_t hash_value(const mpc_complex_backend<Digits10>& val)
{
   std::size_t result = 0;
   std::size_t len    = val.data()[0].re[0]._mpfr_prec / mp_bits_per_limb;
   if (val.data()[0].re[0]._mpfr_prec % mp_bits_per_limb)
      ++len;
   for (std::size_t i = 0; i < len; ++i)
      boost::multiprecision::detail::hash_combine(result, val.data()[0].re[0]._mpfr_d[i]);
   boost::multiprecision::detail::hash_combine(result, val.data()[0].re[0]._mpfr_exp, val.data()[0].re[0]._mpfr_sign);

   len = val.data()[0].im[0]._mpfr_prec / mp_bits_per_limb;
   if (val.data()[0].im[0]._mpfr_prec % mp_bits_per_limb)
      ++len;
   for (std::size_t i = 0; i < len; ++i)
      boost::multiprecision::detail::hash_combine(result, val.data()[0].im[0]._mpfr_d[i]);
   boost::multiprecision::detail::hash_combine(result, val.data()[0].im[0]._mpfr_exp, val.data()[0].im[0]._mpfr_sign);
   return result;
}

} // namespace backends

namespace detail {
template <>
struct is_variable_precision<backends::mpc_complex_backend<0> > : public std::integral_constant<bool, true>
{};
} // namespace detail

template <>
struct number_category<detail::canonical<mpc_t, backends::mpc_complex_backend<0> >::type> : public std::integral_constant<int, number_kind_floating_point>
{};

using boost::multiprecision::backends::mpc_complex_backend;

using mpc_complex_50 = number<mpc_complex_backend<50> >  ;
using mpc_complex_100 = number<mpc_complex_backend<100> > ;
using mpc_complex_500 = number<mpc_complex_backend<500> > ;
using mpc_complex_1000 = number<mpc_complex_backend<1000> >;
using mpc_complex = number<mpc_complex_backend<0> >   ;

template <unsigned Digits10, expression_template_option ExpressionTemplates>
struct component_type<number<mpc_complex_backend<Digits10>, ExpressionTemplates> >
{
   using type = number<mpfr_float_backend<Digits10>, ExpressionTemplates>;
};

template <unsigned Digits10, expression_template_option ExpressionTemplates>
struct component_type<number<backends::logged_adaptor<mpc_complex_backend<Digits10> >, ExpressionTemplates> >
{
   using type = number<mpfr_float_backend<Digits10>, ExpressionTemplates>;
};
template <unsigned Digits10, expression_template_option ExpressionTemplates>
struct component_type<number<backends::debug_adaptor<mpc_complex_backend<Digits10> >, ExpressionTemplates> >
{
   using type = number<backends::debug_adaptor<mpfr_float_backend<Digits10> >, ExpressionTemplates>;
};

template <unsigned Digits10, expression_template_option ExpressionTemplates>
struct complex_result_from_scalar<number<mpfr_float_backend<Digits10>, ExpressionTemplates> >
{
   using type = number<mpc_complex_backend<Digits10>, ExpressionTemplates>;
};
template <unsigned Digits10, expression_template_option ExpressionTemplates>
struct complex_result_from_scalar<number<backends::logged_adaptor<mpfr_float_backend<Digits10>>, ExpressionTemplates> >
{
   using type = number<mpc_complex_backend<Digits10>, ExpressionTemplates>;
};
template <unsigned Digits10, expression_template_option ExpressionTemplates>
struct complex_result_from_scalar<number<backends::debug_adaptor<mpfr_float_backend<Digits10>>, ExpressionTemplates> >
{
   using type = number<backends::debug_adaptor<mpc_complex_backend<Digits10> >, ExpressionTemplates>;
};

}

} // namespace boost::multiprecision

#endif
