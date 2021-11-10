// Copyright Kevlin Henney, 2000-2005.
// Copyright Alexander Nasonov, 2006-2010.
// Copyright Antony Polukhin, 2011-2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// what:  lexical_cast custom keyword cast
// who:   contributed by Kevlin Henney,
//        enhanced with contributions from Terje Slettebo,
//        with additional fixes and suggestions from Gennaro Prota,
//        Beman Dawes, Dave Abrahams, Daryle Walker, Peter Dimov,
//        Alexander Nasonov, Antony Polukhin, Justin Viiret, Michael Hofmann,
//        Cheng Yang, Matthew Bradbury, David W. Birdsall, Pavel Korzh and other Boosters
// when:  November 2000, March 2003, June 2005, June 2006, March 2011 - 2014

#ifndef BOOST_LEXICAL_CAST_DETAIL_INF_NAN_HPP
#define BOOST_LEXICAL_CAST_DETAIL_INF_NAN_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#if defined(BOOST_NO_STRINGSTREAM) || defined(BOOST_NO_STD_WSTRING)
#define BOOST_LCAST_NO_WCHAR_T
#endif

#include <boost/limits.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/core/cmath.hpp>
#include <cstddef>
#include <cstring>

#include <boost/lexical_cast/detail/lcast_char_constants.hpp>

namespace boost {
    namespace detail
    {
        template <class CharT>
        bool lc_iequal(const CharT* val, const CharT* lcase, const CharT* ucase, unsigned int len) BOOST_NOEXCEPT {
            for( unsigned int i=0; i < len; ++i ) {
                if ( val[i] != lcase[i] && val[i] != ucase[i] ) return false;
            }

            return true;
        }

        /* Returns true and sets the correct value if found NaN or Inf. */
        template <class CharT, class T>
        inline bool parse_inf_nan_impl(const CharT* begin, const CharT* end, T& value
            , const CharT* lc_NAN, const CharT* lc_nan
            , const CharT* lc_INFINITY, const CharT* lc_infinity
            , const CharT opening_brace, const CharT closing_brace) BOOST_NOEXCEPT
        {
            if (begin == end) return false;
            const CharT minus = lcast_char_constants<CharT>::minus;
            const CharT plus = lcast_char_constants<CharT>::plus;
            const int inifinity_size = 8; // == sizeof("infinity") - 1

            /* Parsing +/- */
            bool const has_minus = (*begin == minus);
            if (has_minus || *begin == plus) {
                ++ begin;
            }

            if (end - begin < 3) return false;
            if (lc_iequal(begin, lc_nan, lc_NAN, 3)) {
                begin += 3;
                if (end != begin) {
                    /* It is 'nan(...)' or some bad input*/

                    if (end - begin < 2) return false; // bad input
                    -- end;
                    if (*begin != opening_brace || *end != closing_brace) return false; // bad input
                }

                if( !has_minus ) value = std::numeric_limits<T>::quiet_NaN();
                else value = boost::core::copysign(std::numeric_limits<T>::quiet_NaN(), static_cast<T>(-1));
                return true;
            } else if (
                ( /* 'INF' or 'inf' */
                  end - begin == 3      // 3 == sizeof('inf') - 1
                  && lc_iequal(begin, lc_infinity, lc_INFINITY, 3)
                )
                ||
                ( /* 'INFINITY' or 'infinity' */
                  end - begin == inifinity_size
                  && lc_iequal(begin, lc_infinity, lc_INFINITY, inifinity_size)
                )
             )
            {
                if( !has_minus ) value = std::numeric_limits<T>::infinity();
                else value = -std::numeric_limits<T>::infinity();
                return true;
            }

            return false;
        }

        template <class CharT, class T>
        bool put_inf_nan_impl(CharT* begin, CharT*& end, const T& value
                         , const CharT* lc_nan
                         , const CharT* lc_infinity) BOOST_NOEXCEPT
        {
            const CharT minus = lcast_char_constants<CharT>::minus;
            if (boost::core::isnan(value)) {
                if (boost::core::signbit(value)) {
                    *begin = minus;
                    ++ begin;
                }

                std::memcpy(begin, lc_nan, 3 * sizeof(CharT));
                end = begin + 3;
                return true;
            } else if (boost::core::isinf(value)) {
                if (boost::core::signbit(value)) {
                    *begin = minus;
                    ++ begin;
                }

                std::memcpy(begin, lc_infinity, 3 * sizeof(CharT));
                end = begin + 3;
                return true;
            }

            return false;
        }


#ifndef BOOST_LCAST_NO_WCHAR_T
        template <class T>
        bool parse_inf_nan(const wchar_t* begin, const wchar_t* end, T& value) BOOST_NOEXCEPT {
            return parse_inf_nan_impl(begin, end, value
                               , L"NAN", L"nan"
                               , L"INFINITY", L"infinity"
                               , L'(', L')');
        }

        template <class T>
        bool put_inf_nan(wchar_t* begin, wchar_t*& end, const T& value) BOOST_NOEXCEPT {
            return put_inf_nan_impl(begin, end, value, L"nan", L"infinity");
        }

#endif
#if !defined(BOOST_NO_CXX11_CHAR16_T) && !defined(BOOST_NO_CXX11_UNICODE_LITERALS)
        template <class T>
        bool parse_inf_nan(const char16_t* begin, const char16_t* end, T& value) BOOST_NOEXCEPT {
            return parse_inf_nan_impl(begin, end, value
                               , u"NAN", u"nan"
                               , u"INFINITY", u"infinity"
                               , u'(', u')');
        }

        template <class T>
        bool put_inf_nan(char16_t* begin, char16_t*& end, const T& value) BOOST_NOEXCEPT {
            return put_inf_nan_impl(begin, end, value, u"nan", u"infinity");
        }
#endif
#if !defined(BOOST_NO_CXX11_CHAR32_T) && !defined(BOOST_NO_CXX11_UNICODE_LITERALS)
        template <class T>
        bool parse_inf_nan(const char32_t* begin, const char32_t* end, T& value) BOOST_NOEXCEPT {
            return parse_inf_nan_impl(begin, end, value
                               , U"NAN", U"nan"
                               , U"INFINITY", U"infinity"
                               , U'(', U')');
        }

        template <class T>
        bool put_inf_nan(char32_t* begin, char32_t*& end, const T& value) BOOST_NOEXCEPT {
            return put_inf_nan_impl(begin, end, value, U"nan", U"infinity");
        }
#endif

        template <class CharT, class T>
        bool parse_inf_nan(const CharT* begin, const CharT* end, T& value) BOOST_NOEXCEPT {
            return parse_inf_nan_impl(begin, end, value
                               , "NAN", "nan"
                               , "INFINITY", "infinity"
                               , '(', ')');
        }

        template <class CharT, class T>
        bool put_inf_nan(CharT* begin, CharT*& end, const T& value) BOOST_NOEXCEPT {
            return put_inf_nan_impl(begin, end, value, "nan", "infinity");
        }
    }
} // namespace boost

#undef BOOST_LCAST_NO_WCHAR_T

#endif // BOOST_LEXICAL_CAST_DETAIL_INF_NAN_HPP

