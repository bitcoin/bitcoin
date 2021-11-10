///////////////////////////////////////////////////////////////////////////////
// Copyright Christopher Kormanyos 2014.
// Copyright John Maddock 2014.
// Copyright Paul Bristow 2014.
// Distributed under the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Implement quadruple-precision I/O stream operations.

#ifndef BOOST_MATH_CSTDFLOAT_IOSTREAM_2014_02_15_HPP_
  #define BOOST_MATH_CSTDFLOAT_IOSTREAM_2014_02_15_HPP_

  #include <boost/math/cstdfloat/cstdfloat_types.hpp>
  #include <boost/math/cstdfloat/cstdfloat_limits.hpp>
  #include <boost/math/cstdfloat/cstdfloat_cmath.hpp>

  #if defined(BOOST_CSTDFLOAT_NO_LIBQUADMATH_CMATH)
  #error You can not use <boost/math/cstdfloat/cstdfloat_iostream.hpp> with BOOST_CSTDFLOAT_NO_LIBQUADMATH_CMATH defined.
  #endif

  #if defined(BOOST_CSTDFLOAT_HAS_INTERNAL_FLOAT128_T) && defined(BOOST_MATH_USE_FLOAT128) && !defined(BOOST_CSTDFLOAT_NO_LIBQUADMATH_SUPPORT)

  #include <cstddef>
  #include <istream>
  #include <ostream>
  #include <sstream>
  #include <stdexcept>
  #include <string>
  #include <boost/math/tools/assert.hpp>
  #include <boost/math/tools/throw_exception.hpp>

//  #if (0)
  #if defined(__GNUC__)

  // Forward declarations of quadruple-precision string functions.
  extern "C" int quadmath_snprintf(char *str, size_t size, const char *format, ...) throw();
  extern "C" boost::math::cstdfloat::detail::float_internal128_t strtoflt128(const char*, char **) throw();

  namespace std
  {
    template<typename char_type, class traits_type>
    inline std::basic_ostream<char_type, traits_type>& operator<<(std::basic_ostream<char_type, traits_type>& os, const boost::math::cstdfloat::detail::float_internal128_t& x)
    {
      std::basic_ostringstream<char_type, traits_type> ostr;
      ostr.flags(os.flags());
      ostr.imbue(os.getloc());
      ostr.precision(os.precision());

      char my_buffer[64U];

      const int my_prec   = static_cast<int>(os.precision());
      const int my_digits = ((my_prec == 0) ? 36 : my_prec);

      const std::ios_base::fmtflags my_flags  = os.flags();

      char my_format_string[8U];

      std::size_t my_format_string_index = 0U;

      my_format_string[my_format_string_index] = '%';
      ++my_format_string_index;

      if(my_flags & std::ios_base::showpos)   { my_format_string[my_format_string_index] = '+'; ++my_format_string_index; }
      if(my_flags & std::ios_base::showpoint) { my_format_string[my_format_string_index] = '#'; ++my_format_string_index; }

      my_format_string[my_format_string_index + 0U] = '.';
      my_format_string[my_format_string_index + 1U] = '*';
      my_format_string[my_format_string_index + 2U] = 'Q';

      my_format_string_index += 3U;

      char the_notation_char;

      if     (my_flags & std::ios_base::scientific) { the_notation_char = 'e'; }
      else if(my_flags & std::ios_base::fixed)      { the_notation_char = 'f'; }
      else                                          { the_notation_char = 'g'; }

      my_format_string[my_format_string_index + 0U] = the_notation_char;
      my_format_string[my_format_string_index + 1U] = 0;

      const int v = ::quadmath_snprintf(my_buffer,
                                        static_cast<int>(sizeof(my_buffer)),
                                        my_format_string,
                                        my_digits,
                                        x);

      if(v < 0) { BOOST_MATH_THROW_EXCEPTION(std::runtime_error("Formatting of boost::float128_t failed internally in quadmath_snprintf().")); }

      if(v >= static_cast<int>(sizeof(my_buffer) - 1U))
      {
        // Evidently there is a really long floating-point string here,
        // such as a small decimal representation in non-scientific notation.
        // So we have to use dynamic memory allocation for the output
        // string buffer.

        char* my_buffer2 = static_cast<char*>(0U);

#ifndef BOOST_NO_EXCEPTIONS
        try
        {
#endif
          my_buffer2 = new char[v + 3];
#ifndef BOOST_NO_EXCEPTIONS
        }
        catch(const std::bad_alloc&)
        {
          BOOST_MATH_THROW_EXCEPTION(std::runtime_error("Formatting of boost::float128_t failed while allocating memory."));
        }
#endif
        const int v2 = ::quadmath_snprintf(my_buffer2,
                                            v + 3,
                                            my_format_string,
                                            my_digits,
                                            x);

        if(v2 >= v + 3)
        {
          BOOST_MATH_THROW_EXCEPTION(std::runtime_error("Formatting of boost::float128_t failed."));
        }

        static_cast<void>(ostr << my_buffer2);

        delete [] my_buffer2;
      }
      else
      {
        static_cast<void>(ostr << my_buffer);
      }

      return (os << ostr.str());
    }

    template<typename char_type, class traits_type>
    inline std::basic_istream<char_type, traits_type>& operator>>(std::basic_istream<char_type, traits_type>& is, boost::math::cstdfloat::detail::float_internal128_t& x)
    {
      std::string str;

      static_cast<void>(is >> str);

      char* p_end;

      x = strtoflt128(str.c_str(), &p_end);

      if(static_cast<std::ptrdiff_t>(p_end - str.c_str()) != static_cast<std::ptrdiff_t>(str.length()))
      {
        for(std::string::const_reverse_iterator it = str.rbegin(); it != str.rend(); ++it)
        {
          static_cast<void>(is.putback(*it));
        }

        is.setstate(ios_base::failbit);

        BOOST_MATH_THROW_EXCEPTION(std::runtime_error("Unable to interpret input string as a boost::float128_t"));
      }

      return is;
    }
  }

//  #elif defined(__GNUC__)
  #elif defined(__INTEL_COMPILER)

  // The section for I/O stream support for the ICC compiler is particularly
  // long, because these functions must be painstakingly synthesized from
  // manually-written routines (ICC does not support I/O stream operations
  // for its _Quad type).

  // The following string-extraction routines are based on the methodology
  // used in Boost.Multiprecision by John Maddock and Christopher Kormanyos.
  // This methodology has been slightly modified here for boost::float128_t.

  #include <cstring>
  #include <cctype>
  #include <boost/math/tools/lexical_cast.hpp>

  namespace boost { namespace math { namespace cstdfloat { namespace detail {

  template<class string_type>
  void format_float_string(string_type& str,
                            int my_exp,
                            int digits,
                            const std::ios_base::fmtflags f,
                            const bool iszero)
  {
    typedef typename string_type::size_type size_type;

    const bool scientific = ((f & std::ios_base::scientific) == std::ios_base::scientific);
    const bool fixed      = ((f & std::ios_base::fixed)      == std::ios_base::fixed);
    const bool showpoint  = ((f & std::ios_base::showpoint)  == std::ios_base::showpoint);
    const bool showpos    = ((f & std::ios_base::showpos)    == std::ios_base::showpos);

    const bool b_neg = ((str.size() != 0U) && (str[0] == '-'));

    if(b_neg)
    {
      str.erase(0, 1);
    }

    if(digits == 0)
    {
      digits = static_cast<int>((std::max)(str.size(), size_type(16)));
    }

    if(iszero || str.empty() || (str.find_first_not_of('0') == string_type::npos))
    {
      // We will be printing zero, even though the value might not
      // actually be zero (it just may have been rounded to zero).
      str = "0";

      if(scientific || fixed)
      {
        str.append(1, '.');
        str.append(size_type(digits), '0');

        if(scientific)
        {
          str.append("e+00");
        }
      }
      else
      {
        if(showpoint)
        {
          str.append(1, '.');
          if(digits > 1)
          {
            str.append(size_type(digits - 1), '0');
          }
        }
      }

      if(b_neg)
      {
        str.insert(0U, 1U, '-');
      }
      else if(showpos)
      {
        str.insert(0U, 1U, '+');
      }

      return;
    }

    if(!fixed && !scientific && !showpoint)
    {
      // Suppress trailing zeros.
      typename string_type::iterator pos = str.end();

      while(pos != str.begin() && *--pos == '0') { ; }

      if(pos != str.end())
      {
        ++pos;
      }

      str.erase(pos, str.end());

      if(str.empty())
      {
        str = '0';
      }
    }
    else if(!fixed || (my_exp >= 0))
    {
      // Pad out the end with zero's if we need to.

      int chars = static_cast<int>(str.size());
      chars = digits - chars;

      if(scientific)
      {
        ++chars;
      }

      if(chars > 0)
      {
        str.append(static_cast<size_type>(chars), '0');
      }
    }

    if(fixed || (!scientific && (my_exp >= -4) && (my_exp < digits)))
    {
      if((1 + my_exp) > static_cast<int>(str.size()))
      {
        // Just pad out the end with zeros.
        str.append(static_cast<size_type>((1 + my_exp) - static_cast<int>(str.size())), '0');

        if(showpoint || fixed)
        {
          str.append(".");
        }
      }
      else if(my_exp + 1 < static_cast<int>(str.size()))
      {
        if(my_exp < 0)
        {
          str.insert(0U, static_cast<size_type>(-1 - my_exp), '0');
          str.insert(0U, "0.");
        }
        else
        {
          // Insert the decimal point:
          str.insert(static_cast<size_type>(my_exp + 1), 1, '.');
        }
      }
      else if(showpoint || fixed) // we have exactly the digits we require to left of the point
      {
        str += ".";
      }

      if(fixed)
      {
        // We may need to add trailing zeros.
        int l = static_cast<int>(str.find('.') + 1U);
        l = digits - (static_cast<int>(str.size()) - l);

        if(l > 0)
        {
          str.append(size_type(l), '0');
        }
      }
    }
    else
    {
      // Scientific format:
      if(showpoint || (str.size() > 1))
      {
        str.insert(1U, 1U, '.');
      }

      str.append(1U, 'e');

      #ifdef BOOST_MATH_STANDALONE
      static_assert(sizeof(string_type), "IO streams for intel compilers using _Quad types can not be used in standalone mode");
      #else
      string_type e = boost::lexical_cast<string_type>(std::abs(my_exp));
      #endif

      if(e.size() < 2U)
      {
        e.insert(0U, 2U - e.size(), '0');
      }

      if(my_exp < 0)
      {
        e.insert(0U, 1U, '-');
      }
      else
      {
        e.insert(0U, 1U, '+');
      }

      str.append(e);
    }

    if(b_neg)
    {
      str.insert(0U, 1U, '-');
    }
    else if(showpos)
    {
      str.insert(0U, 1U, '+');
    }
  }

  template<class float_type, class type_a> inline void eval_convert_to(type_a* pa,    const float_type& cb)                        { *pa  = static_cast<type_a>(cb); }
  template<class float_type, class type_a> inline void eval_add       (float_type& b, const type_a& a)                             { b   += a; }
  template<class float_type, class type_a> inline void eval_subtract  (float_type& b, const type_a& a)                             { b   -= a; }
  template<class float_type, class type_a> inline void eval_multiply  (float_type& b, const type_a& a)                             { b   *= a; }
  template<class float_type>               inline void eval_multiply  (float_type& b, const float_type& cb, const float_type& cb2) { b    = (cb * cb2); }
  template<class float_type, class type_a> inline void eval_divide    (float_type& b, const type_a& a)                             { b   /= a; }
  template<class float_type>               inline void eval_log10     (float_type& b, const float_type& cb)                        { b    = std::log10(cb); }
  template<class float_type>               inline void eval_floor     (float_type& b, const float_type& cb)                        { b    = std::floor(cb); }

  inline void round_string_up_at(std::string& s, int pos, int& expon)
  {
    // This subroutine rounds up a string representation of a
    // number at the given position pos.

    if(pos < 0)
    {
      s.insert(0U, 1U, '1');
      s.erase(s.size() - 1U);
      ++expon;
    }
    else if(s[pos] == '9')
    {
      s[pos] = '0';
      round_string_up_at(s, pos - 1, expon);
    }
    else
    {
      if((pos == 0) && (s[pos] == '0') && (s.size() == 1))
      {
        ++expon;
      }

      ++s[pos];
    }
  }

  template<class float_type>
  std::string convert_to_string(float_type& x,
                                std::streamsize digits,
                                const std::ios_base::fmtflags f)
  {
    const bool isneg  = (x < 0);
    const bool iszero = ((!isneg) ? bool(+x < (std::numeric_limits<float_type>::min)())
                                  : bool(-x < (std::numeric_limits<float_type>::min)()));
    const bool isnan  = (x != x);
    const bool isinf  = ((!isneg) ? bool(+x > (std::numeric_limits<float_type>::max)())
                                  : bool(-x > (std::numeric_limits<float_type>::max)()));

    int expon = 0;

    if(digits <= 0) { digits = std::numeric_limits<float_type>::max_digits10; }

    const int org_digits = static_cast<int>(digits);

    std::string result;

    if(iszero)
    {
      result = "0";
    }
    else if(isinf)
    {
      if(x < 0)
      {
        return "-inf";
      }
      else
      {
        return ((f & std::ios_base::showpos) == std::ios_base::showpos) ? "+inf" : "inf";
      }
    }
    else if(isnan)
    {
      return "nan";
    }
    else
    {
      // Start by figuring out the base-10 exponent.
      if(isneg) { x = -x; }

      float_type t;
      float_type ten = 10;

      eval_log10(t, x);
      eval_floor(t, t);
      eval_convert_to(&expon, t);

      if(-expon > std::numeric_limits<float_type>::max_exponent10 - 3)
      {
        int e = -expon / 2;

        const float_type t2 = boost::math::cstdfloat::detail::pown(ten, e);

        eval_multiply(t, t2, x);
        eval_multiply(t, t2);

        if((expon & 1) != 0)
        {
          eval_multiply(t, ten);
        }
      }
      else
      {
        t = boost::math::cstdfloat::detail::pown(ten, -expon);
        eval_multiply(t, x);
      }

      // Make sure that the value lies between [1, 10), and adjust if not.
      if(t < 1)
      {
        eval_multiply(t, 10);

        --expon;
      }
      else if(t >= 10)
      {
        eval_divide(t, 10);

        ++expon;
      }

      float_type digit;
      int        cdigit;

      // Adjust the number of digits required based on formatting options.
      if(((f & std::ios_base::fixed) == std::ios_base::fixed) && (expon != -1))
      {
        digits += (expon + 1);
      }

      if((f & std::ios_base::scientific) == std::ios_base::scientific)
      {
        ++digits;
      }

      // Extract the base-10 digits one at a time.
      for(int i = 0; i < digits; ++i)
      {
        eval_floor(digit, t);
        eval_convert_to(&cdigit, digit);

        result += static_cast<char>('0' + cdigit);

        eval_subtract(t, digit);
        eval_multiply(t, ten);
      }

      // Possibly round the result.
      if(digits >= 0)
      {
        eval_floor(digit, t);
        eval_convert_to(&cdigit, digit);
        eval_subtract(t, digit);

        if((cdigit == 5) && (t == 0))
        {
          // Use simple bankers rounding.

          if((static_cast<int>(*result.rbegin() - '0') & 1) != 0)
          {
            round_string_up_at(result, static_cast<int>(result.size() - 1U), expon);
          }
        }
        else if(cdigit >= 5)
        {
          round_string_up_at(result, static_cast<int>(result.size() - 1), expon);
        }
      }
    }

    while((result.size() > static_cast<std::string::size_type>(digits)) && result.size())
    {
      // We may get here as a result of rounding.

      if(result.size() > 1U)
      {
        result.erase(result.size() - 1U);
      }
      else
      {
        if(expon > 0)
        {
          --expon; // so we put less padding in the result.
        }
        else
        {
          ++expon;
        }

        ++digits;
      }
    }

    if(isneg)
    {
      result.insert(0U, 1U, '-');
    }

    format_float_string(result, expon, org_digits, f, iszero);

    return result;
  }

  template <class float_type>
  bool convert_from_string(float_type& value, const char* p)
  {
    value = 0;

    if((p == static_cast<const char*>(0U)) || (*p == static_cast<char>(0)))
    {
      return;
    }

    bool is_neg       = false;
    bool is_neg_expon = false;

    constexpr int ten = 10;

    int expon       = 0;
    int digits_seen = 0;

    constexpr int max_digits = std::numeric_limits<float_type>::max_digits10 + 1;

    if(*p == static_cast<char>('+'))
    {
      ++p;
    }
    else if(*p == static_cast<char>('-'))
    {
      is_neg = true;
      ++p;
    }

    const bool isnan = ((std::strcmp(p, "nan") == 0) || (std::strcmp(p, "NaN") == 0) || (std::strcmp(p, "NAN") == 0));

    if(isnan)
    {
      eval_divide(value, 0);

      if(is_neg)
      {
        value = -value;
      }

      return true;
    }

    const bool isinf = ((std::strcmp(p, "inf") == 0) || (std::strcmp(p, "Inf") == 0) || (std::strcmp(p, "INF") == 0));

    if(isinf)
    {
      value = 1;
      eval_divide(value, 0);

      if(is_neg)
      {
        value = -value;
      }

      return true;
    }

    // Grab all the leading digits before the decimal point.
    while(std::isdigit(*p))
    {
      eval_multiply(value, ten);
      eval_add(value, static_cast<int>(*p - '0'));
      ++p;
      ++digits_seen;
    }

    if(*p == static_cast<char>('.'))
    {
      // Grab everything after the point, stop when we've seen
      // enough digits, even if there are actually more available.

      ++p;

      while(std::isdigit(*p))
      {
        eval_multiply(value, ten);
        eval_add(value, static_cast<int>(*p - '0'));
        ++p;
        --expon;

        if(++digits_seen > max_digits)
        {
          break;
        }
      }

      while(std::isdigit(*p))
      {
        ++p;
      }
    }

    // Parse the exponent.
    if((*p == static_cast<char>('e')) || (*p == static_cast<char>('E')))
    {
      ++p;

      if(*p == static_cast<char>('+'))
      {
        ++p;
      }
      else if(*p == static_cast<char>('-'))
      {
        is_neg_expon = true;
        ++p;
      }

      int e2 = 0;

      while(std::isdigit(*p))
      {
        e2 *= 10;
        e2 += (*p - '0');
        ++p;
      }

      if(is_neg_expon)
      {
        e2 = -e2;
      }

      expon += e2;
    }

    if(expon)
    {
      // Scale by 10^expon. Note that 10^expon can be outside the range
      // of our number type, even though the result is within range.
      // If that looks likely, then split the calculation in two parts.
      float_type t;
      t = ten;

      if(expon > (std::numeric_limits<float_type>::min_exponent10 + 2))
      {
        t = boost::math::cstdfloat::detail::pown(t, expon);
        eval_multiply(value, t);
      }
      else
      {
        t = boost::math::cstdfloat::detail::pown(t, (expon + digits_seen + 1));
        eval_multiply(value, t);
        t = ten;
        t = boost::math::cstdfloat::detail::pown(t, (-digits_seen - 1));
        eval_multiply(value, t);
      }
    }

    if(is_neg)
    {
      value = -value;
    }

    return (*p == static_cast<char>(0));
  }
  } } } } // boost::math::cstdfloat::detail

  namespace std
  {
    template<typename char_type, class traits_type>
    inline std::basic_ostream<char_type, traits_type>& operator<<(std::basic_ostream<char_type, traits_type>& os, const boost::math::cstdfloat::detail::float_internal128_t& x)
    {
      boost::math::cstdfloat::detail::float_internal128_t non_const_x = x;

      const std::string str = boost::math::cstdfloat::detail::convert_to_string(non_const_x,
                                                                                os.precision(),
                                                                                os.flags());

      std::basic_ostringstream<char_type, traits_type> ostr;
      ostr.flags(os.flags());
      ostr.imbue(os.getloc());
      ostr.precision(os.precision());

      static_cast<void>(ostr << str);

      return (os << ostr.str());
    }

    template<typename char_type, class traits_type>
    inline std::basic_istream<char_type, traits_type>& operator>>(std::basic_istream<char_type, traits_type>& is, boost::math::cstdfloat::detail::float_internal128_t& x)
    {
      std::string str;

      static_cast<void>(is >> str);

      const bool conversion_is_ok = boost::math::cstdfloat::detail::convert_from_string(x, str.c_str());

      if(false == conversion_is_ok)
      {
        for(std::string::const_reverse_iterator it = str.rbegin(); it != str.rend(); ++it)
        {
          static_cast<void>(is.putback(*it));
        }

        is.setstate(ios_base::failbit);

        BOOST_MATH_THROW_EXCEPTION(std::runtime_error("Unable to interpret input string as a boost::float128_t"));
      }

      return is;
    }
  }

  #endif // Use __GNUC__ or __INTEL_COMPILER libquadmath

  #endif // Not BOOST_CSTDFLOAT_NO_LIBQUADMATH_SUPPORT (i.e., the user would like to have libquadmath support)

#endif // BOOST_MATH_CSTDFLOAT_IOSTREAM_2014_02_15_HPP_
