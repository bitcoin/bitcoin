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
  *   FILE         sub_match.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares template class sub_match.
  */

#ifndef BOOST_REGEX_V4_SUB_MATCH_HPP
#define BOOST_REGEX_V4_SUB_MATCH_HPP

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

template <class BidiIterator>
struct sub_match : public std::pair<BidiIterator, BidiIterator>
{
   typedef typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<BidiIterator>::value_type       value_type;
#if defined(BOOST_NO_STD_ITERATOR_TRAITS)
   typedef          std::ptrdiff_t                                                   difference_type;
#else
   typedef typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<BidiIterator>::difference_type  difference_type;
#endif
   typedef          BidiIterator                                                     iterator_type;
   typedef          BidiIterator                                                     iterator;
   typedef          BidiIterator                                                     const_iterator;

   bool matched;

   sub_match() : std::pair<BidiIterator, BidiIterator>(), matched(false) {}
   sub_match(BidiIterator i) : std::pair<BidiIterator, BidiIterator>(i, i), matched(false) {}
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)\
               && !BOOST_WORKAROUND(BOOST_BORLANDC, <= 0x0551)\
               && !BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042))
   template <class T, class A>
   operator std::basic_string<value_type, T, A> ()const
   {
      return matched ? std::basic_string<value_type, T, A>(this->first, this->second) : std::basic_string<value_type, T, A>();
   }
#else
   operator std::basic_string<value_type> ()const
   {
      return str();
   }
#endif
   difference_type BOOST_REGEX_CALL length()const
   {
      difference_type n = matched ? ::boost::BOOST_REGEX_DETAIL_NS::distance((BidiIterator)this->first, (BidiIterator)this->second) : 0;
      return n;
   }
   std::basic_string<value_type> str()const
   {
      std::basic_string<value_type> result;
      if(matched)
      {
         std::size_t len = ::boost::BOOST_REGEX_DETAIL_NS::distance((BidiIterator)this->first, (BidiIterator)this->second);
         result.reserve(len);
         BidiIterator i = this->first;
         while(i != this->second)
         {
            result.append(1, *i);
            ++i;
         }
      }
      return result;
   }
   int compare(const sub_match& s)const
   {
      if(matched != s.matched)
         return static_cast<int>(matched) - static_cast<int>(s.matched);
      return str().compare(s.str());
   }
   int compare(const std::basic_string<value_type>& s)const
   {
      return str().compare(s);
   }
   int compare(const value_type* p)const
   {
      return str().compare(p);
   }

   bool operator==(const sub_match& that)const
   { return compare(that) == 0; }
   bool BOOST_REGEX_CALL operator !=(const sub_match& that)const
   { return compare(that) != 0; }
   bool operator<(const sub_match& that)const
   { return compare(that) < 0; }
   bool operator>(const sub_match& that)const
   { return compare(that) > 0; }
   bool operator<=(const sub_match& that)const
   { return compare(that) <= 0; }
   bool operator>=(const sub_match& that)const
   { return compare(that) >= 0; }

#ifdef BOOST_REGEX_MATCH_EXTRA
   typedef std::vector<sub_match<BidiIterator> > capture_sequence_type;

   const capture_sequence_type& captures()const
   {
      if(!m_captures) 
         m_captures.reset(new capture_sequence_type());
      return *m_captures;
   }
   //
   // Private implementation API: DO NOT USE!
   //
   capture_sequence_type& get_captures()const
   {
      if(!m_captures) 
         m_captures.reset(new capture_sequence_type());
      return *m_captures;
   }

private:
   mutable boost::scoped_ptr<capture_sequence_type> m_captures;
public:

#endif
   sub_match(const sub_match& that, bool 
#ifdef BOOST_REGEX_MATCH_EXTRA
      deep_copy
#endif
      = true
      ) 
      : std::pair<BidiIterator, BidiIterator>(that), 
        matched(that.matched) 
   {
#ifdef BOOST_REGEX_MATCH_EXTRA
      if(that.m_captures)
         if(deep_copy)
            m_captures.reset(new capture_sequence_type(*(that.m_captures)));
#endif
   }
   sub_match& operator=(const sub_match& that)
   {
      this->first = that.first;
      this->second = that.second;
      matched = that.matched;
#ifdef BOOST_REGEX_MATCH_EXTRA
      if(that.m_captures)
         get_captures() = *(that.m_captures);
#endif
      return *this;
   }
   //
   // Make this type a range, for both Boost.Range, and C++11:
   //
   BidiIterator begin()const { return this->first; }
   BidiIterator end()const { return this->second; }


#ifdef BOOST_OLD_REGEX_H
   //
   // the following are deprecated, do not use!!
   //
   operator int()const;
   operator unsigned int()const;
   operator short()const
   {
      return (short)(int)(*this);
   }
   operator unsigned short()const
   {
      return (unsigned short)(unsigned int)(*this);
   }
#endif
};

typedef sub_match<const char*> csub_match;
typedef sub_match<std::string::const_iterator> ssub_match;
#ifndef BOOST_NO_WREGEX
typedef sub_match<const wchar_t*> wcsub_match;
typedef sub_match<std::wstring::const_iterator> wssub_match;
#endif

// comparison to std::basic_string<> part 1:
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator == (const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) == 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator != (const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) != 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator < (const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                 const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) < 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator <= (const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) <= 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator >= (const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) >= 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator > (const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                 const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) > 0; }
// comparison to std::basic_string<> part 2:
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator == (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) == 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator != (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) != 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator < (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) < 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator > (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) > 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator <= (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) <= 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator >= (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) >= 0; }
// comparison to const charT* part 1:
template <class RandomAccessIterator>
inline bool operator == (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) == 0; }
template <class RandomAccessIterator>
inline bool operator != (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) != 0; }
template <class RandomAccessIterator>
inline bool operator > (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) > 0; }
template <class RandomAccessIterator>
inline bool operator < (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) < 0; }
template <class RandomAccessIterator>
inline bool operator >= (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) >= 0; }
template <class RandomAccessIterator>
inline bool operator <= (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) <= 0; }
// comparison to const charT* part 2:
template <class RandomAccessIterator>
inline bool operator == (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) == 0; }
template <class RandomAccessIterator>
inline bool operator != (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) != 0; }
template <class RandomAccessIterator>
inline bool operator < (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) > 0; }
template <class RandomAccessIterator>
inline bool operator > (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) < 0; }
template <class RandomAccessIterator>
inline bool operator <= (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) >= 0; }
template <class RandomAccessIterator>
inline bool operator >= (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) <= 0; }

// comparison to const charT& part 1:
template <class RandomAccessIterator>
inline bool operator == (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) == 0; }
template <class RandomAccessIterator>
inline bool operator != (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) != 0; }
template <class RandomAccessIterator>
inline bool operator > (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) > 0; }
template <class RandomAccessIterator>
inline bool operator < (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) < 0; }
template <class RandomAccessIterator>
inline bool operator >= (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) >= 0; }
template <class RandomAccessIterator>
inline bool operator <= (const sub_match<RandomAccessIterator>& m,
                  typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) <= 0; }
// comparison to const charT* part 2:
template <class RandomAccessIterator>
inline bool operator == (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) == 0; }
template <class RandomAccessIterator>
inline bool operator != (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) != 0; }
template <class RandomAccessIterator>
inline bool operator < (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) > 0; }
template <class RandomAccessIterator>
inline bool operator > (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) < 0; }
template <class RandomAccessIterator>
inline bool operator <= (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) >= 0; }
template <class RandomAccessIterator>
inline bool operator >= (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) <= 0; }

// addition operators:
template <class RandomAccessIterator, class traits, class Allocator>
inline std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator> 
operator + (const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{
   std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator> result;
   result.reserve(s.size() + m.length() + 1);
   return result.append(s).append(m.first, m.second);
}
template <class RandomAccessIterator, class traits, class Allocator>
inline std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator> 
operator + (const sub_match<RandomAccessIterator>& m,
            const std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{
   std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type, traits, Allocator> result;
   result.reserve(s.size() + m.length() + 1);
   return result.append(m.first, m.second).append(s);
}
#if !(defined(__GNUC__) && defined(BOOST_NO_STD_LOCALE))
template <class RandomAccessIterator>
inline std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> 
operator + (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{
   std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(std::char_traits<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type>::length(s) + m.length() + 1);
   return result.append(s).append(m.first, m.second);
}
template <class RandomAccessIterator>
inline std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> 
operator + (const sub_match<RandomAccessIterator>& m,
            typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const * s)
{
   std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(std::char_traits<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type>::length(s) + m.length() + 1);
   return result.append(m.first, m.second).append(s);
}
#else
// worwaround versions:
template <class RandomAccessIterator>
inline std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> 
operator + (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{
   return s + m.str();
}
template <class RandomAccessIterator>
inline std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> 
operator + (const sub_match<RandomAccessIterator>& m,
            typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const * s)
{
   return m.str() + s;
}
#endif
template <class RandomAccessIterator>
inline std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> 
operator + (typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{
   std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(m.length() + 2);
   return result.append(1, s).append(m.first, m.second);
}
template <class RandomAccessIterator>
inline std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> 
operator + (const sub_match<RandomAccessIterator>& m,
            typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type const& s)
{
   std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(m.length() + 2);
   return result.append(m.first, m.second).append(1, s);
}
template <class RandomAccessIterator>
inline std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> 
operator + (const sub_match<RandomAccessIterator>& m1,
            const sub_match<RandomAccessIterator>& m2)
{
   std::basic_string<typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(m1.length() + m2.length() + 1);
   return result.append(m1.first, m1.second).append(m2.first, m2.second);
}
#ifndef BOOST_NO_STD_LOCALE
template <class charT, class traits, class RandomAccessIterator>
std::basic_ostream<charT, traits>&
   operator << (std::basic_ostream<charT, traits>& os,
                const sub_match<RandomAccessIterator>& s)
{
   return (os << s.str());
}
#else
template <class RandomAccessIterator>
std::ostream& operator << (std::ostream& os,
                           const sub_match<RandomAccessIterator>& s)
{
   return (os << s.str());
}
#endif

#ifdef BOOST_OLD_REGEX_H
namespace BOOST_REGEX_DETAIL_NS{
template <class BidiIterator, class charT>
int do_toi(BidiIterator i, BidiIterator j, char c, int radix)
{
   std::string s(i, j);
   char* p;
   int result = std::strtol(s.c_str(), &p, radix);
   if(*p)raise_regex_exception("Bad sub-expression");
   return result;
}

//
// helper:
template <class I, class charT>
int do_toi(I& i, I j, charT c)
{
   int result = 0;
   while((i != j) && (isdigit(*i)))
   {
      result = result*10 + (*i - '0');
      ++i;
   }
   return result;
}
}


template <class BidiIterator>
sub_match<BidiIterator>::operator int()const
{
   BidiIterator i = first;
   BidiIterator j = second;
   if(i == j)raise_regex_exception("Bad sub-expression");
   int neg = 1;
   if((i != j) && (*i == '-'))
   {
      neg = -1;
      ++i;
   }
   neg *= BOOST_REGEX_DETAIL_NS::do_toi(i, j, *i);
   if(i != j)raise_regex_exception("Bad sub-expression");
   return neg;
}
template <class BidiIterator>
sub_match<BidiIterator>::operator unsigned int()const
{
   BidiIterator i = first;
   BidiIterator j = second;
   if(i == j)
      raise_regex_exception("Bad sub-expression");
   return BOOST_REGEX_DETAIL_NS::do_toi(i, j, *first);
}
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

