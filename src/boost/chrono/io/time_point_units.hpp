//  (C) Copyright Howard Hinnant
//  (C) Copyright 2011 Vicente J. Botet Escriba
//  Copyright (c) Microsoft Corporation 2014
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//

#ifndef BOOST_CHRONO_IO_TIME_POINT_UNITS_HPP
#define BOOST_CHRONO_IO_TIME_POINT_UNITS_HPP

#include <boost/chrono/config.hpp>
#include <boost/chrono/process_cpu_clocks.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/thread_clock.hpp>
#include <boost/chrono/io/ios_base_state.hpp>
#include <string>
#include <iosfwd>
#include <ios>
#include <locale>
#include <algorithm>

namespace boost
{
  namespace chrono
  {
    /**
     * customization point to the epoch associated to the clock @c Clock
     * The default calls @c f.do_get_epoch(Clock()). The user can overload this function.
     * @return the string epoch associated to the @c Clock
     */
    template <typename CharT, typename Clock, typename TPUFacet>
    std::basic_string<CharT> get_epoch_custom(Clock, TPUFacet& f)
    {
      return f.do_get_epoch(Clock());
    }

    /**
     * @c time_point_units facet gives useful information about the time_point pattern,
     * the text associated to a time_point's epoch,
     */
    template <typename CharT=char>
    class time_point_units: public std::locale::facet
    {
    public:
      /**
       * Type of character the facet is instantiated on.
       */
      typedef CharT char_type;
      /**
       * Type of character string used by member functions.
       */
      typedef std::basic_string<char_type> string_type;

      /**
       * Unique identifier for this type of facet.
       */
      static std::locale::id id;

      /**
       * Construct a @c time_point_units facet.
       * @param refs
       * @Effects Construct a @c time_point_units facet.
       * If the @c refs argument is @c 0 then destruction of the object is
       * delegated to the @c locale, or locales, containing it. This allows
       * the user to ignore lifetime management issues. On the other had,
       * if @c refs is @c 1 then the object must be explicitly deleted;
       * the @c locale will not do so. In this case, the object can be
       * maintained across the lifetime of multiple locales.
       */
      explicit time_point_units(size_t refs = 0) :
        std::locale::facet(refs)
      {
      }

      /**
       * @return the pattern to be used by default.
       */
      virtual string_type get_pattern() const =0;

      /**
       * @return the epoch associated to the clock @c Clock calling @c do_get_epoch(Clock())
       */
      template <typename Clock>
      string_type get_epoch() const
      {
        return get_epoch_custom<CharT>(Clock(), *this);
      }

    protected:
      /**
       * Destroy the facet.
       */
      virtual ~time_point_units() {}

    public:

      /**
       *
       * @param c a dummy instance of @c system_clock.
       * @return The epoch string associated to the @c system_clock.
       */
      virtual string_type do_get_epoch(system_clock) const=0;

      /**
       *
       * @param c a dummy instance of @c steady_clock.
       * @return The epoch string associated to the @c steady_clock.
       */
      virtual string_type do_get_epoch(steady_clock) const=0;

#if defined(BOOST_CHRONO_HAS_PROCESS_CLOCKS)
      /**
       *
       * @param c a dummy instance of @c process_real_cpu_clock.
       * @return The epoch string associated to the @c process_real_cpu_clock.
       */
      virtual string_type do_get_epoch(process_real_cpu_clock) const=0;
#if ! BOOST_OS_WINDOWS || BOOST_PLAT_WINDOWS_DESKTOP
      /**
       *
       * @param c a dummy instance of @c process_user_cpu_clock.
       * @return The epoch string associated to the @c process_user_cpu_clock.
       */
      virtual string_type do_get_epoch(process_user_cpu_clock) const=0;
      /**
       *
       * @param c a dummy instance of @c process_system_cpu_clock.
       * @return The epoch string associated to the @c process_system_cpu_clock.
       */
      virtual string_type do_get_epoch(process_system_cpu_clock) const=0;
      /**
       *
       * @param c a dummy instance of @c process_cpu_clock.
       * @return The epoch string associated to the @c process_cpu_clock.
       */
      virtual string_type do_get_epoch(process_cpu_clock) const=0;
#endif
#endif
#if defined(BOOST_CHRONO_HAS_THREAD_CLOCK)
      /**
       *
       * @param c a dummy instance of @c thread_clock.
       * @return The epoch string associated to the @c thread_clock.
       */
      virtual string_type do_get_epoch(thread_clock) const=0;
#endif

    };

    template <typename CharT>
    std::locale::id time_point_units<CharT>::id;


    // This class is used to define the strings for the default English
    template <typename CharT=char>
    class time_point_units_default: public time_point_units<CharT>
    {
    public:
      /**
       * Type of character the facet is instantiated on.
       */
      typedef CharT char_type;
      /**
       * Type of character string returned by member functions.
       */
      typedef std::basic_string<char_type> string_type;

      explicit time_point_units_default(size_t refs = 0) :
        time_point_units<CharT> (refs)
      {
      }
      ~time_point_units_default() {}

      /**
       * @return the default pattern "%d%e".
       */
      string_type get_pattern() const
      {
        static const CharT t[] =
        { '%', 'd', '%', 'e' };
        static const string_type pattern(t, t + sizeof (t) / sizeof (t[0]));

        return pattern;
      }

    //protected:
      /**
       * @param c a dummy instance of @c system_clock.
       * @return The epoch string returned by @c clock_string<system_clock,CharT>::since().
       */
      string_type do_get_epoch(system_clock ) const
      {
        return clock_string<system_clock,CharT>::since();
      }
      /**
       * @param c a dummy instance of @c steady_clock.
       * @return The epoch string returned by @c clock_string<steady_clock,CharT>::since().
       */
      string_type do_get_epoch(steady_clock ) const
      {
        return clock_string<steady_clock,CharT>::since();
      }

#if defined(BOOST_CHRONO_HAS_PROCESS_CLOCKS)
      /**
       * @param c a dummy instance of @c process_real_cpu_clock.
       * @return The epoch string returned by @c clock_string<process_real_cpu_clock,CharT>::since().
       */
      string_type do_get_epoch(process_real_cpu_clock ) const
      {
        return clock_string<process_real_cpu_clock,CharT>::since();
      }
#if ! BOOST_OS_WINDOWS || BOOST_PLAT_WINDOWS_DESKTOP
      /**
       * @param c a dummy instance of @c process_user_cpu_clock.
       * @return The epoch string returned by @c clock_string<process_user_cpu_clock,CharT>::since().
       */
      string_type do_get_epoch(process_user_cpu_clock ) const
      {
        return clock_string<process_user_cpu_clock,CharT>::since();
      }
      /**
       * @param c a dummy instance of @c process_system_cpu_clock.
       * @return The epoch string returned by @c clock_string<process_system_cpu_clock,CharT>::since().
       */
      string_type do_get_epoch(process_system_cpu_clock ) const
      {
        return clock_string<process_system_cpu_clock,CharT>::since();
      }
      /**
       * @param c a dummy instance of @c process_cpu_clock.
       * @return The epoch string returned by @c clock_string<process_cpu_clock,CharT>::since().
       */
      string_type do_get_epoch(process_cpu_clock ) const
      {
        return clock_string<process_cpu_clock,CharT>::since();
      }

#endif
#endif
#if defined(BOOST_CHRONO_HAS_THREAD_CLOCK)
      /**
       * @param c a dummy instance of @c thread_clock.
       * @return The epoch string returned by @c clock_string<thread_clock,CharT>::since().
       */
      string_type do_get_epoch(thread_clock ) const
      {
        return clock_string<thread_clock,CharT>::since();
      }
#endif

    };


  } // chrono

} // boost

#endif  // header
