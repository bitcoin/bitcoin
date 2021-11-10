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
  *   FILE         regex_search.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Provides regex_search implementation.
  */

#ifndef BOOST_REGEX_V4_REGEX_SEARCH_HPP
#define BOOST_REGEX_V4_REGEX_SEARCH_HPP


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

template <class BidiIterator, class Allocator, class charT, class traits>
bool regex_search(BidiIterator first, BidiIterator last, 
                  match_results<BidiIterator, Allocator>& m, 
                  const basic_regex<charT, traits>& e, 
                  match_flag_type flags = match_default)
{
   return regex_search(first, last, m, e, flags, first);
}

template <class BidiIterator, class Allocator, class charT, class traits>
bool regex_search(BidiIterator first, BidiIterator last, 
                  match_results<BidiIterator, Allocator>& m, 
                  const basic_regex<charT, traits>& e, 
                  match_flag_type flags,
                  BidiIterator base)
{
   if(e.flags() & regex_constants::failbit)
      return false;

   BOOST_REGEX_DETAIL_NS::perl_matcher<BidiIterator, Allocator, traits> matcher(first, last, m, e, flags, base);
   return matcher.find();
}

//
// regex_search convenience interfaces:
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
//
// this isn't really a partial specialisation, but template function
// overloading - if the compiler doesn't support partial specialisation
// then it really won't support this either:
template <class charT, class Allocator, class traits>
inline bool regex_search(const charT* str, 
                        match_results<const charT*, Allocator>& m, 
                        const basic_regex<charT, traits>& e, 
                        match_flag_type flags = match_default)
{
   return regex_search(str, str + traits::length(str), m, e, flags);
}

template <class ST, class SA, class Allocator, class charT, class traits>
inline bool regex_search(const std::basic_string<charT, ST, SA>& s, 
                 match_results<typename std::basic_string<charT, ST, SA>::const_iterator, Allocator>& m, 
                 const basic_regex<charT, traits>& e, 
                 match_flag_type flags = match_default)
{
   return regex_search(s.begin(), s.end(), m, e, flags);
}
#else  // partial overloads:
inline bool regex_search(const char* str, 
                        cmatch& m, 
                        const regex& e, 
                        match_flag_type flags = match_default)
{
   return regex_search(str, str + regex::traits_type::length(str), m, e, flags);
}
inline bool regex_search(const char* first, const char* last, 
                  const regex& e, 
                  match_flag_type flags = match_default)
{
   cmatch m;
   return regex_search(first, last, m, e, flags | regex_constants::match_any);
}

#ifndef BOOST_NO_WREGEX
inline bool regex_search(const wchar_t* str, 
                        wcmatch& m, 
                        const wregex& e, 
                        match_flag_type flags = match_default)
{
   return regex_search(str, str + wregex::traits_type::length(str), m, e, flags);
}
inline bool regex_search(const wchar_t* first, const wchar_t* last, 
                  const wregex& e, 
                  match_flag_type flags = match_default)
{
   wcmatch m;
   return regex_search(first, last, m, e, flags | regex_constants::match_any);
}
#endif
inline bool regex_search(const std::string& s, 
                        smatch& m,
                        const regex& e, 
                        match_flag_type flags = match_default)
{
   return regex_search(s.begin(), s.end(), m, e, flags);
}
#if !defined(BOOST_NO_WREGEX)
inline bool regex_search(const std::basic_string<wchar_t>& s, 
                        wsmatch& m,
                        const wregex& e, 
                        match_flag_type flags = match_default)
{
   return regex_search(s.begin(), s.end(), m, e, flags);
}
#endif

#endif

template <class BidiIterator, class charT, class traits>
bool regex_search(BidiIterator first, BidiIterator last, 
                  const basic_regex<charT, traits>& e, 
                  match_flag_type flags = match_default)
{
   if(e.flags() & regex_constants::failbit)
      return false;

   match_results<BidiIterator> m;
   typedef typename match_results<BidiIterator>::allocator_type match_alloc_type;
   BOOST_REGEX_DETAIL_NS::perl_matcher<BidiIterator, match_alloc_type, traits> matcher(first, last, m, e, flags | regex_constants::match_any, first);
   return matcher.find();
}

#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

template <class charT, class traits>
inline bool regex_search(const charT* str, 
                        const basic_regex<charT, traits>& e, 
                        match_flag_type flags = match_default)
{
   return regex_search(str, str + traits::length(str), e, flags);
}

template <class ST, class SA, class charT, class traits>
inline bool regex_search(const std::basic_string<charT, ST, SA>& s, 
                 const basic_regex<charT, traits>& e, 
                 match_flag_type flags = match_default)
{
   return regex_search(s.begin(), s.end(), e, flags);
}
#else  // non-template function overloads
inline bool regex_search(const char* str, 
                        const regex& e, 
                        match_flag_type flags = match_default)
{
   cmatch m;
   return regex_search(str, str + regex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#ifndef BOOST_NO_WREGEX
inline bool regex_search(const wchar_t* str, 
                        const wregex& e, 
                        match_flag_type flags = match_default)
{
   wcmatch m;
   return regex_search(str, str + wregex::traits_type::length(str), m, e, flags | regex_constants::match_any);
}
#endif
inline bool regex_search(const std::string& s, 
                        const regex& e, 
                        match_flag_type flags = match_default)
{
   smatch m;
   return regex_search(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}
#if !defined(BOOST_NO_WREGEX)
inline bool regex_search(const std::basic_string<wchar_t>& s, 
                        const wregex& e, 
                        match_flag_type flags = match_default)
{
   wsmatch m;
   return regex_search(s.begin(), s.end(), m, e, flags | regex_constants::match_any);
}

#endif // BOOST_NO_WREGEX

#endif // partial overload

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

#endif  // BOOST_REGEX_V4_REGEX_SEARCH_HPP


