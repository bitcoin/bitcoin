//  (C) Copyright Howard Hinnant
//  (C) Copyright 2011 Vicente J. Botet Escriba
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//

/**
 * Duration formatting facet for output.
 */
#ifndef BOOST_CHRONO_IO_DURATION_PUT_HPP
#define BOOST_CHRONO_IO_DURATION_PUT_HPP

#include <boost/chrono/config.hpp>
#include <boost/chrono/io/duration_units.hpp>
#include <boost/chrono/process_cpu_clocks.hpp>
#include <boost/assert.hpp>
#include <locale>

namespace boost
{
  namespace chrono
  {

    namespace detail
    {
      template <class T>
      struct propagate {
        typedef T type;
      };
      template <>
      struct propagate<boost::int_least32_t> {
        typedef boost::int_least64_t type;
      };
    }
    /**
     * @tparam ChatT a character type
     * @tparam OutputIterator a model of @c OutputIterator
     *
     * The @c duration_put facet provides facilities for formatted output of duration values.
     * The member function of @c duration_put take a duration and format it into character string representation.
     *
     */
    template <class CharT, class OutputIterator = std::ostreambuf_iterator<CharT> >
    class duration_put: public std::locale::facet
    {
    public:
      /**
       * Type of character the facet is instantiated on.
       */
      typedef CharT char_type;
      /**
       * Type of character string passed to member functions.
       */
      typedef std::basic_string<CharT> string_type;
      /**
       * Type of iterator used to write in the character buffer.
       */
      typedef OutputIterator iter_type;

      /**
       * Construct a duration_put facet.
       * @param refs
       * @Effects Construct a duration_put facet.
       * If the @c refs argument is @c 0 then destruction of the object is
       * delegated to the @c locale, or locales, containing it. This allows
       * the user to ignore lifetime management issues. On the other had,
       * if @c refs is @c 1 then the object must be explicitly deleted;
       * the @c locale will not do so. In this case, the object can be
       * maintained across the lifetime of multiple locales.
       */
      explicit duration_put(size_t refs = 0) :
        std::locale::facet(refs)
      {
      }

      /**
       *
       * @param s an output stream iterator
       * @param ios a reference to a ios_base
       * @param fill the character used as filler
       * @param d the duration
       * @param pattern begin of the formatting pattern
       * @param pat_end end of the formatting pattern
       *
       * @Effects Steps through the sequence from @c pattern to @c pat_end,
       * identifying characters that are part of a pattern sequence. Each character
       * that is not part of a pattern sequence is written to @c s immediately, and
       * each pattern sequence, as it is identified, results in a call to
       * @c put_value or @c put_unit;
       * thus, pattern elements and other characters are interleaved in the output
       * in the order in which they appear in the pattern. Pattern sequences are
       * identified by converting each character @c c to a @c char value as if by
       * @c ct.narrow(c,0), where @c ct is a reference to @c ctype<charT> obtained from
       * @c ios.getloc(). The first character of each sequence is equal to @c '%',
       * followed by a pattern specifier character @c spec, which can be @c 'v' for
       * the duration value or @c 'u' for the duration unit. .
       * For each valid pattern sequence identified, calls
       * <c>put_value(s, ios, fill, d)</c> or <c>put_unit(s, ios, fill, d)</c>.
       *
       * @Returns An iterator pointing immediately after the last character produced.
       */
      template <typename Rep, typename Period>
      iter_type put(iter_type s, std::ios_base& ios, char_type fill, duration<Rep, Period> const& d, const CharT* pattern,
          const CharT* pat_end, const char_type* val = 0) const
      {
        if (std::has_facet<duration_units<CharT> >(ios.getloc()))
        {
          duration_units<CharT> const&facet = std::use_facet<duration_units<CharT> >(
              ios.getloc());
          return put(facet, s, ios, fill, d, pattern, pat_end, val);
        }
        else
        {
          duration_units_default<CharT> facet;
          return put(facet, s, ios, fill, d, pattern, pat_end, val);
        }
      }

      template <typename Rep, typename Period>
      iter_type put(duration_units<CharT> const& units_facet, iter_type s, std::ios_base& ios, char_type fill,
          duration<Rep, Period> const& d, const CharT* pattern, const CharT* pat_end, const char_type* val = 0) const
      {

        const std::ctype<char_type>& ct = std::use_facet<std::ctype<char_type> >(ios.getloc());
        for (; pattern != pat_end; ++pattern)
        {
          if (ct.narrow(*pattern, 0) == '%')
          {
            if (++pattern == pat_end)
            {
              *s++ = pattern[-1];
              break;
            }
            char fmt = ct.narrow(*pattern, 0);
            switch (fmt)
            {
            case 'v':
            {
              s = put_value(s, ios, fill, d, val);
              break;
            }
            case 'u':
            {
              s = put_unit(units_facet, s, ios, fill, d);
              break;
            }
            default:
              BOOST_ASSERT(false && "Boost::Chrono internal error.");
              break;
            }
          }
          else
            *s++ = *pattern;
        }
        return s;
      }

      /**
       *
       * @param s an output stream iterator
       * @param ios a reference to a ios_base
       * @param fill the character used as filler
       * @param d the duration
       * @Effects imbue in @c ios the @c duration_units_default facet if not already present.
       * Retrieves Stores the duration pattern from the @c duration_unit facet in let say @c str. Last as if
       * @code
       *   return put(s, ios, d, str.data(), str.data() + str.size());
       * @endcode
       * @Returns An iterator pointing immediately after the last character produced.
       */
      template <typename Rep, typename Period>
      iter_type put(iter_type s, std::ios_base& ios, char_type fill, duration<Rep, Period> const& d, const char_type* val = 0) const
      {
        if (std::has_facet<duration_units<CharT> >(ios.getloc()))
        {
          duration_units<CharT> const&facet = std::use_facet<duration_units<CharT> >(
              ios.getloc());
          std::basic_string<CharT> str = facet.get_pattern();
          return put(facet, s, ios, fill, d, str.data(), str.data() + str.size(), val);
        }
        else
        {
          duration_units_default<CharT> facet;
          std::basic_string<CharT> str = facet.get_pattern();

          return put(facet, s, ios, fill, d, str.data(), str.data() + str.size(), val);
        }
      }

      /**
       *
       * @param s an output stream iterator
       * @param ios a reference to a ios_base
       * @param fill the character used as filler
       * @param d the duration
       * @Effects As if s=std::use_facet<std::num_put<CharT, iter_type> >(ios.getloc()).put(s, ios, fill, static_cast<long int> (d.count())).
       * @Returns s, iterator pointing immediately after the last character produced.
       */
      template <typename Rep, typename Period>
      iter_type put_value(iter_type s, std::ios_base& ios, char_type fill, duration<Rep, Period> const& d, const char_type* val = 0) const
      {
        if (val)
        {
          while (*val) {
            *s = *val;
            s++; val++;
          }
          return s;
        }
        return std::use_facet<std::num_put<CharT, iter_type> >(ios.getloc()).put(s, ios, fill,
            static_cast<typename detail::propagate<Rep>::type> (d.count()));
      }

      template <typename Rep, typename Period>
      iter_type put_value(iter_type s, std::ios_base& ios, char_type fill, duration<process_times<Rep>, Period> const& d, const char_type* = 0) const
      {
        *s++ = CharT('{');
        s = put_value(s, ios, fill, process_real_cpu_clock::duration(d.count().real));
        *s++ = CharT(';');
        s = put_value(s, ios, fill, process_user_cpu_clock::duration(d.count().user));
        *s++ = CharT(';');
        s = put_value(s, ios, fill, process_system_cpu_clock::duration(d.count().system));
        *s++ = CharT('}');
        return s;
      }

      /**
       *
       * @param s an output stream iterator
       * @param ios a reference to a ios_base
       * @param fill the character used as filler
       * @param d the duration
       * @Effects Let facet be the duration_units<CharT> facet associated to ios. If the associated unit is named,
       * as if
       * @code
          string_type str = facet.get_unit(get_duration_style(ios), d);
          s=std::copy(str.begin(), str.end(), s);
       * @endcode
       * Otherwise, format the unit as "[Period::num/Period::den]" followed by the unit associated to [N/D] obtained using facet.get_n_d_unit(get_duration_style(ios), d)
       * @Returns s, iterator pointing immediately after the last character produced.
       */
      template <typename Rep, typename Period>
      iter_type put_unit(iter_type s, std::ios_base& ios, char_type fill, duration<Rep, Period> const& d) const
      {
        if (std::has_facet<duration_units<CharT> >(ios.getloc()))
        {
          duration_units<CharT> const&facet = std::use_facet<duration_units<CharT> >(
              ios.getloc());
          return put_unit(facet, s, ios, fill, d);
        }
        else
        {
          duration_units_default<CharT> facet;
          return put_unit(facet, s, ios, fill, d);
        }
      }

      template <typename Rep, typename Period>
      iter_type put_unit(duration_units<CharT> const& facet, iter_type s, std::ios_base& ios, char_type fill,
          duration<Rep, Period> const& d) const
      {
        if (facet.template is_named_unit<Period>()) {
          string_type str = facet.get_unit(get_duration_style(ios), d);
          s=std::copy(str.begin(), str.end(), s);
        } else {
          *s++ = CharT('[');
          std::use_facet<std::num_put<CharT, iter_type> >(ios.getloc()).put(s, ios, fill, Period::num);
          *s++ = CharT('/');
          std::use_facet<std::num_put<CharT, iter_type> >(ios.getloc()).put(s, ios, fill, Period::den);
          *s++ = CharT(']');
          string_type str = facet.get_n_d_unit(get_duration_style(ios), d);
          s=std::copy(str.begin(), str.end(), s);
        }
        return s;
      }
      template <typename Rep, typename Period>
      iter_type put_unit(duration_units<CharT> const& facet, iter_type s, std::ios_base& ios, char_type fill,
          duration<process_times<Rep>, Period> const& d) const
      {
        duration<Rep,Period> real(d.count().real);
        if (facet.template is_named_unit<Period>()) {
          string_type str = facet.get_unit(get_duration_style(ios), real);
          s=std::copy(str.begin(), str.end(), s);
        } else {
          *s++ = CharT('[');
          std::use_facet<std::num_put<CharT, iter_type> >(ios.getloc()).put(s, ios, fill, Period::num);
          *s++ = CharT('/');
          std::use_facet<std::num_put<CharT, iter_type> >(ios.getloc()).put(s, ios, fill, Period::den);
          *s++ = CharT(']');
          string_type str = facet.get_n_d_unit(get_duration_style(ios), real);
          s=std::copy(str.begin(), str.end(), s);
        }
        return s;
      }

      /**
       * Unique identifier for this type of facet.
       */
      static std::locale::id id;

      /**
       * @Effects Destroy the facet
       */
      ~duration_put()
      {
      }

    };

    template <class CharT, class OutputIterator>
    std::locale::id duration_put<CharT, OutputIterator>::id;

  } // chrono
} // boost

#endif  // header
