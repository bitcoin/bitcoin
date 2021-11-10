//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_GENERATOR_HPP
#define BOOST_LOCALE_GENERATOR_HPP
#include <boost/locale/config.hpp>
#include <boost/cstdint.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <string>
#include <locale>
#include <memory>
#include <boost/locale/hold_ptr.hpp>

namespace boost {

    template<typename Type>
    class shared_ptr;

    ///
    /// \brief This is the main namespace that encloses all localization classes 
    ///
    namespace locale {

        class localization_backend;
        class localization_backend_manager;

        static const uint32_t nochar_facet    = 0;        ///< Unspecified character category for character independent facets
        static const uint32_t char_facet      = 1 << 0;   ///< 8-bit character facets
        static const uint32_t wchar_t_facet   = 1 << 1;   ///< wide character facets
        static const uint32_t char16_t_facet  = 1 << 2;   ///< C++0x char16_t facets
        static const uint32_t char32_t_facet  = 1 << 3;   ///< C++0x char32_t facets

        static const uint32_t character_first_facet = char_facet;  ///< First facet specific for character type
        static const uint32_t character_last_facet = char32_t_facet; ///< Last facet specific for character type
        static const uint32_t all_characters = 0xFFFF;     ///< Special mask -- generate all
        
        typedef uint32_t character_facet_type; ///<type that specifies the character type that locales can be generated for

        static const uint32_t     convert_facet   = 1 << 0;   ///< Generate conversion facets
        static const uint32_t     collation_facet = 1 << 1;   ///< Generate collation facets
        static const uint32_t     formatting_facet= 1 << 2;   ///< Generate numbers, currency, date-time formatting facets
        static const uint32_t     parsing_facet   = 1 << 3;   ///< Generate numbers, currency, date-time formatting facets
        static const uint32_t     message_facet   = 1 << 4;   ///< Generate message facets
        static const uint32_t     codepage_facet  = 1 << 5;   ///< Generate character set conversion facets (derived from std::codecvt)
        static const uint32_t     boundary_facet  = 1 << 6;   ///< Generate boundary analysis facet
            
        static const uint32_t     per_character_facet_first = convert_facet; ///< First facet specific for character
        static const uint32_t     per_character_facet_last = boundary_facet; ///< Last facet specific for character

        static const uint32_t     calendar_facet  = 1 << 16;   ///< Generate boundary analysis facet
        static const uint32_t     information_facet = 1 << 17;   ///< Generate general locale information facet

        static const uint32_t    non_character_facet_first = calendar_facet; ///< First character independent facet 
        static const uint32_t    non_character_facet_last = information_facet;///< Last character independent facet 

            
        static const uint32_t    all_categories  = 0xFFFFFFFFu;   ///< Generate all of them
        
        typedef uint32_t locale_category_type; ///< a type used for more fine grained generation of facets

        ///
        /// \brief the major class used for locale generation
        ///
        /// This class is used for specification of all parameters required for locale generation and
        /// caching. This class const member functions are thread safe if locale class implementation is thread safe.
        ///

        class BOOST_LOCALE_DECL generator {
        public:

            ///
            /// Create new generator using global localization_backend_manager 
            ///
            generator();
            ///
            /// Create new generator using specific localization_backend_manager 
            ///
            generator(localization_backend_manager const &);

            ~generator();

            ///
            /// Set types of facets that should be generated, default all
            ///
            void categories(locale_category_type cats);
            ///
            /// Get types of facets that should be generated, default all
            ///
            locale_category_type categories() const;
            
            ///
            /// Set the characters type for which the facets should be generated, default all supported
            ///
            void characters(character_facet_type chars);
            ///
            /// Get the characters type for which the facets should be generated, default all supported
            ///
            character_facet_type characters() const;

            ///
            /// Add a new domain of messages that would be generated. It should be set in order to enable
            /// messages support.
            ///
            /// Messages domain has following format: "name" or "name/encoding"
            /// where name is the base name of the "mo" file where the catalog is stored
            /// without ".mo" extension. For example for file \c /usr/share/locale/he/LC_MESSAGES/blog.mo
            /// it would be \c blog.
            ///
            /// You can optionally specify the encoding of the keys in the sources by adding "/encoding_name"
            /// For example blog/cp1255.
            ///
            /// If not defined all keys are assumed to be UTF-8 encoded.
            ///
            /// \note When you select a domain for the program using dgettext or message API, you
            /// do not specify the encoding part. So for example if the provided
            /// domain name was "blog/windows-1255" then for translation
            /// you should use dgettext("blog","Hello")
            ///
            ///
            void add_messages_domain(std::string const &domain);
            ///
            /// Set default message domain. If this member was not called, the first added messages domain is used.
            /// If the domain \a domain is not added yet it is added.
            ///
            void set_default_messages_domain(std::string const &domain);

            ///
            /// Remove all added domains from the list
            ///
            void clear_domains();

            ///
            /// Add a search path where dictionaries are looked in.
            ///
            /// \note
            ///
            /// - Under the Windows platform the path is treated as a path in the locale's encoding so
            ///   if you create locale "en_US.windows-1251" then path would be treated as cp1255,
            ///   and if it is en_US.UTF-8 it is treated as UTF-8. File name is always opened with 
            ///   a wide file name as wide file names are the native file name on Windows.
            ///
            /// - Under POSIX platforms all paths passed as-is regardless of encoding as narrow
            ///   encodings are the native encodings for POSIX platforms.
            ///   
            /// 
            void add_messages_path(std::string const &path);

            ///
            /// Remove all added paths
            ///
            void clear_paths();

            ///
            /// Remove all cached locales
            ///
            void clear_cache();

            ///
            /// Turn locale caching ON
            ///
            void locale_cache_enabled(bool on);

            ///
            /// Get locale cache option
            ///
            bool locale_cache_enabled() const;

            ///
            /// Check if by default ANSI encoding is selected or UTF-8 onces. The default is false.
            ///
            bool use_ansi_encoding() const;

            ///
            /// Select ANSI encodings as default system encoding rather then UTF-8 by default
            /// under Windows.
            ///
            /// The default is the most portable and most powerful encoding, UTF-8, but the user
            /// can select "system" one if dealing with legacy applications
            ///
            void use_ansi_encoding(bool enc);

            ///
            /// Generate a locale with id \a id
            ///
            std::locale generate(std::string const &id) const;
            ///
            /// Generate a locale with id \a id. Use \a base as a locale to which all facets are added,
            /// instead of std::locale::classic().
            ///
            std::locale generate(std::locale const &base,std::string const &id) const;
            ///
            /// Shortcut to generate(id)
            ///
            std::locale operator()(std::string const &id) const
            {
                return generate(id);
            }
            
            ///
            /// Set backend specific option
            ///
            void set_option(std::string const &name,std::string const &value);

            ///
            /// Clear backend specific options
            ///
            void clear_options();

        private:

            void set_all_options(shared_ptr<localization_backend> backend,std::string const &id) const;

            generator(generator const &);
            void operator=(generator const &);

            struct data;
            hold_ptr<data> d;
        };

    }
}
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4 

