//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_BOUNDARY_FACETS_HPP_INCLUDED
#define BOOST_LOCALE_BOUNDARY_FACETS_HPP_INCLUDED

#include <boost/locale/config.hpp>
#include <boost/locale/boundary/types.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <locale>
#include <vector>




namespace boost {

    namespace locale {
        
        ///
        /// \brief This namespae contains all operations required for boundary analysis of text
        ///
        namespace boundary {
            ///
            /// \addtogroup boundary
            ///
            /// @{
            ///

            
            ///
            /// \brief This structure is used for representing boundary point
            /// that follows the offset.
            ///
            struct break_info {

                ///
                /// Create empty break point at beginning
                ///
                break_info() : 
                    offset(0),
                    rule(0)
                {
                }
                ///
                /// Create empty break point at offset v.
                /// it is useful for order comparison with other points.
                ///
                break_info(size_t v) :
                    offset(v),
                    rule(0)
                {
                }

                ///
                /// Offset from the beggining of the text where a break occurs.
                ///
                size_t offset;
                ///
                /// The identification of this break point according to 
                /// various break types
                ///
                rule_type rule;
               
                ///
                /// Compare two break points' offset. Allows to search with
                /// standard algorithms over the index.
                ///
                bool operator<(break_info const &other) const
                {
                    return offset < other.offset;
                }
            };
            
            ///
            /// This type holds the analysis of the text - all its break points
            /// with marks
            ///
            typedef std::vector<break_info> index_type;


            template<typename CharType>
            class boundary_indexing;

            #ifdef BOOST_LOCALE_DOXYGEN
            ///
            /// \brief This facet generates an index for boundary analysis
            /// for a given text.
            ///
            /// It is specialized for 4 types of characters \c char_t, \c wchar_t, \c char16_t and \c char32_t
            ///
            template<typename Char>
            class BOOST_LOCALE_DECL boundary_indexing : public std::locale::facet {
            public:
                ///
                /// Default constructor typical for facets
                ///
                boundary_indexing(size_t refs=0) : std::locale::facet(refs)
                {
                }
                ///
                /// Create index for boundary type \a t for text in range [begin,end)
                ///
                /// The returned value is an index of type \ref index_type. Note that this
                /// index is never empty, even if the range [begin,end) is empty it consists
                /// of at least one boundary point with the offset 0.
                ///
                virtual index_type map(boundary_type t,Char const *begin,Char const *end) const = 0;
                ///
                /// Identification of this facet
                ///
                static std::locale::id id;
                
                #if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
                std::locale::id& __get_id (void) const { return id; }
                #endif
            };

            #else

            template<>
            class BOOST_LOCALE_DECL boundary_indexing<char> : public std::locale::facet {
            public:
                boundary_indexing(size_t refs=0) : std::locale::facet(refs)
                {
                }
                virtual index_type map(boundary_type t,char const *begin,char const *end) const = 0;
                static std::locale::id id;
                #if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
                std::locale::id& __get_id (void) const { return id; }
                #endif
            };
            
            template<>
            class BOOST_LOCALE_DECL boundary_indexing<wchar_t> : public std::locale::facet {
            public:
                boundary_indexing(size_t refs=0) : std::locale::facet(refs)
                {
                }
                virtual index_type map(boundary_type t,wchar_t const *begin,wchar_t const *end) const = 0;

                static std::locale::id id;
                #if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
                std::locale::id& __get_id (void) const { return id; }
                #endif
            };
            
            #ifdef BOOST_LOCALE_ENABLE_CHAR16_T
            template<>
            class BOOST_LOCALE_DECL boundary_indexing<char16_t> : public std::locale::facet {
            public:
                boundary_indexing(size_t refs=0) : std::locale::facet(refs)
                {
                }
                virtual index_type map(boundary_type t,char16_t const *begin,char16_t const *end) const = 0;
                static std::locale::id id;
                #if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
                std::locale::id& __get_id (void) const { return id; }
                #endif
            };
            #endif
            
            #ifdef BOOST_LOCALE_ENABLE_CHAR32_T
            template<>
            class BOOST_LOCALE_DECL boundary_indexing<char32_t> : public std::locale::facet {
            public:
                boundary_indexing(size_t refs=0) : std::locale::facet(refs)
                {
                }
                virtual index_type map(boundary_type t,char32_t const *begin,char32_t const *end) const = 0;
                static std::locale::id id;
                #if defined (__SUNPRO_CC) && defined (_RWSTD_VER)
                std::locale::id& __get_id (void) const { return id; }
                #endif
            };
            #endif

            #endif

            ///
            /// @}
            ///


        } // boundary

    } // locale
} // boost


#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
