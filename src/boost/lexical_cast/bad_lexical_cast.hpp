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

#ifndef BOOST_LEXICAL_CAST_BAD_LEXICAL_CAST_HPP
#define BOOST_LEXICAL_CAST_BAD_LEXICAL_CAST_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <exception>
#include <typeinfo>
#include <boost/throw_exception.hpp>

namespace boost
{
    // exception used to indicate runtime lexical_cast failure
    class BOOST_SYMBOL_VISIBLE bad_lexical_cast :
    // workaround MSVC bug with std::bad_cast when _HAS_EXCEPTIONS == 0
#if defined(BOOST_MSVC) && defined(_HAS_EXCEPTIONS) && !_HAS_EXCEPTIONS
        public std::exception
#else
        public std::bad_cast
#endif

#if defined(BOOST_BORLANDC) && BOOST_WORKAROUND( BOOST_BORLANDC, < 0x560 )
        // under bcc32 5.5.1 bad_cast doesn't derive from exception
        , public std::exception
#endif

    {
    public:
        bad_lexical_cast() BOOST_NOEXCEPT
#ifndef BOOST_NO_TYPEID
           : source(&typeid(void)), target(&typeid(void))
#endif
        {}

        const char *what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {
            return "bad lexical cast: "
                   "source type value could not be interpreted as target";
        }

        ~bad_lexical_cast() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
        {}

#ifndef BOOST_NO_TYPEID
    private:
#ifdef BOOST_NO_STD_TYPEINFO
        typedef ::type_info type_info_t;
#else
        typedef ::std::type_info type_info_t;
#endif
    public:
        bad_lexical_cast(
                const type_info_t &source_type_arg,
                const type_info_t &target_type_arg) BOOST_NOEXCEPT
            : source(&source_type_arg), target(&target_type_arg)
        {}

        const type_info_t &source_type() const BOOST_NOEXCEPT {
            return *source;
        }

        const type_info_t &target_type() const BOOST_NOEXCEPT {
            return *target;
        }

    private:
        const type_info_t *source;
        const type_info_t *target;
#endif
    };

    namespace conversion { namespace detail {
#ifdef BOOST_NO_TYPEID
        template <class S, class T>
        inline void throw_bad_cast() {
            boost::throw_exception(bad_lexical_cast());
        }
#else
        template <class S, class T>
        inline void throw_bad_cast() {
            boost::throw_exception(bad_lexical_cast(typeid(S), typeid(T)));
        }
#endif
    }} // namespace conversion::detail

} // namespace boost

#endif // BOOST_LEXICAL_CAST_BAD_LEXICAL_CAST_HPP
