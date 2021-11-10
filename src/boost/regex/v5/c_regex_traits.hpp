/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         c_regex_traits.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares regular expression traits class that wraps the global C locale.
  */

#ifndef BOOST_C_REGEX_TRAITS_HPP_INCLUDED
#define BOOST_C_REGEX_TRAITS_HPP_INCLUDED

#include <boost/regex/config.hpp>
#include <boost/regex/v5/regex_workaround.hpp>
#include <cctype>

namespace boost{

   namespace BOOST_REGEX_DETAIL_NS {

      enum
      {
         char_class_space = 1 << 0,
         char_class_print = 1 << 1,
         char_class_cntrl = 1 << 2,
         char_class_upper = 1 << 3,
         char_class_lower = 1 << 4,
         char_class_alpha = 1 << 5,
         char_class_digit = 1 << 6,
         char_class_punct = 1 << 7,
         char_class_xdigit = 1 << 8,
         char_class_alnum = char_class_alpha | char_class_digit,
         char_class_graph = char_class_alnum | char_class_punct,
         char_class_blank = 1 << 9,
         char_class_word = 1 << 10,
         char_class_unicode = 1 << 11,
         char_class_horizontal = 1 << 12,
         char_class_vertical = 1 << 13
      };

   }

template <class charT>
struct c_regex_traits;

template<>
struct c_regex_traits<char>
{
   c_regex_traits(){}
   typedef char char_type;
   typedef std::size_t size_type;
   typedef std::string string_type;
   struct locale_type{};
   typedef std::uint32_t char_class_type;

   static size_type length(const char_type* p) 
   { 
      return (std::strlen)(p); 
   }

   char translate(char c) const 
   { 
      return c; 
   }
   char translate_nocase(char c) const 
   { 
      return static_cast<char>((std::tolower)(static_cast<unsigned char>(c))); 
   }

   static string_type  transform(const char* p1, const char* p2);
   static string_type  transform_primary(const char* p1, const char* p2);

   static char_class_type  lookup_classname(const char* p1, const char* p2);
   static string_type  lookup_collatename(const char* p1, const char* p2);

   static bool  isctype(char, char_class_type);
   static int  value(char, int);

   locale_type imbue(locale_type l)
   { return l; }
   locale_type getloc()const
   { return locale_type(); }

private:
   // this type is not copyable:
   c_regex_traits(const c_regex_traits&);
   c_regex_traits& operator=(const c_regex_traits&);
};

#ifndef BOOST_NO_WREGEX
template<>
struct c_regex_traits<wchar_t>
{
   c_regex_traits(){}
   typedef wchar_t char_type;
   typedef std::size_t size_type;
   typedef std::wstring string_type;
   struct locale_type{};
   typedef std::uint32_t char_class_type;

   static size_type length(const char_type* p) 
   { 
      return (std::wcslen)(p); 
   }

   wchar_t translate(wchar_t c) const 
   { 
      return c; 
   }
   wchar_t translate_nocase(wchar_t c) const 
   { 
      return (std::towlower)(c); 
   }

   static string_type  transform(const wchar_t* p1, const wchar_t* p2);
   static string_type  transform_primary(const wchar_t* p1, const wchar_t* p2);

   static char_class_type  lookup_classname(const wchar_t* p1, const wchar_t* p2);
   static string_type  lookup_collatename(const wchar_t* p1, const wchar_t* p2);

   static bool  isctype(wchar_t, char_class_type);
   static int  value(wchar_t, int);

   locale_type imbue(locale_type l)
   { return l; }
   locale_type getloc()const
   { return locale_type(); }

private:
   // this type is not copyable:
   c_regex_traits(const c_regex_traits&);
   c_regex_traits& operator=(const c_regex_traits&);
};

#endif // BOOST_NO_WREGEX

inline c_regex_traits<char>::string_type  c_regex_traits<char>::transform(const char* p1, const char* p2)
{
   std::string result(10, ' ');
   std::size_t s = result.size();
   std::size_t r;
   std::string src(p1, p2);
   while (s < (r = std::strxfrm(&*result.begin(), src.c_str(), s)))
   {
#if defined(_CPPLIB_VER)
      //
      // A bug in VC11 and 12 causes the program to hang if we pass a null-string
      // to std::strxfrm, but only for certain locales :-(
      // Probably effects Intel and Clang or any compiler using the VC std library (Dinkumware).
      //
      if (r == INT_MAX)
      {
         result.erase();
         result.insert(result.begin(), static_cast<char>(0));
         return result;
      }
#endif
      result.append(r - s + 3, ' ');
      s = result.size();
   }
   result.erase(r);
   return result;
}

inline c_regex_traits<char>::string_type  c_regex_traits<char>::transform_primary(const char* p1, const char* p2)
{
   static char s_delim;
   static const int s_collate_type = ::boost::BOOST_REGEX_DETAIL_NS::find_sort_syntax(static_cast<c_regex_traits<char>*>(0), &s_delim);
   std::string result;
   //
   // What we do here depends upon the format of the sort key returned by
   // sort key returned by this->transform:
   //
   switch (s_collate_type)
   {
   case ::boost::BOOST_REGEX_DETAIL_NS::sort_C:
   case ::boost::BOOST_REGEX_DETAIL_NS::sort_unknown:
      // the best we can do is translate to lower case, then get a regular sort key:
   {
      result.assign(p1, p2);
      for (std::string::size_type i = 0; i < result.size(); ++i)
         result[i] = static_cast<char>((std::tolower)(static_cast<unsigned char>(result[i])));
      result = transform(&*result.begin(), &*result.begin() + result.size());
      break;
   }
   case ::boost::BOOST_REGEX_DETAIL_NS::sort_fixed:
   {
      // get a regular sort key, and then truncate it:
      result = transform(p1, p2);
      result.erase(s_delim);
      break;
   }
   case ::boost::BOOST_REGEX_DETAIL_NS::sort_delim:
      // get a regular sort key, and then truncate everything after the delim:
      result = transform(p1, p2);
      if ((!result.empty()) && (result[0] == s_delim))
         break;
      std::size_t i;
      for (i = 0; i < result.size(); ++i)
      {
         if (result[i] == s_delim)
            break;
      }
      result.erase(i);
      break;
   }
   if (result.empty())
      result = std::string(1, char(0));
   return result;
}

inline c_regex_traits<char>::char_class_type  c_regex_traits<char>::lookup_classname(const char* p1, const char* p2)
{
   using namespace BOOST_REGEX_DETAIL_NS;
   static const char_class_type masks[] =
   {
      0,
      char_class_alnum,
      char_class_alpha,
      char_class_blank,
      char_class_cntrl,
      char_class_digit,
      char_class_digit,
      char_class_graph,
      char_class_horizontal,
      char_class_lower,
      char_class_lower,
      char_class_print,
      char_class_punct,
      char_class_space,
      char_class_space,
      char_class_upper,
      char_class_unicode,
      char_class_upper,
      char_class_vertical,
      char_class_alnum | char_class_word,
      char_class_alnum | char_class_word,
      char_class_xdigit,
   };

   int idx = ::boost::BOOST_REGEX_DETAIL_NS::get_default_class_id(p1, p2);
   if (idx < 0)
   {
      std::string s(p1, p2);
      for (std::string::size_type i = 0; i < s.size(); ++i)
         s[i] = static_cast<char>((std::tolower)(static_cast<unsigned char>(s[i])));
      idx = ::boost::BOOST_REGEX_DETAIL_NS::get_default_class_id(&*s.begin(), &*s.begin() + s.size());
   }
   BOOST_REGEX_ASSERT(std::size_t(idx) + 1u < sizeof(masks) / sizeof(masks[0]));
   return masks[idx + 1];
}

inline bool  c_regex_traits<char>::isctype(char c, char_class_type mask)
{
   using namespace BOOST_REGEX_DETAIL_NS;
   return
      ((mask & char_class_space) && (std::isspace)(static_cast<unsigned char>(c)))
      || ((mask & char_class_print) && (std::isprint)(static_cast<unsigned char>(c)))
      || ((mask & char_class_cntrl) && (std::iscntrl)(static_cast<unsigned char>(c)))
      || ((mask & char_class_upper) && (std::isupper)(static_cast<unsigned char>(c)))
      || ((mask & char_class_lower) && (std::islower)(static_cast<unsigned char>(c)))
      || ((mask & char_class_alpha) && (std::isalpha)(static_cast<unsigned char>(c)))
      || ((mask & char_class_digit) && (std::isdigit)(static_cast<unsigned char>(c)))
      || ((mask & char_class_punct) && (std::ispunct)(static_cast<unsigned char>(c)))
      || ((mask & char_class_xdigit) && (std::isxdigit)(static_cast<unsigned char>(c)))
      || ((mask & char_class_blank) && (std::isspace)(static_cast<unsigned char>(c)) && !::boost::BOOST_REGEX_DETAIL_NS::is_separator(c))
      || ((mask & char_class_word) && (c == '_'))
      || ((mask & char_class_vertical) && (::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) || (c == '\v')))
      || ((mask & char_class_horizontal) && (std::isspace)(static_cast<unsigned char>(c)) && !::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) && (c != '\v'));
}

inline c_regex_traits<char>::string_type  c_regex_traits<char>::lookup_collatename(const char* p1, const char* p2)
{
   std::string s(p1, p2);
   s = ::boost::BOOST_REGEX_DETAIL_NS::lookup_default_collate_name(s);
   if (s.empty() && (p2 - p1 == 1))
      s.append(1, *p1);
   return s;
}

inline int  c_regex_traits<char>::value(char c, int radix)
{
   char b[2] = { c, '\0', };
   char* ep;
   int result = std::strtol(b, &ep, radix);
   if (ep == b)
      return -1;
   return result;
}

#ifndef BOOST_NO_WREGEX

inline c_regex_traits<wchar_t>::string_type  c_regex_traits<wchar_t>::transform(const wchar_t* p1, const wchar_t* p2)
{
   std::size_t r;
   std::size_t s = 10;
   std::wstring src(p1, p2);
   std::wstring result(s, L' ');
   while (s < (r = std::wcsxfrm(&*result.begin(), src.c_str(), s)))
   {
#if defined(_CPPLIB_VER)
      //
      // A bug in VC11 and 12 causes the program to hang if we pass a null-string
      // to std::strxfrm, but only for certain locales :-(
      // Probably effects Intel and Clang or any compiler using the VC std library (Dinkumware).
      //
      if (r == INT_MAX)
      {
         result.erase();
         result.insert(result.begin(), static_cast<wchar_t>(0));
         return result;
      }
#endif
      result.append(r - s + 3, L' ');
      s = result.size();
   }
   result.erase(r);
   return result;
}

inline c_regex_traits<wchar_t>::string_type  c_regex_traits<wchar_t>::transform_primary(const wchar_t* p1, const wchar_t* p2)
{
   static wchar_t s_delim;
   static const int s_collate_type = ::boost::BOOST_REGEX_DETAIL_NS::find_sort_syntax(static_cast<const c_regex_traits<wchar_t>*>(0), &s_delim);
   std::wstring result;
   //
   // What we do here depends upon the format of the sort key returned by
   // sort key returned by this->transform:
   //
   switch (s_collate_type)
   {
   case ::boost::BOOST_REGEX_DETAIL_NS::sort_C:
   case ::boost::BOOST_REGEX_DETAIL_NS::sort_unknown:
      // the best we can do is translate to lower case, then get a regular sort key:
   {
      result.assign(p1, p2);
      for (std::wstring::size_type i = 0; i < result.size(); ++i)
         result[i] = (std::towlower)(result[i]);
      result = c_regex_traits<wchar_t>::transform(&*result.begin(), &*result.begin() + result.size());
      break;
   }
   case ::boost::BOOST_REGEX_DETAIL_NS::sort_fixed:
   {
      // get a regular sort key, and then truncate it:
      result = c_regex_traits<wchar_t>::transform(&*result.begin(), &*result.begin() + result.size());
      result.erase(s_delim);
      break;
   }
   case ::boost::BOOST_REGEX_DETAIL_NS::sort_delim:
      // get a regular sort key, and then truncate everything after the delim:
      result = c_regex_traits<wchar_t>::transform(&*result.begin(), &*result.begin() + result.size());
      if ((!result.empty()) && (result[0] == s_delim))
         break;
      std::size_t i;
      for (i = 0; i < result.size(); ++i)
      {
         if (result[i] == s_delim)
            break;
      }
      result.erase(i);
      break;
   }
   if (result.empty())
      result = std::wstring(1, char(0));
   return result;
}

inline c_regex_traits<wchar_t>::char_class_type  c_regex_traits<wchar_t>::lookup_classname(const wchar_t* p1, const wchar_t* p2)
{
   using namespace BOOST_REGEX_DETAIL_NS;
   static const char_class_type masks[] =
   {
      0,
      char_class_alnum,
      char_class_alpha,
      char_class_blank,
      char_class_cntrl,
      char_class_digit,
      char_class_digit,
      char_class_graph,
      char_class_horizontal,
      char_class_lower,
      char_class_lower,
      char_class_print,
      char_class_punct,
      char_class_space,
      char_class_space,
      char_class_upper,
      char_class_unicode,
      char_class_upper,
      char_class_vertical,
      char_class_alnum | char_class_word,
      char_class_alnum | char_class_word,
      char_class_xdigit,
   };

   int idx = ::boost::BOOST_REGEX_DETAIL_NS::get_default_class_id(p1, p2);
   if (idx < 0)
   {
      std::wstring s(p1, p2);
      for (std::wstring::size_type i = 0; i < s.size(); ++i)
         s[i] = (std::towlower)(s[i]);
      idx = ::boost::BOOST_REGEX_DETAIL_NS::get_default_class_id(&*s.begin(), &*s.begin() + s.size());
   }
   BOOST_REGEX_ASSERT(idx + 1 < static_cast<int>(sizeof(masks) / sizeof(masks[0])));
   return masks[idx + 1];
}

inline bool  c_regex_traits<wchar_t>::isctype(wchar_t c, char_class_type mask)
{
   using namespace BOOST_REGEX_DETAIL_NS;
   return
      ((mask & char_class_space) && (std::iswspace)(c))
      || ((mask & char_class_print) && (std::iswprint)(c))
      || ((mask & char_class_cntrl) && (std::iswcntrl)(c))
      || ((mask & char_class_upper) && (std::iswupper)(c))
      || ((mask & char_class_lower) && (std::iswlower)(c))
      || ((mask & char_class_alpha) && (std::iswalpha)(c))
      || ((mask & char_class_digit) && (std::iswdigit)(c))
      || ((mask & char_class_punct) && (std::iswpunct)(c))
      || ((mask & char_class_xdigit) && (std::iswxdigit)(c))
      || ((mask & char_class_blank) && (std::iswspace)(c) && !::boost::BOOST_REGEX_DETAIL_NS::is_separator(c))
      || ((mask & char_class_word) && (c == '_'))
      || ((mask & char_class_unicode) && (c & ~static_cast<wchar_t>(0xff)))
      || ((mask & char_class_vertical) && (::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) || (c == L'\v')))
      || ((mask & char_class_horizontal) && (std::iswspace)(c) && !::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) && (c != L'\v'));
}

inline c_regex_traits<wchar_t>::string_type  c_regex_traits<wchar_t>::lookup_collatename(const wchar_t* p1, const wchar_t* p2)
{
   std::string name;
   // Usual msvc warning suppression does not work here with std::string template constructor.... use a workaround instead:
   for (const wchar_t* pos = p1; pos != p2; ++pos)
      name.push_back((char)*pos);
   name = ::boost::BOOST_REGEX_DETAIL_NS::lookup_default_collate_name(name);
   if (!name.empty())
      return string_type(name.begin(), name.end());
   if (p2 - p1 == 1)
      return string_type(1, *p1);
   return string_type();
}

inline int  c_regex_traits<wchar_t>::value(wchar_t c, int radix)
{
#ifdef BOOST_BORLANDC
   // workaround for broken wcstol:
   if ((std::iswxdigit)(c) == 0)
      return -1;
#endif
   wchar_t b[2] = { c, '\0', };
   wchar_t* ep;
   int result = std::wcstol(b, &ep, radix);
   if (ep == b)
      return -1;
   return result;
}

#endif

}

#endif



