///////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt
//
// Generic routines for converting floating point values to and from decimal strings.
// Note that these use "naive" algorithms which result in rounding error - so they
// do not round trip to and from the string representation (but should only be out
// in the last bit).
//

#ifndef BOOST_MP_FLOAT_STRING_CVT_HPP
#define BOOST_MP_FLOAT_STRING_CVT_HPP

#include <cctype>

namespace boost { namespace multiprecision { namespace detail {

template <class I>
inline void round_string_up_at(std::string& s, int pos, I& expon)
{
   //
   // Rounds up a string representation of a number at pos:
   //
   if (pos < 0)
   {
      s.insert(static_cast<std::string::size_type>(0), 1, '1');
      s.erase(s.size() - 1);
      ++expon;
   }
   else if (s[pos] == '9')
   {
      s[pos] = '0';
      round_string_up_at(s, pos - 1, expon);
   }
   else
   {
      if ((pos == 0) && (s[pos] == '0') && (s.size() == 1))
         ++expon;
      ++s[pos];
   }
}

template <class Backend>
std::string convert_to_string(Backend b, std::streamsize digits, std::ios_base::fmtflags f)
{
   using default_ops::eval_convert_to;
   using default_ops::eval_divide;
   using default_ops::eval_floor;
   using default_ops::eval_fpclassify;
   using default_ops::eval_log10;
   using default_ops::eval_multiply;
   using default_ops::eval_pow;
   using default_ops::eval_subtract;

   using ui_type = typename std::tuple_element<0, typename Backend::unsigned_types>::type;
   using exponent_type = typename Backend::exponent_type                            ;

   std::string     result;
   bool            iszero     = false;
   bool            isneg      = false;
   exponent_type   expon      = 0;
   std::streamsize org_digits = digits;
   BOOST_ASSERT(digits > 0);

   int fpt = eval_fpclassify(b);

   if (fpt == (int)FP_ZERO)
   {
      result = "0";
      iszero = true;
   }
   else if (fpt == (int)FP_INFINITE)
   {
      if (b.compare(ui_type(0)) < 0)
         return "-inf";
      else
         return ((f & std::ios_base::showpos) == std::ios_base::showpos) ? "+inf" : "inf";
   }
   else if (fpt == (int)FP_NAN)
   {
      return "nan";
   }
   else
   {
      //
      // Start by figuring out the exponent:
      //
      isneg = b.compare(ui_type(0)) < 0;
      if (isneg)
         b.negate();
      Backend t;
      Backend ten;
      ten = ui_type(10);

      eval_log10(t, b);
      eval_floor(t, t);
      eval_convert_to(&expon, t);
      if (-expon > std::numeric_limits<number<Backend> >::max_exponent10 - 3)
      {
         int     e = -expon / 2;
         Backend t2;
         eval_pow(t2, ten, e);
         eval_multiply(t, t2, b);
         eval_multiply(t, t2);
         if (expon & 1)
            eval_multiply(t, ten);
      }
      else
      {
         eval_pow(t, ten, -expon);
         eval_multiply(t, b);
      }
      //
      // Make sure we're between [1,10) and adjust if not:
      //
      if (t.compare(ui_type(1)) < 0)
      {
         eval_multiply(t, ui_type(10));
         --expon;
      }
      else if (t.compare(ui_type(10)) >= 0)
      {
         eval_divide(t, ui_type(10));
         ++expon;
      }
      Backend digit;
      ui_type cdigit;
      //
      // Adjust the number of digits required based on formatting options:
      //
      if (((f & std::ios_base::fixed) == std::ios_base::fixed) && (expon != -1))
         digits += expon + 1;
      if ((f & std::ios_base::scientific) == std::ios_base::scientific)
         ++digits;
      //
      // Extract the digits one at a time:
      //
      for (unsigned i = 0; i < digits; ++i)
      {
         eval_floor(digit, t);
         eval_convert_to(&cdigit, digit);
         result += static_cast<char>('0' + cdigit);
         eval_subtract(t, digit);
         eval_multiply(t, ten);
      }
      //
      // Possibly round result:
      //
      if (digits >= 0)
      {
         eval_floor(digit, t);
         eval_convert_to(&cdigit, digit);
         eval_subtract(t, digit);
         if ((cdigit == 5) && (t.compare(ui_type(0)) == 0))
         {
            // Bankers rounding:
            if ((*result.rbegin() - '0') & 1)
            {
               round_string_up_at(result, result.size() - 1, expon);
            }
         }
         else if (cdigit >= 5)
         {
            round_string_up_at(result, result.size() - 1, expon);
         }
      }
   }
   while ((static_cast<std::streamsize>(result.size()) > digits) && (result.size() != 0U))
   {
      // We may get here as a result of rounding...
      if (result.size() > 1)
         result.erase(result.size() - 1);
      else
      {
         if (expon > 0)
            --expon; // so we put less padding in the result.
         else
            ++expon;
         ++digits;
      }
   }
   BOOST_ASSERT(org_digits >= 0);
   if (isneg)
      result.insert(static_cast<std::string::size_type>(0), 1, '-');
   format_float_string(result, expon, org_digits, f, iszero);

   return result;
}

template <class Backend>
void convert_from_string(Backend& b, const char* p)
{
   using default_ops::eval_add;
   using default_ops::eval_divide;
   using default_ops::eval_multiply;
   using default_ops::eval_pow;

   using ui_type = typename std::tuple_element<0, typename Backend::unsigned_types>::type;
   b = ui_type(0);
   if (!p || (*p == 0))
      return;

   bool                                                  is_neg       = false;
   bool                                                  is_neg_expon = false;
   constexpr const ui_type                               ten          = ui_type(10);
   typename Backend::exponent_type                       expon        = 0;
   int                                                   digits_seen  = 0;
   using limits = std::numeric_limits<number<Backend, et_off> >;
   constexpr const int                                   max_digits = limits::is_specialized ? limits::max_digits10 + 1 : INT_MAX;

   if (*p == '+')
      ++p;
   else if (*p == '-')
   {
      is_neg = true;
      ++p;
   }
   if ((std::strcmp(p, "nan") == 0) || (std::strcmp(p, "NaN") == 0) || (std::strcmp(p, "NAN") == 0))
   {
      eval_divide(b, ui_type(0));
      if (is_neg)
         b.negate();
      return;
   }
   if ((std::strcmp(p, "inf") == 0) || (std::strcmp(p, "Inf") == 0) || (std::strcmp(p, "INF") == 0))
   {
      b = ui_type(1);
      eval_divide(b, ui_type(0));
      if (is_neg)
         b.negate();
      return;
   }
   //
   // Grab all the leading digits before the decimal point:
   //
   while (std::isdigit(*p))
   {
      eval_multiply(b, ten);
      eval_add(b, ui_type(*p - '0'));
      ++p;
      ++digits_seen;
   }
   if (*p == '.')
   {
      //
      // Grab everything after the point, stop when we've seen
      // enough digits, even if there are actually more available:
      //
      ++p;
      while (std::isdigit(*p))
      {
         eval_multiply(b, ten);
         eval_add(b, ui_type(*p - '0'));
         ++p;
         --expon;
         if (++digits_seen > max_digits)
            break;
      }
      while (std::isdigit(*p))
         ++p;
   }
   //
   // Parse the exponent:
   //
   if ((*p == 'e') || (*p == 'E'))
   {
      ++p;
      if (*p == '+')
         ++p;
      else if (*p == '-')
      {
         is_neg_expon = true;
         ++p;
      }
      typename Backend::exponent_type e2 = 0;
      while (std::isdigit(*p))
      {
         e2 *= 10;
         e2 += (*p - '0');
         ++p;
      }
      if (is_neg_expon)
         e2 = -e2;
      expon += e2;
   }
   if (expon)
   {
      // Scale by 10^expon, note that 10^expon can be
      // outside the range of our number type, even though the
      // result is within range, if that looks likely, then split
      // the calculation in two:
      Backend t;
      t = ten;
      if (expon > limits::min_exponent10 + 2)
      {
         eval_pow(t, t, expon);
         eval_multiply(b, t);
      }
      else
      {
         eval_pow(t, t, expon + digits_seen + 1);
         eval_multiply(b, t);
         t = ten;
         eval_pow(t, t, -digits_seen - 1);
         eval_multiply(b, t);
      }
   }
   if (is_neg)
      b.negate();
   if (*p)
   {
      // Unexpected input in string:
      BOOST_THROW_EXCEPTION(std::runtime_error("Unexpected characters in string being interpreted as a float128."));
   }
}

}}} // namespace boost::multiprecision::detail

#endif
