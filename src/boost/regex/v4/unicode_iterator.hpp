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
  *   FILE         unicode_iterator.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Iterator adapters for converting between different Unicode encodings.
  */

/****************************************************************************

Contents:
~~~~~~~~~

1) Read Only, Input Adapters:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template <class BaseIterator, class U8Type = ::boost::uint8_t>
class u32_to_u8_iterator;

Adapts sequence of UTF-32 code points to "look like" a sequence of UTF-8.

template <class BaseIterator, class U32Type = ::boost::uint32_t>
class u8_to_u32_iterator;

Adapts sequence of UTF-8 code points to "look like" a sequence of UTF-32.

template <class BaseIterator, class U16Type = ::boost::uint16_t>
class u32_to_u16_iterator;

Adapts sequence of UTF-32 code points to "look like" a sequence of UTF-16.

template <class BaseIterator, class U32Type = ::boost::uint32_t>
class u16_to_u32_iterator;

Adapts sequence of UTF-16 code points to "look like" a sequence of UTF-32.

2) Single pass output iterator adapters:

template <class BaseIterator>
class utf8_output_iterator;

Accepts UTF-32 code points and forwards them on as UTF-8 code points.

template <class BaseIterator>
class utf16_output_iterator;

Accepts UTF-32 code points and forwards them on as UTF-16 code points.

****************************************************************************/

#ifndef BOOST_REGEX_V4_UNICODE_ITERATOR_HPP
#define BOOST_REGEX_V4_UNICODE_ITERATOR_HPP
#include <boost/cstdint.hpp>
#include <boost/regex/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#ifndef BOOST_NO_STD_LOCALE
#include <sstream>
#include <ios>
#endif
#include <limits.h> // CHAR_BIT

#ifdef BOOST_REGEX_CXX03

#else
#endif

namespace boost{

namespace detail{

static const ::boost::uint16_t high_surrogate_base = 0xD7C0u;
static const ::boost::uint16_t low_surrogate_base = 0xDC00u;
static const ::boost::uint32_t ten_bit_mask = 0x3FFu;

inline bool is_high_surrogate(::boost::uint16_t v)
{
   return (v & 0xFFFFFC00u) == 0xd800u;
}
inline bool is_low_surrogate(::boost::uint16_t v)
{
   return (v & 0xFFFFFC00u) == 0xdc00u;
}
template <class T>
inline bool is_surrogate(T v)
{
   return (v & 0xFFFFF800u) == 0xd800;
}

inline unsigned utf8_byte_count(boost::uint8_t c)
{
   // if the most significant bit with a zero in it is in position
   // 8-N then there are N bytes in this UTF-8 sequence:
   boost::uint8_t mask = 0x80u;
   unsigned result = 0;
   while(c & mask)
   {
      ++result;
      mask >>= 1;
   }
   return (result == 0) ? 1 : ((result > 4) ? 4 : result);
}

inline unsigned utf8_trailing_byte_count(boost::uint8_t c)
{
   return utf8_byte_count(c) - 1;
}

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4100)
#endif
#ifndef BOOST_NO_EXCEPTIONS
BOOST_NORETURN
#endif
inline void invalid_utf32_code_point(::boost::uint32_t val)
{
#ifndef BOOST_NO_STD_LOCALE
   std::stringstream ss;
   ss << "Invalid UTF-32 code point U+" << std::showbase << std::hex << val << " encountered while trying to encode UTF-16 sequence";
   std::out_of_range e(ss.str());
#else
   std::out_of_range e("Invalid UTF-32 code point encountered while trying to encode UTF-16 sequence");
#endif
   boost::throw_exception(e);
}
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


} // namespace detail

template <class BaseIterator, class U16Type = ::boost::uint16_t>
class u32_to_u16_iterator
{
#if !defined(BOOST_NO_STD_ITERATOR_TRAITS)
   typedef typename std::iterator_traits<BaseIterator>::value_type base_value_type;

   BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
   BOOST_STATIC_ASSERT(sizeof(U16Type)*CHAR_BIT == 16);
#endif

public:
   typedef std::ptrdiff_t     difference_type;
   typedef U16Type            value_type;
   typedef value_type const*  pointer;
   typedef value_type const   reference;
   typedef std::bidirectional_iterator_tag iterator_category;

   reference operator*()const
   {
      if(m_current == 2)
         extract_current();
      return m_values[m_current];
   }
   bool operator==(const u32_to_u16_iterator& that)const
   {
      if(m_position == that.m_position)
      {
         // Both m_currents must be equal, or both even
         // this is the same as saying their sum must be even:
         return (m_current + that.m_current) & 1u ? false : true;
      }
      return false;
   }
   bool operator!=(const u32_to_u16_iterator& that)const
   {
      return !(*this == that);
   }
   u32_to_u16_iterator& operator++()
   {
      // if we have a pending read then read now, so that we know whether
      // to skip a position, or move to a low-surrogate:
      if(m_current == 2)
      {
         // pending read:
         extract_current();
      }
      // move to the next surrogate position:
      ++m_current;
      // if we've reached the end skip a position:
      if(m_values[m_current] == 0)
      {
         m_current = 2;
         ++m_position;
      }
      return *this;
   }
   u32_to_u16_iterator operator++(int)
   {
      u32_to_u16_iterator r(*this);
      ++(*this);
      return r;
   }
   u32_to_u16_iterator& operator--()
   {
      if(m_current != 1)
      {
         // decrementing an iterator always leads to a valid position:
         --m_position;
         extract_current();
         m_current = m_values[1] ? 1 : 0;
      }
      else
      {
         m_current = 0;
      }
      return *this;
   }
   u32_to_u16_iterator operator--(int)
   {
      u32_to_u16_iterator r(*this);
      --(*this);
      return r;
   }
   BaseIterator base()const
   {
      return m_position;
   }
   // construct:
   u32_to_u16_iterator() : m_position(), m_current(0)
   {
      m_values[0] = 0;
      m_values[1] = 0;
      m_values[2] = 0;
   }
   u32_to_u16_iterator(BaseIterator b) : m_position(b), m_current(2)
   {
      m_values[0] = 0;
      m_values[1] = 0;
      m_values[2] = 0;
   }
private:

   void extract_current()const
   {
      // begin by checking for a code point out of range:
      ::boost::uint32_t v = *m_position;
      if(v >= 0x10000u)
      {
         if(v > 0x10FFFFu)
            detail::invalid_utf32_code_point(*m_position);
         // split into two surrogates:
         m_values[0] = static_cast<U16Type>(v >> 10) + detail::high_surrogate_base;
         m_values[1] = static_cast<U16Type>(v & detail::ten_bit_mask) + detail::low_surrogate_base;
         m_current = 0;
         BOOST_REGEX_ASSERT(detail::is_high_surrogate(m_values[0]));
         BOOST_REGEX_ASSERT(detail::is_low_surrogate(m_values[1]));
      }
      else
      {
         // 16-bit code point:
         m_values[0] = static_cast<U16Type>(*m_position);
         m_values[1] = 0;
         m_current = 0;
         // value must not be a surrogate:
         if(detail::is_surrogate(m_values[0]))
            detail::invalid_utf32_code_point(*m_position);
      }
   }
   BaseIterator m_position;
   mutable U16Type m_values[3];
   mutable unsigned m_current;
};

template <class BaseIterator, class U32Type = ::boost::uint32_t>
class u16_to_u32_iterator
{
   // special values for pending iterator reads:
   BOOST_STATIC_CONSTANT(U32Type, pending_read = 0xffffffffu);

#if !defined(BOOST_NO_STD_ITERATOR_TRAITS)
   typedef typename std::iterator_traits<BaseIterator>::value_type base_value_type;

   BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 16);
   BOOST_STATIC_ASSERT(sizeof(U32Type)*CHAR_BIT == 32);
#endif

public:
   typedef std::ptrdiff_t     difference_type;
   typedef U32Type            value_type;
   typedef value_type const*  pointer;
   typedef value_type const   reference;
   typedef std::bidirectional_iterator_tag iterator_category;

   reference operator*()const
   {
      if(m_value == pending_read)
         extract_current();
      return m_value;
   }
   bool operator==(const u16_to_u32_iterator& that)const
   {
      return m_position == that.m_position;
   }
   bool operator!=(const u16_to_u32_iterator& that)const
   {
      return !(*this == that);
   }
   u16_to_u32_iterator& operator++()
   {
      // skip high surrogate first if there is one:
      if(detail::is_high_surrogate(*m_position)) ++m_position;
      ++m_position;
      m_value = pending_read;
      return *this;
   }
   u16_to_u32_iterator operator++(int)
   {
      u16_to_u32_iterator r(*this);
      ++(*this);
      return r;
   }
   u16_to_u32_iterator& operator--()
   {
      --m_position;
      // if we have a low surrogate then go back one more:
      if(detail::is_low_surrogate(*m_position)) 
         --m_position;
      m_value = pending_read;
      return *this;
   }
   u16_to_u32_iterator operator--(int)
   {
      u16_to_u32_iterator r(*this);
      --(*this);
      return r;
   }
   BaseIterator base()const
   {
      return m_position;
   }
   // construct:
   u16_to_u32_iterator() : m_position()
   {
      m_value = pending_read;
   }
   u16_to_u32_iterator(BaseIterator b) : m_position(b)
   {
      m_value = pending_read;
   }
   //
   // Range checked version:
   //
   u16_to_u32_iterator(BaseIterator b, BaseIterator start, BaseIterator end) : m_position(b)
   {
      m_value = pending_read;
      //
      // The range must not start with a low surrogate, or end in a high surrogate,
      // otherwise we run the risk of running outside the underlying input range.
      // Likewise b must not be located at a low surrogate.
      //
      boost::uint16_t val;
      if(start != end)
      {
         if((b != start) && (b != end))
         {
            val = *b;
            if(detail::is_surrogate(val) && ((val & 0xFC00u) == 0xDC00u))
               invalid_code_point(val);
         }
         val = *start;
         if(detail::is_surrogate(val) && ((val & 0xFC00u) == 0xDC00u))
            invalid_code_point(val);
         val = *--end;
         if(detail::is_high_surrogate(val))
            invalid_code_point(val);
      }
   }
private:
   static void invalid_code_point(::boost::uint16_t val)
   {
#ifndef BOOST_NO_STD_LOCALE
      std::stringstream ss;
      ss << "Misplaced UTF-16 surrogate U+" << std::showbase << std::hex << val << " encountered while trying to encode UTF-32 sequence";
      std::out_of_range e(ss.str());
#else
      std::out_of_range e("Misplaced UTF-16 surrogate encountered while trying to encode UTF-32 sequence");
#endif
      boost::throw_exception(e);
   }
   void extract_current()const
   {
      m_value = static_cast<U32Type>(static_cast< ::boost::uint16_t>(*m_position));
      // if the last value is a high surrogate then adjust m_position and m_value as needed:
      if(detail::is_high_surrogate(*m_position))
      {
         // precondition; next value must have be a low-surrogate:
         BaseIterator next(m_position);
         ::boost::uint16_t t = *++next;
         if((t & 0xFC00u) != 0xDC00u)
            invalid_code_point(t);
         m_value = (m_value - detail::high_surrogate_base) << 10;
         m_value |= (static_cast<U32Type>(static_cast< ::boost::uint16_t>(t)) & detail::ten_bit_mask);
      }
      // postcondition; result must not be a surrogate:
      if(detail::is_surrogate(m_value))
         invalid_code_point(static_cast< ::boost::uint16_t>(m_value));
   }
   BaseIterator m_position;
   mutable U32Type m_value;
};

template <class BaseIterator, class U8Type = ::boost::uint8_t>
class u32_to_u8_iterator
{
#if !defined(BOOST_NO_STD_ITERATOR_TRAITS)
   typedef typename std::iterator_traits<BaseIterator>::value_type base_value_type;

   BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 32);
   BOOST_STATIC_ASSERT(sizeof(U8Type)*CHAR_BIT == 8);
#endif

public:
   typedef std::ptrdiff_t     difference_type;
   typedef U8Type             value_type;
   typedef value_type const*  pointer;
   typedef value_type const   reference;
   typedef std::bidirectional_iterator_tag iterator_category;

   reference operator*()const
   {
      if(m_current == 4)
         extract_current();
      return m_values[m_current];
   }
   bool operator==(const u32_to_u8_iterator& that)const
   {
      if(m_position == that.m_position)
      {
         // either the m_current's must be equal, or one must be 0 and 
         // the other 4: which means neither must have bits 1 or 2 set:
         return (m_current == that.m_current)
            || (((m_current | that.m_current) & 3) == 0);
      }
      return false;
   }
   bool operator!=(const u32_to_u8_iterator& that)const
   {
      return !(*this == that);
   }
   u32_to_u8_iterator& operator++()
   {
      // if we have a pending read then read now, so that we know whether
      // to skip a position, or move to a low-surrogate:
      if(m_current == 4)
      {
         // pending read:
         extract_current();
      }
      // move to the next surrogate position:
      ++m_current;
      // if we've reached the end skip a position:
      if(m_values[m_current] == 0)
      {
         m_current = 4;
         ++m_position;
      }
      return *this;
   }
   u32_to_u8_iterator operator++(int)
   {
      u32_to_u8_iterator r(*this);
      ++(*this);
      return r;
   }
   u32_to_u8_iterator& operator--()
   {
      if((m_current & 3) == 0)
      {
         --m_position;
         extract_current();
         m_current = 3;
         while(m_current && (m_values[m_current] == 0))
            --m_current;
      }
      else
         --m_current;
      return *this;
   }
   u32_to_u8_iterator operator--(int)
   {
      u32_to_u8_iterator r(*this);
      --(*this);
      return r;
   }
   BaseIterator base()const
   {
      return m_position;
   }
   // construct:
   u32_to_u8_iterator() : m_position(), m_current(0)
   {
      m_values[0] = 0;
      m_values[1] = 0;
      m_values[2] = 0;
      m_values[3] = 0;
      m_values[4] = 0;
   }
   u32_to_u8_iterator(BaseIterator b) : m_position(b), m_current(4)
   {
      m_values[0] = 0;
      m_values[1] = 0;
      m_values[2] = 0;
      m_values[3] = 0;
      m_values[4] = 0;
   }
private:

   void extract_current()const
   {
      boost::uint32_t c = *m_position;
      if(c > 0x10FFFFu)
         detail::invalid_utf32_code_point(c);
      if(c < 0x80u)
      {
         m_values[0] = static_cast<unsigned char>(c);
         m_values[1] = static_cast<unsigned char>(0u);
         m_values[2] = static_cast<unsigned char>(0u);
         m_values[3] = static_cast<unsigned char>(0u);
      }
      else if(c < 0x800u)
      {
         m_values[0] = static_cast<unsigned char>(0xC0u + (c >> 6));
         m_values[1] = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
         m_values[2] = static_cast<unsigned char>(0u);
         m_values[3] = static_cast<unsigned char>(0u);
      }
      else if(c < 0x10000u)
      {
         m_values[0] = static_cast<unsigned char>(0xE0u + (c >> 12));
         m_values[1] = static_cast<unsigned char>(0x80u + ((c >> 6) & 0x3Fu));
         m_values[2] = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
         m_values[3] = static_cast<unsigned char>(0u);
      }
      else
      {
         m_values[0] = static_cast<unsigned char>(0xF0u + (c >> 18));
         m_values[1] = static_cast<unsigned char>(0x80u + ((c >> 12) & 0x3Fu));
         m_values[2] = static_cast<unsigned char>(0x80u + ((c >> 6) & 0x3Fu));
         m_values[3] = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
      }
      m_current= 0;
   }
   BaseIterator m_position;
   mutable U8Type m_values[5];
   mutable unsigned m_current;
};

template <class BaseIterator, class U32Type = ::boost::uint32_t>
class u8_to_u32_iterator
{
   // special values for pending iterator reads:
   BOOST_STATIC_CONSTANT(U32Type, pending_read = 0xffffffffu);

#if !defined(BOOST_NO_STD_ITERATOR_TRAITS)
   typedef typename std::iterator_traits<BaseIterator>::value_type base_value_type;

   BOOST_STATIC_ASSERT(sizeof(base_value_type)*CHAR_BIT == 8);
   BOOST_STATIC_ASSERT(sizeof(U32Type)*CHAR_BIT == 32);
#endif

public:
   typedef std::ptrdiff_t     difference_type;
   typedef U32Type            value_type;
   typedef value_type const*  pointer;
   typedef value_type const   reference;
   typedef std::bidirectional_iterator_tag iterator_category;

   reference operator*()const
   {
      if(m_value == pending_read)
         extract_current();
      return m_value;
   }
   bool operator==(const u8_to_u32_iterator& that)const
   {
      return m_position == that.m_position;
   }
   bool operator!=(const u8_to_u32_iterator& that)const
   {
      return !(*this == that);
   }
   u8_to_u32_iterator& operator++()
   {
      // We must not start with a continuation character:
      if((static_cast<boost::uint8_t>(*m_position) & 0xC0) == 0x80)
         invalid_sequence();
      // skip high surrogate first if there is one:
      unsigned c = detail::utf8_byte_count(*m_position);
      if(m_value == pending_read)
      {
         // Since we haven't read in a value, we need to validate the code points:
         for(unsigned i = 0; i < c; ++i)
         {
            ++m_position;
            // We must have a continuation byte:
            if((i != c - 1) && ((static_cast<boost::uint8_t>(*m_position) & 0xC0) != 0x80))
               invalid_sequence();
         }
      }
      else
      {
         std::advance(m_position, c);
      }
      m_value = pending_read;
      return *this;
   }
   u8_to_u32_iterator operator++(int)
   {
      u8_to_u32_iterator r(*this);
      ++(*this);
      return r;
   }
   u8_to_u32_iterator& operator--()
   {
      // Keep backtracking until we don't have a trailing character:
      unsigned count = 0;
      while((*--m_position & 0xC0u) == 0x80u) ++count;
      // now check that the sequence was valid:
      if(count != detail::utf8_trailing_byte_count(*m_position))
         invalid_sequence();
      m_value = pending_read;
      return *this;
   }
   u8_to_u32_iterator operator--(int)
   {
      u8_to_u32_iterator r(*this);
      --(*this);
      return r;
   }
   BaseIterator base()const
   {
      return m_position;
   }
   // construct:
   u8_to_u32_iterator() : m_position()
   {
      m_value = pending_read;
   }
   u8_to_u32_iterator(BaseIterator b) : m_position(b)
   {
      m_value = pending_read;
   }
   //
   // Checked constructor:
   //
   u8_to_u32_iterator(BaseIterator b, BaseIterator start, BaseIterator end) : m_position(b)
   {
      m_value = pending_read;
      //
      // We must not start with a continuation character, or end with a 
      // truncated UTF-8 sequence otherwise we run the risk of going past
      // the start/end of the underlying sequence:
      //
      if(start != end)
      {
         unsigned char v = *start;
         if((v & 0xC0u) == 0x80u)
            invalid_sequence();
         if((b != start) && (b != end) && ((*b & 0xC0u) == 0x80u))
            invalid_sequence();
         BaseIterator pos = end;
         do
         {
            v = *--pos;
         }
         while((start != pos) && ((v & 0xC0u) == 0x80u));
         std::ptrdiff_t extra = detail::utf8_byte_count(v);
         if(std::distance(pos, end) < extra)
            invalid_sequence();
      }
   }
private:
   static void invalid_sequence()
   {
      std::out_of_range e("Invalid UTF-8 sequence encountered while trying to encode UTF-32 character");
      boost::throw_exception(e);
   }
   void extract_current()const
   {
      m_value = static_cast<U32Type>(static_cast< ::boost::uint8_t>(*m_position));
      // we must not have a continuation character:
      if((m_value & 0xC0u) == 0x80u)
         invalid_sequence();
      // see how many extra bytes we have:
      unsigned extra = detail::utf8_trailing_byte_count(*m_position);
      // extract the extra bits, 6 from each extra byte:
      BaseIterator next(m_position);
      for(unsigned c = 0; c < extra; ++c)
      {
         ++next;
         m_value <<= 6;
         // We must have a continuation byte:
         if((static_cast<boost::uint8_t>(*next) & 0xC0) != 0x80)
            invalid_sequence();
         m_value += static_cast<boost::uint8_t>(*next) & 0x3Fu;
      }
      // we now need to remove a few of the leftmost bits, but how many depends
      // upon how many extra bytes we've extracted:
      static const boost::uint32_t masks[4] = 
      {
         0x7Fu,
         0x7FFu,
         0xFFFFu,
         0x1FFFFFu,
      };
      m_value &= masks[extra];
      // check the result is in range:
      if(m_value > static_cast<U32Type>(0x10FFFFu))
         invalid_sequence();
      // The result must not be a surrogate:
      if((m_value >= static_cast<U32Type>(0xD800)) && (m_value <= static_cast<U32Type>(0xDFFF)))
         invalid_sequence();
      // We should not have had an invalidly encoded UTF8 sequence:
      if((extra > 0) && (m_value <= static_cast<U32Type>(masks[extra - 1])))
         invalid_sequence();
   }
   BaseIterator m_position;
   mutable U32Type m_value;
};

template <class BaseIterator>
class utf16_output_iterator
{
public:
   typedef void                                   difference_type;
   typedef void                                   value_type;
   typedef boost::uint32_t*                       pointer;
   typedef boost::uint32_t&                       reference;
   typedef std::output_iterator_tag               iterator_category;

   utf16_output_iterator(const BaseIterator& b)
      : m_position(b){}
   utf16_output_iterator(const utf16_output_iterator& that)
      : m_position(that.m_position){}
   utf16_output_iterator& operator=(const utf16_output_iterator& that)
   {
      m_position = that.m_position;
      return *this;
   }
   const utf16_output_iterator& operator*()const
   {
      return *this;
   }
   void operator=(boost::uint32_t val)const
   {
      push(val);
   }
   utf16_output_iterator& operator++()
   {
      return *this;
   }
   utf16_output_iterator& operator++(int)
   {
      return *this;
   }
   BaseIterator base()const
   {
      return m_position;
   }
private:
   void push(boost::uint32_t v)const
   {
      if(v >= 0x10000u)
      {
         // begin by checking for a code point out of range:
         if(v > 0x10FFFFu)
            detail::invalid_utf32_code_point(v);
         // split into two surrogates:
         *m_position++ = static_cast<boost::uint16_t>(v >> 10) + detail::high_surrogate_base;
         *m_position++ = static_cast<boost::uint16_t>(v & detail::ten_bit_mask) + detail::low_surrogate_base;
      }
      else
      {
         // 16-bit code point:
         // value must not be a surrogate:
         if(detail::is_surrogate(v))
            detail::invalid_utf32_code_point(v);
         *m_position++ = static_cast<boost::uint16_t>(v);
      }
   }
   mutable BaseIterator m_position;
};

template <class BaseIterator>
class utf8_output_iterator
{
public:
   typedef void                                   difference_type;
   typedef void                                   value_type;
   typedef boost::uint32_t*                       pointer;
   typedef boost::uint32_t&                       reference;
   typedef std::output_iterator_tag               iterator_category;

   utf8_output_iterator(const BaseIterator& b)
      : m_position(b){}
   utf8_output_iterator(const utf8_output_iterator& that)
      : m_position(that.m_position){}
   utf8_output_iterator& operator=(const utf8_output_iterator& that)
   {
      m_position = that.m_position;
      return *this;
   }
   const utf8_output_iterator& operator*()const
   {
      return *this;
   }
   void operator=(boost::uint32_t val)const
   {
      push(val);
   }
   utf8_output_iterator& operator++()
   {
      return *this;
   }
   utf8_output_iterator& operator++(int)
   {
      return *this;
   }
   BaseIterator base()const
   {
      return m_position;
   }
private:
   void push(boost::uint32_t c)const
   {
      if(c > 0x10FFFFu)
         detail::invalid_utf32_code_point(c);
      if(c < 0x80u)
      {
         *m_position++ = static_cast<unsigned char>(c);
      }
      else if(c < 0x800u)
      {
         *m_position++ = static_cast<unsigned char>(0xC0u + (c >> 6));
         *m_position++ = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
      }
      else if(c < 0x10000u)
      {
         *m_position++ = static_cast<unsigned char>(0xE0u + (c >> 12));
         *m_position++ = static_cast<unsigned char>(0x80u + ((c >> 6) & 0x3Fu));
         *m_position++ = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
      }
      else
      {
         *m_position++ = static_cast<unsigned char>(0xF0u + (c >> 18));
         *m_position++ = static_cast<unsigned char>(0x80u + ((c >> 12) & 0x3Fu));
         *m_position++ = static_cast<unsigned char>(0x80u + ((c >> 6) & 0x3Fu));
         *m_position++ = static_cast<unsigned char>(0x80u + (c & 0x3Fu));
      }
   }
   mutable BaseIterator m_position;
};

} // namespace boost

#endif // BOOST_REGEX_UNICODE_ITERATOR_HPP

