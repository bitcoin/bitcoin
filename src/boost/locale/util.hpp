//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_UTIL_HPP
#define BOOST_LOCALE_UTIL_HPP
#include <locale>
#include <typeinfo>
#include <boost/cstdint.hpp>
#include <boost/locale/utf.hpp>
#include <boost/locale/generator.hpp>
#include <boost/assert.hpp>

#include <vector>
namespace boost {
namespace locale {
///
/// \brief This namespace provides various utility function useful for Boost.Locale backends
/// implementations
///
namespace util {
    
    ///
    /// \brief Return default system locale name in POSIX format.
    ///
    /// This function tries to detect the locale using, LC_CTYPE, LC_ALL and LANG environment
    /// variables in this order and if all of them unset, in POSIX platforms it returns "C"
    /// 
    /// On Windows additionally to check the above environment variables, this function
    /// tries to creates locale name from ISO-339 and ISO-3199 country codes defined
    /// for user default locale.
    /// If \a use_utf8_on_windows is true it sets the encoding to UTF-8, otherwise, if system
    /// locale supports ANSI code-page it defines the ANSI encoding like windows-1252, otherwise it fall-backs
    /// to UTF-8 encoding if ANSI code-page is not available.
    ///
    BOOST_LOCALE_DECL
    std::string get_system_locale(bool use_utf8_on_windows = false);

    ///
    /// \brief Installs information facet to locale in based on locale name \a name
    ///
    /// This function installs boost::locale::info facet into the locale \a in and returns
    /// newly created locale.
    ///
    /// Note: all information is based only on parsing of string \a name;
    ///
    /// The name has following format: language[_COUNTRY][.encoding][\@variant]
    /// Where language is ISO-639 language code like "en" or "ru", COUNTRY is ISO-3166
    /// country identifier like "US" or "RU". the Encoding is a charracter set name
    /// like UTF-8 or ISO-8859-1. Variant is backend specific variant like \c euro or
    /// calendar=hebrew.
    ///
    /// If some parameters are missing they are specified as blanks, default encoding
    /// is assumed to be US-ASCII and missing language is assumed to be "C"
    ///
    BOOST_LOCALE_DECL
    std::locale create_info(std::locale const &in,std::string const &name); 


    ///
    /// \brief This class represent a simple stateless converter from UCS-4 and to UCS-4 for
    ///  each single code point
    ///
    /// This class is used for creation of std::codecvt facet for converting utf-16/utf-32 encoding
    /// to encoding supported by this converter
    ///
    /// Please note, this converter should be fully stateless. Fully stateless means it should
    /// never assume that it is called in any specific order on the text. Even if the
    /// encoding itself seems to be stateless like windows-1255 or shift-jis, some
    /// encoders (most notably iconv) can actually compose several code-point into one or
    /// decompose them in case composite characters are found. So be very careful when implementing
    /// these converters for certain character set.
    ///
    class base_converter {
    public:

        ///
        /// This value should be returned when an illegal input sequence or code-point is observed:
        /// For example if a UCS-32 code-point is in the range reserved for UTF-16 surrogates
        /// or an invalid UTF-8 sequence is found
        ///
        static const uint32_t illegal=utf::illegal;

        ///
        /// This value is returned in following cases: The of incomplete input sequence was found or 
        /// insufficient output buffer was provided so complete output could not be written.
        ///
        static const uint32_t incomplete=utf::incomplete;
        
        virtual ~base_converter() 
        {
        }
        ///
        /// Return the maximal length that one Unicode code-point can be converted to, for example
        /// for UTF-8 it is 4, for Shift-JIS it is 2 and ISO-8859-1 is 1
        ///
        virtual int max_len() const 
        {
            return 1;
        }
        ///
        /// Returns true if calling the functions from_unicode, to_unicode, and max_len is thread safe.
        ///
        /// Rule of thumb: if this class' implementation uses simple tables that are unchanged
        /// or is purely algorithmic like UTF-8 - so it does not share any mutable bit for
        /// independent to_unicode, from_unicode calls, you may set it to true, otherwise,
        /// for example if you use iconv_t descriptor or UConverter as conversion object return false,
        /// and this object will be cloned for each use.
        ///
        virtual bool is_thread_safe() const 
        {
            return false;
        }
        ///
        /// Create a polymorphic copy of this object, usually called only if is_thread_safe() return false
        ///
        virtual base_converter *clone() const 
        {
            BOOST_ASSERT(typeid(*this)==typeid(base_converter));
            return new base_converter();
        }

        ///
        /// Convert a single character starting at begin and ending at most at end to Unicode code-point.
        ///
        /// if valid input sequence found in [\a begin,\a code_point_end) such as \a begin < \a code_point_end && \a code_point_end <= \a end
        /// it is converted to its Unicode code point equivalent, \a begin is set to \a code_point_end
        ///
        /// if incomplete input sequence found in [\a begin,\a end), i.e. there my be such \a code_point_end that \a code_point_end > \a end
        /// and [\a begin, \a code_point_end) would be valid input sequence, then \a incomplete is returned begin stays unchanged, for example
        /// for UTF-8 conversion a *begin = 0xc2, \a begin +1 = \a end is such situation.
        ///
        /// if invalid input sequence found, i.e. there is a sequence [\a begin, \a code_point_end) such as \a code_point_end <= \a end
        /// that is illegal for this encoding, \a illegal is returned and begin stays unchanged. For example if *begin = 0xFF and begin < end
        /// for UTF-8, then \a illegal is returned.
        /// 
        ///
        virtual uint32_t to_unicode(char const *&begin,char const *end) 
        {
            if(begin == end)
                return incomplete;
            unsigned char cp = *begin;
            if(cp <= 0x7F) {
                begin++;
                return cp;
            }
            return illegal;
        }
        ///
        /// Convert a single code-point \a u into encoding and store it in [begin,end) range.
        ///
        /// If u is invalid Unicode code-point, or it can not be mapped correctly to represented character set,
        /// \a illegal should be returned
        ///
        /// If u can be converted to a sequence of bytes c1, ... , cN (1<= N <= max_len() ) then
        /// 
        /// -# If end - begin >= N, c1, ... cN are written starting at begin and N is returned
        /// -# If end - begin < N, incomplete is returned, it is unspecified what would be
        ///    stored in bytes in range [begin,end)

        virtual uint32_t from_unicode(uint32_t u,char *begin,char const *end) 
        {
            if(begin==end)
                return incomplete;
            if(u >= 0x80)
                return illegal;
            *begin = static_cast<char>(u);
            return 1;
        }
    };

    #if !defined(BOOST_LOCALE_HIDE_AUTO_PTR) && !defined(BOOST_NO_AUTO_PTR)
    ///
    /// This function creates a \a base_converter that can be used for conversion between UTF-8 and
    /// unicode code points
    ///
    BOOST_LOCALE_DECL std::auto_ptr<base_converter> create_utf8_converter();
    ///
    /// This function creates a \a base_converter that can be used for conversion between single byte
    /// character encodings like ISO-8859-1, koi8-r, windows-1255 and Unicode code points,
    /// 
    /// If \a encoding is not supported, empty pointer is returned. You should check if
    /// std::auto_ptr<base_converter>::get() != 0
    ///
    BOOST_LOCALE_DECL std::auto_ptr<base_converter> create_simple_converter(std::string const &encoding);


    ///
    /// Install codecvt facet into locale \a in and return new locale that is based on \a in and uses new
    /// facet.
    ///
    /// codecvt facet would convert between narrow and wide/char16_t/char32_t encodings using \a cvt converter.
    /// If \a cvt is null pointer, always failure conversion would be used that fails on every first input or output.
    /// 
    /// Note: the codecvt facet handles both UTF-16 and UTF-32 wide encodings, it knows to break and join
    /// Unicode code-points above 0xFFFF to and from surrogate pairs correctly. \a cvt should be unaware
    /// of wide encoding type
    ///
    BOOST_LOCALE_DECL
    std::locale create_codecvt(std::locale const &in,std::auto_ptr<base_converter> cvt,character_facet_type type);
    #endif

    #ifndef BOOST_NO_CXX11_SMART_PTR
    ///
    /// This function creates a \a base_converter that can be used for conversion between UTF-8 and
    /// unicode code points
    ///
    BOOST_LOCALE_DECL std::unique_ptr<base_converter> create_utf8_converter_unique_ptr();
    ///
    /// This function creates a \a base_converter that can be used for conversion between single byte
    /// character encodings like ISO-8859-1, koi8-r, windows-1255 and Unicode code points,
    /// 
    /// If \a encoding is not supported, empty pointer is returned. You should check if
    /// std::unique_ptr<base_converter>::get() != 0
    ///
    BOOST_LOCALE_DECL std::unique_ptr<base_converter> create_simple_converter_unique_ptr(std::string const &encoding);

    ///
    /// Install codecvt facet into locale \a in and return new locale that is based on \a in and uses new
    /// facet.
    ///
    /// codecvt facet would convert between narrow and wide/char16_t/char32_t encodings using \a cvt converter.
    /// If \a cvt is null pointer, always failure conversion would be used that fails on every first input or output.
    /// 
    /// Note: the codecvt facet handles both UTF-16 and UTF-32 wide encodings, it knows to break and join
    /// Unicode code-points above 0xFFFF to and from surrogate pairs correctly. \a cvt should be unaware
    /// of wide encoding type
    ///
    BOOST_LOCALE_DECL
    std::locale create_codecvt(std::locale const &in,std::unique_ptr<base_converter> cvt,character_facet_type type);
    #endif

    ///
    /// This function creates a \a base_converter that can be used for conversion between UTF-8 and
    /// unicode code points
    ///
    BOOST_LOCALE_DECL base_converter *create_utf8_converter_new_ptr();
    ///
    /// This function creates a \a base_converter that can be used for conversion between single byte
    /// character encodings like ISO-8859-1, koi8-r, windows-1255 and Unicode code points,
    /// 
    /// If \a encoding is not supported, empty pointer is returned. You should check if
    /// std::unique_ptr<base_converter>::get() != 0
    ///
    BOOST_LOCALE_DECL base_converter *create_simple_converter_new_ptr(std::string const &encoding);

    ///
    /// Install codecvt facet into locale \a in and return new locale that is based on \a in and uses new
    /// facet.
    ///
    /// codecvt facet would convert between narrow and wide/char16_t/char32_t encodings using \a cvt converter.
    /// If \a cvt is null pointer, always failure conversion would be used that fails on every first input or output.
    /// 
    /// Note: the codecvt facet handles both UTF-16 and UTF-32 wide encodings, it knows to break and join
    /// Unicode code-points above 0xFFFF to and from surrogate pairs correctly. \a cvt should be unaware
    /// of wide encoding type
    ///
    /// ownership of cvt is transfered 
    ///
    BOOST_LOCALE_DECL
    std::locale create_codecvt_from_pointer(std::locale const &in,base_converter *cvt,character_facet_type type);

    /// 
    /// Install utf8 codecvt to UTF-16 or UTF-32 into locale \a in and return
    /// new locale that is based on \a in and uses new facet. 
    /// 
    BOOST_LOCALE_DECL
    std::locale create_utf8_codecvt(std::locale const &in,character_facet_type type);

    ///
    /// This function installs codecvt that can be used for conversion between single byte
    /// character encodings like ISO-8859-1, koi8-r, windows-1255 and Unicode code points,
    /// 
    /// Throws boost::locale::conv::invalid_charset_error if the chacater set is not supported or isn't single byte character
    /// set
    BOOST_LOCALE_DECL
    std::locale create_simple_codecvt(std::locale const &in,std::string const &encoding,character_facet_type type);
} // util
} // locale 
} // boost

#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
