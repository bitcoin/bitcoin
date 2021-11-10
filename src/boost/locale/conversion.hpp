//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_CONVERTER_HPP_INCLUDED
#define BOOST_LOCALE_CONVERTER_HPP_INCLUDED

#include <boost/locale/config.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <locale>


namespace boost {
    namespace locale {
        
        ///
        /// \defgroup convert Text Conversions 
        ///
        ///  This module provides various function for string manipulation like Unicode normalization, case conversion etc.
        /// @{
        ///

        
        ///
        /// \brief This class provides base flags for text manipulation. It is used as base for converter facet.
        ///
        class converter_base {
        public:
            ///
            /// The flag used for facet - the type of operation to perform
            ///
            typedef enum {
                normalization,  ///< Apply Unicode normalization on the text
                upper_case,     ///< Convert text to upper case
                lower_case,     ///< Convert text to lower case
                case_folding,   ///< Fold case in the text
                title_case      ///< Convert text to title case
            } conversion_type;
        };

        template<typename CharType>
        class converter;

        #ifdef BOOST_LOCALE_DOXYGEN
        ///
        /// \brief The facet that implements text manipulation
        ///
        /// It is used to performs text conversion operations defined by \ref conversion_type. It is specialized
        /// for four types of characters \c char, \c wchar_t, \c char16_t, \c char32_t
        ///
        template<typename Char>
        class BOOST_LOCALE_DECL converter: public converter_base, public std::locale::facet {
        public:
            /// Locale identification
            static std::locale::id id;

            /// Standard constructor
            converter(size_t refs = 0) : std::locale::facet(refs)
            {
            }
            ///
            /// Convert text in range [\a begin, \a end) according to conversion method \a how. Parameter
            /// \a flags is used for specification of normalization method like nfd, nfc etc.
            ///
            virtual std::basic_string<Char> convert(conversion_type how,Char const *begin,Char const *end,int flags = 0) const = 0;
#if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
            std::locale::id& __get_id (void) const { return id; }
#endif
        };
        #else

        template<>
        class BOOST_LOCALE_DECL converter<char> : public converter_base, public std::locale::facet {
        public:
            static std::locale::id id;

            converter(size_t refs = 0) : std::locale::facet(refs)
            {
            }
            virtual std::string convert(conversion_type how,char const *begin,char const *end,int flags = 0) const = 0;
#if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
            std::locale::id& __get_id (void) const { return id; }
#endif
        };

        template<>
        class BOOST_LOCALE_DECL converter<wchar_t> : public converter_base, public std::locale::facet {
        public:
            static std::locale::id id;
            converter(size_t refs = 0) : std::locale::facet(refs)
            {
            }
             virtual std::wstring convert(conversion_type how,wchar_t const *begin,wchar_t const *end,int flags = 0) const = 0;
#if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
            std::locale::id& __get_id (void) const { return id; }
#endif
        };

        #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
        template<>
        class BOOST_LOCALE_DECL converter<char16_t> : public converter_base, public std::locale::facet {
        public:
            static std::locale::id id;
            converter(size_t refs = 0) : std::locale::facet(refs)
            {
            }
            virtual std::u16string convert(conversion_type how,char16_t const *begin,char16_t const *end,int flags = 0) const = 0; 
#if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
            std::locale::id& __get_id (void) const { return id; }
#endif
        };
        #endif

        #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
        template<>
        class BOOST_LOCALE_DECL converter<char32_t> : public converter_base, public std::locale::facet {
        public:
            static std::locale::id id;
            converter(size_t refs = 0) : std::locale::facet(refs)
            {
            }
            virtual std::u32string convert(conversion_type how,char32_t const *begin,char32_t const *end,int flags = 0) const = 0;
#if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
            std::locale::id& __get_id (void) const { return id; }
#endif
        };
        #endif

        #endif

        ///
        /// The type that defined <a href="http://unicode.org/reports/tr15/#Norm_Forms">normalization form</a>
        ///

        typedef enum {
            norm_nfd,   ///< Canonical decomposition
            norm_nfc,   ///< Canonical decomposition followed by canonical composition
            norm_nfkd,  ///< Compatibility decomposition
            norm_nfkc,  ///< Compatibility decomposition followed by canonical composition.
            norm_default = norm_nfc, ///< Default normalization - canonical decomposition followed by canonical composition
        } norm_type;
       
        ///
        /// Normalize Unicode string \a str according to \ref norm_type "normalization form" \a n
        ///
        /// Note: This function receives only Unicode strings, i.e.: UTF-8, UTF-16 or UTF-32. It does not take
        /// in account the locale encoding, because Unicode decomposition and composition are meaningless outside 
        /// of a Unicode character set.
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> normalize(std::basic_string<CharType> const &str,norm_type n=norm_default,std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::normalization,str.data(),str.data() + str.size(),n);
        }

        ///
        /// Normalize NUL terminated Unicode string \a str according to \ref norm_type "normalization form" \a n
        ///
        /// Note: This function receives only Unicode strings, i.e.: UTF-8, UTF-16 or UTF-32. It does not take
        /// in account the locale encoding, because Unicode decomposition and composition are meaningless outside 
        /// of a Unicode character set.
        /// 
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> normalize(CharType const *str,norm_type n=norm_default,std::locale const &loc=std::locale())
        {
            CharType const *end=str;
            while(*end)
                end++;
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::normalization,str,end,n);
        }
        
        ///
        /// Normalize Unicode string in range [begin,end) according to \ref norm_type "normalization form" \a n
        ///
        /// Note: This function receives only Unicode strings, i.e.: UTF-8, UTF-16 or UTF-32. It does not take
        /// in account the locale encoding, because Unicode decomposition and composition are meaningless outside 
        /// of a Unicode character set.
        /// 
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> normalize(  CharType const *begin,
                                                CharType const *end,
                                                norm_type n=norm_default,
                                                std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::normalization,begin,end,n);
        }

        ///////////////////////////////////////////////////
        
        ///
        /// Convert a string \a str to upper case according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 

        template<typename CharType>
        std::basic_string<CharType> to_upper(std::basic_string<CharType> const &str,std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::upper_case,str.data(),str.data()+str.size());
        }
        
        ///
        /// Convert a NUL terminated string \a str to upper case according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> to_upper(CharType const *str,std::locale const &loc=std::locale())
        {
            CharType const *end=str;
            while(*end)
                end++;
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::upper_case,str,end);
        }
        
        ///
        /// Convert a string in range [begin,end) to upper case according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> to_upper(CharType const *begin,CharType const *end,std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::upper_case,begin,end);
        }

        ///////////////////////////////////////////////////
        
        ///
        /// Convert a string \a str to lower case according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 

        template<typename CharType>
        std::basic_string<CharType> to_lower(std::basic_string<CharType> const &str,std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::lower_case,str.data(),str.data()+str.size());
        }
        
        ///
        /// Convert a NUL terminated string \a str to lower case according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> to_lower(CharType const *str,std::locale const &loc=std::locale())
        {
            CharType const *end=str;
            while(*end)
                end++;
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::lower_case,str,end);
        }
        
        ///
        /// Convert a string in range [begin,end) to lower case according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> to_lower(CharType const *begin,CharType const *end,std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::lower_case,begin,end);
        }
        ///////////////////////////////////////////////////
        
        ///
        /// Convert a string \a str to title case according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 

        template<typename CharType>
        std::basic_string<CharType> to_title(std::basic_string<CharType> const &str,std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::title_case,str.data(),str.data()+str.size());
        }
        
        ///
        /// Convert a NUL terminated string \a str to title case according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> to_title(CharType const *str,std::locale const &loc=std::locale())
        {
            CharType const *end=str;
            while(*end)
                end++;
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::title_case,str,end);
        }
        
        ///
        /// Convert a string in range [begin,end) to title case according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> to_title(CharType const *begin,CharType const *end,std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::title_case,begin,end);
        }

        ///////////////////////////////////////////////////
        
        ///
        /// Fold case of a string \a str according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 

        template<typename CharType>
        std::basic_string<CharType> fold_case(std::basic_string<CharType> const &str,std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::case_folding,str.data(),str.data()+str.size());
        }
        
        ///
        /// Fold case of a NUL terminated string \a str according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> fold_case(CharType const *str,std::locale const &loc=std::locale())
        {
            CharType const *end=str;
            while(*end)
                end++;
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::case_folding,str,end);
        }
        
        ///
        /// Fold case of a string in range [begin,end) according to locale \a loc
        ///
        /// \note throws std::bad_cast if loc does not have \ref converter facet installed
        /// 
        template<typename CharType>
        std::basic_string<CharType> fold_case(CharType const *begin,CharType const *end,std::locale const &loc=std::locale())
        {
            return std::use_facet<converter<CharType> >(loc).convert(converter_base::case_folding,begin,end);
        }

        ///
        ///@}
        ///
    } // locale

} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


#endif

///
/// \example conversions.cpp
///
/// Example of using various text conversion functions.
///
/// \example wconversions.cpp
///
/// Example of using various text conversion functions with wide strings.
///

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

