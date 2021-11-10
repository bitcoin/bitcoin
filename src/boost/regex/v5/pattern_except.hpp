/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         pattern_except.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares pattern-matching exception classes.
  */

#ifndef BOOST_RE_V5_PAT_EXCEPT_HPP
#define BOOST_RE_V5_PAT_EXCEPT_HPP

#ifndef BOOST_REGEX_CONFIG_HPP
#include <boost/regex/config.hpp>
#endif

#include <cstddef>
#include <stdexcept>
#include <boost/regex/v5/error_type.hpp>
#include <boost/regex/v5/regex_traits_defaults.hpp>

namespace boost{

#ifdef BOOST_REGEX_MSVC
#pragma warning(push)
#pragma warning(disable : 4275)
#if BOOST_REGEX_MSVC >= 1800
#pragma warning(disable : 26812 4459)
#endif
#endif
class regex_error : public std::runtime_error
{
public:
   explicit regex_error(const std::string& s, regex_constants::error_type err = regex_constants::error_unknown, std::ptrdiff_t pos = 0)
      : std::runtime_error(s)
      , m_error_code(err)
      , m_position(pos)
   {
   }
   explicit regex_error(regex_constants::error_type err)
      : std::runtime_error(::boost::BOOST_REGEX_DETAIL_NS::get_default_error_string(err))
      , m_error_code(err)
      , m_position(0)
   {
   }
   ~regex_error() noexcept override {}
   regex_constants::error_type code()const
   { return m_error_code; }
   std::ptrdiff_t position()const
   { return m_position; }
   void raise()const 
   {
#ifndef BOOST_NO_EXCEPTIONS
#ifndef BOOST_REGEX_STANDALONE
      ::boost::throw_exception(*this);
#else
      throw* this;
#endif
#endif
   }
private:
   regex_constants::error_type m_error_code;
   std::ptrdiff_t m_position;
};

typedef regex_error bad_pattern;
typedef regex_error bad_expression;

namespace BOOST_REGEX_DETAIL_NS{

inline void raise_runtime_error(const std::runtime_error& ex)
{
#ifndef BOOST_REGEX_STANDALONE
   ::boost::throw_exception(ex);
#else
   throw ex;
#endif
}

template <class traits>
void raise_error(const traits& t, regex_constants::error_type code)
{
   (void)t;  // warning suppression
   std::runtime_error e(t.error_string(code));
   ::boost::BOOST_REGEX_DETAIL_NS::raise_runtime_error(e);
}

}

#ifdef BOOST_REGEX_MSVC
#pragma warning(pop)
#endif

} // namespace boost

#endif
