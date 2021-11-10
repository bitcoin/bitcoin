/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters/c_decorator.hpp
 * \author Andrey Semashev
 * \date   18.11.2012
 *
 * The header contains implementation of C-style character decorators.
 */

#ifndef BOOST_LOG_EXPRESSIONS_FORMATTERS_C_DECORATOR_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_FORMATTERS_C_DECORATOR_HPP_INCLUDED_

#include <limits>
#include <boost/range/iterator_range_core.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/snprintf.hpp>
#include <boost/log/expressions/formatters/char_decorator.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

template< typename >
struct c_decorator_traits;

#ifdef BOOST_LOG_USE_CHAR
template< >
struct c_decorator_traits< char >
{
    static boost::iterator_range< const char* const* > get_patterns()
    {
        static const char* const patterns[] =
        {
            "\\", "\a", "\b", "\f", "\n", "\r", "\t", "\v", "'", "\"", "?"
        };
        return boost::make_iterator_range(patterns);
    }
    static boost::iterator_range< const char* const* > get_replacements()
    {
        static const char* const replacements[] =
        {
            "\\\\", "\\a", "\\b", "\\f", "\\n", "\\r", "\\t", "\\v", "\\'", "\\\"", "\\?"
        };
        return boost::make_iterator_range(replacements);
    }
    template< unsigned int N >
    static std::size_t print_escaped(char (&buf)[N], char c)
    {
        int n = boost::log::aux::snprintf(buf, N, "\\x%.2X", static_cast< unsigned int >(static_cast< uint8_t >(c)));
        if (n < 0)
        {
            n = 0;
            buf[0] = '\0';
        }
        return static_cast< unsigned int >(n) >= N ? N - 1 : static_cast< unsigned int >(n);
    }
};
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
template< >
struct c_decorator_traits< wchar_t >
{
    static boost::iterator_range< const wchar_t* const* > get_patterns()
    {
        static const wchar_t* const patterns[] =
        {
            L"\\", L"\a", L"\b", L"\f", L"\n", L"\r", L"\t", L"\v", L"'", L"\"", L"?"
        };
        return boost::make_iterator_range(patterns);
    }
    static boost::iterator_range< const wchar_t* const* > get_replacements()
    {
        static const wchar_t* const replacements[] =
        {
            L"\\\\", L"\\a", L"\\b", L"\\f", L"\\n", L"\\r", L"\\t", L"\\v", L"\\'", L"\\\"", L"\\?"
        };
        return boost::make_iterator_range(replacements);
    }
    template< unsigned int N >
    static std::size_t print_escaped(wchar_t (&buf)[N], wchar_t c)
    {
        const wchar_t* format;
        unsigned int val;
        if (sizeof(wchar_t) == 1)
        {
            format = L"\\x%.2X";
            val = static_cast< uint8_t >(c);
        }
        else if (sizeof(wchar_t) == 2)
        {
            format = L"\\x%.4X";
            val = static_cast< uint16_t >(c);
        }
        else
        {
            format = L"\\x%.8X";
            val = static_cast< uint32_t >(c);
        }

        int n = boost::log::aux::swprintf(buf, N, format, val);
        if (n < 0)
        {
            n = 0;
            buf[0] = L'\0';
        }
        return static_cast< unsigned int >(n) >= N ? N - 1 : static_cast< unsigned int >(n);
    }
};
#endif // BOOST_LOG_USE_WCHAR_T

template< typename CharT >
struct c_decorator_gen
{
    typedef CharT char_type;

    template< typename SubactorT >
    BOOST_FORCEINLINE char_decorator_actor< SubactorT, pattern_replacer< char_type > > operator[] (SubactorT const& subactor) const
    {
        typedef c_decorator_traits< char_type > traits_type;
        typedef pattern_replacer< char_type > replacer_type;
        typedef char_decorator_actor< SubactorT, replacer_type > result_type;
        typedef typename result_type::terminal_type terminal_type;
        typename result_type::base_type act = {{ terminal_type(subactor, replacer_type(traits_type::get_patterns(), traits_type::get_replacements())) }};
        return result_type(act);
    }
};

} // namespace aux

/*!
 * C-style decorator generator object. The decorator replaces characters with specific meaning in C
 * language with the corresponding escape sequences. The generator provides <tt>operator[]</tt> that
 * can be used to construct the actual decorator. For example:
 *
 * <code>
 * c_decor[ stream << attr< std::string >("MyAttr") ]
 * </code>
 *
 * For wide-character formatting there is the similar \c wc_decor decorator generator object.
 */
#ifdef BOOST_LOG_USE_CHAR
BOOST_INLINE_VARIABLE const aux::c_decorator_gen< char > c_decor = {};
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
BOOST_INLINE_VARIABLE const aux::c_decorator_gen< wchar_t > wc_decor = {};
#endif

/*!
 * The function creates a C-style decorator generator for arbitrary character type.
 */
template< typename CharT >
BOOST_FORCEINLINE aux::c_decorator_gen< CharT > make_c_decor()
{
    return aux::c_decorator_gen< CharT >();
}

/*!
 * A character decorator implementation that escapes all non-prontable and non-ASCII characters
 * in the output with C-style escape sequences.
 */
template< typename CharT >
class c_ascii_pattern_replacer :
    public pattern_replacer< CharT >
{
private:
    //! Base type
    typedef pattern_replacer< CharT > base_type;

public:
    //! Result type
    typedef typename base_type::result_type result_type;
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;

private:
    //! Traits type
    typedef aux::c_decorator_traits< char_type > traits_type;

public:
    //! Default constructor
    c_ascii_pattern_replacer() : base_type(traits_type::get_patterns(), traits_type::get_replacements())
    {
    }

    //! Applies string replacements starting from the specified position
    result_type operator() (string_type& str, typename string_type::size_type start_pos = 0) const
    {
        base_type::operator() (str, start_pos);

        typedef typename string_type::iterator string_iterator;
        for (string_iterator it = str.begin() + start_pos, end = str.end(); it != end; ++it)
        {
            char_type c = *it;
            if (c < 0x20 || c > 0x7e)
            {
                char_type buf[(std::numeric_limits< char_type >::digits + 3) / 4 + 3];
                std::size_t n = traits_type::print_escaped(buf, c);
                std::size_t pos = it - str.begin();
                str.replace(pos, 1, buf, n);
                it = str.begin() + n - 1;
                end = str.end();
            }
        }
    }
};

namespace aux {

template< typename CharT >
struct c_ascii_decorator_gen
{
    typedef CharT char_type;

    template< typename SubactorT >
    BOOST_FORCEINLINE char_decorator_actor< SubactorT, c_ascii_pattern_replacer< char_type > > operator[] (SubactorT const& subactor) const
    {
        typedef c_ascii_pattern_replacer< char_type > replacer_type;
        typedef char_decorator_actor< SubactorT, replacer_type > result_type;
        typedef typename result_type::terminal_type terminal_type;
        typename result_type::base_type act = {{ terminal_type(subactor, replacer_type()) }};
        return result_type(act);
    }
};

} // namespace aux

/*!
 * C-style decorator generator object. Acts similarly to \c c_decor, except that \c c_ascii_decor also
 * converts all non-ASCII and non-printable ASCII characters, except for space character, into
 * C-style hexadecimal escape sequences. The generator provides <tt>operator[]</tt> that
 * can be used to construct the actual decorator. For example:
 *
 * <code>
 * c_ascii_decor[ stream << attr< std::string >("MyAttr") ]
 * </code>
 *
 * For wide-character formatting there is the similar \c wc_ascii_decor decorator generator object.
 */
#ifdef BOOST_LOG_USE_CHAR
BOOST_INLINE_VARIABLE const aux::c_ascii_decorator_gen< char > c_ascii_decor = {};
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
BOOST_INLINE_VARIABLE const aux::c_ascii_decorator_gen< wchar_t > wc_ascii_decor = {};
#endif

/*!
 * The function creates a C-style decorator generator for arbitrary character type.
 */
template< typename CharT >
BOOST_FORCEINLINE aux::c_ascii_decorator_gen< CharT > make_c_ascii_decor()
{
    return aux::c_ascii_decorator_gen< CharT >();
}

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_EXPRESSIONS_FORMATTERS_C_DECORATOR_HPP_INCLUDED_
