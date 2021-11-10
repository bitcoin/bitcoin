/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ESCAPE_CHAR_IPP
#define BOOST_SPIRIT_ESCAPE_CHAR_IPP

#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/primitives/numerics.hpp>
#include <boost/spirit/home/classic/core/composite/difference.hpp>
#include <boost/spirit/home/classic/core/composite/sequence.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  escape_char_parser class
//
///////////////////////////////////////////////////////////////////////////////

const unsigned long c_escapes = 1;
const unsigned long lex_escapes = c_escapes << 1;

//////////////////////////////////
namespace impl {

    //////////////////////////////////
#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4127)
#endif
    template <unsigned long Flags, typename CharT>
    struct escape_char_action_parse {

        template <typename ParserT, typename ScannerT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ScannerT const& scan, ParserT const &p)
        {
            // Actually decode the escape char.
            typedef CharT char_t;
            typedef typename ScannerT::iterator_t iterator_t;
            typedef typename parser_result<ParserT, ScannerT>::type result_t;

            if (scan.first != scan.last) {

                iterator_t save = scan.first;
                if (result_t hit = p.subject().parse(scan)) {

                    char_t unescaped;

                    scan.first = save;
                    if (*scan.first == '\\') {

                        ++scan.first;
                        switch (*scan.first) {
                        case 'b':   unescaped = '\b';   ++scan.first; break;
                        case 't':   unescaped = '\t';   ++scan.first; break;
                        case 'n':   unescaped = '\n';   ++scan.first; break;
                        case 'f':   unescaped = '\f';   ++scan.first; break;
                        case 'r':   unescaped = '\r';   ++scan.first; break;
                        case '"':   unescaped = '"';    ++scan.first; break;
                        case '\'':  unescaped = '\'';   ++scan.first; break;
                        case '\\':  unescaped = '\\';   ++scan.first; break;

                        case 'x': case 'X':
                            {
                                char_t hex = 0;
                                char_t const lim =
                                    (std::numeric_limits<char_t>::max)() >> 4;

                                ++scan.first;
                                while (scan.first != scan.last)
                                {
                                    char_t c = *scan.first;
                                    if (hex > lim && impl::isxdigit_(c))
                                    {
                                        // overflow detected
                                        scan.first = save;
                                        return scan.no_match();
                                    }
                                    if (impl::isdigit_(c))
                                    {
                                        hex <<= 4;
                                        hex |= c - '0';
                                        ++scan.first;
                                    }
                                    else if (impl::isxdigit_(c))
                                    {
                                        hex <<= 4;
                                        c = impl::toupper_(c);
                                        hex |= c - 'A' + 0xA;
                                        ++scan.first;
                                    }
                                    else
                                    {
                                        break; // reached the end of the number
                                    }
                                }
                                unescaped = hex;
                            }
                            break;

                        case '0': case '1': case '2': case '3':
                        case '4': case '5': case '6': case '7':
                            {
                                char_t oct = 0;
                                char_t const lim =
                                    (std::numeric_limits<char_t>::max)() >> 3;
                                while (scan.first != scan.last)
                                {
                                    char_t c = *scan.first;
                                    if (oct > lim && (c >= '0' && c <= '7'))
                                    {
                                        // overflow detected
                                        scan.first = save;
                                        return scan.no_match();
                                    }

                                    if (c >= '0' && c <= '7')
                                    {
                                        oct <<= 3;
                                        oct |= c - '0';
                                        ++scan.first;
                                    }
                                    else
                                    {
                                        break; // reached end of digits
                                    }
                                }
                                unescaped = oct;
                            }
                            break;

                        default:
                            if (Flags & c_escapes)
                            {
                                // illegal C escape sequence
                                scan.first = save;
                                return scan.no_match();
                            }
                            else
                            {
                                unescaped = *scan.first;
                                ++scan.first;
                            }
                            break;
                        }
                    }
                    else {
                        unescaped = *scan.first;
                        ++scan.first;
                    }

                    scan.do_action(p.predicate(), unescaped, save, scan.first);
                    return hit;
                }
            }
            return scan.no_match(); // overflow detected
        }
    };
#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

    //////////////////////////////////
    template <typename CharT>
    struct escape_char_parse {

        template <typename ScannerT, typename ParserT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ScannerT const &scan, ParserT const &/*p*/)
        {
            typedef
                uint_parser<CharT, 8, 1,
                    std::numeric_limits<CharT>::digits / 3 + 1
                >
                oct_parser_t;
            typedef
                uint_parser<CharT, 16, 1,
                    std::numeric_limits<CharT>::digits / 4 + 1
                >
                hex_parser_t;

            typedef alternative<difference<anychar_parser, chlit<CharT> >,
                sequence<chlit<CharT>, alternative<alternative<oct_parser_t,
                sequence<inhibit_case<chlit<CharT> >, hex_parser_t > >,
                difference<difference<anychar_parser,
                inhibit_case<chlit<CharT> > >, oct_parser_t > > > >
                parser_t;

            static parser_t p =
                ( (anychar_p - chlit<CharT>(CharT('\\')))
                | (chlit<CharT>(CharT('\\')) >>
                    (  oct_parser_t()
                     | as_lower_d[chlit<CharT>(CharT('x'))] >> hex_parser_t()
                     | (anychar_p - as_lower_d[chlit<CharT>(CharT('x'))] - oct_parser_t())
                    )
                ));

            BOOST_SPIRIT_DEBUG_TRACE_NODE(p,
                (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_ESCAPE_CHAR) != 0);

            return p.parse(scan);
        }
    };

///////////////////////////////////////////////////////////////////////////////
} // namespace impl

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif

