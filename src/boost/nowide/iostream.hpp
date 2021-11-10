//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//  Copyright (c) 2020-2021 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_NOWIDE_IOSTREAM_HPP_INCLUDED
#define BOOST_NOWIDE_IOSTREAM_HPP_INCLUDED

#include <boost/nowide/config.hpp>
#ifdef BOOST_WINDOWS
#include <istream>
#include <memory>
#include <ostream>

#include <boost/config/abi_prefix.hpp> // must be the last #include
#else
#include <iostream>
#endif

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace boost {
namespace nowide {
#if !defined(BOOST_WINDOWS) && !defined(BOOST_NOWIDE_DOXYGEN)
    using std::cout;
    using std::cerr;
    using std::cin;
    using std::clog;
#else

    /// \cond INTERNAL
    namespace detail {
        class console_output_buffer;
        class console_input_buffer;

        class BOOST_NOWIDE_DECL winconsole_ostream : public std::ostream
        {
        public:
            winconsole_ostream(int fd, winconsole_ostream* tieStream);
            ~winconsole_ostream();

        private:
            std::unique_ptr<console_output_buffer> d;
            // Ensure the std streams are initialized and alive during the lifetime of this instance
            std::ios_base::Init init_;
        };

        class BOOST_NOWIDE_DECL winconsole_istream : public std::istream
        {
        public:
            explicit winconsole_istream(winconsole_ostream* tieStream);
            ~winconsole_istream();

        private:
            std::unique_ptr<console_input_buffer> d;
            // Ensure the std streams are initialized and alive during the lifetime of this instance
            std::ios_base::Init init_;
        };
    } // namespace detail

    /// \endcond

    ///
    /// \brief Same as std::cin, but uses UTF-8
    ///
    /// Note, the stream is not synchronized with stdio and not affected by std::ios::sync_with_stdio
    ///
    extern BOOST_NOWIDE_DECL detail::winconsole_istream cin;
    ///
    /// \brief Same as std::cout, but uses UTF-8
    ///
    /// Note, the stream is not synchronized with stdio and not affected by std::ios::sync_with_stdio
    ///
    extern BOOST_NOWIDE_DECL detail::winconsole_ostream cout;
    ///
    /// \brief Same as std::cerr, but uses UTF-8
    ///
    /// Note, the stream is not synchronized with stdio and not affected by std::ios::sync_with_stdio
    ///
    extern BOOST_NOWIDE_DECL detail::winconsole_ostream cerr;
    ///
    /// \brief Same as std::clog, but uses UTF-8
    ///
    /// Note, the stream is not synchronized with stdio and not affected by std::ios::sync_with_stdio
    ///
    extern BOOST_NOWIDE_DECL detail::winconsole_ostream clog;

#endif

} // namespace nowide
} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#ifdef BOOST_WINDOWS
#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas
#endif

#endif
