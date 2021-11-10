/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_PRIMITIVES_IPP)
#define BOOST_SPIRIT_PRIMITIVES_IPP

#include <cctype>
#if !defined(BOOST_NO_CWCTYPE)
#include <cwctype>
#endif

#include <string> // char_traits

#if defined(BOOST_MSVC)
#  pragma warning (push)
#  pragma warning(disable:4800)
#endif

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    template <typename DrivedT> struct char_parser;

    namespace impl
    {
        template <typename IteratorT>
        inline IteratorT
        get_last(IteratorT first)
        {
            while (*first)
                first++;
            return first;
        }

        template<
            typename RT,
            typename IteratorT,
            typename ScannerT>
        inline RT
        string_parser_parse(
            IteratorT str_first,
            IteratorT str_last,
            ScannerT& scan)
        {
            typedef typename ScannerT::iterator_t iterator_t;
            iterator_t saved = scan.first;
            std::size_t slen = str_last - str_first;

            while (str_first != str_last)
            {
                if (scan.at_end() || (*str_first != *scan))
                    return scan.no_match();
                ++str_first;
                ++scan;
            }

            return scan.create_match(slen, nil_t(), saved, scan.first);
        }

    ///////////////////////////////////////////////////////////////////////////
    //
    // Conversion from char_type to int_type
    //
    ///////////////////////////////////////////////////////////////////////////

        //  Use char_traits for char and wchar_t only, as these are the only
        //  specializations provided in the standard. Other types are on their
        //  own.
        //
        //  For UDT, one may override:
        //
        //      isalnum
        //      isalpha
        //      iscntrl
        //      isdigit
        //      isgraph
        //      islower
        //      isprint
        //      ispunct
        //      isspace
        //      isupper
        //      isxdigit
        //      isblank
        //      isupper
        //      tolower
        //      toupper
        //
        //  in a namespace suitable for Argument Dependent lookup or in
        //  namespace std (disallowed by the standard).

        template <typename CharT>
        struct char_type_char_traits_helper
        {
            typedef CharT char_type;
            typedef typename std::char_traits<CharT>::int_type int_type;

            static int_type to_int_type(CharT c)
            {
                return std::char_traits<CharT>::to_int_type(c);
            }

            static char_type to_char_type(int_type i)
            {
                return std::char_traits<CharT>::to_char_type(i);
            }
        };

        template <typename CharT>
        struct char_traits_helper
        {
            typedef CharT char_type;
            typedef CharT int_type;

            static CharT & to_int_type(CharT & c)
            {
                return c;
            }

            static CharT & to_char_type(CharT & c)
            {
                return c;
            }
        };

        template <>
        struct char_traits_helper<char>
            : char_type_char_traits_helper<char>
        {
        };

#if !defined(BOOST_NO_CWCTYPE)

        template <>
        struct char_traits_helper<wchar_t>
            : char_type_char_traits_helper<wchar_t>
        {
        };

#endif

        template <typename CharT>
        inline typename char_traits_helper<CharT>::int_type
        to_int_type(CharT c)
        {
            return char_traits_helper<CharT>::to_int_type(c);
        }

        template <typename CharT>
        inline CharT
        to_char_type(typename char_traits_helper<CharT>::int_type c)
        {
            return char_traits_helper<CharT>::to_char_type(c);
        }

        ///////////////////////////////////////////////////////////////////////
        //
        //  Convenience functions
        //
        ///////////////////////////////////////////////////////////////////////

        template <typename CharT>
        inline bool
        isalnum_(CharT c)
        {
            using namespace std;
            return isalnum(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        isalpha_(CharT c)
        {
            using namespace std;
            return isalpha(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        iscntrl_(CharT c)
        {
            using namespace std;
            return iscntrl(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        isdigit_(CharT c)
        {
            using namespace std;
            return isdigit(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        isgraph_(CharT c)
        {
            using namespace std;
            return isgraph(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        islower_(CharT c)
        {
            using namespace std;
            return islower(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        isprint_(CharT c)
        {
            using namespace std;
            return isprint(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        ispunct_(CharT c)
        {
            using namespace std;
            return ispunct(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        isspace_(CharT c)
        {
            using namespace std;
            return isspace(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        isupper_(CharT c)
        {
            using namespace std;
            return isupper(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        isxdigit_(CharT c)
        {
            using namespace std;
            return isxdigit(to_int_type(c)) ? true : false;
        }

        template <typename CharT>
        inline bool
        isblank_(CharT c)
        {
            return (c == ' ' || c == '\t');
        }

        template <typename CharT>
        inline CharT
        tolower_(CharT c)
        {
            using namespace std;
            return to_char_type<CharT>(tolower(to_int_type(c)));
        }

        template <typename CharT>
        inline CharT
        toupper_(CharT c)
        {
            using namespace std;
            return to_char_type<CharT>(toupper(to_int_type(c)));
        }

#if !defined(BOOST_NO_CWCTYPE)

        inline bool
        isalnum_(wchar_t c)
        {
            using namespace std;
            return iswalnum(to_int_type(c)) ? true : false;
        }

        inline bool
        isalpha_(wchar_t c)
        {
            using namespace std;
            return iswalpha(to_int_type(c)) ? true : false;
        }

        inline bool
        iscntrl_(wchar_t c)
        {
            using namespace std;
            return iswcntrl(to_int_type(c)) ? true : false;
        }

        inline bool
        isdigit_(wchar_t c)
        {
            using namespace std;
            return iswdigit(to_int_type(c)) ? true : false;
        }

        inline bool
        isgraph_(wchar_t c)
        {
            using namespace std;
            return iswgraph(to_int_type(c)) ? true : false;
        }

        inline bool
        islower_(wchar_t c)
        {
            using namespace std;
            return iswlower(to_int_type(c)) ? true : false;
        }

        inline bool
        isprint_(wchar_t c)
        {
            using namespace std;
            return iswprint(to_int_type(c)) ? true : false;
        }

        inline bool
        ispunct_(wchar_t c)
        {
            using namespace std;
            return iswpunct(to_int_type(c)) ? true : false;
        }

        inline bool
        isspace_(wchar_t c)
        {
            using namespace std;
            return iswspace(to_int_type(c)) ? true : false;
        }

        inline bool
        isupper_(wchar_t c)
        {
            using namespace std;
            return iswupper(to_int_type(c)) ? true : false;
        }

        inline bool
        isxdigit_(wchar_t c)
        {
            using namespace std;
            return iswxdigit(to_int_type(c)) ? true : false;
        }

        inline bool
        isblank_(wchar_t c)
        {
            return (c == L' ' || c == L'\t');
        }

        inline wchar_t
        tolower_(wchar_t c)
        {
            using namespace std;
            return to_char_type<wchar_t>(towlower(to_int_type(c)));
        }

        inline wchar_t
        toupper_(wchar_t c)
        {
            using namespace std;
            return to_char_type<wchar_t>(towupper(to_int_type(c)));
        }

        inline bool
        isblank_(bool)
        {
            return false;
        }

#endif // !defined(BOOST_NO_CWCTYPE)

}

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit::impl

#ifdef BOOST_MSVC
#pragma warning (pop)
#endif

#endif
