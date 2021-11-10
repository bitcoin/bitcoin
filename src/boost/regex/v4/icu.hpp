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
  *   FILE         icu.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Unicode regular expressions on top of the ICU Library.
  */

#ifndef BOOST_REGEX_ICU_V4_HPP
#define BOOST_REGEX_ICU_V4_HPP

#include <boost/config.hpp>
#include <unicode/utypes.h>
#include <unicode/uchar.h>
#include <unicode/coll.h>
#include <boost/regex.hpp>
#include <boost/regex/v4/unicode_iterator.hpp>
#include <boost/mpl/int_fwd.hpp>
#include <boost/static_assert.hpp>
#include <bitset>

#ifdef BOOST_MSVC
#pragma warning (push)
#pragma warning (disable: 4251)
#endif

namespace boost {

   namespace BOOST_REGEX_DETAIL_NS {

      // 
      // Implementation details:
      //
      class icu_regex_traits_implementation
      {
         typedef UChar32                      char_type;
         typedef std::size_t                  size_type;
         typedef std::vector<char_type>       string_type;
         typedef U_NAMESPACE_QUALIFIER Locale locale_type;
         typedef boost::uint_least32_t        char_class_type;
      public:
         icu_regex_traits_implementation(const U_NAMESPACE_QUALIFIER Locale& l)
            : m_locale(l)
         {
            UErrorCode success = U_ZERO_ERROR;
            m_collator.reset(U_NAMESPACE_QUALIFIER Collator::createInstance(l, success));
            if (U_SUCCESS(success) == 0)
               init_error();
            m_collator->setStrength(U_NAMESPACE_QUALIFIER Collator::IDENTICAL);
            success = U_ZERO_ERROR;
            m_primary_collator.reset(U_NAMESPACE_QUALIFIER Collator::createInstance(l, success));
            if (U_SUCCESS(success) == 0)
               init_error();
            m_primary_collator->setStrength(U_NAMESPACE_QUALIFIER Collator::PRIMARY);
         }
         U_NAMESPACE_QUALIFIER Locale getloc()const
         {
            return m_locale;
         }
         string_type do_transform(const char_type* p1, const char_type* p2, const U_NAMESPACE_QUALIFIER Collator* pcoll) const
         {
            // TODO make thread safe!!!! :
            typedef u32_to_u16_iterator<const char_type*, ::UChar> itt;
            itt i(p1), j(p2);
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
            std::vector< ::UChar> t(i, j);
#else
            std::vector< ::UChar> t;
            while (i != j)
               t.push_back(*i++);
#endif
            ::uint8_t result[100];
            ::int32_t len;
            if (!t.empty())
               len = pcoll->getSortKey(&*t.begin(), static_cast< ::int32_t>(t.size()), result, sizeof(result));
            else
               len = pcoll->getSortKey(static_cast<UChar const*>(0), static_cast< ::int32_t>(0), result, sizeof(result));
            if (std::size_t(len) > sizeof(result))
            {
               scoped_array< ::uint8_t> presult(new ::uint8_t[len + 1]);
               if (!t.empty())
                  len = pcoll->getSortKey(&*t.begin(), static_cast< ::int32_t>(t.size()), presult.get(), len + 1);
               else
                  len = pcoll->getSortKey(static_cast<UChar const*>(0), static_cast< ::int32_t>(0), presult.get(), len + 1);
               if ((0 == presult[len - 1]) && (len > 1))
                  --len;
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
               return string_type(presult.get(), presult.get() + len);
#else
               string_type sresult;
               ::uint8_t const* ia = presult.get();
               ::uint8_t const* ib = presult.get() + len;
               while (ia != ib)
                  sresult.push_back(*ia++);
               return sresult;
#endif
            }
            if ((0 == result[len - 1]) && (len > 1))
               --len;
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
            return string_type(result, result + len);
#else
            string_type sresult;
            ::uint8_t const* ia = result;
            ::uint8_t const* ib = result + len;
            while (ia != ib)
               sresult.push_back(*ia++);
            return sresult;
#endif
         }
         string_type transform(const char_type* p1, const char_type* p2) const
         {
            return do_transform(p1, p2, m_collator.get());
         }
         string_type transform_primary(const char_type* p1, const char_type* p2) const
         {
            return do_transform(p1, p2, m_primary_collator.get());
         }
      private:
         void init_error()
         {
            std::runtime_error e("Could not initialize ICU resources");
            boost::throw_exception(e);
         }
         U_NAMESPACE_QUALIFIER Locale m_locale;                                  // The ICU locale that we're using
         boost::scoped_ptr< U_NAMESPACE_QUALIFIER Collator> m_collator;          // The full collation object
         boost::scoped_ptr< U_NAMESPACE_QUALIFIER Collator> m_primary_collator;  // The primary collation object
      };

      inline boost::shared_ptr<icu_regex_traits_implementation> get_icu_regex_traits_implementation(const U_NAMESPACE_QUALIFIER Locale& loc)
      {
         return boost::shared_ptr<icu_regex_traits_implementation>(new icu_regex_traits_implementation(loc));
      }

   }

   class icu_regex_traits
   {
   public:
      typedef UChar32                      char_type;
      typedef std::size_t                  size_type;
      typedef std::vector<char_type>       string_type;
      typedef U_NAMESPACE_QUALIFIER Locale locale_type;
#ifdef BOOST_NO_INT64_T
      typedef std::bitset<64>              char_class_type;
#else
      typedef boost::uint64_t              char_class_type;
#endif

      struct boost_extensions_tag {};

      icu_regex_traits()
         : m_pimpl(BOOST_REGEX_DETAIL_NS::get_icu_regex_traits_implementation(U_NAMESPACE_QUALIFIER Locale()))
      {
      }
      static size_type length(const char_type* p)
      {
         size_type result = 0;
         while (*p)
         {
            ++p;
            ++result;
         }
         return result;
      }
      ::boost::regex_constants::syntax_type syntax_type(char_type c)const
      {
         return ((c < 0x7f) && (c > 0)) ? BOOST_REGEX_DETAIL_NS::get_default_syntax_type(static_cast<char>(c)) : regex_constants::syntax_char;
      }
      ::boost::regex_constants::escape_syntax_type escape_syntax_type(char_type c) const
      {
         return ((c < 0x7f) && (c > 0)) ? BOOST_REGEX_DETAIL_NS::get_default_escape_syntax_type(static_cast<char>(c)) : regex_constants::syntax_char;
      }
      char_type translate(char_type c) const
      {
         return c;
      }
      char_type translate_nocase(char_type c) const
      {
         return ::u_tolower(c);
      }
      char_type translate(char_type c, bool icase) const
      {
         return icase ? translate_nocase(c) : translate(c);
      }
      char_type tolower(char_type c) const
      {
         return ::u_tolower(c);
      }
      char_type toupper(char_type c) const
      {
         return ::u_toupper(c);
      }
      string_type transform(const char_type* p1, const char_type* p2) const
      {
         return m_pimpl->transform(p1, p2);
      }
      string_type transform_primary(const char_type* p1, const char_type* p2) const
      {
         return m_pimpl->transform_primary(p1, p2);
      }
      char_class_type lookup_classname(const char_type* p1, const char_type* p2) const
      {
         static const char_class_type mask_blank = char_class_type(1) << offset_blank;
         static const char_class_type mask_space = char_class_type(1) << offset_space;
         static const char_class_type mask_xdigit = char_class_type(1) << offset_xdigit;
         static const char_class_type mask_underscore = char_class_type(1) << offset_underscore;
         static const char_class_type mask_unicode = char_class_type(1) << offset_unicode;
         static const char_class_type mask_any = char_class_type(1) << offset_any;
         static const char_class_type mask_ascii = char_class_type(1) << offset_ascii;
         static const char_class_type mask_horizontal = char_class_type(1) << offset_horizontal;
         static const char_class_type mask_vertical = char_class_type(1) << offset_vertical;

         static const char_class_type masks[] =
         {
            0,
            U_GC_L_MASK | U_GC_ND_MASK,
            U_GC_L_MASK,
            mask_blank,
            U_GC_CC_MASK | U_GC_CF_MASK | U_GC_ZL_MASK | U_GC_ZP_MASK,
            U_GC_ND_MASK,
            U_GC_ND_MASK,
            (0x3FFFFFFFu) & ~(U_GC_CC_MASK | U_GC_CF_MASK | U_GC_CS_MASK | U_GC_CN_MASK | U_GC_Z_MASK),
            mask_horizontal,
            U_GC_LL_MASK,
            U_GC_LL_MASK,
            ~(U_GC_C_MASK),
            U_GC_P_MASK,
            char_class_type(U_GC_Z_MASK) | mask_space,
            char_class_type(U_GC_Z_MASK) | mask_space,
            U_GC_LU_MASK,
            mask_unicode,
            U_GC_LU_MASK,
            mask_vertical,
            char_class_type(U_GC_L_MASK | U_GC_ND_MASK | U_GC_MN_MASK) | mask_underscore,
            char_class_type(U_GC_L_MASK | U_GC_ND_MASK | U_GC_MN_MASK) | mask_underscore,
            char_class_type(U_GC_ND_MASK) | mask_xdigit,
         };

         int idx = ::boost::BOOST_REGEX_DETAIL_NS::get_default_class_id(p1, p2);
         if (idx >= 0)
            return masks[idx + 1];
         char_class_type result = lookup_icu_mask(p1, p2);
         if (result != 0)
            return result;

         if (idx < 0)
         {
            string_type s(p1, p2);
            string_type::size_type i = 0;
            while (i < s.size())
            {
               s[i] = static_cast<char>((::u_tolower)(s[i]));
               if (::u_isspace(s[i]) || (s[i] == '-') || (s[i] == '_'))
                  s.erase(s.begin() + i, s.begin() + i + 1);
               else
               {
                  s[i] = static_cast<char>((::u_tolower)(s[i]));
                  ++i;
               }
            }
            if (!s.empty())
               idx = ::boost::BOOST_REGEX_DETAIL_NS::get_default_class_id(&*s.begin(), &*s.begin() + s.size());
            if (idx >= 0)
               return masks[idx + 1];
            if (!s.empty())
               result = lookup_icu_mask(&*s.begin(), &*s.begin() + s.size());
            if (result != 0)
               return result;
         }
         BOOST_ASSERT(std::size_t(idx + 1) < sizeof(masks) / sizeof(masks[0]));
         return masks[idx + 1];
      }
      string_type lookup_collatename(const char_type* p1, const char_type* p2) const
      {
         string_type result;
#ifdef BOOST_NO_CXX98_BINDERS
         if (std::find_if(p1, p2, std::bind(std::greater< ::UChar32>(), std::placeholders::_1, 0x7f)) == p2)
#else
         if (std::find_if(p1, p2, std::bind2nd(std::greater< ::UChar32>(), 0x7f)) == p2)
#endif
         {
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
            std::string s(p1, p2);
#else
            std::string s;
            const char_type* p3 = p1;
            while (p3 != p2)
               s.append(1, *p3++);
#endif
            // Try Unicode name:
            UErrorCode err = U_ZERO_ERROR;
            UChar32 c = ::u_charFromName(U_UNICODE_CHAR_NAME, s.c_str(), &err);
            if (U_SUCCESS(err))
            {
               result.push_back(c);
               return result;
            }
            // Try Unicode-extended name:
            err = U_ZERO_ERROR;
            c = ::u_charFromName(U_EXTENDED_CHAR_NAME, s.c_str(), &err);
            if (U_SUCCESS(err))
            {
               result.push_back(c);
               return result;
            }
            // try POSIX name:
            s = ::boost::BOOST_REGEX_DETAIL_NS::lookup_default_collate_name(s);
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
            result.assign(s.begin(), s.end());
#else
            result.clear();
            std::string::const_iterator si, sj;
            si = s.begin();
            sj = s.end();
            while (si != sj)
               result.push_back(*si++);
#endif
         }
         if (result.empty() && (p2 - p1 == 1))
            result.push_back(*p1);
         return result;
      }
      bool isctype(char_type c, char_class_type f) const
      {
         static const char_class_type mask_blank = char_class_type(1) << offset_blank;
         static const char_class_type mask_space = char_class_type(1) << offset_space;
         static const char_class_type mask_xdigit = char_class_type(1) << offset_xdigit;
         static const char_class_type mask_underscore = char_class_type(1) << offset_underscore;
         static const char_class_type mask_unicode = char_class_type(1) << offset_unicode;
         static const char_class_type mask_any = char_class_type(1) << offset_any;
         static const char_class_type mask_ascii = char_class_type(1) << offset_ascii;
         static const char_class_type mask_horizontal = char_class_type(1) << offset_horizontal;
         static const char_class_type mask_vertical = char_class_type(1) << offset_vertical;

         // check for standard catagories first:
         char_class_type m = char_class_type(static_cast<char_class_type>(1) << u_charType(c));
         if ((m & f) != 0)
            return true;
         // now check for special cases:
         if (((f & mask_blank) != 0) && u_isblank(c))
            return true;
         if (((f & mask_space) != 0) && u_isspace(c))
            return true;
         if (((f & mask_xdigit) != 0) && (u_digit(c, 16) >= 0))
            return true;
         if (((f & mask_unicode) != 0) && (c >= 0x100))
            return true;
         if (((f & mask_underscore) != 0) && (c == '_'))
            return true;
         if (((f & mask_any) != 0) && (c <= 0x10FFFF))
            return true;
         if (((f & mask_ascii) != 0) && (c <= 0x7F))
            return true;
         if (((f & mask_vertical) != 0) && (::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) || (c == static_cast<char_type>('\v')) || (m == U_GC_ZL_MASK) || (m == U_GC_ZP_MASK)))
            return true;
         if (((f & mask_horizontal) != 0) && !::boost::BOOST_REGEX_DETAIL_NS::is_separator(c) && u_isspace(c) && (c != static_cast<char_type>('\v')))
            return true;
         return false;
      }
      boost::intmax_t toi(const char_type*& p1, const char_type* p2, int radix)const
      {
         return BOOST_REGEX_DETAIL_NS::global_toi(p1, p2, radix, *this);
      }
      int value(char_type c, int radix)const
      {
         return u_digit(c, static_cast< ::int8_t>(radix));
      }
      locale_type imbue(locale_type l)
      {
         locale_type result(m_pimpl->getloc());
         m_pimpl = BOOST_REGEX_DETAIL_NS::get_icu_regex_traits_implementation(l);
         return result;
      }
      locale_type getloc()const
      {
         return locale_type();
      }
      std::string error_string(::boost::regex_constants::error_type n) const
      {
         return BOOST_REGEX_DETAIL_NS::get_default_error_string(n);
      }
   private:
      icu_regex_traits(const icu_regex_traits&);
      icu_regex_traits& operator=(const icu_regex_traits&);

      //
      // define the bitmasks offsets we need for additional character properties:
      //
      enum {
         offset_blank = U_CHAR_CATEGORY_COUNT,
         offset_space = U_CHAR_CATEGORY_COUNT + 1,
         offset_xdigit = U_CHAR_CATEGORY_COUNT + 2,
         offset_underscore = U_CHAR_CATEGORY_COUNT + 3,
         offset_unicode = U_CHAR_CATEGORY_COUNT + 4,
         offset_any = U_CHAR_CATEGORY_COUNT + 5,
         offset_ascii = U_CHAR_CATEGORY_COUNT + 6,
         offset_horizontal = U_CHAR_CATEGORY_COUNT + 7,
         offset_vertical = U_CHAR_CATEGORY_COUNT + 8
      };

      static char_class_type lookup_icu_mask(const ::UChar32* p1, const ::UChar32* p2)
      {
         static const char_class_type mask_blank = char_class_type(1) << offset_blank;
         static const char_class_type mask_space = char_class_type(1) << offset_space;
         static const char_class_type mask_xdigit = char_class_type(1) << offset_xdigit;
         static const char_class_type mask_underscore = char_class_type(1) << offset_underscore;
         static const char_class_type mask_unicode = char_class_type(1) << offset_unicode;
         static const char_class_type mask_any = char_class_type(1) << offset_any;
         static const char_class_type mask_ascii = char_class_type(1) << offset_ascii;
         static const char_class_type mask_horizontal = char_class_type(1) << offset_horizontal;
         static const char_class_type mask_vertical = char_class_type(1) << offset_vertical;

         static const ::UChar32 prop_name_table[] = {
            /* any */  'a', 'n', 'y',
            /* ascii */  'a', 's', 'c', 'i', 'i',
            /* assigned */  'a', 's', 's', 'i', 'g', 'n', 'e', 'd',
            /* c* */  'c', '*',
            /* cc */  'c', 'c',
            /* cf */  'c', 'f',
            /* closepunctuation */  'c', 'l', 'o', 's', 'e', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
            /* cn */  'c', 'n',
            /* co */  'c', 'o',
            /* connectorpunctuation */  'c', 'o', 'n', 'n', 'e', 'c', 't', 'o', 'r', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
            /* control */  'c', 'o', 'n', 't', 'r', 'o', 'l',
            /* cs */  'c', 's',
            /* currencysymbol */  'c', 'u', 'r', 'r', 'e', 'n', 'c', 'y', 's', 'y', 'm', 'b', 'o', 'l',
            /* dashpunctuation */  'd', 'a', 's', 'h', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
            /* decimaldigitnumber */  'd', 'e', 'c', 'i', 'm', 'a', 'l', 'd', 'i', 'g', 'i', 't', 'n', 'u', 'm', 'b', 'e', 'r',
            /* enclosingmark */  'e', 'n', 'c', 'l', 'o', 's', 'i', 'n', 'g', 'm', 'a', 'r', 'k',
            /* finalpunctuation */  'f', 'i', 'n', 'a', 'l', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
            /* format */  'f', 'o', 'r', 'm', 'a', 't',
            /* initialpunctuation */  'i', 'n', 'i', 't', 'i', 'a', 'l', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
            /* l* */  'l', '*',
            /* letter */  'l', 'e', 't', 't', 'e', 'r',
            /* letternumber */  'l', 'e', 't', 't', 'e', 'r', 'n', 'u', 'm', 'b', 'e', 'r',
            /* lineseparator */  'l', 'i', 'n', 'e', 's', 'e', 'p', 'a', 'r', 'a', 't', 'o', 'r',
            /* ll */  'l', 'l',
            /* lm */  'l', 'm',
            /* lo */  'l', 'o',
            /* lowercaseletter */  'l', 'o', 'w', 'e', 'r', 'c', 'a', 's', 'e', 'l', 'e', 't', 't', 'e', 'r',
            /* lt */  'l', 't',
            /* lu */  'l', 'u',
            /* m* */  'm', '*',
            /* mark */  'm', 'a', 'r', 'k',
            /* mathsymbol */  'm', 'a', 't', 'h', 's', 'y', 'm', 'b', 'o', 'l',
            /* mc */  'm', 'c',
            /* me */  'm', 'e',
            /* mn */  'm', 'n',
            /* modifierletter */  'm', 'o', 'd', 'i', 'f', 'i', 'e', 'r', 'l', 'e', 't', 't', 'e', 'r',
            /* modifiersymbol */  'm', 'o', 'd', 'i', 'f', 'i', 'e', 'r', 's', 'y', 'm', 'b', 'o', 'l',
            /* n* */  'n', '*',
            /* nd */  'n', 'd',
            /* nl */  'n', 'l',
            /* no */  'n', 'o',
            /* nonspacingmark */  'n', 'o', 'n', 's', 'p', 'a', 'c', 'i', 'n', 'g', 'm', 'a', 'r', 'k',
            /* notassigned */  'n', 'o', 't', 'a', 's', 's', 'i', 'g', 'n', 'e', 'd',
            /* number */  'n', 'u', 'm', 'b', 'e', 'r',
            /* openpunctuation */  'o', 'p', 'e', 'n', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
            /* other */  'o', 't', 'h', 'e', 'r',
            /* otherletter */  'o', 't', 'h', 'e', 'r', 'l', 'e', 't', 't', 'e', 'r',
            /* othernumber */  'o', 't', 'h', 'e', 'r', 'n', 'u', 'm', 'b', 'e', 'r',
            /* otherpunctuation */  'o', 't', 'h', 'e', 'r', 'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
            /* othersymbol */  'o', 't', 'h', 'e', 'r', 's', 'y', 'm', 'b', 'o', 'l',
            /* p* */  'p', '*',
            /* paragraphseparator */  'p', 'a', 'r', 'a', 'g', 'r', 'a', 'p', 'h', 's', 'e', 'p', 'a', 'r', 'a', 't', 'o', 'r',
            /* pc */  'p', 'c',
            /* pd */  'p', 'd',
            /* pe */  'p', 'e',
            /* pf */  'p', 'f',
            /* pi */  'p', 'i',
            /* po */  'p', 'o',
            /* privateuse */  'p', 'r', 'i', 'v', 'a', 't', 'e', 'u', 's', 'e',
            /* ps */  'p', 's',
            /* punctuation */  'p', 'u', 'n', 'c', 't', 'u', 'a', 't', 'i', 'o', 'n',
            /* s* */  's', '*',
            /* sc */  's', 'c',
            /* separator */  's', 'e', 'p', 'a', 'r', 'a', 't', 'o', 'r',
            /* sk */  's', 'k',
            /* sm */  's', 'm',
            /* so */  's', 'o',
            /* spaceseparator */  's', 'p', 'a', 'c', 'e', 's', 'e', 'p', 'a', 'r', 'a', 't', 'o', 'r',
            /* spacingcombiningmark */  's', 'p', 'a', 'c', 'i', 'n', 'g', 'c', 'o', 'm', 'b', 'i', 'n', 'i', 'n', 'g', 'm', 'a', 'r', 'k',
            /* surrogate */  's', 'u', 'r', 'r', 'o', 'g', 'a', 't', 'e',
            /* symbol */  's', 'y', 'm', 'b', 'o', 'l',
            /* titlecase */  't', 'i', 't', 'l', 'e', 'c', 'a', 's', 'e',
            /* titlecaseletter */  't', 'i', 't', 'l', 'e', 'c', 'a', 's', 'e', 'l', 'e', 't', 't', 'e', 'r',
            /* uppercaseletter */  'u', 'p', 'p', 'e', 'r', 'c', 'a', 's', 'e', 'l', 'e', 't', 't', 'e', 'r',
            /* z* */  'z', '*',
            /* zl */  'z', 'l',
            /* zp */  'z', 'p',
            /* zs */  'z', 's',
         };

         static const BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32> range_data[] = {
            { prop_name_table + 0, prop_name_table + 3, }, // any
            { prop_name_table + 3, prop_name_table + 8, }, // ascii
            { prop_name_table + 8, prop_name_table + 16, }, // assigned
            { prop_name_table + 16, prop_name_table + 18, }, // c*
            { prop_name_table + 18, prop_name_table + 20, }, // cc
            { prop_name_table + 20, prop_name_table + 22, }, // cf
            { prop_name_table + 22, prop_name_table + 38, }, // closepunctuation
            { prop_name_table + 38, prop_name_table + 40, }, // cn
            { prop_name_table + 40, prop_name_table + 42, }, // co
            { prop_name_table + 42, prop_name_table + 62, }, // connectorpunctuation
            { prop_name_table + 62, prop_name_table + 69, }, // control
            { prop_name_table + 69, prop_name_table + 71, }, // cs
            { prop_name_table + 71, prop_name_table + 85, }, // currencysymbol
            { prop_name_table + 85, prop_name_table + 100, }, // dashpunctuation
            { prop_name_table + 100, prop_name_table + 118, }, // decimaldigitnumber
            { prop_name_table + 118, prop_name_table + 131, }, // enclosingmark
            { prop_name_table + 131, prop_name_table + 147, }, // finalpunctuation
            { prop_name_table + 147, prop_name_table + 153, }, // format
            { prop_name_table + 153, prop_name_table + 171, }, // initialpunctuation
            { prop_name_table + 171, prop_name_table + 173, }, // l*
            { prop_name_table + 173, prop_name_table + 179, }, // letter
            { prop_name_table + 179, prop_name_table + 191, }, // letternumber
            { prop_name_table + 191, prop_name_table + 204, }, // lineseparator
            { prop_name_table + 204, prop_name_table + 206, }, // ll
            { prop_name_table + 206, prop_name_table + 208, }, // lm
            { prop_name_table + 208, prop_name_table + 210, }, // lo
            { prop_name_table + 210, prop_name_table + 225, }, // lowercaseletter
            { prop_name_table + 225, prop_name_table + 227, }, // lt
            { prop_name_table + 227, prop_name_table + 229, }, // lu
            { prop_name_table + 229, prop_name_table + 231, }, // m*
            { prop_name_table + 231, prop_name_table + 235, }, // mark
            { prop_name_table + 235, prop_name_table + 245, }, // mathsymbol
            { prop_name_table + 245, prop_name_table + 247, }, // mc
            { prop_name_table + 247, prop_name_table + 249, }, // me
            { prop_name_table + 249, prop_name_table + 251, }, // mn
            { prop_name_table + 251, prop_name_table + 265, }, // modifierletter
            { prop_name_table + 265, prop_name_table + 279, }, // modifiersymbol
            { prop_name_table + 279, prop_name_table + 281, }, // n*
            { prop_name_table + 281, prop_name_table + 283, }, // nd
            { prop_name_table + 283, prop_name_table + 285, }, // nl
            { prop_name_table + 285, prop_name_table + 287, }, // no
            { prop_name_table + 287, prop_name_table + 301, }, // nonspacingmark
            { prop_name_table + 301, prop_name_table + 312, }, // notassigned
            { prop_name_table + 312, prop_name_table + 318, }, // number
            { prop_name_table + 318, prop_name_table + 333, }, // openpunctuation
            { prop_name_table + 333, prop_name_table + 338, }, // other
            { prop_name_table + 338, prop_name_table + 349, }, // otherletter
            { prop_name_table + 349, prop_name_table + 360, }, // othernumber
            { prop_name_table + 360, prop_name_table + 376, }, // otherpunctuation
            { prop_name_table + 376, prop_name_table + 387, }, // othersymbol
            { prop_name_table + 387, prop_name_table + 389, }, // p*
            { prop_name_table + 389, prop_name_table + 407, }, // paragraphseparator
            { prop_name_table + 407, prop_name_table + 409, }, // pc
            { prop_name_table + 409, prop_name_table + 411, }, // pd
            { prop_name_table + 411, prop_name_table + 413, }, // pe
            { prop_name_table + 413, prop_name_table + 415, }, // pf
            { prop_name_table + 415, prop_name_table + 417, }, // pi
            { prop_name_table + 417, prop_name_table + 419, }, // po
            { prop_name_table + 419, prop_name_table + 429, }, // privateuse
            { prop_name_table + 429, prop_name_table + 431, }, // ps
            { prop_name_table + 431, prop_name_table + 442, }, // punctuation
            { prop_name_table + 442, prop_name_table + 444, }, // s*
            { prop_name_table + 444, prop_name_table + 446, }, // sc
            { prop_name_table + 446, prop_name_table + 455, }, // separator
            { prop_name_table + 455, prop_name_table + 457, }, // sk
            { prop_name_table + 457, prop_name_table + 459, }, // sm
            { prop_name_table + 459, prop_name_table + 461, }, // so
            { prop_name_table + 461, prop_name_table + 475, }, // spaceseparator
            { prop_name_table + 475, prop_name_table + 495, }, // spacingcombiningmark
            { prop_name_table + 495, prop_name_table + 504, }, // surrogate
            { prop_name_table + 504, prop_name_table + 510, }, // symbol
            { prop_name_table + 510, prop_name_table + 519, }, // titlecase
            { prop_name_table + 519, prop_name_table + 534, }, // titlecaseletter
            { prop_name_table + 534, prop_name_table + 549, }, // uppercaseletter
            { prop_name_table + 549, prop_name_table + 551, }, // z*
            { prop_name_table + 551, prop_name_table + 553, }, // zl
            { prop_name_table + 553, prop_name_table + 555, }, // zp
            { prop_name_table + 555, prop_name_table + 557, }, // zs
         };

         static const icu_regex_traits::char_class_type icu_class_map[] = {
            mask_any, // any
            mask_ascii, // ascii
            (0x3FFFFFFFu) & ~(U_GC_CN_MASK), // assigned
            U_GC_C_MASK, // c*
            U_GC_CC_MASK, // cc
            U_GC_CF_MASK, // cf
            U_GC_PE_MASK, // closepunctuation
            U_GC_CN_MASK, // cn
            U_GC_CO_MASK, // co
            U_GC_PC_MASK, // connectorpunctuation
            U_GC_CC_MASK, // control
            U_GC_CS_MASK, // cs
            U_GC_SC_MASK, // currencysymbol
            U_GC_PD_MASK, // dashpunctuation
            U_GC_ND_MASK, // decimaldigitnumber
            U_GC_ME_MASK, // enclosingmark
            U_GC_PF_MASK, // finalpunctuation
            U_GC_CF_MASK, // format
            U_GC_PI_MASK, // initialpunctuation
            U_GC_L_MASK, // l*
            U_GC_L_MASK, // letter
            U_GC_NL_MASK, // letternumber
            U_GC_ZL_MASK, // lineseparator
            U_GC_LL_MASK, // ll
            U_GC_LM_MASK, // lm
            U_GC_LO_MASK, // lo
            U_GC_LL_MASK, // lowercaseletter
            U_GC_LT_MASK, // lt
            U_GC_LU_MASK, // lu
            U_GC_M_MASK, // m*
            U_GC_M_MASK, // mark
            U_GC_SM_MASK, // mathsymbol
            U_GC_MC_MASK, // mc
            U_GC_ME_MASK, // me
            U_GC_MN_MASK, // mn
            U_GC_LM_MASK, // modifierletter
            U_GC_SK_MASK, // modifiersymbol
            U_GC_N_MASK, // n*
            U_GC_ND_MASK, // nd
            U_GC_NL_MASK, // nl
            U_GC_NO_MASK, // no
            U_GC_MN_MASK, // nonspacingmark
            U_GC_CN_MASK, // notassigned
            U_GC_N_MASK, // number
            U_GC_PS_MASK, // openpunctuation
            U_GC_C_MASK, // other
            U_GC_LO_MASK, // otherletter
            U_GC_NO_MASK, // othernumber
            U_GC_PO_MASK, // otherpunctuation
            U_GC_SO_MASK, // othersymbol
            U_GC_P_MASK, // p*
            U_GC_ZP_MASK, // paragraphseparator
            U_GC_PC_MASK, // pc
            U_GC_PD_MASK, // pd
            U_GC_PE_MASK, // pe
            U_GC_PF_MASK, // pf
            U_GC_PI_MASK, // pi
            U_GC_PO_MASK, // po
            U_GC_CO_MASK, // privateuse
            U_GC_PS_MASK, // ps
            U_GC_P_MASK, // punctuation
            U_GC_S_MASK, // s*
            U_GC_SC_MASK, // sc
            U_GC_Z_MASK, // separator
            U_GC_SK_MASK, // sk
            U_GC_SM_MASK, // sm
            U_GC_SO_MASK, // so
            U_GC_ZS_MASK, // spaceseparator
            U_GC_MC_MASK, // spacingcombiningmark
            U_GC_CS_MASK, // surrogate
            U_GC_S_MASK, // symbol
            U_GC_LT_MASK, // titlecase
            U_GC_LT_MASK, // titlecaseletter
            U_GC_LU_MASK, // uppercaseletter
            U_GC_Z_MASK, // z*
            U_GC_ZL_MASK, // zl
            U_GC_ZP_MASK, // zp
            U_GC_ZS_MASK, // zs
         };


         const BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32>* ranges_begin = range_data;
         const BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32>* ranges_end = range_data + (sizeof(range_data) / sizeof(range_data[0]));

         BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32> t = { p1, p2, };
         const BOOST_REGEX_DETAIL_NS::character_pointer_range< ::UChar32>* p = std::lower_bound(ranges_begin, ranges_end, t);
         if ((p != ranges_end) && (t == *p))
            return icu_class_map[p - ranges_begin];
         return 0;
      }

      boost::shared_ptr< ::boost::BOOST_REGEX_DETAIL_NS::icu_regex_traits_implementation> m_pimpl;
   };

} // namespace boost

namespace boost {

   // types:
   typedef basic_regex< ::UChar32, icu_regex_traits> u32regex;
   typedef match_results<const ::UChar32*> u32match;
   typedef match_results<const ::UChar*> u16match;

   //
   // Construction of 32-bit regex types from UTF-8 and UTF-16 primitives:
   //
   namespace BOOST_REGEX_DETAIL_NS {

#if !defined(BOOST_NO_MEMBER_TEMPLATES) && !defined(__IBMCPP__)
      template <class InputIterator>
      inline u32regex do_make_u32regex(InputIterator i,
         InputIterator j,
         boost::regex_constants::syntax_option_type opt,
         const boost::mpl::int_<1>*)
      {
         typedef boost::u8_to_u32_iterator<InputIterator, UChar32> conv_type;
         return u32regex(conv_type(i, i, j), conv_type(j, i, j), opt);
      }

      template <class InputIterator>
      inline u32regex do_make_u32regex(InputIterator i,
         InputIterator j,
         boost::regex_constants::syntax_option_type opt,
         const boost::mpl::int_<2>*)
      {
         typedef boost::u16_to_u32_iterator<InputIterator, UChar32> conv_type;
         return u32regex(conv_type(i, i, j), conv_type(j, i, j), opt);
      }

      template <class InputIterator>
      inline u32regex do_make_u32regex(InputIterator i,
         InputIterator j,
         boost::regex_constants::syntax_option_type opt,
         const boost::mpl::int_<4>*)
      {
         return u32regex(i, j, opt);
      }
#else
      template <class InputIterator>
      inline u32regex do_make_u32regex(InputIterator i,
         InputIterator j,
         boost::regex_constants::syntax_option_type opt,
         const boost::mpl::int_<1>*)
      {
         typedef boost::u8_to_u32_iterator<InputIterator, UChar32> conv_type;
         typedef std::vector<UChar32> vector_type;
         vector_type v;
         conv_type a(i, i, j), b(j, i, j);
         while (a != b)
         {
            v.push_back(*a);
            ++a;
         }
         if (v.size())
            return u32regex(&*v.begin(), v.size(), opt);
         return u32regex(static_cast<UChar32 const*>(0), static_cast<u32regex::size_type>(0), opt);
      }

      template <class InputIterator>
      inline u32regex do_make_u32regex(InputIterator i,
         InputIterator j,
         boost::regex_constants::syntax_option_type opt,
         const boost::mpl::int_<2>*)
      {
         typedef boost::u16_to_u32_iterator<InputIterator, UChar32> conv_type;
         typedef std::vector<UChar32> vector_type;
         vector_type v;
         conv_type a(i, i, j), b(j, i, j);
         while (a != b)
         {
            v.push_back(*a);
            ++a;
         }
         if (v.size())
            return u32regex(&*v.begin(), v.size(), opt);
         return u32regex(static_cast<UChar32 const*>(0), static_cast<u32regex::size_type>(0), opt);
      }

      template <class InputIterator>
      inline u32regex do_make_u32regex(InputIterator i,
         InputIterator j,
         boost::regex_constants::syntax_option_type opt,
         const boost::mpl::int_<4>*)
      {
         typedef std::vector<UChar32> vector_type;
         vector_type v;
         while (i != j)
         {
            v.push_back((UChar32)(*i));
            ++i;
         }
         if (v.size())
            return u32regex(&*v.begin(), v.size(), opt);
         return u32regex(static_cast<UChar32 const*>(0), static_cast<u32regex::size_type>(0), opt);
      }
#endif
   }

   // BOOST_REGEX_UCHAR_IS_WCHAR_T
   //
   // Source inspection of unicode/umachine.h in ICU version 59 indicates that:
   //
   // On version 59, UChar is always char16_t in C++ mode (and uint16_t in C mode)
   //
   // On earlier versions, the logic is
   //
   // #if U_SIZEOF_WCHAR_T==2
   //   typedef wchar_t OldUChar;
   // #elif defined(__CHAR16_TYPE__)
   //   typedef __CHAR16_TYPE__ OldUChar;
   // #else
   //   typedef uint16_t OldUChar;
   // #endif
   //
   // That is, UChar is wchar_t only on versions below 59, when U_SIZEOF_WCHAR_T==2
   //
   // Hence,

#define BOOST_REGEX_UCHAR_IS_WCHAR_T (U_ICU_VERSION_MAJOR_NUM < 59 && U_SIZEOF_WCHAR_T == 2)

#if BOOST_REGEX_UCHAR_IS_WCHAR_T
   BOOST_STATIC_ASSERT((boost::is_same<UChar, wchar_t>::value));
#else
   BOOST_STATIC_ASSERT(!(boost::is_same<UChar, wchar_t>::value));
#endif

   //
   // Construction from an iterator pair:
   //
   template <class InputIterator>
   inline u32regex make_u32regex(InputIterator i,
      InputIterator j,
      boost::regex_constants::syntax_option_type opt)
   {
      return BOOST_REGEX_DETAIL_NS::do_make_u32regex(i, j, opt, static_cast<boost::mpl::int_<sizeof(*i)> const*>(0));
   }
   //
   // construction from UTF-8 nul-terminated strings:
   //
   inline u32regex make_u32regex(const char* p, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
   {
      return BOOST_REGEX_DETAIL_NS::do_make_u32regex(p, p + std::strlen(p), opt, static_cast<boost::mpl::int_<1> const*>(0));
   }
   inline u32regex make_u32regex(const unsigned char* p, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
   {
      return BOOST_REGEX_DETAIL_NS::do_make_u32regex(p, p + std::strlen(reinterpret_cast<const char*>(p)), opt, static_cast<boost::mpl::int_<1> const*>(0));
   }
   //
   // construction from UTF-16 nul-terminated strings:
   //
#ifndef BOOST_NO_WREGEX
   inline u32regex make_u32regex(const wchar_t* p, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
   {
      return BOOST_REGEX_DETAIL_NS::do_make_u32regex(p, p + std::wcslen(p), opt, static_cast<boost::mpl::int_<sizeof(wchar_t)> const*>(0));
   }
#endif
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T
   inline u32regex make_u32regex(const UChar* p, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
   {
      return BOOST_REGEX_DETAIL_NS::do_make_u32regex(p, p + u_strlen(p), opt, static_cast<boost::mpl::int_<2> const*>(0));
   }
#endif
   //
   // construction from basic_string class-template:
   //
   template<class C, class T, class A>
   inline u32regex make_u32regex(const std::basic_string<C, T, A>& s, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
   {
      return BOOST_REGEX_DETAIL_NS::do_make_u32regex(s.begin(), s.end(), opt, static_cast<boost::mpl::int_<sizeof(C)> const*>(0));
   }
   //
   // Construction from ICU string type:
   //
   inline u32regex make_u32regex(const U_NAMESPACE_QUALIFIER UnicodeString& s, boost::regex_constants::syntax_option_type opt = boost::regex_constants::perl)
   {
      return BOOST_REGEX_DETAIL_NS::do_make_u32regex(s.getBuffer(), s.getBuffer() + s.length(), opt, static_cast<boost::mpl::int_<2> const*>(0));
   }

   //
   // regex_match overloads that widen the character type as appropriate:
   //
   namespace BOOST_REGEX_DETAIL_NS {
      template<class MR1, class MR2, class NSubs>
      void copy_results(MR1& out, MR2 const& in, NSubs named_subs)
      {
         // copy results from an adapted MR2 match_results:
         out.set_size(in.size(), in.prefix().first.base(), in.suffix().second.base());
         out.set_base(in.base().base());
         out.set_named_subs(named_subs);
         for (int i = 0; i < (int)in.size(); ++i)
         {
            if (in[i].matched || !i)
            {
               out.set_first(in[i].first.base(), i);
               out.set_second(in[i].second.base(), i, in[i].matched);
            }
         }
#ifdef BOOST_REGEX_MATCH_EXTRA
         // Copy full capture info as well:
         for (int i = 0; i < (int)in.size(); ++i)
         {
            if (in[i].captures().size())
            {
               out[i].get_captures().assign(in[i].captures().size(), typename MR1::value_type());
               for (int j = 0; j < (int)out[i].captures().size(); ++j)
               {
                  out[i].get_captures()[j].first = in[i].captures()[j].first.base();
                  out[i].get_captures()[j].second = in[i].captures()[j].second.base();
                  out[i].get_captures()[j].matched = in[i].captures()[j].matched;
               }
            }
         }
#endif
      }

      template <class BidiIterator, class Allocator>
      inline bool do_regex_match(BidiIterator first, BidiIterator last,
         match_results<BidiIterator, Allocator>& m,
         const u32regex& e,
         match_flag_type flags,
         boost::mpl::int_<4> const*)
      {
         return ::boost::regex_match(first, last, m, e, flags);
      }
      template <class BidiIterator, class Allocator>
      bool do_regex_match(BidiIterator first, BidiIterator last,
         match_results<BidiIterator, Allocator>& m,
         const u32regex& e,
         match_flag_type flags,
         boost::mpl::int_<2> const*)
      {
         typedef u16_to_u32_iterator<BidiIterator, UChar32> conv_type;
         typedef match_results<conv_type>                   match_type;
         //typedef typename match_type::allocator_type        alloc_type;
         match_type what;
         bool result = ::boost::regex_match(conv_type(first, first, last), conv_type(last, first, last), what, e, flags);
         // copy results across to m:
         if (result) copy_results(m, what, e.get_named_subs());
         return result;
      }
      template <class BidiIterator, class Allocator>
      bool do_regex_match(BidiIterator first, BidiIterator last,
         match_results<BidiIterator, Allocator>& m,
         const u32regex& e,
         match_flag_type flags,
         boost::mpl::int_<1> const*)
      {
         typedef u8_to_u32_iterator<BidiIterator, UChar32>  conv_type;
         typedef match_results<conv_type>                   match_type;
         //typedef typename match_type::allocator_type        alloc_type;
         match_type what;
         bool result = ::boost::regex_match(conv_type(first, first, last), conv_type(last, first, last), what, e, flags);
         // copy results across to m:
         if (result) copy_results(m, what, e.get_named_subs());
         return result;
      }
   } // namespace BOOST_REGEX_DETAIL_NS

   template <class BidiIterator, class Allocator>
   inline bool u32regex_match(BidiIterator first, BidiIterator last,
      match_results<BidiIterator, Allocator>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_match(first, last, m, e, flags, static_cast<mpl::int_<sizeof(*first)> const*>(0));
   }
   inline bool u32regex_match(const UChar* p,
      match_results<const UChar*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p + u_strlen(p), m, e, flags, static_cast<mpl::int_<2> const*>(0));
   }
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T && !defined(BOOST_NO_WREGEX)
   inline bool u32regex_match(const wchar_t* p,
      match_results<const wchar_t*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p + std::wcslen(p), m, e, flags, static_cast<mpl::int_<sizeof(wchar_t)> const*>(0));
   }
#endif
   inline bool u32regex_match(const char* p,
      match_results<const char*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p + std::strlen(p), m, e, flags, static_cast<mpl::int_<1> const*>(0));
   }
   inline bool u32regex_match(const unsigned char* p,
      match_results<const unsigned char*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p + std::strlen((const char*)p), m, e, flags, static_cast<mpl::int_<1> const*>(0));
   }
   inline bool u32regex_match(const std::string& s,
      match_results<std::string::const_iterator>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_match(s.begin(), s.end(), m, e, flags, static_cast<mpl::int_<1> const*>(0));
   }
#ifndef BOOST_NO_STD_WSTRING
   inline bool u32regex_match(const std::wstring& s,
      match_results<std::wstring::const_iterator>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_match(s.begin(), s.end(), m, e, flags, static_cast<mpl::int_<sizeof(wchar_t)> const*>(0));
   }
#endif
   inline bool u32regex_match(const U_NAMESPACE_QUALIFIER UnicodeString& s,
      match_results<const UChar*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_match(s.getBuffer(), s.getBuffer() + s.length(), m, e, flags, static_cast<mpl::int_<2> const*>(0));
   }
   //
   // regex_match overloads that do not return what matched:
   //
   template <class BidiIterator>
   inline bool u32regex_match(BidiIterator first, BidiIterator last,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<BidiIterator> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_match(first, last, m, e, flags, static_cast<mpl::int_<sizeof(*first)> const*>(0));
   }
   inline bool u32regex_match(const UChar* p,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const UChar*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p + u_strlen(p), m, e, flags, static_cast<mpl::int_<2> const*>(0));
   }
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T && !defined(BOOST_NO_WREGEX)
   inline bool u32regex_match(const wchar_t* p,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const wchar_t*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p + std::wcslen(p), m, e, flags, static_cast<mpl::int_<sizeof(wchar_t)> const*>(0));
   }
#endif
   inline bool u32regex_match(const char* p,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const char*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p + std::strlen(p), m, e, flags, static_cast<mpl::int_<1> const*>(0));
   }
   inline bool u32regex_match(const unsigned char* p,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const unsigned char*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_match(p, p + std::strlen((const char*)p), m, e, flags, static_cast<mpl::int_<1> const*>(0));
   }
   inline bool u32regex_match(const std::string& s,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<std::string::const_iterator> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_match(s.begin(), s.end(), m, e, flags, static_cast<mpl::int_<1> const*>(0));
   }
#ifndef BOOST_NO_STD_WSTRING
   inline bool u32regex_match(const std::wstring& s,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<std::wstring::const_iterator> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_match(s.begin(), s.end(), m, e, flags, static_cast<mpl::int_<sizeof(wchar_t)> const*>(0));
   }
#endif
   inline bool u32regex_match(const U_NAMESPACE_QUALIFIER UnicodeString& s,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const UChar*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_match(s.getBuffer(), s.getBuffer() + s.length(), m, e, flags, static_cast<mpl::int_<2> const*>(0));
   }

   //
   // regex_search overloads that widen the character type as appropriate:
   //
   namespace BOOST_REGEX_DETAIL_NS {
      template <class BidiIterator, class Allocator>
      inline bool do_regex_search(BidiIterator first, BidiIterator last,
         match_results<BidiIterator, Allocator>& m,
         const u32regex& e,
         match_flag_type flags,
         BidiIterator base,
         boost::mpl::int_<4> const*)
      {
         return ::boost::regex_search(first, last, m, e, flags, base);
      }
      template <class BidiIterator, class Allocator>
      bool do_regex_search(BidiIterator first, BidiIterator last,
         match_results<BidiIterator, Allocator>& m,
         const u32regex& e,
         match_flag_type flags,
         BidiIterator base,
         boost::mpl::int_<2> const*)
      {
         typedef u16_to_u32_iterator<BidiIterator, UChar32> conv_type;
         typedef match_results<conv_type>                   match_type;
         //typedef typename match_type::allocator_type        alloc_type;
         match_type what;
         bool result = ::boost::regex_search(conv_type(first, first, last), conv_type(last, first, last), what, e, flags, conv_type(base));
         // copy results across to m:
         if (result) copy_results(m, what, e.get_named_subs());
         return result;
      }
      template <class BidiIterator, class Allocator>
      bool do_regex_search(BidiIterator first, BidiIterator last,
         match_results<BidiIterator, Allocator>& m,
         const u32regex& e,
         match_flag_type flags,
         BidiIterator base,
         boost::mpl::int_<1> const*)
      {
         typedef u8_to_u32_iterator<BidiIterator, UChar32>  conv_type;
         typedef match_results<conv_type>                   match_type;
         //typedef typename match_type::allocator_type        alloc_type;
         match_type what;
         bool result = ::boost::regex_search(conv_type(first, first, last), conv_type(last, first, last), what, e, flags, conv_type(base));
         // copy results across to m:
         if (result) copy_results(m, what, e.get_named_subs());
         return result;
      }
   }

   template <class BidiIterator, class Allocator>
   inline bool u32regex_search(BidiIterator first, BidiIterator last,
      match_results<BidiIterator, Allocator>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_search(first, last, m, e, flags, first, static_cast<mpl::int_<sizeof(*first)> const*>(0));
   }
   template <class BidiIterator, class Allocator>
   inline bool u32regex_search(BidiIterator first, BidiIterator last,
      match_results<BidiIterator, Allocator>& m,
      const u32regex& e,
      match_flag_type flags,
      BidiIterator base)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_search(first, last, m, e, flags, base, static_cast<mpl::int_<sizeof(*first)> const*>(0));
   }
   inline bool u32regex_search(const UChar* p,
      match_results<const UChar*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p + u_strlen(p), m, e, flags, p, static_cast<mpl::int_<2> const*>(0));
   }
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T && !defined(BOOST_NO_WREGEX)
   inline bool u32regex_search(const wchar_t* p,
      match_results<const wchar_t*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p + std::wcslen(p), m, e, flags, p, static_cast<mpl::int_<sizeof(wchar_t)> const*>(0));
   }
#endif
   inline bool u32regex_search(const char* p,
      match_results<const char*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p + std::strlen(p), m, e, flags, p, static_cast<mpl::int_<1> const*>(0));
   }
   inline bool u32regex_search(const unsigned char* p,
      match_results<const unsigned char*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p + std::strlen((const char*)p), m, e, flags, p, static_cast<mpl::int_<1> const*>(0));
   }
   inline bool u32regex_search(const std::string& s,
      match_results<std::string::const_iterator>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_search(s.begin(), s.end(), m, e, flags, s.begin(), static_cast<mpl::int_<1> const*>(0));
   }
#ifndef BOOST_NO_STD_WSTRING
   inline bool u32regex_search(const std::wstring& s,
      match_results<std::wstring::const_iterator>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_search(s.begin(), s.end(), m, e, flags, s.begin(), static_cast<mpl::int_<sizeof(wchar_t)> const*>(0));
   }
#endif
   inline bool u32regex_search(const U_NAMESPACE_QUALIFIER UnicodeString& s,
      match_results<const UChar*>& m,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::do_regex_search(s.getBuffer(), s.getBuffer() + s.length(), m, e, flags, s.getBuffer(), static_cast<mpl::int_<2> const*>(0));
   }
   template <class BidiIterator>
   inline bool u32regex_search(BidiIterator first, BidiIterator last,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<BidiIterator> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_search(first, last, m, e, flags, first, static_cast<mpl::int_<sizeof(*first)> const*>(0));
   }
   inline bool u32regex_search(const UChar* p,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const UChar*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p + u_strlen(p), m, e, flags, p, static_cast<mpl::int_<2> const*>(0));
   }
#if !BOOST_REGEX_UCHAR_IS_WCHAR_T && !defined(BOOST_NO_WREGEX)
   inline bool u32regex_search(const wchar_t* p,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const wchar_t*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p + std::wcslen(p), m, e, flags, p, static_cast<mpl::int_<sizeof(wchar_t)> const*>(0));
   }
#endif
   inline bool u32regex_search(const char* p,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const char*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p + std::strlen(p), m, e, flags, p, static_cast<mpl::int_<1> const*>(0));
   }
   inline bool u32regex_search(const unsigned char* p,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const unsigned char*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_search(p, p + std::strlen((const char*)p), m, e, flags, p, static_cast<mpl::int_<1> const*>(0));
   }
   inline bool u32regex_search(const std::string& s,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<std::string::const_iterator> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_search(s.begin(), s.end(), m, e, flags, s.begin(), static_cast<mpl::int_<1> const*>(0));
   }
#ifndef BOOST_NO_STD_WSTRING
   inline bool u32regex_search(const std::wstring& s,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<std::wstring::const_iterator> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_search(s.begin(), s.end(), m, e, flags, s.begin(), static_cast<mpl::int_<sizeof(wchar_t)> const*>(0));
   }
#endif
   inline bool u32regex_search(const U_NAMESPACE_QUALIFIER UnicodeString& s,
      const u32regex& e,
      match_flag_type flags = match_default)
   {
      match_results<const UChar*> m;
      return BOOST_REGEX_DETAIL_NS::do_regex_search(s.getBuffer(), s.getBuffer() + s.length(), m, e, flags, s.getBuffer(), static_cast<mpl::int_<2> const*>(0));
   }

   //
   // overloads for regex_replace with utf-8 and utf-16 data types:
   //
   namespace BOOST_REGEX_DETAIL_NS {
      template <class I>
      inline std::pair< boost::u8_to_u32_iterator<I>, boost::u8_to_u32_iterator<I> >
         make_utf32_seq(I i, I j, mpl::int_<1> const*)
      {
         return std::pair< boost::u8_to_u32_iterator<I>, boost::u8_to_u32_iterator<I> >(boost::u8_to_u32_iterator<I>(i, i, j), boost::u8_to_u32_iterator<I>(j, i, j));
      }
      template <class I>
      inline std::pair< boost::u16_to_u32_iterator<I>, boost::u16_to_u32_iterator<I> >
         make_utf32_seq(I i, I j, mpl::int_<2> const*)
      {
         return std::pair< boost::u16_to_u32_iterator<I>, boost::u16_to_u32_iterator<I> >(boost::u16_to_u32_iterator<I>(i, i, j), boost::u16_to_u32_iterator<I>(j, i, j));
      }
      template <class I>
      inline std::pair< I, I >
         make_utf32_seq(I i, I j, mpl::int_<4> const*)
      {
         return std::pair< I, I >(i, j);
      }
      template <class charT>
      inline std::pair< boost::u8_to_u32_iterator<const charT*>, boost::u8_to_u32_iterator<const charT*> >
         make_utf32_seq(const charT* p, mpl::int_<1> const*)
      {
         std::size_t len = std::strlen((const char*)p);
         return std::pair< boost::u8_to_u32_iterator<const charT*>, boost::u8_to_u32_iterator<const charT*> >(boost::u8_to_u32_iterator<const charT*>(p, p, p + len), boost::u8_to_u32_iterator<const charT*>(p + len, p, p + len));
      }
      template <class charT>
      inline std::pair< boost::u16_to_u32_iterator<const charT*>, boost::u16_to_u32_iterator<const charT*> >
         make_utf32_seq(const charT* p, mpl::int_<2> const*)
      {
         std::size_t len = u_strlen((const UChar*)p);
         return std::pair< boost::u16_to_u32_iterator<const charT*>, boost::u16_to_u32_iterator<const charT*> >(boost::u16_to_u32_iterator<const charT*>(p, p, p + len), boost::u16_to_u32_iterator<const charT*>(p + len, p, p + len));
      }
      template <class charT>
      inline std::pair< const charT*, const charT* >
         make_utf32_seq(const charT* p, mpl::int_<4> const*)
      {
         return std::pair< const charT*, const charT* >(p, p + icu_regex_traits::length((UChar32 const*)p));
      }
      template <class OutputIterator>
      inline OutputIterator make_utf32_out(OutputIterator o, mpl::int_<4> const*)
      {
         return o;
      }
      template <class OutputIterator>
      inline utf16_output_iterator<OutputIterator> make_utf32_out(OutputIterator o, mpl::int_<2> const*)
      {
         return o;
      }
      template <class OutputIterator>
      inline utf8_output_iterator<OutputIterator> make_utf32_out(OutputIterator o, mpl::int_<1> const*)
      {
         return o;
      }

      template <class OutputIterator, class I1, class I2>
      OutputIterator do_regex_replace(OutputIterator out,
         std::pair<I1, I1> const& in,
         const u32regex& e,
         const std::pair<I2, I2>& fmt,
         match_flag_type flags
      )
      {
         // unfortunately we have to copy the format string in order to pass in onward:
         std::vector<UChar32> f;
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
         f.assign(fmt.first, fmt.second);
#else
         f.clear();
         I2 pos = fmt.first;
         while (pos != fmt.second)
            f.push_back(*pos++);
#endif

         regex_iterator<I1, UChar32, icu_regex_traits> i(in.first, in.second, e, flags);
         regex_iterator<I1, UChar32, icu_regex_traits> j;
         if (i == j)
         {
            if (!(flags & regex_constants::format_no_copy))
               out = BOOST_REGEX_DETAIL_NS::copy(in.first, in.second, out);
         }
         else
         {
            I1 last_m = in.first;
            while (i != j)
            {
               if (!(flags & regex_constants::format_no_copy))
                  out = BOOST_REGEX_DETAIL_NS::copy(i->prefix().first, i->prefix().second, out);
               if (!f.empty())
                  out = ::boost::BOOST_REGEX_DETAIL_NS::regex_format_imp(out, *i, &*f.begin(), &*f.begin() + f.size(), flags, e.get_traits());
               else
                  out = ::boost::BOOST_REGEX_DETAIL_NS::regex_format_imp(out, *i, static_cast<UChar32 const*>(0), static_cast<UChar32 const*>(0), flags, e.get_traits());
               last_m = (*i)[0].second;
               if (flags & regex_constants::format_first_only)
                  break;
               ++i;
            }
            if (!(flags & regex_constants::format_no_copy))
               out = BOOST_REGEX_DETAIL_NS::copy(last_m, in.second, out);
         }
         return out;
      }
      template <class BaseIterator>
      inline const BaseIterator& extract_output_base(const BaseIterator& b)
      {
         return b;
      }
      template <class BaseIterator>
      inline BaseIterator extract_output_base(const utf8_output_iterator<BaseIterator>& b)
      {
         return b.base();
      }
      template <class BaseIterator>
      inline BaseIterator extract_output_base(const utf16_output_iterator<BaseIterator>& b)
      {
         return b.base();
      }
   }  // BOOST_REGEX_DETAIL_NS

   template <class OutputIterator, class BidirectionalIterator, class charT>
   inline OutputIterator u32regex_replace(OutputIterator out,
      BidirectionalIterator first,
      BidirectionalIterator last,
      const u32regex& e,
      const charT* fmt,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::extract_output_base
      (
         BOOST_REGEX_DETAIL_NS::do_regex_replace(
            BOOST_REGEX_DETAIL_NS::make_utf32_out(out, static_cast<mpl::int_<sizeof(*first)> const*>(0)),
            BOOST_REGEX_DETAIL_NS::make_utf32_seq(first, last, static_cast<mpl::int_<sizeof(*first)> const*>(0)),
            e,
            BOOST_REGEX_DETAIL_NS::make_utf32_seq(fmt, static_cast<mpl::int_<sizeof(*fmt)> const*>(0)),
            flags)
      );
   }

   template <class OutputIterator, class Iterator, class charT>
   inline OutputIterator u32regex_replace(OutputIterator out,
      Iterator first,
      Iterator last,
      const u32regex& e,
      const std::basic_string<charT>& fmt,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::extract_output_base
      (
         BOOST_REGEX_DETAIL_NS::do_regex_replace(
            BOOST_REGEX_DETAIL_NS::make_utf32_out(out, static_cast<mpl::int_<sizeof(*first)> const*>(0)),
            BOOST_REGEX_DETAIL_NS::make_utf32_seq(first, last, static_cast<mpl::int_<sizeof(*first)> const*>(0)),
            e,
            BOOST_REGEX_DETAIL_NS::make_utf32_seq(fmt.begin(), fmt.end(), static_cast<mpl::int_<sizeof(charT)> const*>(0)),
            flags)
      );
   }

   template <class OutputIterator, class Iterator>
   inline OutputIterator u32regex_replace(OutputIterator out,
      Iterator first,
      Iterator last,
      const u32regex& e,
      const U_NAMESPACE_QUALIFIER UnicodeString& fmt,
      match_flag_type flags = match_default)
   {
      return BOOST_REGEX_DETAIL_NS::extract_output_base
      (
         BOOST_REGEX_DETAIL_NS::do_regex_replace(
            BOOST_REGEX_DETAIL_NS::make_utf32_out(out, static_cast<mpl::int_<sizeof(*first)> const*>(0)),
            BOOST_REGEX_DETAIL_NS::make_utf32_seq(first, last, static_cast<mpl::int_<sizeof(*first)> const*>(0)),
            e,
            BOOST_REGEX_DETAIL_NS::make_utf32_seq(fmt.getBuffer(), fmt.getBuffer() + fmt.length(), static_cast<mpl::int_<2> const*>(0)),
            flags)
      );
   }

   template <class charT>
   std::basic_string<charT> u32regex_replace(const std::basic_string<charT>& s,
      const u32regex& e,
      const charT* fmt,
      match_flag_type flags = match_default)
   {
      std::basic_string<charT> result;
      BOOST_REGEX_DETAIL_NS::string_out_iterator<std::basic_string<charT> > i(result);
      u32regex_replace(i, s.begin(), s.end(), e, fmt, flags);
      return result;
   }

   template <class charT>
   std::basic_string<charT> u32regex_replace(const std::basic_string<charT>& s,
      const u32regex& e,
      const std::basic_string<charT>& fmt,
      match_flag_type flags = match_default)
   {
      std::basic_string<charT> result;
      BOOST_REGEX_DETAIL_NS::string_out_iterator<std::basic_string<charT> > i(result);
      u32regex_replace(i, s.begin(), s.end(), e, fmt.c_str(), flags);
      return result;
   }

   namespace BOOST_REGEX_DETAIL_NS {

      class unicode_string_out_iterator
      {
         U_NAMESPACE_QUALIFIER UnicodeString* out;
      public:
         unicode_string_out_iterator(U_NAMESPACE_QUALIFIER UnicodeString& s) : out(&s) {}
         unicode_string_out_iterator& operator++() { return *this; }
         unicode_string_out_iterator& operator++(int) { return *this; }
         unicode_string_out_iterator& operator*() { return *this; }
         unicode_string_out_iterator& operator=(UChar v)
         {
            *out += v;
            return *this;
         }
         typedef std::ptrdiff_t difference_type;
         typedef UChar value_type;
         typedef value_type* pointer;
         typedef value_type& reference;
         typedef std::output_iterator_tag iterator_category;
      };

   }

   inline U_NAMESPACE_QUALIFIER UnicodeString u32regex_replace(const U_NAMESPACE_QUALIFIER UnicodeString& s,
      const u32regex& e,
      const UChar* fmt,
      match_flag_type flags = match_default)
   {
      U_NAMESPACE_QUALIFIER UnicodeString result;
      BOOST_REGEX_DETAIL_NS::unicode_string_out_iterator i(result);
      u32regex_replace(i, s.getBuffer(), s.getBuffer() + s.length(), e, fmt, flags);
      return result;
   }

   inline U_NAMESPACE_QUALIFIER UnicodeString u32regex_replace(const U_NAMESPACE_QUALIFIER UnicodeString& s,
      const u32regex& e,
      const U_NAMESPACE_QUALIFIER UnicodeString& fmt,
      match_flag_type flags = match_default)
   {
      U_NAMESPACE_QUALIFIER UnicodeString result;
      BOOST_REGEX_DETAIL_NS::unicode_string_out_iterator i(result);
      BOOST_REGEX_DETAIL_NS::do_regex_replace(
         BOOST_REGEX_DETAIL_NS::make_utf32_out(i, static_cast<mpl::int_<2> const*>(0)),
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(s.getBuffer(), s.getBuffer() + s.length(), static_cast<mpl::int_<2> const*>(0)),
         e,
         BOOST_REGEX_DETAIL_NS::make_utf32_seq(fmt.getBuffer(), fmt.getBuffer() + fmt.length(), static_cast<mpl::int_<2> const*>(0)),
         flags);
      return result;
   }

} // namespace boost.

#ifdef BOOST_MSVC
#pragma warning (pop)
#endif

#include <boost/regex/v4/u32regex_iterator.hpp>
#include <boost/regex/v4/u32regex_token_iterator.hpp>

#endif
