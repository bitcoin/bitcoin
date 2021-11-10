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
  *   FILE         w32_regex_traits.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares regular expression traits class w32_regex_traits.
  */

#ifndef BOOST_W32_REGEX_TRAITS_HPP_INCLUDED
#define BOOST_W32_REGEX_TRAITS_HPP_INCLUDED

#ifndef BOOST_REGEX_NO_WIN32_LOCALE

#ifndef BOOST_RE_PAT_EXCEPT_HPP
#include <boost/regex/pattern_except.hpp>
#endif
#ifndef BOOST_REGEX_TRAITS_DEFAULTS_HPP_INCLUDED
#include <boost/regex/v4/regex_traits_defaults.hpp>
#endif
#ifdef BOOST_HAS_THREADS
#include <boost/regex/pending/static_mutex.hpp>
#endif
#ifndef BOOST_REGEX_PRIMARY_TRANSFORM
#include <boost/regex/v4/primary_transform.hpp>
#endif
#ifndef BOOST_REGEX_OBJECT_CACHE_HPP
#include <boost/regex/v4/object_cache.hpp>
#endif

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if defined(_MSC_VER) && !defined(_WIN32_WCE) && !defined(UNDER_CE)
#pragma comment(lib, "user32.lib")
#endif


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

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4786)
#if BOOST_MSVC < 1910
#pragma warning(disable:4800)
#endif
#endif

namespace boost{ 

//
// forward declaration is needed by some compilers:
//
template <class charT>
class w32_regex_traits;
   
namespace BOOST_REGEX_DETAIL_NS{

//
// start by typedeffing the types we'll need:
//
typedef ::boost::uint32_t lcid_type;   // placeholder for LCID.
typedef ::boost::shared_ptr<void> cat_type; // placeholder for dll HANDLE.

//
// then add wrappers around the actual Win32 API's (ie implementation hiding):
//
lcid_type BOOST_REGEX_CALL w32_get_default_locale();
bool BOOST_REGEX_CALL w32_is_lower(char, lcid_type);
#ifndef BOOST_NO_WREGEX
bool BOOST_REGEX_CALL w32_is_lower(wchar_t, lcid_type);
#endif
bool BOOST_REGEX_CALL w32_is_upper(char, lcid_type);
#ifndef BOOST_NO_WREGEX
bool BOOST_REGEX_CALL w32_is_upper(wchar_t, lcid_type);
#endif
cat_type BOOST_REGEX_CALL w32_cat_open(const std::string& name);
std::string BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type state_id, int i, const std::string& def);
#ifndef BOOST_NO_WREGEX
std::wstring BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type state_id, int i, const std::wstring& def);
#endif
std::string BOOST_REGEX_CALL w32_transform(lcid_type state_id, const char* p1, const char* p2);
#ifndef BOOST_NO_WREGEX
std::wstring BOOST_REGEX_CALL w32_transform(lcid_type state_id, const wchar_t* p1, const wchar_t* p2);
#endif
char BOOST_REGEX_CALL w32_tolower(char c, lcid_type);
#ifndef BOOST_NO_WREGEX
wchar_t BOOST_REGEX_CALL w32_tolower(wchar_t c, lcid_type);
#endif
char BOOST_REGEX_CALL w32_toupper(char c, lcid_type);
#ifndef BOOST_NO_WREGEX
wchar_t BOOST_REGEX_CALL w32_toupper(wchar_t c, lcid_type);
#endif
bool BOOST_REGEX_CALL w32_is(lcid_type, boost::uint32_t mask, char c);
#ifndef BOOST_NO_WREGEX
bool BOOST_REGEX_CALL w32_is(lcid_type, boost::uint32_t mask, wchar_t c);
#endif
//
// class w32_regex_traits_base:
// acts as a container for locale and the facets we are using.
//
template <class charT>
struct w32_regex_traits_base
{
   w32_regex_traits_base(lcid_type l)
   { imbue(l); }
   lcid_type imbue(lcid_type l);

   lcid_type m_locale;
};

template <class charT>
inline lcid_type w32_regex_traits_base<charT>::imbue(lcid_type l)
{
   lcid_type result(m_locale);
   m_locale = l;
   return result;
}

//
// class w32_regex_traits_char_layer:
// implements methods that require specialisation for narrow characters:
//
template <class charT>
class w32_regex_traits_char_layer : public w32_regex_traits_base<charT>
{
   typedef std::basic_string<charT> string_type;
   typedef std::map<charT, regex_constants::syntax_type> map_type;
   typedef typename map_type::const_iterator map_iterator_type;
public:
   w32_regex_traits_char_layer(const lcid_type l);

   regex_constants::syntax_type syntax_type(charT c)const
   {
      map_iterator_type i = m_char_map.find(c);
      return ((i == m_char_map.end()) ? 0 : i->second);
   }
   regex_constants::escape_syntax_type escape_syntax_type(charT c) const
   {
      map_iterator_type i = m_char_map.find(c);
      if(i == m_char_map.end())
      {
         if(::boost::BOOST_REGEX_DETAIL_NS::w32_is_lower(c, this->m_locale)) return regex_constants::escape_type_class;
         if(::boost::BOOST_REGEX_DETAIL_NS::w32_is_upper(c, this->m_locale)) return regex_constants::escape_type_not_class;
         return 0;
      }
      return i->second;
   }
   charT tolower(charT c)const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::w32_tolower(c, this->m_locale);
   }
   bool isctype(boost::uint32_t mask, charT c)const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::w32_is(this->m_locale, mask, c);
   }

private:
   string_type get_default_message(regex_constants::syntax_type);
   // TODO: use a hash table when available!
   map_type m_char_map;
};

template <class charT>
w32_regex_traits_char_layer<charT>::w32_regex_traits_char_layer(::boost::BOOST_REGEX_DETAIL_NS::lcid_type l) 
   : w32_regex_traits_base<charT>(l)
{
   // we need to start by initialising our syntax map so we know which
   // character is used for which purpose:
   cat_type cat;
   std::string cat_name(w32_regex_traits<charT>::get_catalog_name());
   if(cat_name.size())
   {
      cat = ::boost::BOOST_REGEX_DETAIL_NS::w32_cat_open(cat_name);
      if(!cat)
      {
         std::string m("Unable to open message catalog: ");
         std::runtime_error err(m + cat_name);
         boost::BOOST_REGEX_DETAIL_NS::raise_runtime_error(err);
      }
   }
   //
   // if we have a valid catalog then load our messages:
   //
   if(cat)
   {
      for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
      {
         string_type mss = ::boost::BOOST_REGEX_DETAIL_NS::w32_cat_get(cat, this->m_locale, i, get_default_message(i));
         for(typename string_type::size_type j = 0; j < mss.size(); ++j)
         {
            this->m_char_map[mss[j]] = i;
         }
      }
   }
   else
   {
      for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
      {
         const char* ptr = get_default_syntax(i);
         while(ptr && *ptr)
         {
            this->m_char_map[static_cast<charT>(*ptr)] = i;
            ++ptr;
         }
      }
   }
}

template <class charT>
typename w32_regex_traits_char_layer<charT>::string_type 
   w32_regex_traits_char_layer<charT>::get_default_message(regex_constants::syntax_type i)
{
   const char* ptr = get_default_syntax(i);
   string_type result;
   while(ptr && *ptr)
   {
      result.append(1, static_cast<charT>(*ptr));
      ++ptr;
   }
   return result;
}

//
// specialised version for narrow characters:
//
template <>
class w32_regex_traits_char_layer<char> : public w32_regex_traits_base<char>
{
   typedef std::string string_type;
public:
   w32_regex_traits_char_layer(::boost::BOOST_REGEX_DETAIL_NS::lcid_type l)
   : w32_regex_traits_base<char>(l)
   {
      init<char>();
   }

   regex_constants::syntax_type syntax_type(char c)const
   {
      return m_char_map[static_cast<unsigned char>(c)];
   }
   regex_constants::escape_syntax_type escape_syntax_type(char c) const
   {
      return m_char_map[static_cast<unsigned char>(c)];
   }
   char tolower(char c)const
   {
      return m_lower_map[static_cast<unsigned char>(c)];
   }
   bool isctype(boost::uint32_t mask, char c)const
   {
      return m_type_map[static_cast<unsigned char>(c)] & mask;
   }

private:
   regex_constants::syntax_type m_char_map[1u << CHAR_BIT];
   char m_lower_map[1u << CHAR_BIT];
   boost::uint16_t m_type_map[1u << CHAR_BIT];
   template <class U>
   void init();
};

//
// class w32_regex_traits_implementation:
// provides pimpl implementation for w32_regex_traits.
//
template <class charT>
class w32_regex_traits_implementation : public w32_regex_traits_char_layer<charT>
{
public:
   typedef typename w32_regex_traits<charT>::char_class_type char_class_type;
   BOOST_STATIC_CONSTANT(char_class_type, mask_word = 0x0400); // must be C1_DEFINED << 1
   BOOST_STATIC_CONSTANT(char_class_type, mask_unicode = 0x0800); // must be C1_DEFINED << 2
   BOOST_STATIC_CONSTANT(char_class_type, mask_horizontal = 0x1000); // must be C1_DEFINED << 3
   BOOST_STATIC_CONSTANT(char_class_type, mask_vertical = 0x2000); // must be C1_DEFINED << 4
   BOOST_STATIC_CONSTANT(char_class_type, mask_base = 0x3ff);  // all the masks used by the CT_CTYPE1 group

   typedef std::basic_string<charT> string_type;
   typedef charT char_type;
   w32_regex_traits_implementation(::boost::BOOST_REGEX_DETAIL_NS::lcid_type l);
   std::string error_string(regex_constants::error_type n) const
   {
      if(!m_error_strings.empty())
      {
         std::map<int, std::string>::const_iterator p = m_error_strings.find(n);
         return (p == m_error_strings.end()) ? std::string(get_default_error_string(n)) : p->second;
      }
      return get_default_error_string(n);
   }
   char_class_type lookup_classname(const charT* p1, const charT* p2) const
   {
      char_class_type result = lookup_classname_imp(p1, p2);
      if(result == 0)
      {
         typedef typename string_type::size_type size_type;
         string_type temp(p1, p2);
         for(size_type i = 0; i < temp.size(); ++i)
            temp[i] = this->tolower(temp[i]);
         result = lookup_classname_imp(&*temp.begin(), &*temp.begin() + temp.size());
      }
      return result;
   }
   string_type lookup_collatename(const charT* p1, const charT* p2) const;
   string_type transform_primary(const charT* p1, const charT* p2) const;
   string_type transform(const charT* p1, const charT* p2) const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::w32_transform(this->m_locale, p1, p2);
   }
private:
   std::map<int, std::string>     m_error_strings;   // error messages indexed by numberic ID
   std::map<string_type, char_class_type>  m_custom_class_names; // character class names
   std::map<string_type, string_type>      m_custom_collate_names; // collating element names
   unsigned                       m_collate_type;    // the form of the collation string
   charT                          m_collate_delim;   // the collation group delimiter
   //
   // helpers:
   //
   char_class_type lookup_classname_imp(const charT* p1, const charT* p2) const;
};

template <class charT>
typename w32_regex_traits_implementation<charT>::string_type 
   w32_regex_traits_implementation<charT>::transform_primary(const charT* p1, const charT* p2) const
{
   string_type result;
   //
   // What we do here depends upon the format of the sort key returned by
   // sort key returned by this->transform:
   //
   switch(m_collate_type)
   {
   case sort_C:
   case sort_unknown:
      // the best we can do is translate to lower case, then get a regular sort key:
      {
         result.assign(p1, p2);
         typedef typename string_type::size_type size_type;
         for(size_type i = 0; i < result.size(); ++i)
            result[i] = this->tolower(result[i]);
         result = this->transform(&*result.begin(), &*result.begin() + result.size());
         break;
      }
   case sort_fixed:
      {
         // get a regular sort key, and then truncate it:
         result.assign(this->transform(p1, p2));
         result.erase(this->m_collate_delim);
         break;
      }
   case sort_delim:
         // get a regular sort key, and then truncate everything after the delim:
         result.assign(this->transform(p1, p2));
         std::size_t i;
         for(i = 0; i < result.size(); ++i)
         {
            if(result[i] == m_collate_delim)
               break;
         }
         result.erase(i);
         break;
   }
   if(result.empty())
      result = string_type(1, charT(0));
   return result;
}

template <class charT>
typename w32_regex_traits_implementation<charT>::string_type 
   w32_regex_traits_implementation<charT>::lookup_collatename(const charT* p1, const charT* p2) const
{
   typedef typename std::map<string_type, string_type>::const_iterator iter_type;
   if(m_custom_collate_names.size())
   {
      iter_type pos = m_custom_collate_names.find(string_type(p1, p2));
      if(pos != m_custom_collate_names.end())
         return pos->second;
   }
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)\
               && !BOOST_WORKAROUND(BOOST_BORLANDC, <= 0x0551)
   std::string name(p1, p2);
#else
   std::string name;
   const charT* p0 = p1;
   while(p0 != p2)
      name.append(1, char(*p0++));
#endif
   name = lookup_default_collate_name(name);
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)\
               && !BOOST_WORKAROUND(BOOST_BORLANDC, <= 0x0551)
   if(name.size())
      return string_type(name.begin(), name.end());
#else
   if(name.size())
   {
      string_type result;
      typedef std::string::const_iterator iter;
      iter b = name.begin();
      iter e = name.end();
      while(b != e)
         result.append(1, charT(*b++));
      return result;
   }
#endif
   if(p2 - p1 == 1)
      return string_type(1, *p1);
   return string_type();
}

template <class charT>
w32_regex_traits_implementation<charT>::w32_regex_traits_implementation(::boost::BOOST_REGEX_DETAIL_NS::lcid_type l)
: w32_regex_traits_char_layer<charT>(l)
{
   cat_type cat;
   std::string cat_name(w32_regex_traits<charT>::get_catalog_name());
   if(cat_name.size())
   {
      cat = ::boost::BOOST_REGEX_DETAIL_NS::w32_cat_open(cat_name);
      if(!cat)
      {
         std::string m("Unable to open message catalog: ");
         std::runtime_error err(m + cat_name);
         boost::BOOST_REGEX_DETAIL_NS::raise_runtime_error(err);
      }
   }
   //
   // if we have a valid catalog then load our messages:
   //
   if(cat)
   {
      //
      // Error messages:
      //
      for(boost::regex_constants::error_type i = static_cast<boost::regex_constants::error_type>(0); 
         i <= boost::regex_constants::error_unknown; 
         i = static_cast<boost::regex_constants::error_type>(i + 1))
      {
         const char* p = get_default_error_string(i);
         string_type default_message;
         while(*p)
         {
            default_message.append(1, static_cast<charT>(*p));
            ++p;
         }
         string_type s = ::boost::BOOST_REGEX_DETAIL_NS::w32_cat_get(cat, this->m_locale, i+200, default_message);
         std::string result;
         for(std::string::size_type j = 0; j < s.size(); ++j)
         {
            result.append(1, static_cast<char>(s[j]));
         }
         m_error_strings[i] = result;
      }
      //
      // Custom class names:
      //
      static const char_class_type masks[14] = 
      {
         0x0104u, // C1_ALPHA | C1_DIGIT
         0x0100u, // C1_ALPHA
         0x0020u, // C1_CNTRL
         0x0004u, // C1_DIGIT
         (~(0x0020u|0x0008u) & 0x01ffu) | 0x0400u, // not C1_CNTRL or C1_SPACE
         0x0002u, // C1_LOWER
         (~0x0020u & 0x01ffu) | 0x0400, // not C1_CNTRL
         0x0010u, // C1_PUNCT
         0x0008u, // C1_SPACE
         0x0001u, // C1_UPPER
         0x0080u, // C1_XDIGIT
         0x0040u, // C1_BLANK
         w32_regex_traits_implementation<charT>::mask_word,
         w32_regex_traits_implementation<charT>::mask_unicode,
      };
      static const string_type null_string;
      for(unsigned int j = 0; j <= 13; ++j)
      {
         string_type s(::boost::BOOST_REGEX_DETAIL_NS::w32_cat_get(cat, this->m_locale, j+300, null_string));
         if(s.size())
            this->m_custom_class_names[s] = masks[j];
      }
   }
   //
   // get the collation format used by m_pcollate:
   //
   m_collate_type = BOOST_REGEX_DETAIL_NS::find_sort_syntax(this, &m_collate_delim);
}

template <class charT>
typename w32_regex_traits_implementation<charT>::char_class_type 
   w32_regex_traits_implementation<charT>::lookup_classname_imp(const charT* p1, const charT* p2) const
{
   static const char_class_type masks[22] = 
   {
      0,
      0x0104u, // C1_ALPHA | C1_DIGIT
      0x0100u, // C1_ALPHA
      0x0040u, // C1_BLANK
      0x0020u, // C1_CNTRL
      0x0004u, // C1_DIGIT
      0x0004u, // C1_DIGIT
      (~(0x0020u|0x0008u|0x0040) & 0x01ffu) | 0x0400u, // not C1_CNTRL or C1_SPACE or C1_BLANK
      w32_regex_traits_implementation<charT>::mask_horizontal, 
      0x0002u, // C1_LOWER
      0x0002u, // C1_LOWER
      (~0x0020u & 0x01ffu) | 0x0400, // not C1_CNTRL
      0x0010u, // C1_PUNCT
      0x0008u, // C1_SPACE
      0x0008u, // C1_SPACE
      0x0001u, // C1_UPPER
      w32_regex_traits_implementation<charT>::mask_unicode,
      0x0001u, // C1_UPPER
      w32_regex_traits_implementation<charT>::mask_vertical, 
      0x0104u | w32_regex_traits_implementation<charT>::mask_word, 
      0x0104u | w32_regex_traits_implementation<charT>::mask_word, 
      0x0080u, // C1_XDIGIT
   };
   if(m_custom_class_names.size())
   {
      typedef typename std::map<std::basic_string<charT>, char_class_type>::const_iterator map_iter;
      map_iter pos = m_custom_class_names.find(string_type(p1, p2));
      if(pos != m_custom_class_names.end())
         return pos->second;
   }
   std::size_t state_id = 1u + (std::size_t)BOOST_REGEX_DETAIL_NS::get_default_class_id(p1, p2);
   if(state_id < sizeof(masks) / sizeof(masks[0]))
      return masks[state_id];
   return masks[0];
}


template <class charT>
boost::shared_ptr<const w32_regex_traits_implementation<charT> > create_w32_regex_traits(::boost::BOOST_REGEX_DETAIL_NS::lcid_type l)
{
   // TODO: create a cache for previously constructed objects.
   return boost::object_cache< ::boost::BOOST_REGEX_DETAIL_NS::lcid_type, w32_regex_traits_implementation<charT> >::get(l, 5);
}

} // BOOST_REGEX_DETAIL_NS

template <class charT>
class w32_regex_traits
{
public:
   typedef charT                         char_type;
   typedef std::size_t                   size_type;
   typedef std::basic_string<char_type>  string_type;
   typedef ::boost::BOOST_REGEX_DETAIL_NS::lcid_type locale_type;
   typedef boost::uint_least32_t         char_class_type;

   struct boost_extensions_tag{};

   w32_regex_traits()
      : m_pimpl(BOOST_REGEX_DETAIL_NS::create_w32_regex_traits<charT>(::boost::BOOST_REGEX_DETAIL_NS::w32_get_default_locale()))
   { }
   static size_type length(const char_type* p)
   {
      return std::char_traits<charT>::length(p);
   }
   regex_constants::syntax_type syntax_type(charT c)const
   {
      return m_pimpl->syntax_type(c);
   }
   regex_constants::escape_syntax_type escape_syntax_type(charT c) const
   {
      return m_pimpl->escape_syntax_type(c);
   }
   charT translate(charT c) const
   {
      return c;
   }
   charT translate_nocase(charT c) const
   {
      return this->m_pimpl->tolower(c);
   }
   charT translate(charT c, bool icase) const
   {
      return icase ? this->m_pimpl->tolower(c) : c;
   }
   charT tolower(charT c) const
   {
      return this->m_pimpl->tolower(c);
   }
   charT toupper(charT c) const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::w32_toupper(c, this->m_pimpl->m_locale);
   }
   string_type transform(const charT* p1, const charT* p2) const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::w32_transform(this->m_pimpl->m_locale, p1, p2);
   }
   string_type transform_primary(const charT* p1, const charT* p2) const
   {
      return m_pimpl->transform_primary(p1, p2);
   }
   char_class_type lookup_classname(const charT* p1, const charT* p2) const
   {
      return m_pimpl->lookup_classname(p1, p2);
   }
   string_type lookup_collatename(const charT* p1, const charT* p2) const
   {
      return m_pimpl->lookup_collatename(p1, p2);
   }
   bool isctype(charT c, char_class_type f) const
   {
      if((f & BOOST_REGEX_DETAIL_NS::w32_regex_traits_implementation<charT>::mask_base) 
         && (this->m_pimpl->isctype(f & BOOST_REGEX_DETAIL_NS::w32_regex_traits_implementation<charT>::mask_base, c)))
         return true;
      else if((f & BOOST_REGEX_DETAIL_NS::w32_regex_traits_implementation<charT>::mask_unicode) && BOOST_REGEX_DETAIL_NS::is_extended(c))
         return true;
      else if((f & BOOST_REGEX_DETAIL_NS::w32_regex_traits_implementation<charT>::mask_word) && (c == '_'))
         return true;
      else if((f & BOOST_REGEX_DETAIL_NS::w32_regex_traits_implementation<charT>::mask_vertical)
         && (::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) || (c == '\v')))
         return true;
      else if((f & BOOST_REGEX_DETAIL_NS::w32_regex_traits_implementation<charT>::mask_horizontal) 
         && this->isctype(c, 0x0008u) && !this->isctype(c, BOOST_REGEX_DETAIL_NS::w32_regex_traits_implementation<charT>::mask_vertical))
         return true;
      return false;
   }
   boost::intmax_t toi(const charT*& p1, const charT* p2, int radix)const
   {
      return ::boost::BOOST_REGEX_DETAIL_NS::global_toi(p1, p2, radix, *this);
   }
   int value(charT c, int radix)const
   {
      int result = (int)::boost::BOOST_REGEX_DETAIL_NS::global_value(c);
      return result < radix ? result : -1;
   }
   locale_type imbue(locale_type l)
   {
      ::boost::BOOST_REGEX_DETAIL_NS::lcid_type result(getloc());
      m_pimpl = BOOST_REGEX_DETAIL_NS::create_w32_regex_traits<charT>(l);
      return result;
   }
   locale_type getloc()const
   {
      return m_pimpl->m_locale;
   }
   std::string error_string(regex_constants::error_type n) const
   {
      return m_pimpl->error_string(n);
   }

   //
   // extension:
   // set the name of the message catalog in use (defaults to "boost_regex").
   //
   static std::string catalog_name(const std::string& name);
   static std::string get_catalog_name();

private:
   boost::shared_ptr<const BOOST_REGEX_DETAIL_NS::w32_regex_traits_implementation<charT> > m_pimpl;
   //
   // catalog name handler:
   //
   static std::string& get_catalog_name_inst();

#ifdef BOOST_HAS_THREADS
   static static_mutex& get_mutex_inst();
#endif
};

template <class charT>
std::string w32_regex_traits<charT>::catalog_name(const std::string& name)
{
#ifdef BOOST_HAS_THREADS
   static_mutex::scoped_lock lk(get_mutex_inst());
#endif
   std::string result(get_catalog_name_inst());
   get_catalog_name_inst() = name;
   return result;
}

template <class charT>
std::string& w32_regex_traits<charT>::get_catalog_name_inst()
{
   static std::string s_name;
   return s_name;
}

template <class charT>
std::string w32_regex_traits<charT>::get_catalog_name()
{
#ifdef BOOST_HAS_THREADS
   static_mutex::scoped_lock lk(get_mutex_inst());
#endif
   std::string result(get_catalog_name_inst());
   return result;
}

#ifdef BOOST_HAS_THREADS
template <class charT>
static_mutex& w32_regex_traits<charT>::get_mutex_inst()
{
   static static_mutex s_mutex = BOOST_STATIC_MUTEX_INIT;
   return s_mutex;
}
#endif

namespace BOOST_REGEX_DETAIL_NS {

#ifdef BOOST_NO_ANSI_APIS
   inline UINT get_code_page_for_locale_id(lcid_type idx)
   {
      WCHAR code_page_string[7];
      if (::GetLocaleInfoW(idx, LOCALE_IDEFAULTANSICODEPAGE, code_page_string, 7) == 0)
         return 0;

      return static_cast<UINT>(_wtol(code_page_string));
}
#endif

   template <class U>
   inline void w32_regex_traits_char_layer<char>::init()
   {
      // we need to start by initialising our syntax map so we know which
      // character is used for which purpose:
      std::memset(m_char_map, 0, sizeof(m_char_map));
      cat_type cat;
      std::string cat_name(w32_regex_traits<char>::get_catalog_name());
      if (cat_name.size())
      {
         cat = ::boost::BOOST_REGEX_DETAIL_NS::w32_cat_open(cat_name);
         if (!cat)
         {
            std::string m("Unable to open message catalog: ");
            std::runtime_error err(m + cat_name);
            ::boost::BOOST_REGEX_DETAIL_NS::raise_runtime_error(err);
         }
      }
      //
      // if we have a valid catalog then load our messages:
      //
      if (cat)
      {
         for (regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
         {
            string_type mss = ::boost::BOOST_REGEX_DETAIL_NS::w32_cat_get(cat, this->m_locale, i, get_default_syntax(i));
            for (string_type::size_type j = 0; j < mss.size(); ++j)
            {
               m_char_map[static_cast<unsigned char>(mss[j])] = i;
            }
         }
      }
      else
      {
         for (regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
         {
            const char* ptr = get_default_syntax(i);
            while (ptr && *ptr)
            {
               m_char_map[static_cast<unsigned char>(*ptr)] = i;
               ++ptr;
            }
         }
      }
      //
      // finish off by calculating our escape types:
      //
      unsigned char i = 'A';
      do
      {
         if (m_char_map[i] == 0)
         {
            if (::boost::BOOST_REGEX_DETAIL_NS::w32_is(this->m_locale, 0x0002u, (char)i))
               m_char_map[i] = regex_constants::escape_type_class;
            else if (::boost::BOOST_REGEX_DETAIL_NS::w32_is(this->m_locale, 0x0001u, (char)i))
               m_char_map[i] = regex_constants::escape_type_not_class;
         }
      } while (0xFF != i++);

      //
      // fill in lower case map:
      //
      char char_map[1 << CHAR_BIT];
      for (int ii = 0; ii < (1 << CHAR_BIT); ++ii)
         char_map[ii] = static_cast<char>(ii);
#ifndef BOOST_NO_ANSI_APIS
      int r = ::LCMapStringA(this->m_locale, LCMAP_LOWERCASE, char_map, 1 << CHAR_BIT, this->m_lower_map, 1 << CHAR_BIT);
      BOOST_REGEX_ASSERT(r != 0);
#else
      UINT code_page = get_code_page_for_locale_id(this->m_locale);
      BOOST_REGEX_ASSERT(code_page != 0);

      WCHAR wide_char_map[1 << CHAR_BIT];
      int conv_r = ::MultiByteToWideChar(code_page, 0, char_map, 1 << CHAR_BIT, wide_char_map, 1 << CHAR_BIT);
      BOOST_REGEX_ASSERT(conv_r != 0);

      WCHAR wide_lower_map[1 << CHAR_BIT];
      int r = ::LCMapStringW(this->m_locale, LCMAP_LOWERCASE, wide_char_map, 1 << CHAR_BIT, wide_lower_map, 1 << CHAR_BIT);
      BOOST_REGEX_ASSERT(r != 0);

      conv_r = ::WideCharToMultiByte(code_page, 0, wide_lower_map, r, this->m_lower_map, 1 << CHAR_BIT, NULL, NULL);
      BOOST_REGEX_ASSERT(conv_r != 0);
#endif
      if (r < (1 << CHAR_BIT))
      {
         // if we have multibyte characters then not all may have been given
         // a lower case mapping:
         for (int jj = r; jj < (1 << CHAR_BIT); ++jj)
            this->m_lower_map[jj] = static_cast<char>(jj);
      }

#ifndef BOOST_NO_ANSI_APIS
      r = ::GetStringTypeExA(this->m_locale, CT_CTYPE1, char_map, 1 << CHAR_BIT, this->m_type_map);
#else
      r = ::GetStringTypeExW(this->m_locale, CT_CTYPE1, wide_char_map, 1 << CHAR_BIT, this->m_type_map);
#endif
      BOOST_REGEX_ASSERT(0 != r);
   }

   inline lcid_type BOOST_REGEX_CALL w32_get_default_locale()
   {
      return ::GetUserDefaultLCID();
   }

   inline bool BOOST_REGEX_CALL w32_is_lower(char c, lcid_type idx)
   {
#ifndef BOOST_NO_ANSI_APIS
      WORD mask;
      if (::GetStringTypeExA(idx, CT_CTYPE1, &c, 1, &mask) && (mask & C1_LOWER))
         return true;
      return false;
#else
      UINT code_page = get_code_page_for_locale_id(idx);
      if (code_page == 0)
         return false;

      WCHAR wide_c;
      if (::MultiByteToWideChar(code_page, 0, &c, 1, &wide_c, 1) == 0)
         return false;

      WORD mask;
      if (::GetStringTypeExW(idx, CT_CTYPE1, &wide_c, 1, &mask) && (mask & C1_LOWER))
         return true;
      return false;
#endif
   }

   inline bool BOOST_REGEX_CALL w32_is_lower(wchar_t c, lcid_type idx)
   {
      WORD mask;
      if (::GetStringTypeExW(idx, CT_CTYPE1, &c, 1, &mask) && (mask & C1_LOWER))
         return true;
      return false;
   }

   inline bool BOOST_REGEX_CALL w32_is_upper(char c, lcid_type idx)
   {
#ifndef BOOST_NO_ANSI_APIS
      WORD mask;
      if (::GetStringTypeExA(idx, CT_CTYPE1, &c, 1, &mask) && (mask & C1_UPPER))
         return true;
      return false;
#else
      UINT code_page = get_code_page_for_locale_id(idx);
      if (code_page == 0)
         return false;

      WCHAR wide_c;
      if (::MultiByteToWideChar(code_page, 0, &c, 1, &wide_c, 1) == 0)
         return false;

      WORD mask;
      if (::GetStringTypeExW(idx, CT_CTYPE1, &wide_c, 1, &mask) && (mask & C1_UPPER))
         return true;
      return false;
#endif
   }

   inline bool BOOST_REGEX_CALL w32_is_upper(wchar_t c, lcid_type idx)
   {
      WORD mask;
      if (::GetStringTypeExW(idx, CT_CTYPE1, &c, 1, &mask) && (mask & C1_UPPER))
         return true;
      return false;
   }

   inline void free_module(void* mod)
   {
      ::FreeLibrary(static_cast<HMODULE>(mod));
   }

   inline cat_type BOOST_REGEX_CALL w32_cat_open(const std::string& name)
   {
#ifndef BOOST_NO_ANSI_APIS
      cat_type result(::LoadLibraryA(name.c_str()), &free_module);
      return result;
#else
      LPWSTR wide_name = (LPWSTR)_alloca((name.size() + 1) * sizeof(WCHAR));
      if (::MultiByteToWideChar(CP_ACP, 0, name.c_str(), name.size(), wide_name, name.size() + 1) == 0)
         return cat_type();

      cat_type result(::LoadLibraryW(wide_name), &free_module);
      return result;
#endif
   }

   inline std::string BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type, int i, const std::string& def)
   {
#ifndef BOOST_NO_ANSI_APIS
      char buf[256];
      if (0 == ::LoadStringA(
         static_cast<HMODULE>(cat.get()),
         i,
         buf,
         256
      ))
      {
         return def;
      }
#else
      WCHAR wbuf[256];
      int r = ::LoadStringW(
         static_cast<HMODULE>(cat.get()),
         i,
         wbuf,
         256
      );
      if (r == 0)
         return def;


      int buf_size = 1 + ::WideCharToMultiByte(CP_ACP, 0, wbuf, r, NULL, 0, NULL, NULL);
      LPSTR buf = (LPSTR)_alloca(buf_size);
      if (::WideCharToMultiByte(CP_ACP, 0, wbuf, r, buf, buf_size, NULL, NULL) == 0)
         return def; // failed conversion.
#endif
      return std::string(buf);
   }

#ifndef BOOST_NO_WREGEX
   inline std::wstring BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type, int i, const std::wstring& def)
   {
      wchar_t buf[256];
      if (0 == ::LoadStringW(
         static_cast<HMODULE>(cat.get()),
         i,
         buf,
         256
      ))
      {
         return def;
      }
      return std::wstring(buf);
   }
#endif
   inline std::string BOOST_REGEX_CALL w32_transform(lcid_type idx, const char* p1, const char* p2)
   {
#ifndef BOOST_NO_ANSI_APIS
      int bytes = ::LCMapStringA(
         idx,       // locale identifier
         LCMAP_SORTKEY,  // mapping transformation type
         p1,  // source string
         static_cast<int>(p2 - p1),        // number of characters in source string
         0,  // destination buffer
         0        // size of destination buffer
      );
      if (!bytes)
         return std::string(p1, p2);
      std::string result(++bytes, '\0');
      bytes = ::LCMapStringA(
         idx,       // locale identifier
         LCMAP_SORTKEY,  // mapping transformation type
         p1,  // source string
         static_cast<int>(p2 - p1),        // number of characters in source string
         &*result.begin(),  // destination buffer
         bytes        // size of destination buffer
      );
#else
      UINT code_page = get_code_page_for_locale_id(idx);
      if (code_page == 0)
         return std::string(p1, p2);

      int src_len = static_cast<int>(p2 - p1);
      LPWSTR wide_p1 = (LPWSTR)_alloca((src_len + 1) * 2);
      if (::MultiByteToWideChar(code_page, 0, p1, src_len, wide_p1, src_len + 1) == 0)
         return std::string(p1, p2);

      int bytes = ::LCMapStringW(
         idx,       // locale identifier
         LCMAP_SORTKEY,  // mapping transformation type
         wide_p1,  // source string
         src_len,        // number of characters in source string
         0,  // destination buffer
         0        // size of destination buffer
      );
      if (!bytes)
         return std::string(p1, p2);
      std::string result(++bytes, '\0');
      bytes = ::LCMapStringW(
         idx,       // locale identifier
         LCMAP_SORTKEY,  // mapping transformation type
         wide_p1,  // source string
         src_len,        // number of characters in source string
         (LPWSTR) & *result.begin(),  // destination buffer
         bytes        // size of destination buffer
      );
#endif
      if (bytes > static_cast<int>(result.size()))
         return std::string(p1, p2);
      while (result.size() && result[result.size() - 1] == '\0')
      {
         result.erase(result.size() - 1);
      }
      return result;
   }

#ifndef BOOST_NO_WREGEX
   inline std::wstring BOOST_REGEX_CALL w32_transform(lcid_type idx, const wchar_t* p1, const wchar_t* p2)
   {
      int bytes = ::LCMapStringW(
         idx,       // locale identifier
         LCMAP_SORTKEY,  // mapping transformation type
         p1,  // source string
         static_cast<int>(p2 - p1),        // number of characters in source string
         0,  // destination buffer
         0        // size of destination buffer
      );
      if (!bytes)
         return std::wstring(p1, p2);
      std::string result(++bytes, '\0');
      bytes = ::LCMapStringW(
         idx,       // locale identifier
         LCMAP_SORTKEY,  // mapping transformation type
         p1,  // source string
         static_cast<int>(p2 - p1),        // number of characters in source string
         reinterpret_cast<wchar_t*>(&*result.begin()),  // destination buffer *of bytes*
         bytes        // size of destination buffer
      );
      if (bytes > static_cast<int>(result.size()))
         return std::wstring(p1, p2);
      while (result.size() && result[result.size() - 1] == L'\0')
      {
         result.erase(result.size() - 1);
      }
      std::wstring r2;
      for (std::string::size_type i = 0; i < result.size(); ++i)
         r2.append(1, static_cast<wchar_t>(static_cast<unsigned char>(result[i])));
      return r2;
   }
#endif
   inline char BOOST_REGEX_CALL w32_tolower(char c, lcid_type idx)
   {
      char result[2];
#ifndef BOOST_NO_ANSI_APIS
      int b = ::LCMapStringA(
         idx,       // locale identifier
         LCMAP_LOWERCASE,  // mapping transformation type
         &c,  // source string
         1,        // number of characters in source string
         result,  // destination buffer
         1);        // size of destination buffer
      if (b == 0)
         return c;
#else
      UINT code_page = get_code_page_for_locale_id(idx);
      if (code_page == 0)
         return c;

      WCHAR wide_c;
      if (::MultiByteToWideChar(code_page, 0, &c, 1, &wide_c, 1) == 0)
         return c;

      WCHAR wide_result;
      int b = ::LCMapStringW(
         idx,       // locale identifier
         LCMAP_LOWERCASE,  // mapping transformation type
         &wide_c,  // source string
         1,        // number of characters in source string
         &wide_result,  // destination buffer
         1);        // size of destination buffer
      if (b == 0)
         return c;

      if (::WideCharToMultiByte(code_page, 0, &wide_result, 1, result, 2, NULL, NULL) == 0)
         return c;  // No single byte lower case equivalent available
#endif
      return result[0];
   }

#ifndef BOOST_NO_WREGEX
   inline wchar_t BOOST_REGEX_CALL w32_tolower(wchar_t c, lcid_type idx)
   {
      wchar_t result[2];
      int b = ::LCMapStringW(
         idx,       // locale identifier
         LCMAP_LOWERCASE,  // mapping transformation type
         &c,  // source string
         1,        // number of characters in source string
         result,  // destination buffer
         1);        // size of destination buffer
      if (b == 0)
         return c;
      return result[0];
   }
#endif
   inline char BOOST_REGEX_CALL w32_toupper(char c, lcid_type idx)
   {
      char result[2];
#ifndef BOOST_NO_ANSI_APIS
      int b = ::LCMapStringA(
         idx,       // locale identifier
         LCMAP_UPPERCASE,  // mapping transformation type
         &c,  // source string
         1,        // number of characters in source string
         result,  // destination buffer
         1);        // size of destination buffer
      if (b == 0)
         return c;
#else
      UINT code_page = get_code_page_for_locale_id(idx);
      if (code_page == 0)
         return c;

      WCHAR wide_c;
      if (::MultiByteToWideChar(code_page, 0, &c, 1, &wide_c, 1) == 0)
         return c;

      WCHAR wide_result;
      int b = ::LCMapStringW(
         idx,       // locale identifier
         LCMAP_UPPERCASE,  // mapping transformation type
         &wide_c,  // source string
         1,        // number of characters in source string
         &wide_result,  // destination buffer
         1);        // size of destination buffer
      if (b == 0)
         return c;

      if (::WideCharToMultiByte(code_page, 0, &wide_result, 1, result, 2, NULL, NULL) == 0)
         return c;  // No single byte upper case equivalent available.
#endif
      return result[0];
   }

#ifndef BOOST_NO_WREGEX
   inline wchar_t BOOST_REGEX_CALL w32_toupper(wchar_t c, lcid_type idx)
   {
      wchar_t result[2];
      int b = ::LCMapStringW(
         idx,       // locale identifier
         LCMAP_UPPERCASE,  // mapping transformation type
         &c,  // source string
         1,        // number of characters in source string
         result,  // destination buffer
         1);        // size of destination buffer
      if (b == 0)
         return c;
      return result[0];
   }
#endif
   inline bool BOOST_REGEX_CALL w32_is(lcid_type idx, boost::uint32_t m, char c)
   {
      WORD mask;
#ifndef BOOST_NO_ANSI_APIS
      if (::GetStringTypeExA(idx, CT_CTYPE1, &c, 1, &mask) && (mask & m & w32_regex_traits_implementation<char>::mask_base))
         return true;
#else
      UINT code_page = get_code_page_for_locale_id(idx);
      if (code_page == 0)
         return false;

      WCHAR wide_c;
      if (::MultiByteToWideChar(code_page, 0, &c, 1, &wide_c, 1) == 0)
         return false;

      if (::GetStringTypeExW(idx, CT_CTYPE1, &wide_c, 1, &mask) && (mask & m & w32_regex_traits_implementation<char>::mask_base))
         return true;
#endif
      if ((m & w32_regex_traits_implementation<char>::mask_word) && (c == '_'))
         return true;
      return false;
   }

#ifndef BOOST_NO_WREGEX
   inline bool BOOST_REGEX_CALL w32_is(lcid_type idx, boost::uint32_t m, wchar_t c)
   {
      WORD mask;
      if (::GetStringTypeExW(idx, CT_CTYPE1, &c, 1, &mask) && (mask & m & w32_regex_traits_implementation<wchar_t>::mask_base))
         return true;
      if ((m & w32_regex_traits_implementation<wchar_t>::mask_word) && (c == '_'))
         return true;
      if ((m & w32_regex_traits_implementation<wchar_t>::mask_unicode) && (c > 0xff))
         return true;
      return false;
   }
#endif

} // BOOST_REGEX_DETAIL_NS


} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
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

#endif // BOOST_REGEX_NO_WIN32_LOCALE

#endif
