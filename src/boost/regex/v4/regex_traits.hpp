/*
 *
 * Copyright (c) 2003
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         regex_traits.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares regular expression traits classes.
  */

#ifndef BOOST_REGEX_TRAITS_HPP_INCLUDED
#define BOOST_REGEX_TRAITS_HPP_INCLUDED

#ifndef BOOST_REGEX_CONFIG_HPP
#include <boost/regex/config.hpp>
#endif
#ifndef BOOST_REGEX_WORKAROUND_HPP
#include <boost/regex/v4/regex_workaround.hpp>
#endif
#ifndef BOOST_REGEX_SYNTAX_TYPE_HPP
#include <boost/regex/v4/syntax_type.hpp>
#endif
#ifndef BOOST_REGEX_ERROR_TYPE_HPP
#include <boost/regex/v4/error_type.hpp>
#endif
#ifndef BOOST_REGEX_TRAITS_DEFAULTS_HPP_INCLUDED
#include <boost/regex/v4/regex_traits_defaults.hpp>
#endif
#ifndef BOOST_NO_STD_LOCALE
#  ifndef BOOST_CPP_REGEX_TRAITS_HPP_INCLUDED
#     include <boost/regex/v4/cpp_regex_traits.hpp>
#  endif
#endif
#if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x560)
#  ifndef BOOST_C_REGEX_TRAITS_HPP_INCLUDED
#     include <boost/regex/v4/c_regex_traits.hpp>
#  endif
#endif
#if defined(_WIN32) && !defined(BOOST_REGEX_NO_W32)
#  ifndef BOOST_W32_REGEX_TRAITS_HPP_INCLUDED
#     include <boost/regex/v4/w32_regex_traits.hpp>
#  endif
#endif
#ifndef BOOST_REGEX_FWD_HPP_INCLUDED
#include <boost/regex_fwd.hpp>
#endif

#include "boost/mpl/has_xxx.hpp"
#include <boost/static_assert.hpp>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4103)
#endif
#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

namespace boost{

template <class charT, class implementationT >
struct regex_traits : public implementationT
{
   regex_traits() : implementationT() {}
};

//
// class regex_traits_wrapper.
// this is what our implementation will actually store;
// it provides default implementations of the "optional"
// interfaces that we support, in addition to the
// required "standard" ones:
//
namespace BOOST_REGEX_DETAIL_NS{
#if !BOOST_WORKAROUND(__HP_aCC, < 60000)
BOOST_MPL_HAS_XXX_TRAIT_DEF(boost_extensions_tag)
#else
template<class T>
struct has_boost_extensions_tag
{
   BOOST_STATIC_CONSTANT(bool, value = false);
};
#endif

template <class BaseT>
struct default_wrapper : public BaseT
{
   typedef typename BaseT::char_type char_type;
   std::string error_string(::boost::regex_constants::error_type e)const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::get_default_error_string(e);
   }
   ::boost::regex_constants::syntax_type syntax_type(char_type c)const
   {
      return ((c & 0x7f) == c) ? get_default_syntax_type(static_cast<char>(c)) : ::boost::regex_constants::syntax_char;
   }
   ::boost::regex_constants::escape_syntax_type escape_syntax_type(char_type c)const
   {
      return ((c & 0x7f) == c) ? get_default_escape_syntax_type(static_cast<char>(c)) : ::boost::regex_constants::escape_type_identity;
   }
   boost::intmax_t toi(const char_type*& p1, const char_type* p2, int radix)const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::global_toi(p1, p2, radix, *this);
   }
   char_type translate(char_type c, bool icase)const
   {
      return (icase ? this->translate_nocase(c) : this->translate(c));
   }
   char_type translate(char_type c)const
   {
      return BaseT::translate(c);
   }
   char_type tolower(char_type c)const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::global_lower(c);
   }
   char_type toupper(char_type c)const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::global_upper(c);
   }
};

template <class BaseT, bool has_extensions>
struct compute_wrapper_base
{
   typedef BaseT type;
};
#if !BOOST_WORKAROUND(__HP_aCC, < 60000)
template <class BaseT>
struct compute_wrapper_base<BaseT, false>
{
   typedef default_wrapper<BaseT> type;
};
#else
template <>
struct compute_wrapper_base<c_regex_traits<char>, false>
{
   typedef default_wrapper<c_regex_traits<char> > type;
};
#ifndef BOOST_NO_WREGEX
template <>
struct compute_wrapper_base<c_regex_traits<wchar_t>, false>
{
   typedef default_wrapper<c_regex_traits<wchar_t> > type;
};
#endif
#endif

} // namespace BOOST_REGEX_DETAIL_NS

template <class BaseT>
struct regex_traits_wrapper 
   : public ::boost::BOOST_REGEX_DETAIL_NS::compute_wrapper_base<
               BaseT, 
               ::boost::BOOST_REGEX_DETAIL_NS::has_boost_extensions_tag<BaseT>::value
            >::type
{
   regex_traits_wrapper(){}
private:
   regex_traits_wrapper(const regex_traits_wrapper&);
   regex_traits_wrapper& operator=(const regex_traits_wrapper&);
};

} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4103)
#endif
#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif // include

