/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_PRIMITIVES_HPP)
#define BOOST_SPIRIT_PRIMITIVES_HPP

#include <boost/ref.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/assert.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/impl/directives.ipp>
#include <boost/spirit/home/classic/core/primitives/impl/primitives.ipp>

#ifdef BOOST_MSVC
#pragma warning (push)
#pragma warning(disable : 4512)
#endif

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  char_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename DerivedT>
    struct char_parser : public parser<DerivedT>
    {
        typedef DerivedT self_t;
        template <typename ScannerT>
        struct result
        {
            typedef typename match_result<
                ScannerT,
                typename ScannerT::value_t
            >::type type;
        };

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename ScannerT::value_t value_t;
            typedef typename ScannerT::iterator_t iterator_t;
            typedef scanner_policies<
                no_skipper_iteration_policy<
                BOOST_DEDUCED_TYPENAME ScannerT::iteration_policy_t>,
                BOOST_DEDUCED_TYPENAME ScannerT::match_policy_t,
                BOOST_DEDUCED_TYPENAME ScannerT::action_policy_t
            > policies_t;

            if (!scan.at_end())
            {
                value_t ch = *scan;
                if (this->derived().test(ch))
                {
                    iterator_t save(scan.first);
                    ++scan.change_policies(policies_t(scan));
                    return scan.create_match(1, ch, save, scan.first);
                }
            }
            return scan.no_match();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  negation of char_parsers
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename PositiveT>
    struct negated_char_parser
    : public char_parser<negated_char_parser<PositiveT> >
    {
        typedef negated_char_parser<PositiveT> self_t;
        typedef PositiveT positive_t;

        negated_char_parser(positive_t const& p)
        : positive(p.derived()) {}

        template <typename T>
        bool test(T ch) const
        {
            return !positive.test(ch);
        }

        positive_t const positive;
    };

    template <typename ParserT>
    inline negated_char_parser<ParserT>
    operator~(char_parser<ParserT> const& p)
    {
        return negated_char_parser<ParserT>(p.derived());
    }

    template <typename ParserT>
    inline ParserT
    operator~(negated_char_parser<ParserT> const& n)
    {
        return n.positive;
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  chlit class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT = char>
    struct chlit : public char_parser<chlit<CharT> >
    {
        chlit(CharT ch_)
        : ch(ch_) {}

        template <typename T>
        bool test(T ch_) const
        {
            return ch_ == ch;
        }

        CharT   ch;
    };

    template <typename CharT>
    inline chlit<CharT>
    ch_p(CharT ch)
    {
        return chlit<CharT>(ch);
    }

    // This should take care of ch_p("a") "bugs"
    template <typename CharT, std::size_t N>
    inline chlit<CharT>
    ch_p(CharT const (& str)[N])
    {
        //  ch_p's argument should be a single character or a null-terminated
        //  string with a single character
        BOOST_STATIC_ASSERT(N < 3);
        return chlit<CharT>(str[0]);
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  range class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT = char>
    struct range : public char_parser<range<CharT> >
    {
        range(CharT first_, CharT last_)
        : first(first_), last(last_)
        {
            BOOST_SPIRIT_ASSERT(!(last < first));
        }

        template <typename T>
        bool test(T ch) const
        {
            return !(CharT(ch) < first) && !(last < CharT(ch));
        }

        CharT   first;
        CharT   last;
    };

    template <typename CharT>
    inline range<CharT>
    range_p(CharT first, CharT last)
    {
        return range<CharT>(first, last);
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  chseq class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename IteratorT = char const*>
    class chseq : public parser<chseq<IteratorT> >
    {
    public:

        typedef chseq<IteratorT> self_t;

        chseq(IteratorT first_, IteratorT last_)
        : first(first_), last(last_) {}

        chseq(IteratorT first_)
        : first(first_), last(impl::get_last(first_)) {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename boost::unwrap_reference<IteratorT>::type striter_t;
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            return impl::string_parser_parse<result_t>(
                striter_t(first),
                striter_t(last),
                scan);
        }

    private:

        IteratorT first;
        IteratorT last;
    };

    template <typename CharT>
    inline chseq<CharT const*>
    chseq_p(CharT const* str)
    {
        return chseq<CharT const*>(str);
    }

    template <typename IteratorT>
    inline chseq<IteratorT>
    chseq_p(IteratorT first, IteratorT last)
    {
        return chseq<IteratorT>(first, last);
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  strlit class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename IteratorT = char const*>
    class strlit : public parser<strlit<IteratorT> >
    {
    public:

        typedef strlit<IteratorT> self_t;

        strlit(IteratorT first, IteratorT last)
        : seq(first, last) {}

        strlit(IteratorT first)
        : seq(first) {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            return impl::contiguous_parser_parse<result_t>
                (seq, scan, scan);
        }

    private:

        chseq<IteratorT> seq;
    };

    template <typename CharT>
    inline strlit<CharT const*>
    str_p(CharT const* str)
    {
        return strlit<CharT const*>(str);
    }

    template <typename CharT>
    inline strlit<CharT *>
    str_p(CharT * str)
    {
        return strlit<CharT *>(str);
    }

    template <typename IteratorT>
    inline strlit<IteratorT>
    str_p(IteratorT first, IteratorT last)
    {
        return strlit<IteratorT>(first, last);
    }

    // This should take care of str_p('a') "bugs"
    template <typename CharT>
    inline chlit<CharT>
    str_p(CharT ch)
    {
        return chlit<CharT>(ch);
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  nothing_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct nothing_parser : public parser<nothing_parser>
    {
        typedef nothing_parser self_t;

        nothing_parser() {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            return scan.no_match();
        }
    };

    nothing_parser const nothing_p = nothing_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  anychar_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct anychar_parser : public char_parser<anychar_parser>
    {
        typedef anychar_parser self_t;

        anychar_parser() {}

        template <typename CharT>
        bool test(CharT) const
        {
            return true;
        }
    };

    anychar_parser const anychar_p = anychar_parser();

    inline nothing_parser
    operator~(anychar_parser)
    {
        return nothing_p;
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  alnum_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct alnum_parser : public char_parser<alnum_parser>
    {
        typedef alnum_parser self_t;

        alnum_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::isalnum_(ch);
        }
    };

    alnum_parser const alnum_p = alnum_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  alpha_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct alpha_parser : public char_parser<alpha_parser>
    {
        typedef alpha_parser self_t;

        alpha_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::isalpha_(ch);
        }
    };

    alpha_parser const alpha_p = alpha_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  cntrl_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct cntrl_parser : public char_parser<cntrl_parser>
    {
        typedef cntrl_parser self_t;

        cntrl_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::iscntrl_(ch);
        }
    };

    cntrl_parser const cntrl_p = cntrl_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  digit_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct digit_parser : public char_parser<digit_parser>
    {
        typedef digit_parser self_t;

        digit_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::isdigit_(ch);
        }
    };

    digit_parser const digit_p = digit_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  graph_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct graph_parser : public char_parser<graph_parser>
    {
        typedef graph_parser self_t;

        graph_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::isgraph_(ch);
        }
    };

    graph_parser const graph_p = graph_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  lower_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct lower_parser : public char_parser<lower_parser>
    {
        typedef lower_parser self_t;

        lower_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::islower_(ch);
        }
    };

    lower_parser const lower_p = lower_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  print_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct print_parser : public char_parser<print_parser>
    {
        typedef print_parser self_t;

        print_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::isprint_(ch);
        }
    };

    print_parser const print_p = print_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  punct_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct punct_parser : public char_parser<punct_parser>
    {
        typedef punct_parser self_t;

        punct_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::ispunct_(ch);
        }
    };

    punct_parser const punct_p = punct_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  blank_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct blank_parser : public char_parser<blank_parser>
    {
        typedef blank_parser self_t;

        blank_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::isblank_(ch);
        }
    };

    blank_parser const blank_p = blank_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  space_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct space_parser : public char_parser<space_parser>
    {
        typedef space_parser self_t;

        space_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::isspace_(ch);
        }
    };

    space_parser const space_p = space_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  upper_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct upper_parser : public char_parser<upper_parser>
    {
        typedef upper_parser self_t;

        upper_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::isupper_(ch);
        }
    };

    upper_parser const upper_p = upper_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  xdigit_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct xdigit_parser : public char_parser<xdigit_parser>
    {
        typedef xdigit_parser self_t;

        xdigit_parser() {}

        template <typename CharT>
        bool test(CharT ch) const
        {
            return impl::isxdigit_(ch);
        }
    };

    xdigit_parser const xdigit_p = xdigit_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  eol_parser class (contributed by Martin Wille)
    //
    ///////////////////////////////////////////////////////////////////////////
    struct eol_parser : public parser<eol_parser>
    {
        typedef eol_parser self_t;

        eol_parser() {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef scanner_policies<
                no_skipper_iteration_policy<
                BOOST_DEDUCED_TYPENAME ScannerT::iteration_policy_t>,
                BOOST_DEDUCED_TYPENAME ScannerT::match_policy_t,
                BOOST_DEDUCED_TYPENAME ScannerT::action_policy_t
            > policies_t;

            typename ScannerT::iterator_t save = scan.first;
            std::size_t len = 0;

            if (!scan.at_end() && *scan == '\r')    // CR
            {
                ++scan.change_policies(policies_t(scan));
                ++len;
            }

            // Don't call skipper here
            if (scan.first != scan.last && *scan == '\n')    // LF
            {
                ++scan.change_policies(policies_t(scan));
                ++len;
            }

            if (len)
                return scan.create_match(len, nil_t(), save, scan.first);
            return scan.no_match();
        }
    };

    eol_parser const eol_p = eol_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  end_parser class (suggested by Markus Schoepflin)
    //
    ///////////////////////////////////////////////////////////////////////////
    struct end_parser : public parser<end_parser>
    {
        typedef end_parser self_t;

        end_parser() {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            if (scan.at_end())
                return scan.empty_match();
            return scan.no_match();
        }
    };

    end_parser const end_p = end_parser();

    ///////////////////////////////////////////////////////////////////////////
    //
    //  the pizza_p parser :-)
    //
    ///////////////////////////////////////////////////////////////////////////
    inline strlit<char const*> const
    pizza_p(char const* your_favorite_pizza)
    {
        return your_favorite_pizza;
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#ifdef BOOST_MSVC
#pragma warning (pop)
#endif

#endif
