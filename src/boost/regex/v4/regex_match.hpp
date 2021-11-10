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
  *   FILE         regex_match.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Regular expression matching algorithms.
  *                Note this is an internal header file included
  *                by regex.hpp, do not include on its own.
  */


#ifndef BOOST_REGEX_MATCH_HPP
#define BOOST_REGEX_MATCH_HPP

namespace boost{

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

//
// proc regex_match
// returns true if the specified regular expression matches
// the whole of the input.  Fills in what matched in m.
//
template <class BidiIterator, class Allocator, class charT, class traits>
bool regex_match(BidiIterator first, BidiIterator last, 
                 match_results<BidiIterator, Allocator>& m, 
                 const basic_regex<charT, traits>& e, 
                 match_flag_type flags = match_default)
{
   BOOST_REGEX_DETAIL_NS::perl_matcher<BidiIterator, Allocator, traits> matcher(first, last, m, e, flags, first);
   return matcher.match();
}
template <class iterator, class charT, class traits>
bool regex_match(iterator first, iterator last, 
                 const basic_regex<charT, traits>& e, 
                 match_flag_type flags = match_default)
{
   match_results<iterator> m;
   return regex_match(first, last, m, e, flags | regex_constants::match_any);
}
//
// query_match convenience interfaces:
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
//
// this isn't really a partial specialisation, but template function
// overloading - if the compiler doesn't support partial specialisation
// then it really won't support this either:
template <class charT, class Allocator, class traits>
inline bool regex_match(const charT* str, 
                        match_results<const charT*, Allocator>& m, 
                        const basic_regex<charT, traits>& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + traits::length(str), m, e, flags);
}

template <class ST, class SA, class Allocator, class charT, class traits>
inline bool regex_match(const std::basic_string<charT, ST, SA>& s, 
                 match_results<typename std::basic_string<charT, ST, SA>::const_iterator, Allocator>& m, 
                 const basic_regex<charT, traits>& e, 
                 match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
template <class charT, class traits>
inline bool regex_match(const charT* str, 
                        const basic_regex<charT, traits>& e, 
                        match_flag_type flags = match_default)
{
   match_results<const charT*> m;
   return regex_match(str, str + traits::length(str), m, e, flags | regex_constants::match_any);
}

template <class ST, class SA, class charT, class traits>
inline bool regex_match(const std::basic_string<charT, ST, SA>& s, 
                 const basic_regex<charT, traits>& e, 
                 match_flag_type flags = match_default)
{
   typedef typename std::basic_string<charT, ST, SA>::const_iterator iterator;
   match_results<iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#else  // partial ordering
inline bool regex_match(const char* str, 
                        cmatch& m, 
                        const regex& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + regex::traits_type::length(str), m, e, flags);
}
inline bool regex_match(const char* str, 
                        const regex& e, 
                        match_flag_type flags = match_default)
{
   match_results<const char*> m;
   return regex_match(str, str + regex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#ifndef BOOST_NO_STD_LOCALE
inline bool regex_match(const char* str, 
                        cmatch& m, 
                        const basic_regex<char, cpp_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + regex::traits_type::length(str), m, e, flags);
}
inline bool regex_match(const char* str, 
                        const basic_regex<char, cpp_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<const char*> m;
   return regex_match(str, str + regex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#endif
inline bool regex_match(const char* str, 
                        cmatch& m, 
                        const basic_regex<char, c_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + regex::traits_type::length(str), m, e, flags);
}
inline bool regex_match(const char* str, 
                        const basic_regex<char, c_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<const char*> m;
   return regex_match(str, str + regex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#if defined(_WIN32) && !defined(BOOST_REGEX_NO_W32)
inline bool regex_match(const char* str, 
                        cmatch& m, 
                        const basic_regex<char, w32_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + regex::traits_type::length(str), m, e, flags);
}
inline bool regex_match(const char* str, 
                        const basic_regex<char, w32_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<const char*> m;
   return regex_match(str, str + regex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#endif
#ifndef BOOST_NO_WREGEX
inline bool regex_match(const wchar_t* str, 
                        wcmatch& m, 
                        const wregex& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + wregex::traits_type::length(str), m, e, flags);
}
inline bool regex_match(const wchar_t* str, 
                        const wregex& e, 
                        match_flag_type flags = match_default)
{
   match_results<const wchar_t*> m;
   return regex_match(str, str + wregex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#ifndef BOOST_NO_STD_LOCALE
inline bool regex_match(const wchar_t* str, 
                        wcmatch& m, 
                        const basic_regex<wchar_t, cpp_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + wregex::traits_type::length(str), m, e, flags);
}
inline bool regex_match(const wchar_t* str, 
                        const basic_regex<wchar_t, cpp_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<const wchar_t*> m;
   return regex_match(str, str + wregex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#endif
inline bool regex_match(const wchar_t* str, 
                        wcmatch& m, 
                        const basic_regex<wchar_t, c_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + wregex::traits_type::length(str), m, e, flags);
}
inline bool regex_match(const wchar_t* str, 
                        const basic_regex<wchar_t, c_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<const wchar_t*> m;
   return regex_match(str, str + wregex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#if defined(_WIN32) && !defined(BOOST_REGEX_NO_W32)
inline bool regex_match(const wchar_t* str, 
                        wcmatch& m, 
                        const basic_regex<wchar_t, w32_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(str, str + wregex::traits_type::length(str), m, e, flags);
}
inline bool regex_match(const wchar_t* str, 
                        const basic_regex<wchar_t, w32_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<const wchar_t*> m;
   return regex_match(str, str + wregex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#endif
#endif
inline bool regex_match(const std::string& s, 
                        smatch& m,
                        const regex& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
inline bool regex_match(const std::string& s, 
                        const regex& e, 
                        match_flag_type flags = match_default)
{
   match_results<std::string::const_iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#ifndef BOOST_NO_STD_LOCALE
inline bool regex_match(const std::string& s, 
                        smatch& m,
                        const basic_regex<char, cpp_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
inline bool regex_match(const std::string& s, 
                        const basic_regex<char, cpp_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<std::string::const_iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#endif
inline bool regex_match(const std::string& s, 
                        smatch& m,
                        const basic_regex<char, c_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
inline bool regex_match(const std::string& s, 
                        const basic_regex<char, c_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<std::string::const_iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#if defined(_WIN32) && !defined(BOOST_REGEX_NO_W32)
inline bool regex_match(const std::string& s, 
                        smatch& m,
                        const basic_regex<char, w32_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
inline bool regex_match(const std::string& s, 
                        const basic_regex<char, w32_regex_traits<char> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<std::string::const_iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#endif
#if !defined(BOOST_NO_WREGEX)
inline bool regex_match(const std::basic_string<wchar_t>& s, 
                        match_results<std::basic_string<wchar_t>::const_iterator>& m,
                        const wregex& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
inline bool regex_match(const std::basic_string<wchar_t>& s, 
                        const wregex& e, 
                        match_flag_type flags = match_default)
{
   match_results<std::basic_string<wchar_t>::const_iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#ifndef BOOST_NO_STD_LOCALE
inline bool regex_match(const std::basic_string<wchar_t>& s, 
                        match_results<std::basic_string<wchar_t>::const_iterator>& m,
                        const basic_regex<wchar_t, cpp_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
inline bool regex_match(const std::basic_string<wchar_t>& s, 
                        const basic_regex<wchar_t, cpp_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<std::basic_string<wchar_t>::const_iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#endif
inline bool regex_match(const std::basic_string<wchar_t>& s, 
                        match_results<std::basic_string<wchar_t>::const_iterator>& m,
                        const basic_regex<wchar_t, c_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
inline bool regex_match(const std::basic_string<wchar_t>& s, 
                        const basic_regex<wchar_t, c_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<std::basic_string<wchar_t>::const_iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#if defined(_WIN32) && !defined(BOOST_REGEX_NO_W32)
inline bool regex_match(const std::basic_string<wchar_t>& s, 
                        match_results<std::basic_string<wchar_t>::const_iterator>& m,
                        const basic_regex<wchar_t, w32_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   return regex_match(s.begin(), s.end(), m, e, flags);
}
inline bool regex_match(const std::basic_string<wchar_t>& s, 
                        const basic_regex<wchar_t, w32_regex_traits<wchar_t> >& e, 
                        match_flag_type flags = match_default)
{
   match_results<std::basic_string<wchar_t>::const_iterator> m;
   return regex_match(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#endif
#endif

#endif


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

} // namespace boost

#endif   // BOOST_REGEX_MATCH_HPP


















