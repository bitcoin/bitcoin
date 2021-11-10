//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_LOCALIZATION_BACKEND_HPP
#define BOOST_LOCALE_LOCALIZATION_BACKEND_HPP
#include <boost/locale/config.hpp>
#include <boost/locale/generator.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif
#include <string>
#include <locale>
#include <vector>
#include <memory>
#include <boost/locale/hold_ptr.hpp>

namespace boost {
    namespace locale {

        ///
        /// \brief this class represents a localization backend that can be used for localizing your application.
        /// 
        /// Backends are usually registered inside the localization backends manager and allow transparent support
        /// of different backends, so a user can switch the backend by simply linking the application to the correct one.
        ///
        /// Backends may support different tuning options, but these are the default options available to the user
        /// for all of them
        ///
        /// -# \c locale - the name of the locale in POSIX format like en_US.UTF-8
        /// -# \c use_ansi_encoding - select system locale using ANSI codepages rather then UTF-8 under Windows
        ///     by default
        /// -# \c message_path - path to the location of message catalogs (vector of strings)
        /// -# \c message_application - the name of applications that use message catalogs (vector of strings)
        /// 
        /// Each backend can be installed with a different default priotiry so when you work with two different backends, you
        /// can specify priotiry so this backend will be chosen according to their priority.
        ///
        
        class localization_backend {
            localization_backend(localization_backend const &);
            void operator=(localization_backend const &);
        public:

            localization_backend()
            {
            }
            
            virtual ~localization_backend()
            {
            }

            ///
            /// Make a polymorphic copy of the backend
            ///
            virtual localization_backend *clone() const = 0;

            ///
            /// Set option for backend, for example "locale" or "encoding"
            ///
            virtual void set_option(std::string const &name,std::string const &value) = 0;

            ///
            /// Clear all options
            ///
            virtual void clear_options() = 0;

            ///
            /// Create a facet for category \a category and character type \a type 
            ///
            virtual std::locale install(std::locale const &base,locale_category_type category,character_facet_type type = nochar_facet) = 0;

        }; // localization_backend 


        ///
        /// \brief Localization backend manager is a class that holds various backend and allows creation
        /// of their combination or selection
        ///

        class BOOST_LOCALE_DECL localization_backend_manager {
        public:
            ///
            /// New empty localization_backend_manager 
            ///
            localization_backend_manager();
            ///
            /// Copy localization_backend_manager 
            ///
            localization_backend_manager(localization_backend_manager const &);
            ///
            /// Assign localization_backend_manager 
            ///
            localization_backend_manager const &operator=(localization_backend_manager const &);

            ///
            /// Destructor
            ///
            ~localization_backend_manager();

            #if !defined(BOOST_LOCALE_HIDE_AUTO_PTR) && !defined(BOOST_NO_AUTO_PTR)
            ///
            /// Create new localization backend according to current settings.
            ///
            std::auto_ptr<localization_backend> get() const;

            ///
            /// Add new backend to the manager, each backend should be uniquely defined by its name.
            ///
            /// This library provides: "icu", "posix", "winapi" and "std" backends.
            ///
            void add_backend(std::string const &name,std::auto_ptr<localization_backend> backend);
            #endif

            ///
            /// Create new localization backend according to current settings. Ownership is passed to caller
            ///
            localization_backend *create() const;
            ///
            /// Add new backend to the manager, each backend should be uniquely defined by its name.
            /// ownership on backend is transfered
            ///
            /// This library provides: "icu", "posix", "winapi" and "std" backends.
            ///
            void adopt_backend(std::string const &name,localization_backend *backend);
            #ifndef BOOST_NO_CXX11_SMART_PTR
            ///
            /// Create new localization backend according to current settings.
            ///
            std::unique_ptr<localization_backend> get_unique_ptr() const;

            ///
            /// Add new backend to the manager, each backend should be uniquely defined by its name.
            ///
            /// This library provides: "icu", "posix", "winapi" and "std" backends.
            ///
            void add_backend(std::string const &name,std::unique_ptr<localization_backend> backend);
            #endif

            ///
            /// Clear backend
            ///
            void remove_all_backends();
            
            ///
            /// Get list of all available backends
            ///
            std::vector<std::string> get_all_backends() const;
            
            ///
            /// Select specific backend by name for a category \a category. It allows combining different
            /// backends for user preferences.
            ///
            void select(std::string const &backend_name,locale_category_type category = all_categories);
           
            ///
            /// Set new global backend manager, the old one is returned.
            ///
            /// This function is thread safe
            /// 
            static localization_backend_manager global(localization_backend_manager const &);
            ///
            /// Get global backend manager
            ///
            /// This function is thread safe
            /// 
            static localization_backend_manager global();
        private:
            class impl;
            hold_ptr<impl> pimpl_;
        };

    } // locale
} // boost


#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4 

