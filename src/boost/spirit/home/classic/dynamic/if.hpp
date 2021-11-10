/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002 Juan Carlos Arevalo-Baeza
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_IF_HPP
#define BOOST_SPIRIT_IF_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/dynamic/impl/conditions.ipp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    namespace impl {

    //////////////////////////////////
    // if-else-parser, holds two alternative parsers and a conditional functor
    // that selects between them.
    template <typename ParsableTrueT, typename ParsableFalseT, typename CondT>
    struct if_else_parser
        : public condition_evaluator<typename as_parser<CondT>::type>
        , public binary
        <
            typename as_parser<ParsableTrueT>::type,
            typename as_parser<ParsableFalseT>::type,
            parser< if_else_parser<ParsableTrueT, ParsableFalseT, CondT> >
        >
    {
        typedef if_else_parser<ParsableTrueT, ParsableFalseT, CondT>  self_t;

        typedef as_parser<ParsableTrueT>            as_parser_true_t;
        typedef as_parser<ParsableFalseT>           as_parser_false_t;
        typedef typename as_parser_true_t::type     parser_true_t;
        typedef typename as_parser_false_t::type    parser_false_t;
        typedef as_parser<CondT>                    cond_as_parser_t;
        typedef typename cond_as_parser_t::type     condition_t;

        typedef binary<parser_true_t, parser_false_t, parser<self_t> > base_t;
        typedef condition_evaluator<condition_t>                       eval_t;

        if_else_parser
        (
            ParsableTrueT  const& p_true,
            ParsableFalseT const& p_false,
            CondT          const& cond_
        )
            : eval_t(cond_as_parser_t::convert(cond_))
            , base_t
                (
                    as_parser_true_t::convert(p_true),
                    as_parser_false_t::convert(p_false)
                )
        { }

        template <typename ScannerT>
        struct result
        {
            typedef typename match_result<ScannerT, nil_t>::type type;
        };

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result
                <parser_true_t, ScannerT>::type   then_result_t;
            typedef typename parser_result
                <parser_false_t, ScannerT>::type  else_result_t;

            typename ScannerT::iterator_t const  save(scan.first);

            std::ptrdiff_t length = this->evaluate(scan);
            if (length >= 0)
            {
                then_result_t then_result(this->left().parse(scan));
                if (then_result)
                {
                    length += then_result.length();
                    return scan.create_match(std::size_t(length), nil_t(), save, scan.first);
                }
            }
            else
            {
                else_result_t else_result(this->right().parse(scan));
                if (else_result)
                {
                    length = else_result.length();
                    return scan.create_match(std::size_t(length), nil_t(), save, scan.first);
                }
            }
            return scan.no_match();
        }
    };

    //////////////////////////////////
    // if-else-parser generator, takes the false-parser in brackets
    // and returns the if-else-parser.
    template <typename ParsableTrueT, typename CondT>
    struct if_else_parser_gen
    {
        if_else_parser_gen(ParsableTrueT const& p_true_, CondT const& cond_)
            : p_true(p_true_)
            , cond(cond_) {}

        template <typename ParsableFalseT>
        if_else_parser
        <
            ParsableTrueT,
            ParsableFalseT,
            CondT
        >
        operator[](ParsableFalseT const& p_false) const
        {
            return if_else_parser<ParsableTrueT, ParsableFalseT, CondT>
                (
                    p_true,
                    p_false,
                    cond
                );
        }

        ParsableTrueT const &p_true;
        CondT const &cond;
    };

    //////////////////////////////////
    // if-parser, conditionally runs a parser is a functor condition is true.
    // If the condition is false, it fails the parse.
    // It can optionally become an if-else-parser through the member else_p.
    template <typename ParsableT, typename CondT>
    struct if_parser
        : public condition_evaluator<typename as_parser<CondT>::type>
        , public unary
        <
            typename as_parser<ParsableT>::type,
            parser<if_parser<ParsableT, CondT> > >
    {
        typedef if_parser<ParsableT, CondT>           self_t;
        typedef as_parser<ParsableT>                  as_parser_t;
        typedef typename as_parser_t::type            parser_t;

        typedef as_parser<CondT>                      cond_as_parser_t;
        typedef typename cond_as_parser_t::type       condition_t;
        typedef condition_evaluator<condition_t>      eval_t;
        typedef unary<parser_t, parser<self_t> >      base_t;

        if_parser(ParsableT const& p, CondT const& cond_)
            : eval_t(cond_as_parser_t::convert(cond_))
            , base_t(as_parser_t::convert(p))
            , else_p(p, cond_)
        {}

        template <typename ScannerT>
        struct result
        {
            typedef typename match_result<ScannerT, nil_t>::type type;
        };

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<parser_t, ScannerT>::type t_result_t;
            typename ScannerT::iterator_t const save(scan.first);

            std::ptrdiff_t length = this->evaluate(scan);
            if (length >= 0)
            {
                t_result_t then_result(this->subject().parse(scan));
                if (then_result)
                {
                    length += then_result.length();
                    return scan.create_match(std::size_t(length), nil_t(), save, scan.first);
                }
                return scan.no_match();
            }
            return scan.empty_match();
        }

        if_else_parser_gen<ParsableT, CondT> else_p;
    };

    //////////////////////////////////
    // if-parser generator, takes the true-parser in brackets and returns the
    // if-parser.
    template <typename CondT>
    struct if_parser_gen
    {
        if_parser_gen(CondT const& cond_) : cond(cond_) {}

        template <typename ParsableT>
        if_parser
        <
            ParsableT,
            CondT
        >
        operator[](ParsableT const& subject) const
        {
            return if_parser<ParsableT, CondT>(subject, cond);
        }

        CondT const &cond;
    };

} // namespace impl

//////////////////////////////////
// if_p function, returns "if" parser generator

template <typename CondT>
impl::if_parser_gen<CondT>
if_p(CondT const& cond)
{
    return impl::if_parser_gen<CondT>(cond);
}

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // BOOST_SPIRIT_IF_HPP
