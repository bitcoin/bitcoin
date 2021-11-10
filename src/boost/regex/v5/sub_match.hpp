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

#ifndef BOOST_REGEX_V5_SUB_MATCH_HPP
#define BOOST_REGEX_V5_SUB_MATCH_HPP

namespace boost{

template <class BidiIterator>
struct sub_match : public std::pair<BidiIterator, BidiIterator>
{
   typedef typename std::iterator_traits<BidiIterator>::value_type       value_type;
   typedef typename std::iterator_traits<BidiIterator>::difference_type  difference_type;
   typedef          BidiIterator                                                     iterator_type;
   typedef          BidiIterator                                                     iterator;
   typedef          BidiIterator                                                     const_iterator;

   bool matched;

   sub_match() : std::pair<BidiIterator, BidiIterator>(), matched(false) {}
   sub_match(BidiIterator i) : std::pair<BidiIterator, BidiIterator>(i, i), matched(false) {}
   template <class T, class A>
   operator std::basic_string<value_type, T, A> ()const
   {
      return matched ? std::basic_string<value_type, T, A>(this->first, this->second) : std::basic_string<value_type, T, A>();
   }
   difference_type  length()const
   {
      difference_type n = matched ? std::distance((BidiIterator)this->first, (BidiIterator)this->second) : 0;
      return n;
   }
   std::basic_string<value_type> str()const
   {
      std::basic_string<value_type> result;
      if(matched)
      {
         std::size_t len = std::distance((BidiIterator)this->first, (BidiIterator)this->second);
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
   bool  operator !=(const sub_match& that)const
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
   mutable std::unique_ptr<capture_sequence_type> m_captures;
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
};

typedef sub_match<const char*> csub_match;
typedef sub_match<std::string::const_iterator> ssub_match;
#ifndef BOOST_NO_WREGEX
typedef sub_match<const wchar_t*> wcsub_match;
typedef sub_match<std::wstring::const_iterator> wssub_match;
#endif

// comparison to std::basic_string<> part 1:
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator == (const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) == 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator != (const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) != 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator < (const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                 const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) < 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator <= (const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) <= 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator >= (const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) >= 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator > (const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                 const sub_match<RandomAccessIterator>& m)
{ return s.compare(m.str()) > 0; }
// comparison to std::basic_string<> part 2:
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator == (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) == 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator != (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) != 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator < (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) < 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator > (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) > 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator <= (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) <= 0; }
template <class RandomAccessIterator, class traits, class Allocator>
inline bool operator >= (const sub_match<RandomAccessIterator>& m,
                  const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{ return m.str().compare(s) >= 0; }
// comparison to const charT* part 1:
template <class RandomAccessIterator>
inline bool operator == (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) == 0; }
template <class RandomAccessIterator>
inline bool operator != (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) != 0; }
template <class RandomAccessIterator>
inline bool operator > (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) > 0; }
template <class RandomAccessIterator>
inline bool operator < (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) < 0; }
template <class RandomAccessIterator>
inline bool operator >= (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) >= 0; }
template <class RandomAccessIterator>
inline bool operator <= (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const* s)
{ return m.str().compare(s) <= 0; }
// comparison to const charT* part 2:
template <class RandomAccessIterator>
inline bool operator == (typename std::iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) == 0; }
template <class RandomAccessIterator>
inline bool operator != (typename std::iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) != 0; }
template <class RandomAccessIterator>
inline bool operator < (typename std::iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) > 0; }
template <class RandomAccessIterator>
inline bool operator > (typename std::iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) < 0; }
template <class RandomAccessIterator>
inline bool operator <= (typename std::iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) >= 0; }
template <class RandomAccessIterator>
inline bool operator >= (typename std::iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(s) <= 0; }

// comparison to const charT& part 1:
template <class RandomAccessIterator>
inline bool operator == (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) == 0; }
template <class RandomAccessIterator>
inline bool operator != (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) != 0; }
template <class RandomAccessIterator>
inline bool operator > (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) > 0; }
template <class RandomAccessIterator>
inline bool operator < (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) < 0; }
template <class RandomAccessIterator>
inline bool operator >= (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) >= 0; }
template <class RandomAccessIterator>
inline bool operator <= (const sub_match<RandomAccessIterator>& m,
                  typename std::iterator_traits<RandomAccessIterator>::value_type const& s)
{ return m.str().compare(0, m.length(), &s, 1) <= 0; }
// comparison to const charT* part 2:
template <class RandomAccessIterator>
inline bool operator == (typename std::iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) == 0; }
template <class RandomAccessIterator>
inline bool operator != (typename std::iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) != 0; }
template <class RandomAccessIterator>
inline bool operator < (typename std::iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) > 0; }
template <class RandomAccessIterator>
inline bool operator > (typename std::iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) < 0; }
template <class RandomAccessIterator>
inline bool operator <= (typename std::iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) >= 0; }
template <class RandomAccessIterator>
inline bool operator >= (typename std::iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{ return m.str().compare(0, m.length(), &s, 1) <= 0; }

// addition operators:
template <class RandomAccessIterator, class traits, class Allocator>
inline std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator> 
operator + (const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s,
                  const sub_match<RandomAccessIterator>& m)
{
   std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator> result;
   result.reserve(s.size() + m.length() + 1);
   return result.append(s).append(m.first, m.second);
}
template <class RandomAccessIterator, class traits, class Allocator>
inline std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator> 
operator + (const sub_match<RandomAccessIterator>& m,
            const std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator>& s)
{
   std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type, traits, Allocator> result;
   result.reserve(s.size() + m.length() + 1);
   return result.append(m.first, m.second).append(s);
}
template <class RandomAccessIterator>
inline std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> 
operator + (typename std::iterator_traits<RandomAccessIterator>::value_type const* s,
                  const sub_match<RandomAccessIterator>& m)
{
   std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(std::char_traits<typename std::iterator_traits<RandomAccessIterator>::value_type>::length(s) + m.length() + 1);
   return result.append(s).append(m.first, m.second);
}
template <class RandomAccessIterator>
inline std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> 
operator + (const sub_match<RandomAccessIterator>& m,
            typename std::iterator_traits<RandomAccessIterator>::value_type const * s)
{
   std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(std::char_traits<typename std::iterator_traits<RandomAccessIterator>::value_type>::length(s) + m.length() + 1);
   return result.append(m.first, m.second).append(s);
}
template <class RandomAccessIterator>
inline std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> 
operator + (typename std::iterator_traits<RandomAccessIterator>::value_type const& s,
                  const sub_match<RandomAccessIterator>& m)
{
   std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(m.length() + 2);
   return result.append(1, s).append(m.first, m.second);
}
template <class RandomAccessIterator>
inline std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> 
operator + (const sub_match<RandomAccessIterator>& m,
            typename std::iterator_traits<RandomAccessIterator>::value_type const& s)
{
   std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(m.length() + 2);
   return result.append(m.first, m.second).append(1, s);
}
template <class RandomAccessIterator>
inline std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> 
operator + (const sub_match<RandomAccessIterator>& m1,
            const sub_match<RandomAccessIterator>& m2)
{
   std::basic_string<typename std::iterator_traits<RandomAccessIterator>::value_type> result;
   result.reserve(m1.length() + m2.length() + 1);
   return result.append(m1.first, m1.second).append(m2.first, m2.second);
}
template <class charT, class traits, class RandomAccessIterator>
std::basic_ostream<charT, traits>&
   operator << (std::basic_ostream<charT, traits>& os,
                const sub_match<RandomAccessIterator>& s)
{
   return (os << s.str());
}

} // namespace boost

#endif

