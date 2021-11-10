/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ESCAPE_CHAR_HPP
#define BOOST_SPIRIT_ESCAPE_CHAR_HPP

///////////////////////////////////////////////////////////////////////////////
#include <string>
#include <iterator>
#include <cctype>
#include <boost/limits.hpp>

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/debug.hpp>

#include <boost/spirit/home/classic/utility/escape_char_fwd.hpp>
#include <boost/spirit/home/classic/utility/impl/escape_char.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  escape_char_action class
//
//      Links an escape char parser with a user defined semantic action.
//      The semantic action may be a function or a functor. A function
//      should be compatible with the interface:
//
//          void f(CharT ch);
//
//      A functor should have a member operator() with a compatible signature
//      as above. The matching character is passed into the function/functor.
//      This is the default class that character parsers use when dealing with
//      the construct:
//
//          p[f]
//
//      where p is a parser and f is a function or functor.
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename ParserT, typename ActionT,
    unsigned long Flags, typename CharT
>
struct escape_char_action
:   public unary<ParserT,
        parser<escape_char_action<ParserT, ActionT, Flags, CharT> > >
{
    typedef escape_char_action
        <ParserT, ActionT, Flags, CharT>        self_t;
    typedef action_parser_category              parser_category_t;
    typedef unary<ParserT, parser<self_t> >     base_t;

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, CharT>::type type;
    };

    escape_char_action(ParserT const& p, ActionT const& a)
    : base_t(p), actor(a) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        return impl::escape_char_action_parse<Flags, CharT>::
            parse(scan, *this);
    }

    ActionT const& predicate() const { return actor; }

private:

    ActionT actor;
};

///////////////////////////////////////////////////////////////////////////////
//
//  escape_char_parser class
//
//      The escape_char_parser helps in conjunction with the escape_char_action
//      template class (see above) in parsing escaped characters. There are two
//      different variants of this parser: one for parsing C style escaped
//      characters and one for parsing LEX style escaped characters.
//
//      The C style escaped character parser is generated, when the template
//      parameter 'Flags' is equal to 'c_escapes' (a constant defined in the
//      file impl/escape_char.ipp). This parser recognizes all valid C escape
//      character sequences: '\t', '\b', '\f', '\n', '\r', '\"', '\'', '\\'
//      and the numeric style escapes '\120' (octal) and '\x2f' (hexadecimal)
//      and converts these to their character equivalent, for instance the
//      sequence of a backslash and a 'b' is parsed as the character '\b'.
//      All other escaped characters are rejected by this parser.
//
//      The LEX style escaped character parser is generated, when the template
//      parameter 'Flags' is equal to 'lex_escapes' (a constant defined in the
//      file impl/escape_char.ipp). This parser recognizes all the C style
//      escaped character sequences (as described above) and additionally
//      does not reject all other escape sequences. All not mentioned escape
//      sequences are converted by the parser to the plain character, for
//      instance '\a' will be parsed as 'a'.
//
//      All not escaped characters are parsed without modification.
//
///////////////////////////////////////////////////////////////////////////////

template <unsigned long Flags, typename CharT>
struct escape_char_action_parser_gen;

template <unsigned long Flags, typename CharT>
struct escape_char_parser :
    public parser<escape_char_parser<Flags, CharT> > {

    // only the values c_escapes and lex_escapes are valid for Flags
    BOOST_STATIC_ASSERT(Flags == c_escapes || Flags == lex_escapes);

    typedef escape_char_parser<Flags, CharT> self_t;
    typedef
        escape_char_action_parser_gen<Flags, CharT>
        action_parser_generator_t;

    template <typename ScannerT>
    struct result {

        typedef typename match_result<ScannerT, CharT>::type type;
    };

    template <typename ActionT>
    escape_char_action<self_t, ActionT, Flags, CharT>
    operator[](ActionT const& actor) const
    {
        return escape_char_action<self_t, ActionT, Flags, CharT>(*this, actor);
    }

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const &scan) const
    {
        return impl::escape_char_parse<CharT>::parse(scan, *this);
    }
};

template <unsigned long Flags, typename CharT>
struct escape_char_action_parser_gen {

    template <typename ParserT, typename ActionT>
    static escape_char_action<ParserT, ActionT, Flags, CharT>
    generate (ParserT const &p, ActionT const &actor)
    {
        typedef
            escape_char_action<ParserT, ActionT, Flags, CharT>
            action_parser_t;
        return action_parser_t(p, actor);
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//  predefined escape_char_parser objects
//
//      These objects should be used for generating correct escaped character
//      parsers.
//
///////////////////////////////////////////////////////////////////////////////
const escape_char_parser<lex_escapes> lex_escape_ch_p =
    escape_char_parser<lex_escapes>();

const escape_char_parser<c_escapes> c_escape_ch_p =
    escape_char_parser<c_escapes>();

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
