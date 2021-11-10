/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_WHILE_HPP
#define BOOST_SPIRIT_WHILE_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/dynamic/impl/conditions.ipp>

////////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    namespace impl {

    //////////////////////////////////
    // while parser
    // object are created by while_parser_gen and do_parser_gen
    template <typename ParsableT, typename CondT, bool is_do_parser>
    struct while_parser
        : public condition_evaluator< typename as_parser<CondT>::type >
        , public unary // the parent stores a copy of the body parser
        <
            typename as_parser<ParsableT>::type,
            parser<while_parser<ParsableT, CondT, is_do_parser> >
        >
    {
        typedef while_parser<ParsableT, CondT, is_do_parser> self_t;

        typedef as_parser<ParsableT>            as_parser_t;
        typedef typename as_parser_t::type      parser_t;
        typedef as_parser<CondT>                cond_as_parser_t;
        typedef typename cond_as_parser_t::type condition_t;

        typedef unary<parser_t, parser<self_t> > base_t;
        typedef condition_evaluator<condition_t> eval_t;


        //////////////////////////////
        // constructor, saves condition and body parser
        while_parser(ParsableT const &body, CondT const &cond)
            : eval_t(cond_as_parser_t::convert(cond))
            , base_t(as_parser_t::convert(body))
        {}

        //////////////////////////////
        // result type computer.
        template <typename ScannerT>
        struct result
        {
            typedef typename match_result
                <ScannerT, nil_t>::type type;
        };

        //////////////////////////////
        // parse member function
        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<parser_t, ScannerT>::type sresult_t;
            typedef typename ScannerT::iterator_t                    iterator_t;

            iterator_t save(scan.first);
            std::size_t length = 0;
            std::ptrdiff_t eval_length = 0;

            bool dont_check_condition = is_do_parser;

            while (dont_check_condition || (eval_length=this->evaluate(scan))>=0)
            {
                dont_check_condition = false;
                length += eval_length;
                sresult_t tmp(this->subject().parse(scan));
                if (tmp)
                {
                    length+=tmp.length();
                }
                else
                {
                    return scan.no_match();
                }
            }
            return scan.create_match(length, nil_t(), save, scan.first);
        }
    };

    //////////////////////////////////
    // while-parser generator, takes the body-parser in brackets
    // and returns the actual while-parser.
    template <typename CondT>
    struct while_parser_gen
    {
        //////////////////////////////
        // constructor, saves the condition for use by operator[]
        while_parser_gen(CondT const& cond_) : cond(cond_) {}

        //////////////////////////////
        // operator[] returns the actual while-parser object
        template <typename ParsableT>
        while_parser<ParsableT, CondT, false>
        operator[](ParsableT const &subject) const
        {
            return while_parser<ParsableT, CondT, false>(subject, cond);
        }
    private:

        //////////////////////////////
        // the condition is stored by reference here.
        // this should not cause any harm since object of type
        // while_parser_gen<> are only used as temporaries
        // the while-parser object constructed by the operator[]
        // stores a copy of the condition.
        CondT const &cond;
    };

    //////////////////////////////////
    // do-while-parser generator, takes the condition as
    // parameter to while_p member function and returns the
    // actual do-while-parser.
    template <typename ParsableT>
    struct do_while_parser_gen
    {
        //////////////////////////////
        // constructor. saves the body parser for use by while_p.
        explicit do_while_parser_gen(ParsableT const &body_parser)
            : body(body_parser)
        {}

        //////////////////////////////
        // while_p returns the actual while-parser object
        template <typename CondT>
        while_parser<ParsableT, CondT, true>
        while_p(CondT cond) const
        {
            return while_parser<ParsableT, CondT, true>(body, cond);
        }
    private:

        //////////////////////////////
        // the body is stored by reference here
        // this should not cause any harm since object of type
        // do_while_parser_gen<> are only used as temporaries
        // the while-parser object constructed by the while_p
        // member function stores a copy of the body parser.
        ParsableT const &body;
    };

    struct do_parser_gen
    {
        inline do_parser_gen() {}

        template <typename ParsableT>
        impl::do_while_parser_gen<ParsableT>
        operator[](ParsableT const& body) const
        {
            return impl::do_while_parser_gen<ParsableT>(body);
        }
    };
} // namespace impl

//////////////////////////////////
// while_p function, while-parser generator
// Usage: spirit::while_p(Condition)[Body]
template <typename CondT>
impl::while_parser_gen<CondT>
while_p(CondT const& cond)
{
    return impl::while_parser_gen<CondT>(cond);
}

//////////////////////////////////
// do_p functor, do-while-parser generator
// Usage: spirit::do_p[Body].while_p(Condition)
impl::do_parser_gen const do_p = impl::do_parser_gen();

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // BOOST_SPIRIT_WHILE_HPP
