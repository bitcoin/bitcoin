/*
 *
 * Copyright (c) 1998-2004 John Maddock
 * Copyright 2011 Garmin Ltd. or its subsidiaries
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org/ for most recent version.
  *   FILE         basic_regex.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares template class basic_regex.
  */

#ifndef BOOST_REGEX_V4_BASIC_REGEX_HPP
#define BOOST_REGEX_V4_BASIC_REGEX_HPP

#include <boost/type_traits/is_same.hpp>
#include <boost/container_hash/hash.hpp>

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
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4251)
#if BOOST_MSVC < 1700
#     pragma warning(disable : 4231)
#endif
#if BOOST_MSVC < 1600
#pragma warning(disable : 4660)
#endif
#if BOOST_MSVC < 1910
#pragma warning(disable:4800)
#endif
#endif

namespace BOOST_REGEX_DETAIL_NS{

//
// forward declaration, we will need this one later:
//
template <class charT, class traits>
class basic_regex_parser;

template <class I>
void bubble_down_one(I first, I last)
{
   if(first != last)
   {
      I next = last - 1;
      while((next != first) && (*next < *(next-1)))
      {
         (next-1)->swap(*next);
         --next;
      }
   }
}

static const int hash_value_mask = 1 << (std::numeric_limits<int>::digits - 1);

template <class Iterator>
inline int hash_value_from_capture_name(Iterator i, Iterator j)
{
   std::size_t r = boost::hash_range(i, j);
   r %= ((std::numeric_limits<int>::max)());
   return static_cast<int>(r) | hash_value_mask;
}

class named_subexpressions
{
public:
   struct name
   {
      template <class charT>
      name(const charT* i, const charT* j, int idx)
         : index(idx) 
      { 
         hash = hash_value_from_capture_name(i, j); 
      }
      name(int h, int idx)
         : index(idx), hash(h)
      { 
      }
      int index;
      int hash;
      bool operator < (const name& other)const
      {
         return hash < other.hash;
      }
      bool operator == (const name& other)const
      {
         return hash == other.hash; 
      }
      void swap(name& other)
      {
         std::swap(index, other.index);
         std::swap(hash, other.hash);
      }
   };

   typedef std::vector<name>::const_iterator const_iterator;
   typedef std::pair<const_iterator, const_iterator> range_type;

   named_subexpressions(){}

   template <class charT>
   void set_name(const charT* i, const charT* j, int index)
   {
      m_sub_names.push_back(name(i, j, index));
      bubble_down_one(m_sub_names.begin(), m_sub_names.end());
   }
   template <class charT>
   int get_id(const charT* i, const charT* j)const
   {
      name t(i, j, 0);
      typename std::vector<name>::const_iterator pos = std::lower_bound(m_sub_names.begin(), m_sub_names.end(), t);
      if((pos != m_sub_names.end()) && (*pos == t))
      {
         return pos->index;
      }
      return -1;
   }
   template <class charT>
   range_type equal_range(const charT* i, const charT* j)const
   {
      name t(i, j, 0);
      return std::equal_range(m_sub_names.begin(), m_sub_names.end(), t);
   }
   int get_id(int h)const
   {
      name t(h, 0);
      std::vector<name>::const_iterator pos = std::lower_bound(m_sub_names.begin(), m_sub_names.end(), t);
      if((pos != m_sub_names.end()) && (*pos == t))
      {
         return pos->index;
      }
      return -1;
   }
   range_type equal_range(int h)const
   {
      name t(h, 0);
      return std::equal_range(m_sub_names.begin(), m_sub_names.end(), t);
   }
private:
   std::vector<name> m_sub_names;
};

//
// class regex_data:
// represents the data we wish to expose to the matching algorithms.
//
template <class charT, class traits>
struct regex_data : public named_subexpressions
{
   typedef regex_constants::syntax_option_type   flag_type;
   typedef std::size_t                           size_type;  

   regex_data(const ::boost::shared_ptr<
      ::boost::regex_traits_wrapper<traits> >& t) 
      : m_ptraits(t), m_flags(0), m_status(0), m_expression(0), m_expression_len(0),
         m_mark_count(0), m_first_state(0), m_restart_type(0),
#if !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX) && !(defined(BOOST_MSVC) && (BOOST_MSVC < 1900))
         m_startmap{ 0 },
#endif
         m_can_be_null(0), m_word_mask(0), m_has_recursions(false), m_disable_match_any(false) {}
   regex_data() 
      : m_ptraits(new ::boost::regex_traits_wrapper<traits>()), m_flags(0), m_status(0), m_expression(0), m_expression_len(0), 
         m_mark_count(0), m_first_state(0), m_restart_type(0), 
#if !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX) && !(defined(BOOST_MSVC) && (BOOST_MSVC < 1900))
      m_startmap{ 0 },
#endif
         m_can_be_null(0), m_word_mask(0), m_has_recursions(false), m_disable_match_any(false) {}

   ::boost::shared_ptr<
      ::boost::regex_traits_wrapper<traits>
      >                        m_ptraits;                 // traits class instance
   flag_type                   m_flags;                   // flags with which we were compiled
   int                         m_status;                  // error code (0 implies OK).
   const charT*                m_expression;              // the original expression
   std::ptrdiff_t              m_expression_len;          // the length of the original expression
   size_type                   m_mark_count;              // the number of marked sub-expressions
   BOOST_REGEX_DETAIL_NS::re_syntax_base*  m_first_state;             // the first state of the machine
   unsigned                    m_restart_type;            // search optimisation type
   unsigned char               m_startmap[1 << CHAR_BIT]; // which characters can start a match
   unsigned int                m_can_be_null;             // whether we can match a null string
   BOOST_REGEX_DETAIL_NS::raw_storage      m_data;                    // the buffer in which our states are constructed
   typename traits::char_class_type    m_word_mask;       // mask used to determine if a character is a word character
   std::vector<
      std::pair<
      std::size_t, std::size_t> > m_subs;                 // Position of sub-expressions within the *string*.
   bool                        m_has_recursions;          // whether we have recursive expressions;
   bool                        m_disable_match_any;       // when set we need to disable the match_any flag as it causes different/buggy behaviour.
};
//
// class basic_regex_implementation
// pimpl implementation class for basic_regex.
//
template <class charT, class traits>
class basic_regex_implementation
   : public regex_data<charT, traits>
{
public:
   typedef regex_constants::syntax_option_type   flag_type;
   typedef std::ptrdiff_t                        difference_type;
   typedef std::size_t                           size_type; 
   typedef typename traits::locale_type          locale_type;
   typedef const charT*                          const_iterator;

   basic_regex_implementation(){}
   basic_regex_implementation(const ::boost::shared_ptr<
      ::boost::regex_traits_wrapper<traits> >& t)
      : regex_data<charT, traits>(t) {}
   void assign(const charT* arg_first,
                          const charT* arg_last,
                          flag_type f)
   {
      regex_data<charT, traits>* pdat = this;
      basic_regex_parser<charT, traits> parser(pdat);
      parser.parse(arg_first, arg_last, f);
   }

   locale_type BOOST_REGEX_CALL imbue(locale_type l)
   { 
      return this->m_ptraits->imbue(l); 
   }
   locale_type BOOST_REGEX_CALL getloc()const
   { 
      return this->m_ptraits->getloc(); 
   }
   std::basic_string<charT> BOOST_REGEX_CALL str()const
   {
      std::basic_string<charT> result;
      if(this->m_status == 0)
         result = std::basic_string<charT>(this->m_expression, this->m_expression_len);
      return result;
   }
   const_iterator BOOST_REGEX_CALL expression()const
   {
      return this->m_expression;
   }
   std::pair<const_iterator, const_iterator> BOOST_REGEX_CALL subexpression(std::size_t n)const
   {
      const std::pair<std::size_t, std::size_t>& pi = this->m_subs.at(n);
      std::pair<const_iterator, const_iterator> p(expression() + pi.first, expression() + pi.second);
      return p;
   }
   //
   // begin, end:
   const_iterator BOOST_REGEX_CALL begin()const
   { 
      return (this->m_status ? 0 : this->m_expression); 
   }
   const_iterator BOOST_REGEX_CALL end()const
   { 
      return (this->m_status ? 0 : this->m_expression + this->m_expression_len); 
   }
   flag_type BOOST_REGEX_CALL flags()const
   {
      return this->m_flags;
   }
   size_type BOOST_REGEX_CALL size()const
   {
      return this->m_expression_len;
   }
   int BOOST_REGEX_CALL status()const
   {
      return this->m_status;
   }
   size_type BOOST_REGEX_CALL mark_count()const
   {
      return this->m_mark_count - 1;
   }
   const BOOST_REGEX_DETAIL_NS::re_syntax_base* get_first_state()const
   {
      return this->m_first_state;
   }
   unsigned get_restart_type()const
   {
      return this->m_restart_type;
   }
   const unsigned char* get_map()const
   {
      return this->m_startmap;
   }
   const ::boost::regex_traits_wrapper<traits>& get_traits()const
   {
      return *(this->m_ptraits);
   }
   bool can_be_null()const
   {
      return this->m_can_be_null;
   }
   const regex_data<charT, traits>& get_data()const
   {
      basic_regex_implementation<charT, traits> const* p = this;
      return *static_cast<const regex_data<charT, traits>*>(p);
   }
};

} // namespace BOOST_REGEX_DETAIL_NS
//
// class basic_regex:
// represents the compiled
// regular expression:
//

#ifdef BOOST_REGEX_NO_FWD
template <class charT, class traits = regex_traits<charT> >
#else
template <class charT, class traits >
#endif
class basic_regex : public regbase
{
public:
   // typedefs:
   typedef std::size_t                           traits_size_type;
   typedef typename traits::string_type          traits_string_type;
   typedef charT                                 char_type;
   typedef traits                                traits_type;

   typedef charT                                 value_type;
   typedef charT&                                reference;
   typedef const charT&                          const_reference;
   typedef const charT*                          const_iterator;
   typedef const_iterator                        iterator;
   typedef std::ptrdiff_t                        difference_type;
   typedef std::size_t                           size_type;   
   typedef regex_constants::syntax_option_type   flag_type;
   // locale_type
   // placeholder for actual locale type used by the
   // traits class to localise *this.
   typedef typename traits::locale_type          locale_type;
   
public:
   explicit basic_regex(){}
   explicit basic_regex(const charT* p, flag_type f = regex_constants::normal)
   {
      assign(p, f);
   }
   basic_regex(const charT* p1, const charT* p2, flag_type f = regex_constants::normal)
   {
      assign(p1, p2, f);
   }
   basic_regex(const charT* p, size_type len, flag_type f)
   {
      assign(p, len, f);
   }
   basic_regex(const basic_regex& that)
      : m_pimpl(that.m_pimpl) {}
   ~basic_regex(){}
   basic_regex& BOOST_REGEX_CALL operator=(const basic_regex& that)
   {
      return assign(that);
   }
   basic_regex& BOOST_REGEX_CALL operator=(const charT* ptr)
   {
      return assign(ptr);
   }

   //
   // assign:
   basic_regex& assign(const basic_regex& that)
   { 
      m_pimpl = that.m_pimpl;
      return *this; 
   }
   basic_regex& assign(const charT* p, flag_type f = regex_constants::normal)
   {
      return assign(p, p + traits::length(p), f);
   }
   basic_regex& assign(const charT* p, size_type len, flag_type f)
   {
      return assign(p, p + len, f);
   }
private:
   basic_regex& do_assign(const charT* p1,
                          const charT* p2,
                          flag_type f);
public:
   basic_regex& assign(const charT* p1,
                          const charT* p2,
                          flag_type f = regex_constants::normal)
   {
      return do_assign(p1, p2, f);
   }
#if !defined(BOOST_NO_MEMBER_TEMPLATES)

   template <class ST, class SA>
   unsigned int BOOST_REGEX_CALL set_expression(const std::basic_string<charT, ST, SA>& p, flag_type f = regex_constants::normal)
   { 
      return set_expression(p.data(), p.data() + p.size(), f); 
   }

   template <class ST, class SA>
   explicit basic_regex(const std::basic_string<charT, ST, SA>& p, flag_type f = regex_constants::normal)
   { 
      assign(p, f); 
   }

   template <class InputIterator>
   basic_regex(InputIterator arg_first, InputIterator arg_last, flag_type f = regex_constants::normal)
   {
      typedef typename traits::string_type seq_type;
      seq_type a(arg_first, arg_last);
      if(!a.empty())
         assign(static_cast<const charT*>(&*a.begin()), static_cast<const charT*>(&*a.begin() + a.size()), f);
      else
         assign(static_cast<const charT*>(0), static_cast<const charT*>(0), f);
   }

   template <class ST, class SA>
   basic_regex& BOOST_REGEX_CALL operator=(const std::basic_string<charT, ST, SA>& p)
   {
      return assign(p.data(), p.data() + p.size(), regex_constants::normal);
   }

   template <class string_traits, class A>
   basic_regex& BOOST_REGEX_CALL assign(
       const std::basic_string<charT, string_traits, A>& s,
       flag_type f = regex_constants::normal)
   {
      return assign(s.data(), s.data() + s.size(), f);
   }

   template <class InputIterator>
   basic_regex& BOOST_REGEX_CALL assign(InputIterator arg_first,
                          InputIterator arg_last,
                          flag_type f = regex_constants::normal)
   {
      typedef typename traits::string_type seq_type;
      seq_type a(arg_first, arg_last);
      if(a.size())
      {
         const charT* p1 = &*a.begin();
         const charT* p2 = &*a.begin() + a.size();
         return assign(p1, p2, f);
      }
      return assign(static_cast<const charT*>(0), static_cast<const charT*>(0), f);
   }
#else
   unsigned int BOOST_REGEX_CALL set_expression(const std::basic_string<charT>& p, flag_type f = regex_constants::normal)
   { 
      return set_expression(p.data(), p.data() + p.size(), f); 
   }

   basic_regex(const std::basic_string<charT>& p, flag_type f = regex_constants::normal)
   { 
      assign(p, f); 
   }

   basic_regex& BOOST_REGEX_CALL operator=(const std::basic_string<charT>& p)
   {
      return assign(p.data(), p.data() + p.size(), regex_constants::normal);
   }

   basic_regex& BOOST_REGEX_CALL assign(
       const std::basic_string<charT>& s,
       flag_type f = regex_constants::normal)
   {
      return assign(s.data(), s.data() + s.size(), f);
   }

#endif

   //
   // locale:
   locale_type BOOST_REGEX_CALL imbue(locale_type l);
   locale_type BOOST_REGEX_CALL getloc()const
   { 
      return m_pimpl.get() ? m_pimpl->getloc() : locale_type(); 
   }
   //
   // getflags:
   // retained for backwards compatibility only, "flags"
   // is now the preferred name:
   flag_type BOOST_REGEX_CALL getflags()const
   { 
      return flags();
   }
   flag_type BOOST_REGEX_CALL flags()const
   { 
      return m_pimpl.get() ? m_pimpl->flags() : 0;
   }
   //
   // str:
   std::basic_string<charT> BOOST_REGEX_CALL str()const
   {
      return m_pimpl.get() ? m_pimpl->str() : std::basic_string<charT>();
   }
   //
   // begin, end, subexpression:
   std::pair<const_iterator, const_iterator> BOOST_REGEX_CALL subexpression(std::size_t n)const
   {
      if(!m_pimpl.get())
         boost::throw_exception(std::logic_error("Can't access subexpressions in an invalid regex."));
      return m_pimpl->subexpression(n);
   }
   const_iterator BOOST_REGEX_CALL begin()const
   { 
      return (m_pimpl.get() ? m_pimpl->begin() : 0); 
   }
   const_iterator BOOST_REGEX_CALL end()const
   { 
      return (m_pimpl.get() ? m_pimpl->end() : 0); 
   }
   //
   // swap:
   void BOOST_REGEX_CALL swap(basic_regex& that)throw()
   {
      m_pimpl.swap(that.m_pimpl);
   }
   //
   // size:
   size_type BOOST_REGEX_CALL size()const
   { 
      return (m_pimpl.get() ? m_pimpl->size() : 0); 
   }
   //
   // max_size:
   size_type BOOST_REGEX_CALL max_size()const
   { 
      return UINT_MAX; 
   }
   //
   // empty:
   bool BOOST_REGEX_CALL empty()const
   { 
      return (m_pimpl.get() ? 0 != m_pimpl->status() : true); 
   }

   size_type BOOST_REGEX_CALL mark_count()const 
   { 
      return (m_pimpl.get() ? m_pimpl->mark_count() : 0); 
   }

   int status()const
   {
      return (m_pimpl.get() ? m_pimpl->status() : regex_constants::error_empty);
   }

   int BOOST_REGEX_CALL compare(const basic_regex& that) const
   {
      if(m_pimpl.get() == that.m_pimpl.get())
         return 0;
      if(!m_pimpl.get())
         return -1;
      if(!that.m_pimpl.get())
         return 1;
      if(status() != that.status())
         return status() - that.status();
      if(flags() != that.flags())
         return flags() - that.flags();
      return str().compare(that.str());
   }
   bool BOOST_REGEX_CALL operator==(const basic_regex& e)const
   { 
      return compare(e) == 0; 
   }
   bool BOOST_REGEX_CALL operator != (const basic_regex& e)const
   { 
      return compare(e) != 0; 
   }
   bool BOOST_REGEX_CALL operator<(const basic_regex& e)const
   { 
      return compare(e) < 0; 
   }
   bool BOOST_REGEX_CALL operator>(const basic_regex& e)const
   { 
      return compare(e) > 0; 
   }
   bool BOOST_REGEX_CALL operator<=(const basic_regex& e)const
   { 
      return compare(e) <= 0; 
   }
   bool BOOST_REGEX_CALL operator>=(const basic_regex& e)const
   { 
      return compare(e) >= 0; 
   }

   //
   // The following are deprecated as public interfaces
   // but are available for compatibility with earlier versions.
   const charT* BOOST_REGEX_CALL expression()const 
   { 
      return (m_pimpl.get() && !m_pimpl->status() ? m_pimpl->expression() : 0); 
   }
   unsigned int BOOST_REGEX_CALL set_expression(const charT* p1, const charT* p2, flag_type f = regex_constants::normal)
   {
      assign(p1, p2, f | regex_constants::no_except);
      return status();
   }
   unsigned int BOOST_REGEX_CALL set_expression(const charT* p, flag_type f = regex_constants::normal) 
   { 
      assign(p, f | regex_constants::no_except); 
      return status();
   }
   unsigned int BOOST_REGEX_CALL error_code()const
   {
      return status();
   }
   //
   // private access methods:
   //
   const BOOST_REGEX_DETAIL_NS::re_syntax_base* get_first_state()const
   {
      BOOST_REGEX_ASSERT(0 != m_pimpl.get());
      return m_pimpl->get_first_state();
   }
   unsigned get_restart_type()const
   {
      BOOST_REGEX_ASSERT(0 != m_pimpl.get());
      return m_pimpl->get_restart_type();
   }
   const unsigned char* get_map()const
   {
      BOOST_REGEX_ASSERT(0 != m_pimpl.get());
      return m_pimpl->get_map();
   }
   const ::boost::regex_traits_wrapper<traits>& get_traits()const
   {
      BOOST_REGEX_ASSERT(0 != m_pimpl.get());
      return m_pimpl->get_traits();
   }
   bool can_be_null()const
   {
      BOOST_REGEX_ASSERT(0 != m_pimpl.get());
      return m_pimpl->can_be_null();
   }
   const BOOST_REGEX_DETAIL_NS::regex_data<charT, traits>& get_data()const
   {
      BOOST_REGEX_ASSERT(0 != m_pimpl.get());
      return m_pimpl->get_data();
   }
   boost::shared_ptr<BOOST_REGEX_DETAIL_NS::named_subexpressions > get_named_subs()const
   {
      return m_pimpl;
   }

private:
   shared_ptr<BOOST_REGEX_DETAIL_NS::basic_regex_implementation<charT, traits> > m_pimpl;
};

//
// out of line members;
// these are the only members that mutate the basic_regex object,
// and are designed to provide the strong exception guarantee
// (in the event of a throw, the state of the object remains unchanged).
//
template <class charT, class traits>
basic_regex<charT, traits>& basic_regex<charT, traits>::do_assign(const charT* p1,
                        const charT* p2,
                        flag_type f)
{
   shared_ptr<BOOST_REGEX_DETAIL_NS::basic_regex_implementation<charT, traits> > temp;
   if(!m_pimpl.get())
   {
      temp = shared_ptr<BOOST_REGEX_DETAIL_NS::basic_regex_implementation<charT, traits> >(new BOOST_REGEX_DETAIL_NS::basic_regex_implementation<charT, traits>());
   }
   else
   {
      temp = shared_ptr<BOOST_REGEX_DETAIL_NS::basic_regex_implementation<charT, traits> >(new BOOST_REGEX_DETAIL_NS::basic_regex_implementation<charT, traits>(m_pimpl->m_ptraits));
   }
   temp->assign(p1, p2, f);
   temp.swap(m_pimpl);
   return *this;
}

template <class charT, class traits>
typename basic_regex<charT, traits>::locale_type BOOST_REGEX_CALL basic_regex<charT, traits>::imbue(locale_type l)
{ 
   shared_ptr<BOOST_REGEX_DETAIL_NS::basic_regex_implementation<charT, traits> > temp(new BOOST_REGEX_DETAIL_NS::basic_regex_implementation<charT, traits>());
   locale_type result = temp->imbue(l);
   temp.swap(m_pimpl);
   return result;
}

//
// non-members:
//
template <class charT, class traits>
void swap(basic_regex<charT, traits>& e1, basic_regex<charT, traits>& e2)
{
   e1.swap(e2);
}

#ifndef BOOST_NO_STD_LOCALE
template <class charT, class traits, class traits2>
std::basic_ostream<charT, traits>& 
   operator << (std::basic_ostream<charT, traits>& os, 
                const basic_regex<charT, traits2>& e)
{
   return (os << e.str());
}
#else
template <class traits>
std::ostream& operator << (std::ostream& os, const basic_regex<char, traits>& e)
{
   return (os << e.str());
}
#endif

//
// class reg_expression:
// this is provided for backwards compatibility only,
// it is deprecated, no not use!
//
#ifdef BOOST_REGEX_NO_FWD
template <class charT, class traits = regex_traits<charT> >
#else
template <class charT, class traits >
#endif
class reg_expression : public basic_regex<charT, traits>
{
public:
   typedef typename basic_regex<charT, traits>::flag_type flag_type;
   typedef typename basic_regex<charT, traits>::size_type size_type;
   explicit reg_expression(){}
   explicit reg_expression(const charT* p, flag_type f = regex_constants::normal)
      : basic_regex<charT, traits>(p, f){}
   reg_expression(const charT* p1, const charT* p2, flag_type f = regex_constants::normal)
      : basic_regex<charT, traits>(p1, p2, f){}
   reg_expression(const charT* p, size_type len, flag_type f)
      : basic_regex<charT, traits>(p, len, f){}
   reg_expression(const reg_expression& that)
      : basic_regex<charT, traits>(that) {}
   ~reg_expression(){}
   reg_expression& BOOST_REGEX_CALL operator=(const reg_expression& that)
   {
      return this->assign(that);
   }

#if !defined(BOOST_NO_MEMBER_TEMPLATES)
   template <class ST, class SA>
   explicit reg_expression(const std::basic_string<charT, ST, SA>& p, flag_type f = regex_constants::normal)
   : basic_regex<charT, traits>(p, f)
   { 
   }

   template <class InputIterator>
   reg_expression(InputIterator arg_first, InputIterator arg_last, flag_type f = regex_constants::normal)
   : basic_regex<charT, traits>(arg_first, arg_last, f)
   {
   }

   template <class ST, class SA>
   reg_expression& BOOST_REGEX_CALL operator=(const std::basic_string<charT, ST, SA>& p)
   {
      this->assign(p);
      return *this;
   }
#else
   explicit reg_expression(const std::basic_string<charT>& p, flag_type f = regex_constants::normal)
   : basic_regex<charT, traits>(p, f)
   { 
   }

   reg_expression& BOOST_REGEX_CALL operator=(const std::basic_string<charT>& p)
   {
      this->assign(p);
      return *this;
   }
#endif

};

#ifdef BOOST_MSVC
#pragma warning (pop)
#endif

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

#endif
