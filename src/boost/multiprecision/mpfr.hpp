///////////////////////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BN_MPFR_HPP
#define BOOST_MATH_BN_MPFR_HPP

#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/multiprecision/logged_adaptor.hpp>
#include <boost/multiprecision/gmp.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <cstdint>
#include <boost/multiprecision/detail/digits.hpp>
#include <boost/multiprecision/detail/atomic.hpp>
#include <boost/multiprecision/traits/max_digits10.hpp>
#include <boost/multiprecision/detail/hash.hpp>
#include <mpfr.h>
#include <cmath>
#include <algorithm>
#include <utility>
#include <type_traits>
#include <atomic>

#ifndef BOOST_MULTIPRECISION_MPFR_DEFAULT_PRECISION
#define BOOST_MULTIPRECISION_MPFR_DEFAULT_PRECISION 20
#endif

namespace boost {
namespace multiprecision {

enum mpfr_allocation_type
{
   allocate_stack,
   allocate_dynamic
};

namespace backends {

template <unsigned digits10, mpfr_allocation_type AllocationType = allocate_dynamic>
struct mpfr_float_backend;

template <>
struct mpfr_float_backend<0, allocate_stack>;

} // namespace backends

template <unsigned digits10, mpfr_allocation_type AllocationType>
struct number_category<backends::mpfr_float_backend<digits10, AllocationType> > : public std::integral_constant<int, number_kind_floating_point>
{};

namespace backends {

namespace detail {

template <bool b>
struct mpfr_cleanup
{
   //
   // There are 2 seperate cleanup objects here, one calls
   // mpfr_free_cache on destruction to perform global cleanup
   // the other is declared thread_local and calls
   // mpfr_free_cache2(MPFR_FREE_LOCAL_CACHE) to free thread local data.
   //
   struct initializer
   {
      initializer() {}
      ~initializer() { mpfr_free_cache(); }
      void force_instantiate() const {}
   };
#if MPFR_VERSION_MAJOR >= 4
   struct thread_initializer
   {
      thread_initializer() {}
      ~thread_initializer() { mpfr_free_cache2(MPFR_FREE_LOCAL_CACHE); }
      void force_instantiate() const {}
   };
#endif
   static const initializer init;
   static void              force_instantiate()
   {
#if MPFR_VERSION_MAJOR >= 4
      static const BOOST_MP_THREAD_LOCAL thread_initializer thread_init;
      thread_init.force_instantiate();
#endif
      init.force_instantiate();
   }
};

template <bool b>
typename mpfr_cleanup<b>::initializer const mpfr_cleanup<b>::init;

inline void mpfr_copy_precision(mpfr_t dest, const mpfr_t src)
{
   mpfr_prec_t p_dest = mpfr_get_prec(dest);
   mpfr_prec_t p_src  = mpfr_get_prec(src);
   if (p_dest != p_src)
      mpfr_set_prec(dest, p_src);
}
inline void mpfr_copy_precision(mpfr_t dest, const mpfr_t src1, const mpfr_t src2)
{
   mpfr_prec_t p_dest = mpfr_get_prec(dest);
   mpfr_prec_t p_src1 = mpfr_get_prec(src1);
   mpfr_prec_t p_src2 = mpfr_get_prec(src2);
   if (p_src2 > p_src1)
      p_src1 = p_src2;
   if (p_dest != p_src1)
      mpfr_set_prec(dest, p_src1);
}

template <unsigned digits10, mpfr_allocation_type AllocationType>
struct mpfr_float_imp;

template <unsigned digits10>
struct mpfr_float_imp<digits10, allocate_dynamic>
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

   mpfr_float_imp()
   {
      mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpfr_set_ui(m_data, 0u, GMP_RNDN);
   }
   mpfr_float_imp(unsigned digits2)
   {
      mpfr_init2(m_data, digits2);
      mpfr_set_ui(m_data, 0u, GMP_RNDN);
   }

   mpfr_float_imp(const mpfr_float_imp& o)
   {
      mpfr_init2(m_data, preserve_source_precision() ? mpfr_get_prec(o.data()) : boost::multiprecision::detail::digits10_2_2(get_default_precision()));
      if (o.m_data[0]._mpfr_d)
         mpfr_set(m_data, o.m_data, GMP_RNDN);
   }
   // rvalue copy
   mpfr_float_imp(mpfr_float_imp&& o) noexcept
   {
      mpfr_prec_t binary_default_precision = boost::multiprecision::detail::digits10_2_2(get_default_precision());
      if ((this->get_default_options() != variable_precision_options::preserve_target_precision) || (mpfr_get_prec(o.data()) == binary_default_precision))
      {
         m_data[0] = o.m_data[0];
         o.m_data[0]._mpfr_d = 0;
      }
      else
      {
         // NOTE: C allocation interface must not throw:
         mpfr_init2(m_data, binary_default_precision);
         if (o.m_data[0]._mpfr_d)
            mpfr_set(m_data, o.m_data, GMP_RNDN);
      }
   }
   mpfr_float_imp& operator=(const mpfr_float_imp& o)
   {
      if ((o.m_data[0]._mpfr_d) && (this != &o))
      {
         if (m_data[0]._mpfr_d == 0)
         {
            mpfr_init2(m_data, preserve_source_precision() ? mpfr_get_prec(o.m_data) : boost::multiprecision::detail::digits10_2_2(get_default_precision()));
         }
         else if (preserve_source_precision() && (mpfr_get_prec(o.data()) != mpfr_get_prec(data())))
         {
            mpfr_set_prec(m_data, mpfr_get_prec(o.m_data));
         }
         mpfr_set(m_data, o.m_data, GMP_RNDN);
      }
      return *this;
   }
   // rvalue assign
   mpfr_float_imp& operator=(mpfr_float_imp&& o) noexcept
   {
      if ((this->get_default_options() != variable_precision_options::preserve_target_precision) || (mpfr_get_prec(o.data()) == mpfr_get_prec(data())))
         mpfr_swap(m_data, o.m_data);
      else
         *this = static_cast<const mpfr_float_imp&>(o);
      return *this;
   }
#ifdef BOOST_HAS_LONG_LONG
#ifdef _MPFR_H_HAVE_INTMAX_T
   mpfr_float_imp& operator=(boost::ulong_long_type i)
   {
      if (m_data[0]._mpfr_d == 0)
         mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpfr_set_uj(m_data, i, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(boost::long_long_type i)
   {
      if (m_data[0]._mpfr_d == 0)
         mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpfr_set_sj(m_data, i, GMP_RNDN);
      return *this;
   }
#else
   mpfr_float_imp& operator=(boost::ulong_long_type i)
   {
      if (m_data[0]._mpfr_d == 0)
         mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      boost::ulong_long_type mask  = ((((1uLL << (std::numeric_limits<unsigned long>::digits - 1)) - 1) << 1) | 1uLL);
      unsigned               shift = 0;
      mpfr_t                 t;
      mpfr_init2(t, (std::max)(static_cast<mpfr_prec_t>(std::numeric_limits<boost::ulong_long_type>::digits), static_cast<mpfr_prec_t>(mpfr_get_prec(m_data))));
      mpfr_set_ui(m_data, 0, GMP_RNDN);
      while (i)
      {
         mpfr_set_ui(t, static_cast<unsigned long>(i & mask), GMP_RNDN);
         if (shift)
            mpfr_mul_2exp(t, t, shift, GMP_RNDN);
         mpfr_add(m_data, m_data, t, GMP_RNDN);
         shift += std::numeric_limits<unsigned long>::digits;
         i >>= std::numeric_limits<unsigned long>::digits;
      }
      mpfr_clear(t);
      return *this;
   }
   mpfr_float_imp& operator=(boost::long_long_type i)
   {
      if (m_data[0]._mpfr_d == 0)
         mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      bool neg = i < 0;
      *this    = boost::multiprecision::detail::unsigned_abs(i);
      if (neg)
         mpfr_neg(m_data, m_data, GMP_RNDN);
      return *this;
   }
#endif
#endif
   mpfr_float_imp& operator=(unsigned long i)
   {
      if (m_data[0]._mpfr_d == 0)
         mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpfr_set_ui(m_data, i, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(long i)
   {
      if (m_data[0]._mpfr_d == 0)
         mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpfr_set_si(m_data, i, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(double d)
   {
      if (m_data[0]._mpfr_d == 0)
         mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpfr_set_d(m_data, d, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(long double a)
   {
      if (m_data[0]._mpfr_d == 0)
         mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      mpfr_set_ld(m_data, a, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(const char* s)
   {
      if (m_data[0]._mpfr_d == 0)
         mpfr_init2(m_data, multiprecision::detail::digits10_2_2(digits10 ? digits10 : (unsigned)get_default_precision()));
      if (mpfr_set_str(m_data, s, 10, GMP_RNDN) != 0)
      {
         BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Unable to parse string \"") + s + std::string("\"as a valid floating point number.")));
      }
      return *this;
   }
   void swap(mpfr_float_imp& o) noexcept
   {
      mpfr_swap(m_data, o.m_data);
   }
   std::string str(std::streamsize digits, std::ios_base::fmtflags f) const
   {
      BOOST_ASSERT(m_data[0]._mpfr_d);

      bool scientific = (f & std::ios_base::scientific) == std::ios_base::scientific;
      bool fixed      = (f & std::ios_base::fixed) == std::ios_base::fixed;

      std::streamsize org_digits(digits);

      if (scientific && digits)
         ++digits;

      std::string result;
      mp_exp_t    e;
      if (mpfr_inf_p(m_data))
      {
         if (mpfr_sgn(m_data) < 0)
            result = "-inf";
         else if (f & std::ios_base::showpos)
            result = "+inf";
         else
            result = "inf";
         return result;
      }
      if (mpfr_nan_p(m_data))
      {
         result = "nan";
         return result;
      }
      if (mpfr_zero_p(m_data))
      {
         e      = 0;
         result = "0";
      }
      else
      {
         char* ps = mpfr_get_str(0, &e, 10, static_cast<std::size_t>(digits), m_data, GMP_RNDN);
         --e; // To match with what our formatter expects.
         if (fixed && e != -1)
         {
            // Oops we actually need a different number of digits to what we asked for:
            mpfr_free_str(ps);
            digits += e + 1;
            if (digits == 0)
            {
               // We need to get *all* the digits and then possibly round up,
               // we end up with either "0" or "1" as the result.
               ps = mpfr_get_str(0, &e, 10, 0, m_data, GMP_RNDN);
               --e;
               unsigned offset = *ps == '-' ? 1 : 0;
               if (ps[offset] > '5')
               {
                  ++e;
                  ps[offset]     = '1';
                  ps[offset + 1] = 0;
               }
               else if (ps[offset] == '5')
               {
                  unsigned i        = offset + 1;
                  bool     round_up = false;
                  while (ps[i] != 0)
                  {
                     if (ps[i] != '0')
                     {
                        round_up = true;
                        break;
                     }
                     ++i;
                  }
                  if (round_up)
                  {
                     ++e;
                     ps[offset]     = '1';
                     ps[offset + 1] = 0;
                  }
                  else
                  {
                     ps[offset]     = '0';
                     ps[offset + 1] = 0;
                  }
               }
               else
               {
                  ps[offset]     = '0';
                  ps[offset + 1] = 0;
               }
            }
            else if (digits > 0)
            {
               mp_exp_t old_e = e;
               ps             = mpfr_get_str(0, &e, 10, static_cast<std::size_t>(digits), m_data, GMP_RNDN);
               --e; // To match with what our formatter expects.
               if (old_e > e)
               {
                  // in some cases, when we ask for more digits of precision, it will
                  // change the number of digits to the left of the decimal, if that
                  // happens, account for it here.
                  // example: cout << fixed << setprecision(3) << mpf_float_50("99.9809")
                  mpfr_free_str(ps);
                  digits -= old_e - e;
                  ps = mpfr_get_str(0, &e, 10, static_cast<std::size_t>(digits), m_data, GMP_RNDN);
                  --e; // To match with what our formatter expects.
               }
            }
            else
            {
               ps = mpfr_get_str(0, &e, 10, 1, m_data, GMP_RNDN);
               --e;
               unsigned offset = *ps == '-' ? 1 : 0;
               ps[offset]      = '0';
               ps[offset + 1]  = 0;
            }
         }
         result = ps ? ps : "0";
         if (ps)
            mpfr_free_str(ps);
      }
      boost::multiprecision::detail::format_float_string(result, e, org_digits, f, 0 != mpfr_zero_p(m_data));
      return result;
   }
   ~mpfr_float_imp() noexcept
   {
      if (m_data[0]._mpfr_d)
         mpfr_clear(m_data);
      detail::mpfr_cleanup<true>::force_instantiate();
   }
   void negate() noexcept
   {
      BOOST_ASSERT(m_data[0]._mpfr_d);
      mpfr_neg(m_data, m_data, GMP_RNDN);
   }
   template <mpfr_allocation_type AllocationType>
   int compare(const mpfr_float_backend<digits10, AllocationType>& o) const
   {
      BOOST_ASSERT(m_data[0]._mpfr_d && o.m_data[0]._mpfr_d);
      return mpfr_cmp(m_data, o.m_data);
   }
   int compare(long i) const
   {
      BOOST_ASSERT(m_data[0]._mpfr_d);
      return mpfr_cmp_si(m_data, i);
   }
   int compare(double i) const
   {
      BOOST_ASSERT(m_data[0]._mpfr_d);
      return mpfr_cmp_d(m_data, i);
   }
   int compare(long double i) const
   {
      BOOST_ASSERT(m_data[0]._mpfr_d);
      return mpfr_cmp_ld(m_data, i);
   }
   int compare(unsigned long i) const
   {
      BOOST_ASSERT(m_data[0]._mpfr_d);
      return mpfr_cmp_ui(m_data, i);
   }
   template <class V>
   int compare(V v) const
   {
      mpfr_float_backend<digits10, allocate_dynamic> d(0uL, mpfr_get_prec(m_data));
      d = v;
      return compare(d);
   }
   mpfr_t& data() noexcept
   {
      BOOST_ASSERT(m_data[0]._mpfr_d);
      return m_data;
   }
   const mpfr_t& data() const noexcept
   {
      BOOST_ASSERT(m_data[0]._mpfr_d);
      return m_data;
   }

 protected:
   mpfr_t           m_data;
   static boost::multiprecision::detail::precision_type& get_global_default_precision() noexcept
   {
      static boost::multiprecision::detail::precision_type val(BOOST_MULTIPRECISION_MPFR_DEFAULT_PRECISION);
      return val;
   }
   static unsigned& get_default_precision() noexcept
   {
      static BOOST_MP_THREAD_LOCAL unsigned val(get_global_default_precision());
      return val;
   }
#ifndef BOOST_MT_NO_ATOMIC_INT
   static std::atomic<variable_precision_options>& get_global_default_options() noexcept
   {
      static std::atomic<variable_precision_options> val{variable_precision_options::preserve_related_precision};
      return val;
   }
#else
   static variable_precision_options& get_global_default_options() noexcept
   {
      static variable_precision_options val{variable_precision_options::preserve_related_precision};
      return val;
   }
#endif
   static variable_precision_options& get_default_options()noexcept
   {
      static BOOST_MP_THREAD_LOCAL variable_precision_options val(get_global_default_options());
      return val;
   }
   static bool preserve_source_precision() noexcept
   {
      return get_default_options() >= variable_precision_options::preserve_source_precision;
   }
};

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4127) // Conditional expression is constant
#endif

template <unsigned digits10>
struct mpfr_float_imp<digits10, allocate_stack>
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

   static constexpr const unsigned digits2    = (digits10 * 1000uL) / 301uL + ((digits10 * 1000uL) % 301 ? 2u : 1u);
   static constexpr const unsigned limb_count = mpfr_custom_get_size(digits2) / sizeof(mp_limb_t);

   ~mpfr_float_imp() noexcept
   {
      detail::mpfr_cleanup<true>::force_instantiate();
   }
   mpfr_float_imp()
   {
      mpfr_custom_init(m_buffer, digits2);
      mpfr_custom_init_set(m_data, MPFR_NAN_KIND, 0, digits2, m_buffer);
      mpfr_set_ui(m_data, 0u, GMP_RNDN);
   }

   mpfr_float_imp(const mpfr_float_imp& o)
   {
      mpfr_custom_init(m_buffer, digits2);
      mpfr_custom_init_set(m_data, MPFR_NAN_KIND, 0, digits2, m_buffer);
      mpfr_set(m_data, o.m_data, GMP_RNDN);
   }
   mpfr_float_imp& operator=(const mpfr_float_imp& o)
   {
      mpfr_set(m_data, o.m_data, GMP_RNDN);
      return *this;
   }
#ifdef BOOST_HAS_LONG_LONG
#ifdef _MPFR_H_HAVE_INTMAX_T
   mpfr_float_imp& operator=(boost::ulong_long_type i)
   {
      mpfr_set_uj(m_data, i, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(boost::long_long_type i)
   {
      mpfr_set_sj(m_data, i, GMP_RNDN);
      return *this;
   }
#else
   mpfr_float_imp& operator=(boost::ulong_long_type i)
   {
      boost::ulong_long_type mask  = ((((1uLL << (std::numeric_limits<unsigned long>::digits - 1)) - 1) << 1) | 1uL);
      unsigned               shift = 0;
      mpfr_t                 t;
      mp_limb_t              t_limbs[limb_count];
      mpfr_custom_init(t_limbs, digits2);
      mpfr_custom_init_set(t, MPFR_NAN_KIND, 0, digits2, t_limbs);
      mpfr_set_ui(m_data, 0, GMP_RNDN);
      while (i)
      {
         mpfr_set_ui(t, static_cast<unsigned long>(i & mask), GMP_RNDN);
         if (shift)
            mpfr_mul_2exp(t, t, shift, GMP_RNDN);
         mpfr_add(m_data, m_data, t, GMP_RNDN);
         shift += std::numeric_limits<unsigned long>::digits;
         i >>= std::numeric_limits<unsigned long>::digits;
      }
      return *this;
   }
   mpfr_float_imp& operator=(boost::long_long_type i)
   {
      bool neg = i < 0;
      *this    = boost::multiprecision::detail::unsigned_abs(i);
      if (neg)
         mpfr_neg(m_data, m_data, GMP_RNDN);
      return *this;
   }
#endif
#endif
   mpfr_float_imp& operator=(unsigned long i)
   {
      mpfr_set_ui(m_data, i, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(long i)
   {
      mpfr_set_si(m_data, i, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(double d)
   {
      mpfr_set_d(m_data, d, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(long double a)
   {
      mpfr_set_ld(m_data, a, GMP_RNDN);
      return *this;
   }
   mpfr_float_imp& operator=(const char* s)
   {
      if (mpfr_set_str(m_data, s, 10, GMP_RNDN) != 0)
      {
         BOOST_THROW_EXCEPTION(std::runtime_error(std::string("Unable to parse string \"") + s + std::string("\"as a valid floating point number.")));
      }
      return *this;
   }
   void swap(mpfr_float_imp& o) noexcept
   {
      // We have to swap by copying:
      mpfr_float_imp t(*this);
      *this = o;
      o     = t;
   }
   std::string str(std::streamsize digits, std::ios_base::fmtflags f) const
   {
      BOOST_ASSERT(m_data[0]._mpfr_d);

      bool scientific = (f & std::ios_base::scientific) == std::ios_base::scientific;
      bool fixed      = (f & std::ios_base::fixed) == std::ios_base::fixed;

      std::streamsize org_digits(digits);

      if (scientific && digits)
         ++digits;

      std::string result;
      mp_exp_t    e;
      if (mpfr_inf_p(m_data))
      {
         if (mpfr_sgn(m_data) < 0)
            result = "-inf";
         else if (f & std::ios_base::showpos)
            result = "+inf";
         else
            result = "inf";
         return result;
      }
      if (mpfr_nan_p(m_data))
      {
         result = "nan";
         return result;
      }
      if (mpfr_zero_p(m_data))
      {
         e      = 0;
         result = "0";
      }
      else
      {
         char* ps = mpfr_get_str(0, &e, 10, static_cast<std::size_t>(digits), m_data, GMP_RNDN);
         --e; // To match with what our formatter expects.
         if (fixed && e != -1)
         {
            // Oops we actually need a different number of digits to what we asked for:
            mpfr_free_str(ps);
            digits += e + 1;
            if (digits == 0)
            {
               // We need to get *all* the digits and then possibly round up,
               // we end up with either "0" or "1" as the result.
               ps = mpfr_get_str(0, &e, 10, 0, m_data, GMP_RNDN);
               --e;
               unsigned offset = *ps == '-' ? 1 : 0;
               if (ps[offset] > '5')
               {
                  ++e;
                  ps[offset]     = '1';
                  ps[offset + 1] = 0;
               }
               else if (ps[offset] == '5')
               {
                  unsigned i        = offset + 1;
                  bool     round_up = false;
                  while (ps[i] != 0)
                  {
                     if (ps[i] != '0')
                     {
                        round_up = true;
                        break;
                     }
                  }
                  if (round_up)
                  {
                     ++e;
                     ps[offset]     = '1';
                     ps[offset + 1] = 0;
                  }
                  else
                  {
                     ps[offset]     = '0';
                     ps[offset + 1] = 0;
                  }
               }
               else
               {
                  ps[offset]     = '0';
                  ps[offset + 1] = 0;
               }
            }
            else if (digits > 0)
            {
               ps = mpfr_get_str(0, &e, 10, static_cast<std::size_t>(digits), m_data, GMP_RNDN);
               --e; // To match with what our formatter expects.
            }
            else
            {
               ps = mpfr_get_str(0, &e, 10, 1, m_data, GMP_RNDN);
               --e;
               unsigned offset = *ps == '-' ? 1 : 0;
               ps[offset]      = '0';
               ps[offset + 1]  = 0;
            }
         }
         result = ps ? ps : "0";
         if (ps)
            mpfr_free_str(ps);
      }
      boost::multiprecision::detail::format_float_string(result, e, org_digits, f, 0 != mpfr_zero_p(m_data));
      return result;
   }
   void negate() noexcept
   {
      mpfr_neg(m_data, m_data, GMP_RNDN);
   }
   template <mpfr_allocation_type AllocationType>
   int compare(const mpfr_float_backend<digits10, AllocationType>& o) const
   {
      return mpfr_cmp(m_data, o.m_data);
   }
   int compare(long i) const
   {
      return mpfr_cmp_si(m_data, i);
   }
   int compare(unsigned long i) const
   {
      return mpfr_cmp_ui(m_data, i);
   }
   int compare(double i) const
   {
      return mpfr_cmp_d(m_data, i);
   }
   int compare(long double i) const
   {
      return mpfr_cmp_ld(m_data, i);
   }
   template <class V>
   int compare(V v) const
   {
      mpfr_float_backend<digits10, allocate_stack> d;
      d = v;
      return compare(d);
   }
   mpfr_t& data() noexcept
   {
      return m_data;
   }
   const mpfr_t& data() const noexcept
   {
      return m_data;
   }

 protected:
   mpfr_t    m_data;
   mp_limb_t m_buffer[limb_count];
};

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

} // namespace detail

template <unsigned digits10, mpfr_allocation_type AllocationType>
struct mpfr_float_backend : public detail::mpfr_float_imp<digits10, AllocationType>
{
   mpfr_float_backend() : detail::mpfr_float_imp<digits10, AllocationType>() {}
   mpfr_float_backend(const mpfr_float_backend& o) : detail::mpfr_float_imp<digits10, AllocationType>(o) {}
   // rvalue copy
   mpfr_float_backend(mpfr_float_backend&& o) noexcept : detail::mpfr_float_imp<digits10, AllocationType>(static_cast<detail::mpfr_float_imp<digits10, AllocationType>&&>(o))
   {}
   template <unsigned D, mpfr_allocation_type AT>
   mpfr_float_backend(const mpfr_float_backend<D, AT>& val, typename std::enable_if<D <= digits10>::type* = 0)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D, mpfr_allocation_type AT>
   explicit mpfr_float_backend(const mpfr_float_backend<D, AT>& val, typename std::enable_if<!(D <= digits10)>::type* = 0)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D>
   mpfr_float_backend(const gmp_float<D>& val, typename std::enable_if<D <= digits10>::type* = 0)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set_f(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D>
   mpfr_float_backend(const gmp_float<D>& val, typename std::enable_if<!(D <= digits10)>::type* = 0)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set_f(this->m_data, val.data(), GMP_RNDN);
   }
   mpfr_float_backend(const gmp_int& val)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set_z(this->m_data, val.data(), GMP_RNDN);
   }
   mpfr_float_backend(const gmp_rational& val)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set_q(this->m_data, val.data(), GMP_RNDN);
   }
   mpfr_float_backend(const mpfr_t val)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set(this->m_data, val, GMP_RNDN);
   }
   mpfr_float_backend(const mpf_t val)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set_f(this->m_data, val, GMP_RNDN);
   }
   mpfr_float_backend(const mpz_t val)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set_z(this->m_data, val, GMP_RNDN);
   }
   mpfr_float_backend(const mpq_t val)
       : detail::mpfr_float_imp<digits10, AllocationType>()
   {
      mpfr_set_q(this->m_data, val, GMP_RNDN);
   }
   // Construction with precision: we ignore the precision here.
   template <class V>
   mpfr_float_backend(const V& o, unsigned)
   {
      *this = o;
   }
   mpfr_float_backend& operator=(const mpfr_float_backend& o)
   {
      *static_cast<detail::mpfr_float_imp<digits10, AllocationType>*>(this) = static_cast<detail::mpfr_float_imp<digits10, AllocationType> const&>(o);
      return *this;
   }
   // rvalue assign
   mpfr_float_backend& operator=(mpfr_float_backend&& o) noexcept
   {
      *static_cast<detail::mpfr_float_imp<digits10, AllocationType>*>(this) = static_cast<detail::mpfr_float_imp<digits10, AllocationType>&&>(o);
      return *this;
   }
   template <class V>
   mpfr_float_backend& operator=(const V& v)
   {
      *static_cast<detail::mpfr_float_imp<digits10, AllocationType>*>(this) = v;
      return *this;
   }
   mpfr_float_backend& operator=(const mpfr_t val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(digits10));
      mpfr_set(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const mpf_t val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(digits10));
      mpfr_set_f(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const mpz_t val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(digits10));
      mpfr_set_z(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const mpq_t val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(digits10));
      mpfr_set_q(this->m_data, val, GMP_RNDN);
      return *this;
   }
   // We don't change our precision here, this is a fixed precision type:
   template <unsigned D, mpfr_allocation_type AT>
   mpfr_float_backend& operator=(const mpfr_float_backend<D, AT>& val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(digits10));
      mpfr_set(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   template <unsigned D>
   mpfr_float_backend& operator=(const gmp_float<D>& val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(digits10));
      mpfr_set_f(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const gmp_int& val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(digits10));
      mpfr_set_z(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const gmp_rational& val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(digits10));
      mpfr_set_q(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
};

template <>
struct mpfr_float_backend<0, allocate_dynamic> : public detail::mpfr_float_imp<0, allocate_dynamic>
{
   mpfr_float_backend() : detail::mpfr_float_imp<0, allocate_dynamic>() {}
   mpfr_float_backend(const mpfr_t val)
       : detail::mpfr_float_imp<0, allocate_dynamic>(preserve_all_precision() ? (unsigned)mpfr_get_prec(val) : boost::multiprecision::detail::digits10_2_2(get_default_precision()))
   {
      mpfr_set(this->m_data, val, GMP_RNDN);
   }
   mpfr_float_backend(const mpf_t val)
       : detail::mpfr_float_imp<0, allocate_dynamic>(preserve_all_precision() ? (unsigned)mpf_get_prec(val) : boost::multiprecision::detail::digits10_2_2(get_default_precision()))
   {
      mpfr_set_f(this->m_data, val, GMP_RNDN);
   }
   mpfr_float_backend(const mpz_t val)
       : detail::mpfr_float_imp<0, allocate_dynamic>()
   {
      mpfr_set_z(this->m_data, val, GMP_RNDN);
   }
   mpfr_float_backend(const mpq_t val)
       : detail::mpfr_float_imp<0, allocate_dynamic>()
   {
      mpfr_set_q(this->m_data, val, GMP_RNDN);
   }
   mpfr_float_backend(const mpfr_float_backend& o) : detail::mpfr_float_imp<0, allocate_dynamic>(o) {}
   // rvalue copy
   mpfr_float_backend(mpfr_float_backend&& o) noexcept : detail::mpfr_float_imp<0, allocate_dynamic>(static_cast<detail::mpfr_float_imp<0, allocate_dynamic>&&>(o))
   {}
   template <class V>
   mpfr_float_backend(const V& o, unsigned digits10)
       : detail::mpfr_float_imp<0, allocate_dynamic>(multiprecision::detail::digits10_2_2(digits10))
   {
      *this = o;
   }
#ifndef BOOST_NO_CXX17_HDR_STRING_VIEW
   mpfr_float_backend(const std::string_view& o, unsigned digits10)
       : detail::mpfr_float_imp<0, allocate_dynamic>(multiprecision::detail::digits10_2_2(digits10))
   {
      std::string s(o);
      *this = s.c_str();
   }
#endif
   template <unsigned D>
   mpfr_float_backend(const gmp_float<D>& val, unsigned digits10)
       : detail::mpfr_float_imp<0, allocate_dynamic>(multiprecision::detail::digits10_2_2(digits10))
   {
      mpfr_set_f(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D>
   mpfr_float_backend(const mpfr_float_backend<D>& val, unsigned digits10)
       : detail::mpfr_float_imp<0, allocate_dynamic>(multiprecision::detail::digits10_2_2(digits10))
   {
      mpfr_set(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D>
   mpfr_float_backend(const mpfr_float_backend<D>& val)
       : detail::mpfr_float_imp<0, allocate_dynamic>(preserve_related_precision() ? (unsigned)mpfr_get_prec(val.data()) : boost::multiprecision::detail::digits10_2_2(get_default_precision()))
   {
      mpfr_set(this->m_data, val.data(), GMP_RNDN);
   }
   template <unsigned D>
   mpfr_float_backend(const gmp_float<D>& val)
       : detail::mpfr_float_imp<0, allocate_dynamic>(preserve_all_precision() ? (unsigned)mpf_get_prec(val.data()) : boost::multiprecision::detail::digits10_2_2(get_default_precision()))
   {
      mpfr_set_f(this->m_data, val.data(), GMP_RNDN);
   }
   mpfr_float_backend(const gmp_int& val)
       : detail::mpfr_float_imp<0, allocate_dynamic>(preserve_all_precision() ? used_gmp_int_bits(val) : boost::multiprecision::detail::digits10_2_2(thread_default_precision()))
   {
      mpfr_set_z(this->m_data, val.data(), GMP_RNDN);
   }
   mpfr_float_backend(const gmp_rational& val)
       : detail::mpfr_float_imp<0, allocate_dynamic>(preserve_all_precision() ? used_gmp_rational_bits(val) : boost::multiprecision::detail::digits10_2_2(thread_default_precision()))
   {
      mpfr_set_q(this->m_data, val.data(), GMP_RNDN);
   }

   mpfr_float_backend& operator=(const mpfr_float_backend& o) = default;
   // rvalue assign
   mpfr_float_backend& operator=(mpfr_float_backend&& o) noexcept = default;

   template <class V>
   mpfr_float_backend& operator=(const V& v)
   {
      constexpr unsigned d10 = std::is_floating_point<V>::value ?
         std::numeric_limits<V>::digits10 :
         std::numeric_limits<V>::digits10 ? 1 + std::numeric_limits<V>::digits10 :
         1 + boost::multiprecision::detail::digits2_2_10(std::numeric_limits<V>::digits);

      if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
      {
         BOOST_IF_CONSTEXPR(std::is_floating_point<V>::value)
         {
            if (std::numeric_limits<V>::digits > mpfr_get_prec(this->data()))
               mpfr_set_prec(this->data(), std::numeric_limits<V>::digits);
         }
         else
         {
            if(precision() < d10)
               this->precision(d10);
         }
      }

      *static_cast<detail::mpfr_float_imp<0, allocate_dynamic>*>(this) = v;
      return *this;
   }
   mpfr_float_backend& operator=(const mpfr_t val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, preserve_all_precision() ? mpfr_get_prec(val) : boost::multiprecision::detail::digits10_2_2(get_default_precision()));
      else if(preserve_all_precision())
         mpfr_set_prec(this->m_data, mpfr_get_prec(val));
      mpfr_set(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const mpf_t val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, preserve_all_precision() ? (mpfr_prec_t)mpf_get_prec(val) : boost::multiprecision::detail::digits10_2_2(get_default_precision()));
      else if(preserve_all_precision())
         mpfr_set_prec(this->m_data, (unsigned)mpf_get_prec(val));
      mpfr_set_f(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const mpz_t val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(get_default_precision()));
      mpfr_set_z(this->m_data, val, GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const mpq_t val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(get_default_precision()));
      mpfr_set_q(this->m_data, val, GMP_RNDN);
      return *this;
   }
   template <unsigned D>
   mpfr_float_backend& operator=(const mpfr_float_backend<D>& val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, preserve_related_precision() ? (mpfr_prec_t)mpfr_get_prec(val.data()) : boost::multiprecision::detail::digits10_2_2(get_default_precision()));
      else if (preserve_related_precision())
         mpfr_set_prec(this->m_data, mpfr_get_prec(val.data()));
      mpfr_set(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   template <unsigned D>
   mpfr_float_backend& operator=(const gmp_float<D>& val)
   {
      if (this->m_data[0]._mpfr_d == 0)
         mpfr_init2(this->m_data, preserve_all_precision() ? (mpfr_prec_t)mpf_get_prec(val.data()) : boost::multiprecision::detail::digits10_2_2(get_default_precision()));
      else if (preserve_all_precision())
         mpfr_set_prec(this->m_data, (mpfr_prec_t)mpf_get_prec(val.data()));
      mpfr_set_f(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const gmp_int& val)
   {
      if (this->m_data[0]._mpfr_d == 0)
      {
         unsigned requested_precision = this->thread_default_precision();
         if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
         {
            unsigned d2 = used_gmp_int_bits(val);
            unsigned d10 = 1 + multiprecision::detail::digits2_2_10(d2);
            if (d10 > requested_precision)
               requested_precision = d10;
         }
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(requested_precision));
      }
      else if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
      {
         unsigned requested_precision = this->thread_default_precision();
         unsigned d2 = used_gmp_int_bits(val);
         unsigned d10 = 1 + multiprecision::detail::digits2_2_10(d2);
         if (d10 > requested_precision)
            this->precision(d10);
      }
      mpfr_set_z(this->m_data, val.data(), GMP_RNDN);
      return *this;
   }
   mpfr_float_backend& operator=(const gmp_rational& val)
   {
      if (this->m_data[0]._mpfr_d == 0)
      {
         unsigned requested_precision = this->get_default_precision();
         if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
         {
            unsigned d10 = 1 + multiprecision::detail::digits2_2_10(used_gmp_rational_bits(val));
            if (d10 > requested_precision)
               requested_precision = d10;
         }
         mpfr_init2(this->m_data, multiprecision::detail::digits10_2_2(requested_precision));
      }
      else if (thread_default_variable_precision_options() >= variable_precision_options::preserve_all_precision)
      {
         unsigned requested_precision = this->get_default_precision();
         unsigned d10 = 1 + multiprecision::detail::digits2_2_10(used_gmp_rational_bits(val));
         if (d10 > requested_precision)
            this->precision(d10);
      }
      mpfr_set_q(this->m_data, val.data(), GMP_RNDN);
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
      return multiprecision::detail::digits2_2_10(mpfr_get_prec(this->m_data));
   }
   void precision(unsigned digits10) noexcept
   {
      mpfr_prec_round(this->m_data, multiprecision::detail::digits10_2_2((digits10)), GMP_RNDN);
   }
   //
   // Variable precision options:
   // 
   static variable_precision_options default_variable_precision_options()noexcept
   {
      return get_global_default_options();
   }
   static variable_precision_options thread_default_variable_precision_options()noexcept
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
   static bool preserve_source_precision()
   {
      return get_default_options() >= variable_precision_options::preserve_source_precision;
   }
   static bool preserve_related_precision()
   {
      return get_default_options() >= variable_precision_options::preserve_related_precision;
   }
   static bool preserve_all_precision()
   {
      return get_default_options() >= variable_precision_options::preserve_all_precision;
   }
};

template <unsigned digits10, mpfr_allocation_type AllocationType, class T>
inline typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value, bool>::type eval_eq(const mpfr_float_backend<digits10, AllocationType>& a, const T& b)
{
   return a.compare(b) == 0;
}
template <unsigned digits10, mpfr_allocation_type AllocationType, class T>
inline typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value, bool>::type eval_lt(const mpfr_float_backend<digits10, AllocationType>& a, const T& b)
{
   return a.compare(b) < 0;
}
template <unsigned digits10, mpfr_allocation_type AllocationType, class T>
inline typename std::enable_if<boost::multiprecision::detail::is_arithmetic<T>::value, bool>::type eval_gt(const mpfr_float_backend<digits10, AllocationType>& a, const T& b)
{
   return a.compare(b) > 0;
}

template <unsigned digits10, mpfr_allocation_type AllocationType>
inline bool eval_eq(const mpfr_float_backend<digits10, AllocationType>& a, const mpfr_float_backend<digits10, AllocationType>& b)noexcept
{
   return mpfr_equal_p(a.data(), b.data());
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline bool eval_lt(const mpfr_float_backend<digits10, AllocationType>& a, const mpfr_float_backend<digits10, AllocationType>& b) noexcept
{
   return mpfr_less_p(a.data(), b.data());
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline bool eval_gt(const mpfr_float_backend<digits10, AllocationType>& a, const mpfr_float_backend<digits10, AllocationType>& b) noexcept
{
   return mpfr_greater_p(a.data(), b.data());
}

template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_add(mpfr_float_backend<D1, A1>& result, const mpfr_float_backend<D2, A2>& o)
{
   mpfr_add(result.data(), result.data(), o.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_subtract(mpfr_float_backend<D1, A1>& result, const mpfr_float_backend<D2, A2>& o)
{
   mpfr_sub(result.data(), result.data(), o.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_multiply(mpfr_float_backend<D1, A1>& result, const mpfr_float_backend<D2, A2>& o)
{
   if ((void*)&o == (void*)&result)
      mpfr_sqr(result.data(), o.data(), GMP_RNDN);
   else
      mpfr_mul(result.data(), result.data(), o.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_divide(mpfr_float_backend<D1, A1>& result, const mpfr_float_backend<D2, A2>& o)
{
   mpfr_div(result.data(), result.data(), o.data(), GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_add(mpfr_float_backend<digits10, AllocationType>& result, unsigned long i)
{
   mpfr_add_ui(result.data(), result.data(), i, GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_subtract(mpfr_float_backend<digits10, AllocationType>& result, unsigned long i)
{
   mpfr_sub_ui(result.data(), result.data(), i, GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_multiply(mpfr_float_backend<digits10, AllocationType>& result, unsigned long i)
{
   mpfr_mul_ui(result.data(), result.data(), i, GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_divide(mpfr_float_backend<digits10, AllocationType>& result, unsigned long i)
{
   mpfr_div_ui(result.data(), result.data(), i, GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_add(mpfr_float_backend<digits10, AllocationType>& result, long i)
{
   if (i > 0)
      mpfr_add_ui(result.data(), result.data(), i, GMP_RNDN);
   else
      mpfr_sub_ui(result.data(), result.data(), boost::multiprecision::detail::unsigned_abs(i), GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_subtract(mpfr_float_backend<digits10, AllocationType>& result, long i)
{
   if (i > 0)
      mpfr_sub_ui(result.data(), result.data(), i, GMP_RNDN);
   else
      mpfr_add_ui(result.data(), result.data(), boost::multiprecision::detail::unsigned_abs(i), GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_multiply(mpfr_float_backend<digits10, AllocationType>& result, long i)
{
   mpfr_mul_ui(result.data(), result.data(), boost::multiprecision::detail::unsigned_abs(i), GMP_RNDN);
   if (i < 0)
      mpfr_neg(result.data(), result.data(), GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_divide(mpfr_float_backend<digits10, AllocationType>& result, long i)
{
   mpfr_div_ui(result.data(), result.data(), boost::multiprecision::detail::unsigned_abs(i), GMP_RNDN);
   if (i < 0)
      mpfr_neg(result.data(), result.data(), GMP_RNDN);
}
//
// Specialised 3 arg versions of the basic operators:
//
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2, unsigned D3, mpfr_allocation_type A3>
inline void eval_add(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, const mpfr_float_backend<D3, A3>& y)
{
   mpfr_add(a.data(), x.data(), y.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_add(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, unsigned long y)
{
   mpfr_add_ui(a.data(), x.data(), y, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_add(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, long y)
{
   if (y < 0)
      mpfr_sub_ui(a.data(), x.data(), boost::multiprecision::detail::unsigned_abs(y), GMP_RNDN);
   else
      mpfr_add_ui(a.data(), x.data(), y, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_add(mpfr_float_backend<D1, A1>& a, unsigned long x, const mpfr_float_backend<D2, A2>& y)
{
   mpfr_add_ui(a.data(), y.data(), x, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_add(mpfr_float_backend<D1, A1>& a, long x, const mpfr_float_backend<D2, A2>& y)
{
   if (x < 0)
   {
      mpfr_ui_sub(a.data(), boost::multiprecision::detail::unsigned_abs(x), y.data(), GMP_RNDN);
      mpfr_neg(a.data(), a.data(), GMP_RNDN);
   }
   else
      mpfr_add_ui(a.data(), y.data(), x, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2, unsigned D3, mpfr_allocation_type A3>
inline void eval_subtract(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, const mpfr_float_backend<D3, A3>& y)
{
   mpfr_sub(a.data(), x.data(), y.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_subtract(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, unsigned long y)
{
   mpfr_sub_ui(a.data(), x.data(), y, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_subtract(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, long y)
{
   if (y < 0)
      mpfr_add_ui(a.data(), x.data(), boost::multiprecision::detail::unsigned_abs(y), GMP_RNDN);
   else
      mpfr_sub_ui(a.data(), x.data(), y, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_subtract(mpfr_float_backend<D1, A1>& a, unsigned long x, const mpfr_float_backend<D2, A2>& y)
{
   mpfr_ui_sub(a.data(), x, y.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_subtract(mpfr_float_backend<D1, A1>& a, long x, const mpfr_float_backend<D2, A2>& y)
{
   if (x < 0)
   {
      mpfr_add_ui(a.data(), y.data(), boost::multiprecision::detail::unsigned_abs(x), GMP_RNDN);
      mpfr_neg(a.data(), a.data(), GMP_RNDN);
   }
   else
      mpfr_ui_sub(a.data(), x, y.data(), GMP_RNDN);
}

template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2, unsigned D3, mpfr_allocation_type A3>
inline void eval_multiply(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, const mpfr_float_backend<D3, A3>& y)
{
   if ((void*)&x == (void*)&y)
      mpfr_sqr(a.data(), x.data(), GMP_RNDN);
   else
      mpfr_mul(a.data(), x.data(), y.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_multiply(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, unsigned long y)
{
   mpfr_mul_ui(a.data(), x.data(), y, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_multiply(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, long y)
{
   if (y < 0)
   {
      mpfr_mul_ui(a.data(), x.data(), boost::multiprecision::detail::unsigned_abs(y), GMP_RNDN);
      a.negate();
   }
   else
      mpfr_mul_ui(a.data(), x.data(), y, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_multiply(mpfr_float_backend<D1, A1>& a, unsigned long x, const mpfr_float_backend<D2, A2>& y)
{
   mpfr_mul_ui(a.data(), y.data(), x, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_multiply(mpfr_float_backend<D1, A1>& a, long x, const mpfr_float_backend<D2, A2>& y)
{
   if (x < 0)
   {
      mpfr_mul_ui(a.data(), y.data(), boost::multiprecision::detail::unsigned_abs(x), GMP_RNDN);
      mpfr_neg(a.data(), a.data(), GMP_RNDN);
   }
   else
      mpfr_mul_ui(a.data(), y.data(), x, GMP_RNDN);
}

template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2, unsigned D3, mpfr_allocation_type A3>
inline void eval_divide(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, const mpfr_float_backend<D3, A3>& y)
{
   mpfr_div(a.data(), x.data(), y.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_divide(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, unsigned long y)
{
   mpfr_div_ui(a.data(), x.data(), y, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_divide(mpfr_float_backend<D1, A1>& a, const mpfr_float_backend<D2, A2>& x, long y)
{
   if (y < 0)
   {
      mpfr_div_ui(a.data(), x.data(), boost::multiprecision::detail::unsigned_abs(y), GMP_RNDN);
      a.negate();
   }
   else
      mpfr_div_ui(a.data(), x.data(), y, GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_divide(mpfr_float_backend<D1, A1>& a, unsigned long x, const mpfr_float_backend<D2, A2>& y)
{
   mpfr_ui_div(a.data(), x, y.data(), GMP_RNDN);
}
template <unsigned D1, unsigned D2, mpfr_allocation_type A1, mpfr_allocation_type A2>
inline void eval_divide(mpfr_float_backend<D1, A1>& a, long x, const mpfr_float_backend<D2, A2>& y)
{
   if (x < 0)
   {
      mpfr_ui_div(a.data(), boost::multiprecision::detail::unsigned_abs(x), y.data(), GMP_RNDN);
      mpfr_neg(a.data(), a.data(), GMP_RNDN);
   }
   else
      mpfr_ui_div(a.data(), x, y.data(), GMP_RNDN);
}

template <unsigned digits10, mpfr_allocation_type AllocationType>
inline bool eval_is_zero(const mpfr_float_backend<digits10, AllocationType>& val) noexcept
{
   return 0 != mpfr_zero_p(val.data());
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline int eval_get_sign(const mpfr_float_backend<digits10, AllocationType>& val) noexcept
{
   return mpfr_sgn(val.data());
}

template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_convert_to(unsigned long* result, const mpfr_float_backend<digits10, AllocationType>& val)
{
   if (mpfr_nan_p(val.data()))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert NaN to integer."));
   }
   *result = mpfr_get_ui(val.data(), GMP_RNDZ);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_convert_to(long* result, const mpfr_float_backend<digits10, AllocationType>& val)
{
   if (mpfr_nan_p(val.data()))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert NaN to integer."));
   }
   *result = mpfr_get_si(val.data(), GMP_RNDZ);
}
#ifdef _MPFR_H_HAVE_INTMAX_T
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_convert_to(boost::ulong_long_type* result, const mpfr_float_backend<digits10, AllocationType>& val)
{
   if (mpfr_nan_p(val.data()))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert NaN to integer."));
   }
   *result = mpfr_get_uj(val.data(), GMP_RNDZ);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_convert_to(boost::long_long_type* result, const mpfr_float_backend<digits10, AllocationType>& val)
{
   if (mpfr_nan_p(val.data()))
   {
      BOOST_THROW_EXCEPTION(std::runtime_error("Could not convert NaN to integer."));
   }
   *result = mpfr_get_sj(val.data(), GMP_RNDZ);
}
#endif
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_convert_to(float* result, const mpfr_float_backend<digits10, AllocationType>& val) noexcept
{
   *result = mpfr_get_flt(val.data(), GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_convert_to(double* result, const mpfr_float_backend<digits10, AllocationType>& val) noexcept
{
   *result = mpfr_get_d(val.data(), GMP_RNDN);
}
template <unsigned digits10, mpfr_allocation_type AllocationType>
inline void eval_convert_to(long double* result, const mpfr_float_backend<digits10, AllocationType>& val) noexcept
{
   *result = mpfr_get_ld(val.data(), GMP_RNDN);
}

//
// Native non-member operations:
//
template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_sqrt(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& val)
{
   mpfr_sqrt(result.data(), val.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_abs(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& val)
{
   mpfr_abs(result.data(), val.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_fabs(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& val)
{
   mpfr_abs(result.data(), val.data(), GMP_RNDN);
}
template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_ceil(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& val)
{
   mpfr_ceil(result.data(), val.data());
}
template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_floor(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& val)
{
   mpfr_floor(result.data(), val.data());
}
template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_trunc(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& val)
{
   mpfr_trunc(result.data(), val.data());
}
template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_ldexp(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& val, long e)
{
   if (e > 0)
      mpfr_mul_2exp(result.data(), val.data(), e, GMP_RNDN);
   else if (e < 0)
      mpfr_div_2exp(result.data(), val.data(), -e, GMP_RNDN);
   else
      result = val;
}
template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_frexp(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& val, int* e)
{
   if (mpfr_zero_p(val.data()))
   {
      *e = 0;
      result = val;
      return;
   }
   mp_exp_t v = mpfr_get_exp(val.data());
   *e = v;
   if (v)
      eval_ldexp(result, val, -v);
   else
      result = val;
}
template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_frexp(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& val, long* e)
{
   if (mpfr_zero_p(val.data()))
   {
      *e = 0;
      result = val;
      return;
   }
   mp_exp_t v = mpfr_get_exp(val.data());
   *e = v;
   if(v)
      eval_ldexp(result, val, -v);
   else
      result = val;
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline int eval_fpclassify(const mpfr_float_backend<Digits10, AllocateType>& val) noexcept
{
   return mpfr_inf_p(val.data()) ? FP_INFINITE : mpfr_nan_p(val.data()) ? FP_NAN : mpfr_zero_p(val.data()) ? FP_ZERO : FP_NORMAL;
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_pow(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& b, const mpfr_float_backend<Digits10, AllocateType>& e)
{
   if (mpfr_zero_p(b.data()) && mpfr_integer_p(e.data()) && (mpfr_signbit(e.data()) == 0) && mpfr_fits_ulong_p(e.data(), GMP_RNDN) && (mpfr_get_ui(e.data(), GMP_RNDN) & 1))
   {
      mpfr_set(result.data(), b.data(), GMP_RNDN);
   }
   else
      mpfr_pow(result.data(), b.data(), e.data(), GMP_RNDN);
}

#ifdef BOOST_MSVC
//
// The enable_if usage below doesn't work with msvc - but only when
// certain other enable_if usages are defined first.  It's a capricious
// and rather annoying compiler bug in other words....
//
#define BOOST_MP_ENABLE_IF_WORKAROUND (Digits10 || !Digits10)&&
#else
#define BOOST_MP_ENABLE_IF_WORKAROUND
#endif

template <unsigned Digits10, mpfr_allocation_type AllocateType, class Integer>
inline typename std::enable_if<boost::multiprecision::detail::is_signed<Integer>::value && boost::multiprecision::detail::is_integral<Integer>::value && (BOOST_MP_ENABLE_IF_WORKAROUND(sizeof(Integer) <= sizeof(long)))>::type
eval_pow(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& b, const Integer& e)
{
   mpfr_pow_si(result.data(), b.data(), e, GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType, class Integer>
inline typename std::enable_if<boost::multiprecision::detail::is_unsigned<Integer>::value && (BOOST_MP_ENABLE_IF_WORKAROUND(sizeof(Integer) <= sizeof(long)))>::type
eval_pow(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& b, const Integer& e)
{
   mpfr_pow_ui(result.data(), b.data(), e, GMP_RNDN);
}

#undef BOOST_MP_ENABLE_IF_WORKAROUND

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_exp(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_exp(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_exp2(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_exp2(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_log(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_log(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_log10(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_log10(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_sin(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_sin(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_cos(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_cos(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_tan(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_tan(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_asin(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_asin(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_acos(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_acos(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_atan(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_atan(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_atan2(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg1, const mpfr_float_backend<Digits10, AllocateType>& arg2)
{
   mpfr_atan2(result.data(), arg1.data(), arg2.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_sinh(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_sinh(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_cosh(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_cosh(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_tanh(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_tanh(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_log2(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   mpfr_log2(result.data(), arg.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_modf(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& arg, mpfr_float_backend<Digits10, AllocateType>* pipart)
{
   if (0 == pipart)
   {
      mpfr_float_backend<Digits10, AllocateType> ipart;
      mpfr_modf(ipart.data(), result.data(), arg.data(), GMP_RNDN);
   }
   else
   {
      mpfr_modf(pipart->data(), result.data(), arg.data(), GMP_RNDN);
   }
}
template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_remainder(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& a, const mpfr_float_backend<Digits10, AllocateType>& b)
{
   mpfr_remainder(result.data(), a.data(), b.data(), GMP_RNDN);
}
template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_remquo(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& a, const mpfr_float_backend<Digits10, AllocateType>& b, int* pi)
{
   long l;
   mpfr_remquo(result.data(), &l, a.data(), b.data(), GMP_RNDN);
   if (pi)
      *pi = l;
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_fmod(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& a, const mpfr_float_backend<Digits10, AllocateType>& b)
{
   mpfr_fmod(result.data(), a.data(), b.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_multiply_add(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& a, const mpfr_float_backend<Digits10, AllocateType>& b)
{
   mpfr_fma(result.data(), a.data(), b.data(), result.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_multiply_add(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& a, const mpfr_float_backend<Digits10, AllocateType>& b, const mpfr_float_backend<Digits10, AllocateType>& c)
{
   mpfr_fma(result.data(), a.data(), b.data(), c.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_multiply_subtract(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& a, const mpfr_float_backend<Digits10, AllocateType>& b)
{
   mpfr_fms(result.data(), a.data(), b.data(), result.data(), GMP_RNDN);
   result.negate();
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline void eval_multiply_subtract(mpfr_float_backend<Digits10, AllocateType>& result, const mpfr_float_backend<Digits10, AllocateType>& a, const mpfr_float_backend<Digits10, AllocateType>& b, const mpfr_float_backend<Digits10, AllocateType>& c)
{
   mpfr_fms(result.data(), a.data(), b.data(), c.data(), GMP_RNDN);
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline int eval_signbit BOOST_PREVENT_MACRO_SUBSTITUTION(const mpfr_float_backend<Digits10, AllocateType>& arg)
{
   return (arg.data()[0]._mpfr_sign < 0) ? 1 : 0;
}

template <unsigned Digits10, mpfr_allocation_type AllocateType>
inline std::size_t hash_value(const mpfr_float_backend<Digits10, AllocateType>& val)
{
   std::size_t result = 0;
   std::size_t len    = val.data()[0]._mpfr_prec / mp_bits_per_limb;
   if (val.data()[0]._mpfr_prec % mp_bits_per_limb)
      ++len;
   for (std::size_t i = 0; i < len; ++i)
      boost::multiprecision::detail::hash_combine(result, val.data()[0]._mpfr_d[i]);
   boost::multiprecision::detail::hash_combine(result, val.data()[0]._mpfr_exp, val.data()[0]._mpfr_sign);
   return result;
}

} // namespace backends

namespace detail {
template <>
struct is_variable_precision<backends::mpfr_float_backend<0> > : public std::integral_constant<bool, true>
{};
} // namespace detail

template <>
struct number_category<detail::canonical<mpfr_t, backends::mpfr_float_backend<0> >::type> : public std::integral_constant<int, number_kind_floating_point>
{};

template <unsigned D, boost::multiprecision::mpfr_allocation_type A1, boost::multiprecision::mpfr_allocation_type A2>
struct is_equivalent_number_type<backends::mpfr_float_backend<D, A1>, backends::mpfr_float_backend<D, A2> > : public std::integral_constant<bool, true> {};

using boost::multiprecision::backends::mpfr_float_backend;

using mpfr_float_50 = number<mpfr_float_backend<50> >  ;
using mpfr_float_100 = number<mpfr_float_backend<100> > ;
using mpfr_float_500 = number<mpfr_float_backend<500> > ;
using mpfr_float_1000 = number<mpfr_float_backend<1000> >;
using mpfr_float = number<mpfr_float_backend<0> >   ;

using static_mpfr_float_50 = number<mpfr_float_backend<50, allocate_stack> > ;
using static_mpfr_float_100 = number<mpfr_float_backend<100, allocate_stack> >;

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> copysign BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& a, const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& b)
{
   return (boost::multiprecision::signbit)(a) != (boost::multiprecision::signbit)(b) ? boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(-a) : a;
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> copysign BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& a, const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& b)
{
   return (boost::multiprecision::signbit)(a) != (boost::multiprecision::signbit)(b) ? boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>(-a) : a;
}

} // namespace multiprecision

namespace math {

using boost::multiprecision::copysign;
using boost::multiprecision::signbit;

namespace tools {

inline void set_output_precision(const boost::multiprecision::mpfr_float& val, std::ostream& os)
{
   os << std::setprecision(val.precision());
}

template <>
inline int digits<boost::multiprecision::mpfr_float>()
#ifdef BOOST_MATH_NOEXCEPT
    noexcept
#endif
{
   return multiprecision::detail::digits10_2_2(boost::multiprecision::mpfr_float::thread_default_precision());
}
template <>
inline int digits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, boost::multiprecision::et_off> >()
#ifdef BOOST_MATH_NOEXCEPT
    noexcept
#endif
{
   return multiprecision::detail::digits10_2_2(boost::multiprecision::mpfr_float::thread_default_precision());
}

template <>
inline boost::multiprecision::mpfr_float
max_value<boost::multiprecision::mpfr_float>()
{
   boost::multiprecision::mpfr_float result(0.5);
   mpfr_mul_2exp(result.backend().data(), result.backend().data(), mpfr_get_emax(), GMP_RNDN);
   BOOST_ASSERT(mpfr_number_p(result.backend().data()));
   return result;
}

template <>
inline boost::multiprecision::mpfr_float
min_value<boost::multiprecision::mpfr_float>()
{
   boost::multiprecision::mpfr_float result(0.5);
   mpfr_div_2exp(result.backend().data(), result.backend().data(), -mpfr_get_emin(), GMP_RNDN);
   BOOST_ASSERT(mpfr_number_p(result.backend().data()));
   return result;
}

template <>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, boost::multiprecision::et_off>
max_value<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, boost::multiprecision::et_off> >()
{
   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, boost::multiprecision::et_off> result(0.5);
   mpfr_mul_2exp(result.backend().data(), result.backend().data(), mpfr_get_emax(), GMP_RNDN);
   BOOST_ASSERT(mpfr_number_p(result.backend().data()));
   return result;
}

template <>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, boost::multiprecision::et_off>
min_value<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, boost::multiprecision::et_off> >()
{
   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, boost::multiprecision::et_off> result(0.5);
   mpfr_div_2exp(result.backend().data(), result.backend().data(), -mpfr_get_emin(), GMP_RNDN);
   BOOST_ASSERT(mpfr_number_p(result.backend().data()));
   return result;
}
//
// Over again with debug_adaptor:
//
template <>
inline int digits<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float::backend_type> > >()
#ifdef BOOST_MATH_NOEXCEPT
    noexcept
#endif
{
   return multiprecision::detail::digits10_2_2(boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float::backend_type> >::thread_default_precision());
}
template <>
inline int digits<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off> >()
#ifdef BOOST_MATH_NOEXCEPT
    noexcept
#endif
{
   return multiprecision::detail::digits10_2_2(boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float::backend_type> >::thread_default_precision());
}

template <>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float::backend_type> >
max_value<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float::backend_type> > >()
{
   return max_value<boost::multiprecision::mpfr_float>().backend();
}

template <>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float::backend_type> >
min_value<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float::backend_type> > >()
{
   return min_value<boost::multiprecision::mpfr_float>().backend();
}

template <>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off>
max_value<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off> >()
{
   return max_value<boost::multiprecision::mpfr_float>().backend();
}

template <>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off>
min_value<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off> >()
{
   return min_value<boost::multiprecision::mpfr_float>().backend();
}

//
// Over again with logged_adaptor:
//
template <>
inline int digits<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float::backend_type> > >()
#ifdef BOOST_MATH_NOEXCEPT
    noexcept
#endif
{
   return multiprecision::detail::digits10_2_2(boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float::backend_type> >::default_precision());
}
template <>
inline int digits<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off> >()
#ifdef BOOST_MATH_NOEXCEPT
    noexcept
#endif
{
   return multiprecision::detail::digits10_2_2(boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float::backend_type> >::default_precision());
}

template <>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float::backend_type> >
max_value<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float::backend_type> > >()
{
   return max_value<boost::multiprecision::mpfr_float>().backend();
}

template <>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float::backend_type> >
min_value<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float::backend_type> > >()
{
   return min_value<boost::multiprecision::mpfr_float>().backend();
}

template <>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off>
max_value<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off> >()
{
   return max_value<boost::multiprecision::mpfr_float>().backend();
}

template <>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off>
min_value<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<0> >, boost::multiprecision::et_off> >()
{
   return min_value<boost::multiprecision::mpfr_float>().backend();
}

} // namespace tools

namespace constants { namespace detail {

template <class T>
struct constant_pi;
template <class T>
struct constant_ln_two;
template <class T>
struct constant_euler;
template <class T>
struct constant_catalan;

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_pi<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      // Rely on C++11 thread safe initialization:
      static result_type result{get(std::integral_constant<int, 0>())};
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_pi(result.backend().data(), GMP_RNDN);
      return result;
   }
};
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_ln_two<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      // Rely on C++11 thread safe initialization:
      static result_type result{get(std::integral_constant<int, 0>())};
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_log2(result.backend().data(), GMP_RNDN);
      return result;
   }
};
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_euler<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      // Rely on C++11 thread safe initialization:
      static result_type result{get(std::integral_constant<int, 0>())};
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_euler(result.backend().data(), GMP_RNDN);
      return result;
   }
};
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_catalan<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      // Rely on C++11 thread safe initialization:
      static result_type result{get(std::integral_constant<int, 0>())};
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_catalan(result.backend().data(), GMP_RNDN);
      return result;
   }
};
//
// Over again with debug_adaptor:
//
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_pi<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      // Rely on C++11 thread safe initialization:
      static result_type result{get(std::integral_constant<int, 0>())};
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_pi(result.backend().value().data(), GMP_RNDN);
      result.backend().update_view();
      return result;
   }
};
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_ln_two<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      // Rely on C++11 thread safe initialization:
      static result_type result{get(std::integral_constant<int, 0>())};
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_log2(result.backend().value().data(), GMP_RNDN);
      result.backend().update_view();
      return result;
   }
};
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_euler<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      // Rely on C++11 thread safe initialization:
      static result_type result{get(std::integral_constant<int, 0>())};
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_euler(result.backend().value().data(), GMP_RNDN);
      result.backend().update_view();
      return result;
   }
};
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_catalan<boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      // Rely on C++11 thread safe initialization:
      static result_type result{get(std::integral_constant<int, 0>())};
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_catalan(result.backend().value().data(), GMP_RNDN);
      result.backend().update_view();
      return result;
   }
};

//
// Over again with logged_adaptor:
//
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_pi<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      static result_type result;
      static bool        has_init = false;
      if (!has_init)
      {
         mpfr_const_pi(result.backend().value().data(), GMP_RNDN);
         has_init = true;
      }
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_pi(result.backend().value().data(), GMP_RNDN);
      return result;
   }
};
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_ln_two<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      static result_type result;
      static bool        init = false;
      if (!init)
      {
         mpfr_const_log2(result.backend().value().data(), GMP_RNDN);
         init = true;
      }
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_log2(result.backend().value().data(), GMP_RNDN);
      return result;
   }
};
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_euler<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      static result_type result;
      static bool        init = false;
      if (!init)
      {
         mpfr_const_euler(result.backend().value().data(), GMP_RNDN);
         init = true;
      }
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_euler(result.backend().value().data(), GMP_RNDN);
      return result;
   }
};
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
struct constant_catalan<boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >
{
   using result_type = boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>;
   template <int N>
   static inline const result_type& get(const std::integral_constant<int, N>&)
   {
      static result_type result;
      static bool        init = false;
      if (!init)
      {
         mpfr_const_catalan(result.backend().value().data(), GMP_RNDN);
         init = true;
      }
      return result;
   }
   static inline const result_type get(const std::integral_constant<int, 0>&)
   {
      result_type result;
      mpfr_const_catalan(result.backend().value().data(), GMP_RNDN);
      return result;
   }
};

}} // namespace constants::detail

} // namespace math

namespace multiprecision {
//
// Overloaded special functions which call native mpfr routines:
//
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_asinh(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_acosh(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_atanh(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_cbrt(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_erf(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_erfc(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_expm1(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_lngamma(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_gamma(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_log1p(result.backend().data(), arg.backend().data(), GMP_RNDN);
   return result;
}

//
// Over again with debug_adaptor:
//
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_asinh(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_acosh(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_atanh(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_cbrt(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_erf(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_erfc(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_expm1(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_lngamma(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_gamma(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_log1p(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   result.backend().update_view();
   return result;
}

//
// Over again with logged_adaptor:
//
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_asinh(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_acosh(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_atanh(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_cbrt(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_erf(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_erfc(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_expm1(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_lngamma(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_gamma(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   boost::multiprecision::detail::scoped_default_precision<number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> result;
   mpfr_log1p(result.backend().value().data(), arg.backend().value().data(), GMP_RNDN);
   return result;
}

} // namespace multiprecision

namespace math {
//
// Overloaded special functions which call native mpfr routines:
//
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy&)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_asinh(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("asinh<%1%>(%1%)", 0, Policy());
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("asinh<%1%>(%1%)", "Unknown error, result is a NaN", result, Policy());
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return asinh(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy&)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_acosh(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("acosh<%1%>(%1%)", 0, Policy());
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("acosh<%1%>(%1%)", "Unknown error, result is a NaN", result, Policy());
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return acosh(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy& )
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_atanh(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("atanh<%1%>(%1%)", 0, Policy());
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("atanh<%1%>(%1%)", "Unknown error, result is a NaN", result, Policy());
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return atanh(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy&)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_cbrt(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("cbrt<%1%>(%1%)", 0, Policy());
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("cbrt<%1%>(%1%)", "Unknown error, result is a NaN", result, Policy());
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return cbrt(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy& pol)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_erf(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("erf<%1%>(%1%)", 0, pol);
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("erf<%1%>(%1%)", "Unknown error, result is a NaN", result, pol);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return erf(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy& pol)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_erfc(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("erfc<%1%>(%1%)", 0, pol);
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("erfc<%1%>(%1%)", "Unknown error, result is a NaN", result, pol);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return erfc(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy& pol)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_expm1(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("expm1<%1%>(%1%)", 0, pol);
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("expm1<%1%>(%1%)", "Unknown error, result is a NaN", result, pol);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> exm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return expm1(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> arg, int* sign, const Policy& pol)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);
   (void)precision_guard;  // warning suppression

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   if (arg > 0)
   {
      mpfr_lngamma(result.backend().data(), arg.backend().data(), GMP_RNDN);
      if (sign)
         *sign = 1;
   }
   else
   {
      if (floor(arg) == arg)
         return policies::raise_pole_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >(
             "lgamma<%1%>", "Evaluation of lgamma at a negative integer %1%.", arg, pol);

      boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> t = detail::sinpx(arg);
      arg                                                                                                                     = -arg;
      if (t < 0)
      {
         t = -t;
      }
      result = boost::multiprecision::log(boost::math::constants::pi<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >()) - lgamma(arg, 0, pol) - boost::multiprecision::log(t);
      if (sign)
      {
         boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> phase = 1 - arg;
         phase                                                                                                                       = floor(phase) / 2;
         if (floor(phase) == phase)
            *sign = -1;
         else
            *sign = 1;
      }
   }
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("lgamma<%1%>(%1%)", 0, pol);
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("lgamma<%1%>(%1%)", "Unknown error, result is a NaN", result, pol);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, int* sign)
{
   return lgamma(arg, sign, policies::policy<>());
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy& pol)
{
   return lgamma(arg, 0, pol);
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return lgamma(arg, 0, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline typename std::enable_if<boost::math::policies::is_policy<Policy>::value, boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::type tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy& pol)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_gamma(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("tgamma<%1%>(%1%)", 0, pol);
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("tgamma<%1%>(%1%)", "Unknown error, result is a NaN", result, pol);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return tgamma(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy& pol)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_log1p(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return (arg == -1 ? -1 : 1) * policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("log1p<%1%>(%1%)", 0, pol);
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("log1p<%1%>(%1%)", "Unknown error, result is a NaN", result, pol);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return log1p(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> rsqrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg, const Policy& pol)
{
   boost::multiprecision::detail::scoped_default_precision<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> > precision_guard(arg);

   boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> result;
   mpfr_rec_sqrt(result.backend().data(), arg.backend().data(), GMP_RNDN);
   if (mpfr_inf_p(result.backend().data()))
      return policies::raise_overflow_error<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >("rsqrt<%1%>(%1%)", 0, pol);
   if (mpfr_nan_p(result.backend().data()))
      return policies::raise_evaluation_error("rsqrt<%1%>(%1%)", "Negative argument, result is a NaN", result, pol);
   return result;
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> rsqrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>& arg)
{
   return rsqrt(arg, policies::policy<>());
}

//
// Over again with debug_adaptor:
//
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return asinh(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return asinh(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return acosh(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return acosh(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return atanh(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return atanh(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return cbrt(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return cbrt(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return erf(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return erf(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return erfc(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return erfc(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return expm1(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> exm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return expm1(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> arg, int* sign, const Policy& pol)
{
   return lgamma(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), sign, pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, int* sign)
{
   return lgamma(arg, sign, policies::policy<>());
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return lgamma(arg, 0, pol);
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return lgamma(arg, 0, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline typename std::enable_if<boost::math::policies::is_policy<Policy>::value, boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >::type tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return tgamma(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return tgamma(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return log1p(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return log1p(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> rsqrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return rsqrt(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> rsqrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return rsqrt(arg, policies::policy<>());
}

//
// Over again with logged_adaptor:
//
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return asinh(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> asinh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return asinh(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return acosh(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> acosh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return acosh(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return atanh(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> atanh BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return atanh(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return cbrt(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> cbrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return cbrt(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return erf(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erf BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return erf(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return erfc(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> erfc BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return erfc(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> expm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return expm1(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> exm1 BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return expm1(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> arg, int* sign, const Policy& pol)
{
   return lgamma(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), sign, pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, int* sign)
{
   return lgamma(arg, sign, policies::policy<>());
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return lgamma(arg, 0, pol);
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> lgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return lgamma(arg, 0, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline typename std::enable_if<boost::math::policies::is_policy<Policy>::value, boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> >::type tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return tgamma(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> tgamma BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return tgamma(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return log1p(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> log1p BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return log1p(arg, policies::policy<>());
}

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates, class Policy>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> rsqrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg, const Policy& pol)
{
   return rsqrt(boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>(arg.backend().value()), pol).backend();
}
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates> rsqrt BOOST_PREVENT_MACRO_SUBSTITUTION(const boost::multiprecision::number<boost::multiprecision::backends::logged_adaptor<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType> >, ExpressionTemplates>& arg)
{
   return rsqrt(arg, policies::policy<>());
}

} // namespace math

} // namespace boost

namespace std {

//
// numeric_limits [partial] specializations for the types declared in this header:
//
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
class numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >
{
   using number_type = boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates>;

   static number_type get_min()
   {
      number_type result{0.5};
      mpfr_div_2exp(result.backend().data(), result.backend().data(), -mpfr_get_emin(), GMP_RNDN);
      return result;
   }
   static number_type get_max()
   {
      number_type result{0.5};
      mpfr_mul_2exp(result.backend().data(), result.backend().data(), mpfr_get_emax(), GMP_RNDN);
      return result;
   }
   static number_type get_eps()
   {
      number_type result{1};
      mpfr_div_2exp(result.backend().data(), result.backend().data(), std::numeric_limits<number_type>::digits - 1, GMP_RNDN);
      return result;
   }

 public:
   static constexpr bool is_specialized = true;
   static number_type(min)()
   {
      static number_type value{get_min()};
      return value;
   }
   static number_type(max)()
   {
      static number_type value{get_max()};
      return value;
   }
   static constexpr number_type lowest()
   {
      return -(max)();
   }
   static constexpr int digits   = static_cast<int>((Digits10 * 1000L) / 301L + ((Digits10 * 1000L) % 301 ? 2 : 1));
   static constexpr int digits10 = Digits10;
   // Is this really correct???
   static constexpr int  max_digits10 = boost::multiprecision::detail::calc_max_digits10<digits>::value;
   static constexpr bool is_signed    = true;
   static constexpr bool is_integer   = false;
   static constexpr bool is_exact     = false;
   static constexpr int  radix        = 2;
   static number_type          epsilon()
   {
      static number_type value{get_eps()};
      return value;
   }
   // What value should this be????
   static number_type round_error()
   {
      // returns epsilon/2
      return 0.5;
   }
   static constexpr long min_exponent                  = MPFR_EMIN_DEFAULT;
   static constexpr long min_exponent10                = (MPFR_EMIN_DEFAULT / 1000) * 301L;
   static constexpr long max_exponent                  = MPFR_EMAX_DEFAULT;
   static constexpr long max_exponent10                = (MPFR_EMAX_DEFAULT / 1000) * 301L;
   static constexpr bool has_infinity                  = true;
   static constexpr bool has_quiet_NaN                 = true;
   static constexpr bool has_signaling_NaN             = false;
   static constexpr float_denorm_style has_denorm      = denorm_absent;
   static constexpr bool               has_denorm_loss = false;
   static number_type                        infinity()
   {
      number_type value;
      mpfr_set_inf(value.backend().data(), 1);
      return value;
   }
   static number_type quiet_NaN()
   {
      number_type value;
      mpfr_set_nan(value.backend().data());
      return value;
   }
   static constexpr number_type signaling_NaN()
   {
      return number_type(0);
   }
   static constexpr number_type denorm_min() { return number_type(0); }
   static constexpr bool        is_iec559         = false;
   static constexpr bool        is_bounded        = true;
   static constexpr bool        is_modulo         = false;
   static constexpr bool        traps             = true;
   static constexpr bool        tinyness_before   = false;
   static constexpr float_round_style round_style = round_to_nearest;
};

template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::digits;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::digits10;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::max_digits10;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::is_signed;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::is_integer;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::is_exact;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::radix;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr long numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::min_exponent;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr long numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::min_exponent10;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr long numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::max_exponent;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr long numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::max_exponent10;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::has_infinity;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::has_quiet_NaN;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::has_signaling_NaN;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr float_denorm_style numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::has_denorm;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::has_denorm_loss;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::is_iec559;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::is_bounded;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::is_modulo;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::traps;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::tinyness_before;
template <unsigned Digits10, boost::multiprecision::mpfr_allocation_type AllocateType, boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr float_round_style numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<Digits10, AllocateType>, ExpressionTemplates> >::round_style;

template <boost::multiprecision::expression_template_option ExpressionTemplates>
class numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >
{
   using number_type = boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates>;

 public:
   static constexpr bool is_specialized = false;
   static number_type(min)()
   {
      number_type value(0.5);
      mpfr_div_2exp(value.backend().data(), value.backend().data(), -mpfr_get_emin(), GMP_RNDN);
      return value;
   }
   static number_type(max)()
   {
      number_type value(0.5);
      mpfr_mul_2exp(value.backend().data(), value.backend().data(), mpfr_get_emax(), GMP_RNDN);
      return value;
   }
   static number_type lowest()
   {
      return -(max)();
   }
   static constexpr int  digits       = INT_MAX;
   static constexpr int  digits10     = INT_MAX;
   static constexpr int  max_digits10 = INT_MAX;
   static constexpr bool is_signed    = true;
   static constexpr bool is_integer   = false;
   static constexpr bool is_exact     = false;
   static constexpr int  radix        = 2;
   static number_type          epsilon()
   {
      number_type value(1);
      mpfr_div_2exp(value.backend().data(), value.backend().data(), boost::multiprecision::detail::digits10_2_2(number_type::thread_default_precision()) - 1, GMP_RNDN);
      return value;
   }
   static number_type round_error()
   {
      return 0.5;
   }
   static constexpr long min_exponent                  = MPFR_EMIN_DEFAULT;
   static constexpr long min_exponent10                = (MPFR_EMIN_DEFAULT / 1000) * 301L;
   static constexpr long max_exponent                  = MPFR_EMAX_DEFAULT;
   static constexpr long max_exponent10                = (MPFR_EMAX_DEFAULT / 1000) * 301L;
   static constexpr bool has_infinity                  = true;
   static constexpr bool has_quiet_NaN                 = true;
   static constexpr bool has_signaling_NaN             = false;
   static constexpr float_denorm_style has_denorm      = denorm_absent;
   static constexpr bool               has_denorm_loss = false;
   static number_type                        infinity()
   {
      number_type value;
      mpfr_set_inf(value.backend().data(), 1);
      return value;
   }
   static number_type quiet_NaN()
   {
      number_type value;
      mpfr_set_nan(value.backend().data());
      return value;
   }
   static number_type          signaling_NaN() { return number_type(0); }
   static number_type          denorm_min() { return number_type(0); }
   static constexpr bool is_iec559                = false;
   static constexpr bool is_bounded               = true;
   static constexpr bool is_modulo                = false;
   static constexpr bool traps                    = false;
   static constexpr bool tinyness_before          = false;
   static constexpr float_round_style round_style = round_toward_zero;
};

template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::digits;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::digits10;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::max_digits10;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::is_signed;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::is_integer;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::is_exact;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr int numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::radix;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr long numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::min_exponent;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr long numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::min_exponent10;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr long numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::max_exponent;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr long numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::max_exponent10;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::has_infinity;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::has_quiet_NaN;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::has_signaling_NaN;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr float_denorm_style numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::has_denorm;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::has_denorm_loss;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::is_iec559;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::is_bounded;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::is_modulo;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::traps;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr bool numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::tinyness_before;
template <boost::multiprecision::expression_template_option ExpressionTemplates>
constexpr float_round_style numeric_limits<boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, ExpressionTemplates> >::round_style;

} // namespace std
#endif
