/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_FOR_HPP
#define BOOST_SPIRIT_FOR_HPP
////////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/dynamic/impl/conditions.ipp>

////////////////////////////////////////////////////////////////////////////////

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    namespace impl
    {

        template <typename FuncT>
        struct for_functor
        {
            typedef typename boost::call_traits<FuncT>::param_type  param_t;

            for_functor(param_t f) : func(f) {}
            for_functor() {}
            FuncT func;
        };

        template <typename InitF>
        struct for_init_functor : for_functor<InitF>
        {
            typedef for_functor<InitF>          base_t;
            typedef typename base_t::param_t    param_t;

            for_init_functor(param_t f) : base_t(f) {}
            for_init_functor() : base_t() {}
            void init() const { /*return*/ this->func(); }
        };

        template <typename StepF>
        struct for_step_functor : for_functor<StepF>
        {
            typedef for_functor<StepF>          base_t;
            typedef typename base_t::param_t    param_t;

            for_step_functor(param_t f) : base_t(f) {}
            for_step_functor() : base_t() {}
            void step() const { /*return*/ this->func(); }
        };

        //////////////////////////////////
        // for_parser
        template
        <
            typename InitF, typename CondT, typename StepF,
            typename ParsableT
        >
        struct for_parser
            : private for_init_functor<InitF>
            , private for_step_functor<StepF>
            , private condition_evaluator<typename as_parser<CondT>::type>
            , public unary
            <
                typename as_parser<ParsableT>::type,
                parser< for_parser<InitF, CondT, StepF, ParsableT> >
            >
        {
            typedef for_parser<InitF, CondT, StepF, ParsableT> self_t;
            typedef as_parser<CondT>                           cond_as_parser_t;
            typedef typename cond_as_parser_t::type            condition_t;
            typedef condition_evaluator<condition_t>           eval_t;
            typedef as_parser<ParsableT>                       as_parser_t;
            typedef typename as_parser_t::type                 parser_t;
            typedef unary< parser_t, parser< self_t > >        base_t;


            //////////////////////////////
            // constructor, saves init, condition and step functors
            // for later use the parse member function
            for_parser
            (
                InitF const &i, CondT const &c, StepF const &s,
                ParsableT const &p
            )
                : for_init_functor<InitF>(i)
                , for_step_functor<StepF>(s)
                , eval_t(cond_as_parser_t::convert(c))
                , base_t(as_parser_t::convert(p))
            { }

            for_parser()
                : for_init_functor<InitF>()
                , for_step_functor<StepF>()
                , eval_t()
                , base_t()
            {}

            //////////////////////////////
            // parse member function
            template <typename ScannerT>
            typename parser_result<self_t, ScannerT>::type
            parse(ScannerT const &scan) const
            {
                typedef typename parser_result<parser_t, ScannerT>::type
                    body_result_t;

                typename ScannerT::iterator_t save(scan.first);

                std::size_t length = 0;
                std::ptrdiff_t eval_length = 0;

                this->init();
                while ((eval_length = this->evaluate(scan))>=0)
                {
                    length += eval_length;
                    body_result_t tmp(this->subject().parse(scan));
                    if (tmp)
                    {
                        length+=tmp.length();
                    }
                    else
                    {
                        return scan.no_match();
                    }
                    this->step();
                }

                BOOST_SPIRIT_CLASSIC_NS::nil_t attr;
                return scan.create_match
                    (length, attr, save, scan.first);
            }
        };

        //////////////////////////////////
        // for_parser_gen generates takes the body parser in brackets
        // and returns the for_parser
        template <typename InitF, typename CondT, typename StepF>
        struct for_parser_gen
        {
            for_parser_gen(InitF const &i, CondT const &c, StepF const &s)
                : init(i)
                , condition(c)
                , step(s)
            {}

            template <typename ParsableT>
            for_parser<InitF, CondT, StepF, ParsableT>
            operator[](ParsableT const &p) const
            {
                return for_parser<InitF, CondT, StepF, ParsableT>
                    (init, condition, step, p);
            }

            InitF const &init;
            CondT const &condition;
            StepF const &step;
        };
    } // namespace impl

    //////////////////////////////
    // for_p, returns for-parser generator
    // Usage: spirit::for_p(init-ftor, condition, step-ftor)[body]
    template
    <
        typename InitF, typename ConditionT, typename StepF
    >
    impl::for_parser_gen<InitF, ConditionT, StepF>
    for_p(InitF const &init_f, ConditionT const &condition, StepF const &step_f)
    {
        return impl::for_parser_gen<InitF, ConditionT, StepF>
            (init_f, condition, step_f);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // BOOST_SPIRIT_FOR_HPP
