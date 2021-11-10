#ifndef DATE_TIME_C_TIME_HPP___
#define DATE_TIME_C_TIME_HPP___

/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */


/*! @file c_time.hpp
  Provide workarounds related to the ctime header
*/

#include <ctime>
#include <string> // to be able to convert from string literals to exceptions
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/date_time/compiler_config.hpp>

//Work around libraries that don't put time_t and time in namespace std
#ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::time_t; using ::time; using ::localtime;
                using ::tm;  using ::gmtime; }
#endif // BOOST_NO_STDC_NAMESPACE

//The following is used to support high precision time clocks
#ifdef BOOST_HAS_GETTIMEOFDAY
#include <sys/time.h>
#endif

#ifdef BOOST_HAS_FTIME
#include <time.h>
#endif

namespace boost {
namespace date_time {
  //! Provides a uniform interface to some 'ctime' functions
  /*! Provides a uniform interface to some ctime functions and
   * their '_r' counterparts. The '_r' functions require a pointer to a
   * user created std::tm struct whereas the regular functions use a
   * staticly created struct and return a pointer to that. These wrapper
   * functions require the user to create a std::tm struct and send in a
   * pointer to it. This struct may be used to store the resulting time.
   * The returned pointer may or may not point to this struct, however,
   * it will point to the result of the corresponding function.
   * All functions do proper checking of the C function results and throw
   * exceptions on error. Therefore the functions will never return NULL.
   */
  struct c_time {
    public:
#if defined(BOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS)
      //! requires a pointer to a user created std::tm struct
      inline
      static std::tm* localtime(const std::time_t* t, std::tm* result)
      {
        // localtime_r() not in namespace std???
#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
        std::tm tmp;
        if(!localtime_r(t,&tmp))
            result = 0;
        else
            *result = tmp;
#else
        result = localtime_r(t, result);
#endif
        if (!result)
          boost::throw_exception(std::runtime_error("could not convert calendar time to local time"));
        return result;
      }
      //! requires a pointer to a user created std::tm struct
      inline
      static std::tm* gmtime(const std::time_t* t, std::tm* result)
      {
        // gmtime_r() not in namespace std???
#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
        std::tm tmp;
        if(!gmtime_r(t,&tmp))
          result = 0;
        else
          *result = tmp;
#else
        result = gmtime_r(t, result);
#endif
        if (!result)
          boost::throw_exception(std::runtime_error("could not convert calendar time to UTC time"));
        return result;
      }
#else // BOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS

#if defined(__clang__) // Clang has to be checked before MSVC
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif (defined(_MSC_VER) && (_MSC_VER >= 1400))
#pragma warning(push) // preserve warning settings
#pragma warning(disable : 4996) // disable depricated localtime/gmtime warning on vc8
#endif
      //! requires a pointer to a user created std::tm struct
      inline
      static std::tm* localtime(const std::time_t* t, std::tm* result)
      {
        result = std::localtime(t);
        if (!result)
          boost::throw_exception(std::runtime_error("could not convert calendar time to local time"));
        return result;
      }
      //! requires a pointer to a user created std::tm struct
      inline
      static std::tm* gmtime(const std::time_t* t, std::tm* result)
      {
        result = std::gmtime(t);
        if (!result)
          boost::throw_exception(std::runtime_error("could not convert calendar time to UTC time"));
        return result;
      }
#if defined(__clang__) // Clang has to be checked before MSVC
#pragma clang diagnostic pop
#elif (defined(_MSC_VER) && (_MSC_VER >= 1400))
#pragma warning(pop) // restore warnings to previous state
#endif

#endif // BOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS
  };
}} // namespaces

#endif // DATE_TIME_C_TIME_HPP___
