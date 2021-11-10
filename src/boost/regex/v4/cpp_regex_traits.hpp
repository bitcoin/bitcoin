/*
 *
 * Copyright (c) 2004 John Maddock
 * Copyright 2011 Garmin Ltd. or its subsidiaries
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         cpp_regex_traits.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares regular expression traits class cpp_regex_traits.
  */

#ifndef BOOST_CPP_REGEX_TRAITS_HPP_INCLUDED
#define BOOST_CPP_REGEX_TRAITS_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/integer.hpp>
#include <boost/type_traits/make_unsigned.hpp>

#ifndef BOOST_NO_STD_LOCALE

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

#include <climits>
#include <ios>
#include <istream>

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
#pragma warning(disable:4786 4251)
#endif

namespace boost{ 

//
// forward declaration is needed by some compilers:
//
template <class charT>
class cpp_regex_traits;
   
namespace BOOST_REGEX_DETAIL_NS{

//
// class parser_buf:
// acts as a stream buffer which wraps around a pair of pointers:
//
template <class charT,
          class traits = ::std::char_traits<charT> >
class parser_buf : public ::std::basic_streambuf<charT, traits>
{
   typedef ::std::basic_streambuf<charT, traits> base_type;
   typedef typename base_type::int_type int_type;
   typedef typename base_type::char_type char_type;
   typedef typename base_type::pos_type pos_type;
   typedef ::std::streamsize streamsize;
   typedef typename base_type::off_type off_type;
public:
   parser_buf() : base_type() { setbuf(0, 0); }
   const charT* getnext() { return this->gptr(); }
protected:
   std::basic_streambuf<charT, traits>* setbuf(char_type* s, streamsize n) BOOST_OVERRIDE;
   typename parser_buf<charT, traits>::pos_type seekpos(pos_type sp, ::std::ios_base::openmode which) BOOST_OVERRIDE;
   typename parser_buf<charT, traits>::pos_type seekoff(off_type off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which) BOOST_OVERRIDE;
private:
   parser_buf& operator=(const parser_buf&);
   parser_buf(const parser_buf&);
};

template<class charT, class traits>
std::basic_streambuf<charT, traits>*
parser_buf<charT, traits>::setbuf(char_type* s, streamsize n)
{
   this->setg(s, s, s + n);
   return this;
}

template<class charT, class traits>
typename parser_buf<charT, traits>::pos_type
parser_buf<charT, traits>::seekoff(off_type off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which)
{
   typedef typename boost::int_t<sizeof(way) * CHAR_BIT>::least cast_type;

   if(which & ::std::ios_base::out)
      return pos_type(off_type(-1));
   std::ptrdiff_t size = this->egptr() - this->eback();
   std::ptrdiff_t pos = this->gptr() - this->eback();
   charT* g = this->eback();
   switch(static_cast<cast_type>(way))
   {
   case ::std::ios_base::beg:
      if((off < 0) || (off > size))
         return pos_type(off_type(-1));
      else
         this->setg(g, g + off, g + size);
      break;
   case ::std::ios_base::end:
      if((off < 0) || (off > size))
         return pos_type(off_type(-1));
      else
         this->setg(g, g + size - off, g + size);
      break;
   case ::std::ios_base::cur:
   {
      std::ptrdiff_t newpos = static_cast<std::ptrdiff_t>(pos + off);
      if((newpos < 0) || (newpos > size))
         return pos_type(off_type(-1));
      else
         this->setg(g, g + newpos, g + size);
      break;
   }
   default: ;
   }
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4244)
#endif
   return static_cast<pos_type>(this->gptr() - this->eback());
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template<class charT, class traits>
typename parser_buf<charT, traits>::pos_type
parser_buf<charT, traits>::seekpos(pos_type sp, ::std::ios_base::openmode which)
{
   if(which & ::std::ios_base::out)
      return pos_type(off_type(-1));
   off_type size = static_cast<off_type>(this->egptr() - this->eback());
   charT* g = this->eback();
   if(off_type(sp) <= size)
   {
      this->setg(g, g + off_type(sp), g + size);
   }
   return pos_type(off_type(-1));
}

//
// class cpp_regex_traits_base:
// acts as a container for locale and the facets we are using.
//
template <class charT>
struct cpp_regex_traits_base
{
   cpp_regex_traits_base(const std::locale& l)
   { (void)imbue(l); }
   std::locale imbue(const std::locale& l);

   std::locale m_locale;
   std::ctype<charT> const* m_pctype;
#ifndef BOOST_NO_STD_MESSAGES
   std::messages<charT> const* m_pmessages;
#endif
   std::collate<charT> const* m_pcollate;

   bool operator<(const cpp_regex_traits_base& b)const
   {
      if(m_pctype == b.m_pctype)
      {
#ifndef BOOST_NO_STD_MESSAGES
         if(m_pmessages == b.m_pmessages)
         {
            return m_pcollate < b.m_pcollate;
         }
         return m_pmessages < b.m_pmessages;
#else
         return m_pcollate < b.m_pcollate;
#endif
      }
      return m_pctype < b.m_pctype;
   }
   bool operator==(const cpp_regex_traits_base& b)const
   {
      return (m_pctype == b.m_pctype) 
#ifndef BOOST_NO_STD_MESSAGES
         && (m_pmessages == b.m_pmessages) 
#endif
         && (m_pcollate == b.m_pcollate);
   }
};

template <class charT>
std::locale cpp_regex_traits_base<charT>::imbue(const std::locale& l)
{
   std::locale result(m_locale);
   m_locale = l;
   m_pctype = &BOOST_USE_FACET(std::ctype<charT>, l);
#ifndef BOOST_NO_STD_MESSAGES
   m_pmessages = BOOST_HAS_FACET(std::messages<charT>, l) ? &BOOST_USE_FACET(std::messages<charT>, l) : 0;
#endif
   m_pcollate = &BOOST_USE_FACET(std::collate<charT>, l);
   return result;
}

//
// class cpp_regex_traits_char_layer:
// implements methods that require specialization for narrow characters:
//
template <class charT>
class cpp_regex_traits_char_layer : public cpp_regex_traits_base<charT>
{
   typedef std::basic_string<charT> string_type;
   typedef std::map<charT, regex_constants::syntax_type> map_type;
   typedef typename map_type::const_iterator map_iterator_type;
public:
   cpp_regex_traits_char_layer(const std::locale& l)
      : cpp_regex_traits_base<charT>(l)
   {
      init();
   }
   cpp_regex_traits_char_layer(const cpp_regex_traits_base<charT>& b)
      : cpp_regex_traits_base<charT>(b)
   {
      init();
   }
   void init();

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
         if(this->m_pctype->is(std::ctype_base::lower, c)) return regex_constants::escape_type_class;
         if(this->m_pctype->is(std::ctype_base::upper, c)) return regex_constants::escape_type_not_class;
         return 0;
      }
      return i->second;
   }

private:
   string_type get_default_message(regex_constants::syntax_type);
   // TODO: use a hash table when available!
   map_type m_char_map;
};

template <class charT>
void cpp_regex_traits_char_layer<charT>::init()
{
   // we need to start by initialising our syntax map so we know which
   // character is used for which purpose:
#ifndef BOOST_NO_STD_MESSAGES
#ifndef __IBMCPP__
   typename std::messages<charT>::catalog cat = static_cast<std::messages<char>::catalog>(-1);
#else
   typename std::messages<charT>::catalog cat = reinterpret_cast<std::messages<char>::catalog>(-1);
#endif
   std::string cat_name(cpp_regex_traits<charT>::get_catalog_name());
   if((!cat_name.empty()) && (this->m_pmessages != 0))
   {
      cat = this->m_pmessages->open(
         cat_name, 
         this->m_locale);
      if((int)cat < 0)
      {
         std::string m("Unable to open message catalog: ");
         std::runtime_error err(m + cat_name);
         boost::BOOST_REGEX_DETAIL_NS::raise_runtime_error(err);
      }
   }
   //
   // if we have a valid catalog then load our messages:
   //
   if((int)cat >= 0)
   {
#ifndef BOOST_NO_EXCEPTIONS
      try{
#endif
         for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
         {
            string_type mss = this->m_pmessages->get(cat, 0, i, get_default_message(i));
            for(typename string_type::size_type j = 0; j < mss.size(); ++j)
            {
               m_char_map[mss[j]] = i;
            }
         }
         this->m_pmessages->close(cat);
#ifndef BOOST_NO_EXCEPTIONS
      }
      catch(...)
      {
         if(this->m_pmessages)
            this->m_pmessages->close(cat);
         throw;
      }
#endif
   }
   else
   {
#endif
      for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
      {
         const char* ptr = get_default_syntax(i);
         while(ptr && *ptr)
         {
            m_char_map[this->m_pctype->widen(*ptr)] = i;
            ++ptr;
         }
      }
#ifndef BOOST_NO_STD_MESSAGES
   }
#endif
}

template <class charT>
typename cpp_regex_traits_char_layer<charT>::string_type 
   cpp_regex_traits_char_layer<charT>::get_default_message(regex_constants::syntax_type i)
{
   const char* ptr = get_default_syntax(i);
   string_type result;
   while(ptr && *ptr)
   {
      result.append(1, this->m_pctype->widen(*ptr));
      ++ptr;
   }
   return result;
}

//
// specialized version for narrow characters:
//
template <>
class cpp_regex_traits_char_layer<char> : public cpp_regex_traits_base<char>
{
   typedef std::string string_type;
public:
   cpp_regex_traits_char_layer(const std::locale& l)
   : cpp_regex_traits_base<char>(l)
   {
      init();
   }
   cpp_regex_traits_char_layer(const cpp_regex_traits_base<char>& l)
   : cpp_regex_traits_base<char>(l)
   {
      init();
   }

   regex_constants::syntax_type syntax_type(char c)const
   {
      return m_char_map[static_cast<unsigned char>(c)];
   }
   regex_constants::escape_syntax_type escape_syntax_type(char c) const
   {
      return m_char_map[static_cast<unsigned char>(c)];
   }

private:
   regex_constants::syntax_type m_char_map[1u << CHAR_BIT];
   void init();
};

#ifdef BOOST_REGEX_BUGGY_CTYPE_FACET
enum
{
   char_class_space=1<<0, 
   char_class_print=1<<1, 
   char_class_cntrl=1<<2, 
   char_class_upper=1<<3, 
   char_class_lower=1<<4,
   char_class_alpha=1<<5, 
   char_class_digit=1<<6, 
   char_class_punct=1<<7, 
   char_class_xdigit=1<<8,
   char_class_alnum=char_class_alpha|char_class_digit, 
   char_class_graph=char_class_alnum|char_class_punct,
   char_class_blank=1<<9,
   char_class_word=1<<10,
   char_class_unicode=1<<11,
   char_class_horizontal_space=1<<12,
   char_class_vertical_space=1<<13
};

#endif

//
// class cpp_regex_traits_implementation:
// provides pimpl implementation for cpp_regex_traits.
//
template <class charT>
class cpp_regex_traits_implementation : public cpp_regex_traits_char_layer<charT>
{
public:
   typedef typename cpp_regex_traits<charT>::char_class_type      char_class_type;
   typedef typename std::ctype<charT>::mask                       native_mask_type;
   typedef typename boost::make_unsigned<native_mask_type>::type  unsigned_native_mask_type;
#ifndef BOOST_REGEX_BUGGY_CTYPE_FACET
   BOOST_STATIC_CONSTANT(char_class_type, mask_blank = 1u << 24);
   BOOST_STATIC_CONSTANT(char_class_type, mask_word = 1u << 25);
   BOOST_STATIC_CONSTANT(char_class_type, mask_unicode = 1u << 26);
   BOOST_STATIC_CONSTANT(char_class_type, mask_horizontal = 1u << 27);
   BOOST_STATIC_CONSTANT(char_class_type, mask_vertical = 1u << 28);
#endif

   typedef std::basic_string<charT> string_type;
   typedef charT char_type;
   //cpp_regex_traits_implementation();
   cpp_regex_traits_implementation(const std::locale& l)
      : cpp_regex_traits_char_layer<charT>(l)
   {
      init();
   }
   cpp_regex_traits_implementation(const cpp_regex_traits_base<charT>& l)
      : cpp_regex_traits_char_layer<charT>(l)
   {
      init();
   }
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
         string_type temp(p1, p2);
         this->m_pctype->tolower(&*temp.begin(), &*temp.begin() + temp.size());
         result = lookup_classname_imp(&*temp.begin(), &*temp.begin() + temp.size());
      }
      return result;
   }
   string_type lookup_collatename(const charT* p1, const charT* p2) const;
   string_type transform_primary(const charT* p1, const charT* p2) const;
   string_type transform(const charT* p1, const charT* p2) const;
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
   void init();
#ifdef BOOST_REGEX_BUGGY_CTYPE_FACET
public:
   bool isctype(charT c, char_class_type m)const;
#endif
};

#ifndef BOOST_REGEX_BUGGY_CTYPE_FACET
#if !defined(BOOST_NO_INCLASS_MEMBER_INITIALIZATION)

template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type const cpp_regex_traits_implementation<charT>::mask_blank;
template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type const cpp_regex_traits_implementation<charT>::mask_word;
template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type const cpp_regex_traits_implementation<charT>::mask_unicode;
template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type const cpp_regex_traits_implementation<charT>::mask_vertical;
template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type const cpp_regex_traits_implementation<charT>::mask_horizontal;

#endif
#endif

template <class charT>
typename cpp_regex_traits_implementation<charT>::string_type 
   cpp_regex_traits_implementation<charT>::transform_primary(const charT* p1, const charT* p2) const
{
   //
   // PRECONDITIONS:
   //
   // A bug in gcc 3.2 (and maybe other versions as well) treats
   // p1 as a null terminated string, for efficiency reasons 
   // we work around this elsewhere, but just assert here that
   // we adhere to gcc's (buggy) preconditions...
   //
   BOOST_REGEX_ASSERT(*p2 == 0);
   string_type result;
#if defined(_CPPLIB_VER)
   //
   // A bug in VC11 and 12 causes the program to hang if we pass a null-string
   // to std::collate::transform, but only for certain locales :-(
   // Probably effects Intel and Clang or any compiler using the VC std library (Dinkumware).
   //
   if(*p1 == 0)
   {
      return string_type(1, charT(0));
   }
#endif
   //
   // swallowing all exceptions here is a bad idea
   // however at least one std lib will always throw
   // std::bad_alloc for certain arguments...
   //
#ifndef BOOST_NO_EXCEPTIONS
   try{
#endif
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
            this->m_pctype->tolower(&*result.begin(), &*result.begin() + result.size());
            result = this->m_pcollate->transform(&*result.begin(), &*result.begin() + result.size());
            break;
         }
      case sort_fixed:
         {
            // get a regular sort key, and then truncate it:
            result.assign(this->m_pcollate->transform(p1, p2));
            result.erase(this->m_collate_delim);
            break;
         }
      case sort_delim:
            // get a regular sort key, and then truncate everything after the delim:
            result.assign(this->m_pcollate->transform(p1, p2));
            std::size_t i;
            for(i = 0; i < result.size(); ++i)
            {
               if(result[i] == m_collate_delim)
                  break;
            }
            result.erase(i);
            break;
      }
#ifndef BOOST_NO_EXCEPTIONS
   }catch(...){}
#endif
   while((!result.empty()) && (charT(0) == *result.rbegin()))
      result.erase(result.size() - 1);
   if(result.empty())
   {
      // character is ignorable at the primary level:
      result = string_type(1, charT(0));
   }
   return result;
}

template <class charT>
typename cpp_regex_traits_implementation<charT>::string_type 
   cpp_regex_traits_implementation<charT>::transform(const charT* p1, const charT* p2) const
{
   //
   // PRECONDITIONS:
   //
   // A bug in gcc 3.2 (and maybe other versions as well) treats
   // p1 as a null terminated string, for efficiency reasons 
   // we work around this elsewhere, but just assert here that
   // we adhere to gcc's (buggy) preconditions...
   //
   BOOST_REGEX_ASSERT(*p2 == 0);
   //
   // swallowing all exceptions here is a bad idea
   // however at least one std lib will always throw
   // std::bad_alloc for certain arguments...
   //
   string_type result, result2;
#if defined(_CPPLIB_VER)
   //
   // A bug in VC11 and 12 causes the program to hang if we pass a null-string
   // to std::collate::transform, but only for certain locales :-(
   // Probably effects Intel and Clang or any compiler using the VC std library (Dinkumware).
   //
   if(*p1 == 0)
   {
      return result;
   }
#endif
#ifndef BOOST_NO_EXCEPTIONS
   try{
#endif
      result = this->m_pcollate->transform(p1, p2);
      //
      // Borland's STLPort version returns a NULL-terminated
      // string that has garbage at the end - each call to
      // std::collate<wchar_t>::transform returns a different string!
      // So as a workaround, we'll truncate the string at the first NULL
      // which _seems_ to work....
#if BOOST_WORKAROUND(BOOST_BORLANDC, < 0x580)
      result.erase(result.find(charT(0)));
#else
      //
      // some implementations (Dinkumware) append unnecessary trailing \0's:
      while((!result.empty()) && (charT(0) == *result.rbegin()))
         result.erase(result.size() - 1);
#endif
      //
      // We may have NULL's used as separators between sections of the collate string,
      // an example would be Boost.Locale.  We have no way to detect this case via
      // #defines since this can be used with any compiler/platform combination.
      // Unfortunately our state machine (which was devised when all implementations
      // used underlying C language API's) can't cope with that case.  One workaround
      // is to replace each character with 2, fortunately this code isn't used that
      // much as this is now slower than before :-(
      //
      typedef typename make_unsigned<charT>::type uchar_type;
      result2.reserve(result.size() * 2 + 2);
      for(unsigned i = 0; i < result.size(); ++i)
      {
         if(static_cast<uchar_type>(result[i]) == (std::numeric_limits<uchar_type>::max)())
         {
            result2.append(1, charT((std::numeric_limits<uchar_type>::max)())).append(1, charT('b'));
         }
         else
         {
            result2.append(1, static_cast<charT>(1 + static_cast<uchar_type>(result[i]))).append(1, charT('b') - 1);
         }
      }
      BOOST_REGEX_ASSERT(std::find(result2.begin(), result2.end(), charT(0)) == result2.end());
#ifndef BOOST_NO_EXCEPTIONS
   }
   catch(...)
   {
   }
#endif
   return result2;
}


template <class charT>
typename cpp_regex_traits_implementation<charT>::string_type 
   cpp_regex_traits_implementation<charT>::lookup_collatename(const charT* p1, const charT* p2) const
{
   typedef typename std::map<string_type, string_type>::const_iterator iter_type;
   if(!m_custom_collate_names.empty())
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
   if(!name.empty())
      return string_type(name.begin(), name.end());
#else
   if(!name.empty())
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
void cpp_regex_traits_implementation<charT>::init()
{
#ifndef BOOST_NO_STD_MESSAGES
#ifndef __IBMCPP__
   typename std::messages<charT>::catalog cat = static_cast<std::messages<char>::catalog>(-1);
#else
   typename std::messages<charT>::catalog cat = reinterpret_cast<std::messages<char>::catalog>(-1);
#endif
   std::string cat_name(cpp_regex_traits<charT>::get_catalog_name());
   if((!cat_name.empty()) && (this->m_pmessages != 0))
   {
      cat = this->m_pmessages->open(
         cat_name, 
         this->m_locale);
      if((int)cat < 0)
      {
         std::string m("Unable to open message catalog: ");
         std::runtime_error err(m + cat_name);
         boost::BOOST_REGEX_DETAIL_NS::raise_runtime_error(err);
      }
   }
   //
   // if we have a valid catalog then load our messages:
   //
   if((int)cat >= 0)
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
            default_message.append(1, this->m_pctype->widen(*p));
            ++p;
         }
         string_type s = this->m_pmessages->get(cat, 0, i+200, default_message);
         std::string result;
         for(std::string::size_type j = 0; j < s.size(); ++j)
         {
            result.append(1, this->m_pctype->narrow(s[j], 0));
         }
         m_error_strings[i] = result;
      }
      //
      // Custom class names:
      //
#ifndef BOOST_REGEX_BUGGY_CTYPE_FACET
      static const char_class_type masks[16] = 
      {
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::alnum),
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::alpha),
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::cntrl),
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::digit),
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::graph),
         cpp_regex_traits_implementation<charT>::mask_horizontal,
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::lower),
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::print),
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::punct),
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::space),
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::upper),
         cpp_regex_traits_implementation<charT>::mask_vertical,
         static_cast<unsigned_native_mask_type>(std::ctype<charT>::xdigit),
         cpp_regex_traits_implementation<charT>::mask_blank,
         cpp_regex_traits_implementation<charT>::mask_word,
         cpp_regex_traits_implementation<charT>::mask_unicode,
      };
#else
      static const char_class_type masks[16] = 
      {
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_alnum,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_alpha,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_cntrl,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_digit,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_graph,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_horizontal_space,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_lower,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_print,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_punct,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_space,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_upper,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_vertical_space,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_xdigit,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_blank,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_word,
         ::boost::BOOST_REGEX_DETAIL_NS::char_class_unicode,
      };
#endif
      static const string_type null_string;
      for(unsigned int j = 0; j <= 13; ++j)
      {
         string_type s(this->m_pmessages->get(cat, 0, j+300, null_string));
         if(!s.empty())
            this->m_custom_class_names[s] = masks[j];
      }
   }
#endif
   //
   // get the collation format used by m_pcollate:
   //
   m_collate_type = BOOST_REGEX_DETAIL_NS::find_sort_syntax(this, &m_collate_delim);
}

template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type 
   cpp_regex_traits_implementation<charT>::lookup_classname_imp(const charT* p1, const charT* p2) const
{
#ifndef BOOST_REGEX_BUGGY_CTYPE_FACET
   static const char_class_type masks[22] = 
   {
      0,
      static_cast<unsigned_native_mask_type>(std::ctype<char>::alnum),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::alpha),
      cpp_regex_traits_implementation<charT>::mask_blank,
      static_cast<unsigned_native_mask_type>(std::ctype<char>::cntrl),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::digit),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::digit),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::graph),
      cpp_regex_traits_implementation<charT>::mask_horizontal,
      static_cast<unsigned_native_mask_type>(std::ctype<char>::lower),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::lower),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::print),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::punct),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::space),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::space),
      static_cast<unsigned_native_mask_type>(std::ctype<char>::upper),
      cpp_regex_traits_implementation<charT>::mask_unicode,
      static_cast<unsigned_native_mask_type>(std::ctype<char>::upper),
      cpp_regex_traits_implementation<charT>::mask_vertical,
      static_cast<unsigned_native_mask_type>(std::ctype<char>::alnum) | cpp_regex_traits_implementation<charT>::mask_word, 
      static_cast<unsigned_native_mask_type>(std::ctype<char>::alnum) | cpp_regex_traits_implementation<charT>::mask_word, 
      static_cast<unsigned_native_mask_type>(std::ctype<char>::xdigit),
   };
#else
   static const char_class_type masks[22] = 
   {
      0,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_alnum, 
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_alpha,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_blank,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_cntrl,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_digit,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_digit,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_graph,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_horizontal_space,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_lower,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_lower,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_print,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_punct,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_space,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_space,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_upper,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_unicode,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_upper,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_vertical_space,
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_alnum | ::boost::BOOST_REGEX_DETAIL_NS::char_class_word, 
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_alnum | ::boost::BOOST_REGEX_DETAIL_NS::char_class_word, 
      ::boost::BOOST_REGEX_DETAIL_NS::char_class_xdigit,
   };
#endif
   if(!m_custom_class_names.empty())
   {
      typedef typename std::map<std::basic_string<charT>, char_class_type>::const_iterator map_iter;
      map_iter pos = m_custom_class_names.find(string_type(p1, p2));
      if(pos != m_custom_class_names.end())
         return pos->second;
   }
   std::size_t state_id = 1 + BOOST_REGEX_DETAIL_NS::get_default_class_id(p1, p2);
   BOOST_REGEX_ASSERT(state_id < sizeof(masks) / sizeof(masks[0]));
   return masks[state_id];
}

#ifdef BOOST_REGEX_BUGGY_CTYPE_FACET
template <class charT>
bool cpp_regex_traits_implementation<charT>::isctype(const charT c, char_class_type mask) const
{
   return
      ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_space) && (this->m_pctype->is(std::ctype<charT>::space, c)))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_print) && (this->m_pctype->is(std::ctype<charT>::print, c)))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_cntrl) && (this->m_pctype->is(std::ctype<charT>::cntrl, c)))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_upper) && (this->m_pctype->is(std::ctype<charT>::upper, c)))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_lower) && (this->m_pctype->is(std::ctype<charT>::lower, c)))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_alpha) && (this->m_pctype->is(std::ctype<charT>::alpha, c)))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_digit) && (this->m_pctype->is(std::ctype<charT>::digit, c)))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_punct) && (this->m_pctype->is(std::ctype<charT>::punct, c)))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_xdigit) && (this->m_pctype->is(std::ctype<charT>::xdigit, c)))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_blank) && (this->m_pctype->is(std::ctype<charT>::space, c)) && !::boost::BOOST_REGEX_DETAIL_NS::is_separator(c))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_word) && (c == '_'))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_unicode) && ::boost::BOOST_REGEX_DETAIL_NS::is_extended(c))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_vertical_space) && (is_separator(c) || (c == '\v')))
      || ((mask & ::boost::BOOST_REGEX_DETAIL_NS::char_class_horizontal_space) && this->m_pctype->is(std::ctype<charT>::space, c) && !(is_separator(c) || (c == '\v')));
}
#endif


template <class charT>
inline boost::shared_ptr<const cpp_regex_traits_implementation<charT> > create_cpp_regex_traits(const std::locale& l)
{
   cpp_regex_traits_base<charT> key(l);
   return ::boost::object_cache<cpp_regex_traits_base<charT>, cpp_regex_traits_implementation<charT> >::get(key, 5);
}

} // BOOST_REGEX_DETAIL_NS

template <class charT>
class cpp_regex_traits
{
private:
   typedef std::ctype<charT>            ctype_type;
public:
   typedef charT                        char_type;
   typedef std::size_t                  size_type;
   typedef std::basic_string<char_type> string_type;
   typedef std::locale                  locale_type;
   typedef boost::uint_least32_t        char_class_type;

   struct boost_extensions_tag{};

   cpp_regex_traits()
      : m_pimpl(BOOST_REGEX_DETAIL_NS::create_cpp_regex_traits<charT>(std::locale()))
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
      return m_pimpl->m_pctype->tolower(c);
   }
   charT translate(charT c, bool icase) const
   {
      return icase ? m_pimpl->m_pctype->tolower(c) : c;
   }
   charT tolower(charT c) const
   {
      return m_pimpl->m_pctype->tolower(c);
   }
   charT toupper(charT c) const
   {
      return m_pimpl->m_pctype->toupper(c);
   }
   string_type transform(const charT* p1, const charT* p2) const
   {
      return m_pimpl->transform(p1, p2);
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
#ifndef BOOST_REGEX_BUGGY_CTYPE_FACET
      typedef typename std::ctype<charT>::mask ctype_mask;

      static const ctype_mask mask_base = 
         static_cast<ctype_mask>(
            std::ctype<charT>::alnum 
            | std::ctype<charT>::alpha
            | std::ctype<charT>::cntrl
            | std::ctype<charT>::digit
            | std::ctype<charT>::graph
            | std::ctype<charT>::lower
            | std::ctype<charT>::print
            | std::ctype<charT>::punct
            | std::ctype<charT>::space
            | std::ctype<charT>::upper
            | std::ctype<charT>::xdigit);

      if((f & mask_base) 
         && (m_pimpl->m_pctype->is(
            static_cast<ctype_mask>(f & mask_base), c)))
         return true;
      else if((f & BOOST_REGEX_DETAIL_NS::cpp_regex_traits_implementation<charT>::mask_unicode) && BOOST_REGEX_DETAIL_NS::is_extended(c))
         return true;
      else if((f & BOOST_REGEX_DETAIL_NS::cpp_regex_traits_implementation<charT>::mask_word) && (c == '_'))
         return true;
      else if((f & BOOST_REGEX_DETAIL_NS::cpp_regex_traits_implementation<charT>::mask_blank) 
         && m_pimpl->m_pctype->is(std::ctype<charT>::space, c)
         && !BOOST_REGEX_DETAIL_NS::is_separator(c))
         return true;
      else if((f & BOOST_REGEX_DETAIL_NS::cpp_regex_traits_implementation<charT>::mask_vertical) 
         && (::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) || (c == '\v')))
         return true;
      else if((f & BOOST_REGEX_DETAIL_NS::cpp_regex_traits_implementation<charT>::mask_horizontal) 
         && this->isctype(c, std::ctype<charT>::space) && !this->isctype(c, BOOST_REGEX_DETAIL_NS::cpp_regex_traits_implementation<charT>::mask_vertical))
         return true;
#ifdef __CYGWIN__
      //
      // Cygwin has a buggy ctype facet, see https://www.cygwin.com/ml/cygwin/2012-08/msg00178.html:
      //
      else if((f & std::ctype<charT>::xdigit) == std::ctype<charT>::xdigit)
      {
         if((c >= 'a') && (c <= 'f'))
            return true;
         if((c >= 'A') && (c <= 'F'))
            return true;
      }
#endif
      return false;
#else
      return m_pimpl->isctype(c, f);
#endif
   }
   boost::intmax_t toi(const charT*& p1, const charT* p2, int radix)const;
   int value(charT c, int radix)const
   {
      const charT* pc = &c;
      return (int)toi(pc, pc + 1, radix);
   }
   locale_type imbue(locale_type l)
   {
      std::locale result(getloc());
      m_pimpl = BOOST_REGEX_DETAIL_NS::create_cpp_regex_traits<charT>(l);
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
   boost::shared_ptr<const BOOST_REGEX_DETAIL_NS::cpp_regex_traits_implementation<charT> > m_pimpl;
   //
   // catalog name handler:
   //
   static std::string& get_catalog_name_inst();

#ifdef BOOST_HAS_THREADS
   static static_mutex& get_mutex_inst();
#endif
};


template <class charT>
boost::intmax_t cpp_regex_traits<charT>::toi(const charT*& first, const charT* last, int radix)const
{
   BOOST_REGEX_DETAIL_NS::parser_buf<charT>   sbuf;            // buffer for parsing numbers.
   std::basic_istream<charT>      is(&sbuf);       // stream for parsing numbers.

   // we do NOT want to parse any thousands separators inside the stream:
   last = std::find(first, last, BOOST_USE_FACET(std::numpunct<charT>, is.getloc()).thousands_sep());

   sbuf.pubsetbuf(const_cast<charT*>(static_cast<const charT*>(first)), static_cast<std::streamsize>(last-first));
   is.clear();
   if(std::abs(radix) == 16) is >> std::hex;
   else if(std::abs(radix) == 8) is >> std::oct;
   else is >> std::dec;
   boost::intmax_t val;
   if(is >> val)
   {
      first = first + ((last - first) - sbuf.in_avail());
      return val;
   }
   else
      return -1;
}

template <class charT>
std::string cpp_regex_traits<charT>::catalog_name(const std::string& name)
{
#ifdef BOOST_HAS_THREADS
   static_mutex::scoped_lock lk(get_mutex_inst());
#endif
   std::string result(get_catalog_name_inst());
   get_catalog_name_inst() = name;
   return result;
}

template <class charT>
std::string& cpp_regex_traits<charT>::get_catalog_name_inst()
{
   static std::string s_name;
   return s_name;
}

template <class charT>
std::string cpp_regex_traits<charT>::get_catalog_name()
{
#ifdef BOOST_HAS_THREADS
   static_mutex::scoped_lock lk(get_mutex_inst());
#endif
   std::string result(get_catalog_name_inst());
   return result;
}

#ifdef BOOST_HAS_THREADS
template <class charT>
static_mutex& cpp_regex_traits<charT>::get_mutex_inst()
{
   static static_mutex s_mutex = BOOST_STATIC_MUTEX_INIT;
   return s_mutex;
}
#endif

namespace BOOST_REGEX_DETAIL_NS {

   inline void cpp_regex_traits_char_layer<char>::init()
   {
      // we need to start by initialising our syntax map so we know which
      // character is used for which purpose:
      std::memset(m_char_map, 0, sizeof(m_char_map));
#ifndef BOOST_NO_STD_MESSAGES
#ifndef __IBMCPP__
      std::messages<char>::catalog cat = static_cast<std::messages<char>::catalog>(-1);
#else
      std::messages<char>::catalog cat = reinterpret_cast<std::messages<char>::catalog>(-1);
#endif
      std::string cat_name(cpp_regex_traits<char>::get_catalog_name());
      if ((!cat_name.empty()) && (m_pmessages != 0))
      {
         cat = this->m_pmessages->open(
            cat_name,
            this->m_locale);
         if ((int)cat < 0)
         {
            std::string m("Unable to open message catalog: ");
            std::runtime_error err(m + cat_name);
            boost::BOOST_REGEX_DETAIL_NS::raise_runtime_error(err);
         }
      }
      //
      // if we have a valid catalog then load our messages:
      //
      if ((int)cat >= 0)
      {
#ifndef BOOST_NO_EXCEPTIONS
         try {
#endif
            for (regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
            {
               string_type mss = this->m_pmessages->get(cat, 0, i, get_default_syntax(i));
               for (string_type::size_type j = 0; j < mss.size(); ++j)
               {
                  m_char_map[static_cast<unsigned char>(mss[j])] = i;
               }
            }
            this->m_pmessages->close(cat);
#ifndef BOOST_NO_EXCEPTIONS
         }
         catch (...)
         {
            this->m_pmessages->close(cat);
            throw;
         }
#endif
      }
      else
      {
#endif
         for (regex_constants::syntax_type j = 1; j < regex_constants::syntax_max; ++j)
         {
            const char* ptr = get_default_syntax(j);
            while (ptr && *ptr)
            {
               m_char_map[static_cast<unsigned char>(*ptr)] = j;
               ++ptr;
            }
         }
#ifndef BOOST_NO_STD_MESSAGES
      }
#endif
      //
      // finish off by calculating our escape types:
      //
      unsigned char i = 'A';
      do
      {
         if (m_char_map[i] == 0)
         {
            if (this->m_pctype->is(std::ctype_base::lower, i))
               m_char_map[i] = regex_constants::escape_type_class;
            else if (this->m_pctype->is(std::ctype_base::upper, i))
               m_char_map[i] = regex_constants::escape_type_not_class;
         }
      } while (0xFF != i++);
   }

} // namespace detail


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

#endif

#endif
