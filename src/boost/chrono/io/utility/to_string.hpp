//  boost/chrono/utility/to_string.hpp
//
//  Copyright 2011 Vicente J. Botet Escriba
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#ifndef BOOST_CHRONO_UTILITY_TO_STRING_HPP
#define BOOST_CHRONO_UTILITY_TO_STRING_HPP

#include <boost/chrono/config.hpp>
#include <string>
#include <sstream>

namespace boost
{
  namespace chrono
  {
    template <typename CharT, typename T>
    std::basic_string<CharT> to_basic_string(T const&v) {
      std::basic_stringstream<CharT> sstr;
      sstr << v;
      return sstr.str();
    }

    template <typename T>
    std::string to_string(T const&v) {
      return to_basic_string<char>(v);
    }
#ifndef BOOST_NO_STD_WSTRING
    template <typename T>
    std::wstring to_wstring(T const&v) {
      return to_basic_string<wchar_t>(v);
    }
#endif
#if BOOST_CHRONO_HAS_UNICODE_SUPPORT
    template <typename T>
    std::basic_string<char16_t> to_u16string(T const&v) {
      return to_basic_string<char16_t>(v);
    }
    template <typename T>
    std::basic_string<char32_t> to_u32string(T const&v) {
      return to_basic_string<char32_t>(v);
    }
#endif
  } // chrono

} // boost

#endif  // header
