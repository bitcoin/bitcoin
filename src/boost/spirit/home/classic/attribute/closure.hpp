/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_CLOSURE_HPP
#define BOOST_SPIRIT_CLOSURE_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/core/non_terminal/parser_context.hpp>
#include <boost/spirit/home/classic/attribute/parametric.hpp>
#include <boost/spirit/home/classic/attribute/closure_context.hpp>
#include <boost/spirit/home/classic/attribute/closure_fwd.hpp>

#include <boost/spirit/home/classic/phoenix/closures.hpp>
#include <boost/spirit/home/classic/phoenix/primitives.hpp>
#include <boost/spirit/home/classic/phoenix/casts.hpp>
#include <boost/spirit/home/classic/phoenix/operators.hpp>
#include <boost/spirit/home/classic/phoenix/tuple_helpers.hpp>

#include <boost/static_assert.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit predefined maximum closure limit. This limit defines the maximum
//  number of elements a closure can hold. This number defaults to 3. The
//  actual maximum is rounded up in multiples of 3. Thus, if this value
//  is 4, the actual limit is 6. The ultimate maximum limit in this
//  implementation is 15.
//
//  It should NOT be greater than PHOENIX_LIMIT!
//
///////////////////////////////////////////////////////////////////////////////

#if !defined(BOOST_SPIRIT_CLOSURE_LIMIT)
#define BOOST_SPIRIT_CLOSURE_LIMIT PHOENIX_LIMIT
#endif

///////////////////////////////////////////////////////////////////////////////
//
// ensure BOOST_SPIRIT_CLOSURE_LIMIT <= PHOENIX_LIMIT and SPIRIT_CLOSURE_LIMIT <= 15
//
///////////////////////////////////////////////////////////////////////////////
BOOST_STATIC_ASSERT(BOOST_SPIRIT_CLOSURE_LIMIT <= PHOENIX_LIMIT);
BOOST_STATIC_ASSERT(BOOST_SPIRIT_CLOSURE_LIMIT <= 15);

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  closure_context class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ClosureT>
    class closure_context : public parser_context_base
    {
    public:

        typedef typename ::phoenix::tuple_element<0,
            typename ClosureT::tuple_t>::type attr_t;
        typedef ClosureT base_t;
        typedef closure_context_linker<closure_context<ClosureT> >
        context_linker_t;

        closure_context(ClosureT const& clos)
        : frame(clos) {}

        ~closure_context() {}

        template <typename ParserT, typename ScannerT>
        void pre_parse(ParserT const&, ScannerT const&) {}

        template <typename ResultT, typename ParserT, typename ScannerT>
        ResultT& post_parse(ResultT& hit, ParserT const&, ScannerT const&)
        { hit.value(frame[::phoenix::tuple_index_names::_1]); return hit; }

    private:

        ::phoenix::closure_frame<typename ClosureT::phoenix_closure_t> frame;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  init_closure_context class
    //
    //      The init_closure_context class is a special parser context type
    //      which additionally initializes a closure contained in the derived
    //      parser with values from a given tuple. Please note, that this
    //      given tuple does not contain the required values directly, it
    //      contains phoenix::actor objects. These actors have to be
    //      dereferenced to gain the values to be used for initialization
    //      (this is done by the help of the phoenix::convert_actors<>
    //      template).
    //
    ///////////////////////////////////////////////////////////////////////////

    template <typename ClosureT>
    class init_closure_context : public parser_context_base
    {
        typedef typename ClosureT::tuple_t      tuple_t;
        typedef typename ClosureT::closure_t    closure_t;

    public:

        init_closure_context(ClosureT const& clos)
        : frame(clos.subject(), ::phoenix::convert_actors<tuple_t>(clos.init)) {}

        ~init_closure_context() {}

        template <typename ParserT, typename ScannerT>
        void pre_parse(ParserT const& /*p*/, ScannerT const&) {}

        template <typename ResultT, typename ParserT, typename ScannerT>
        ResultT& post_parse(ResultT& hit, ParserT const&, ScannerT const&)
        { hit.value(frame[::phoenix::tuple_index_names::_1]); return hit; }

    private:

        ::phoenix::closure_frame<closure_t> frame;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  init_closure_parser class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ParserT, typename ActorTupleT>
    struct init_closure_parser
    : public unary<ParserT, parser<init_closure_parser<ParserT, ActorTupleT> > >
    {
        typedef init_closure_parser<ParserT, ActorTupleT>           self_t;
        typedef unary<ParserT, parser<self_t> >                     base_t;
        typedef typename ParserT::phoenix_closure_t                 closure_t;
        typedef typename ParserT::tuple_t                           tuple_t;
        typedef typename ::phoenix::tuple_element<0, tuple_t>::type   attr_t;

        template <typename ScannerT>
        struct result
        {
            typedef typename match_result<ScannerT, attr_t>::type type;
        };

        init_closure_parser(ParserT const& p, ActorTupleT const& init_)
        : base_t(p), init(init_) {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse_main(ScannerT const& scan) const
        {
            return this->subject().parse_main(scan);
        }

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef init_closure_context<self_t> init_context_t;
            typedef parser_scanner_linker<ScannerT> scanner_t;
            typedef closure_context_linker<init_context_t> context_t;
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            BOOST_SPIRIT_CONTEXT_PARSE(
                scan, *this, scanner_t, context_t, result_t);
        }

        ActorTupleT init;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  closure class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <
            typename DerivedT
        ,   typename T0
        ,   typename T1
        ,   typename T2

    #if BOOST_SPIRIT_CLOSURE_LIMIT > 3
        ,   typename T3
        ,   typename T4
        ,   typename T5

    #if BOOST_SPIRIT_CLOSURE_LIMIT > 6
        ,   typename T6
        ,   typename T7
        ,   typename T8

    #if BOOST_SPIRIT_CLOSURE_LIMIT > 9
        ,   typename T9
        ,   typename T10
        ,   typename T11

    #if BOOST_SPIRIT_CLOSURE_LIMIT > 12
        ,   typename T12
        ,   typename T13
        ,   typename T14
    #endif
    #endif
    #endif
    #endif
    >
    struct closure :
        public ::phoenix::closure<
            T0, T1, T2
    #if BOOST_SPIRIT_CLOSURE_LIMIT > 3
        ,   T3, T4, T5
    #if BOOST_SPIRIT_CLOSURE_LIMIT > 6
        ,   T6, T7, T8
    #if BOOST_SPIRIT_CLOSURE_LIMIT > 9
        ,   T9, T10, T11
    #if BOOST_SPIRIT_CLOSURE_LIMIT > 12
        ,   T12, T13, T14
    #endif
    #endif
    #endif
    #endif
        >
    {
        typedef ::phoenix::closure<
                T0, T1, T2
    #if BOOST_SPIRIT_CLOSURE_LIMIT > 3
            ,   T3, T4, T5
    #if BOOST_SPIRIT_CLOSURE_LIMIT > 6
            ,   T6, T7, T8
    #if BOOST_SPIRIT_CLOSURE_LIMIT > 9
            ,   T9, T10, T11
    #if BOOST_SPIRIT_CLOSURE_LIMIT > 12
            ,   T12, T13, T14
    #endif
    #endif
    #endif
    #endif
            > phoenix_closure_t;

        typedef closure_context<DerivedT> context_t;

        template <typename DerivedT2>
        struct aux
        {
            DerivedT2& aux_derived()
            { return *static_cast<DerivedT2*>(this); }

            DerivedT2 const& aux_derived() const
            { return *static_cast<DerivedT2 const*>(this); }

        // initialization functions
            template <typename A>
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type
                >
            >
            operator()(A const &a) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef ::phoenix::tuple<a_t> actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a)
                        )
                    );
            }

            template <typename A, typename B>
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type
                >
            >
            operator()(A const &a, B const &b) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef ::phoenix::tuple<a_t, b_t> actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b)
                        )
                    );
            }

            template <typename A, typename B, typename C>
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type
                >
            >
            operator()(A const &a, B const &b, C const &c) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef ::phoenix::tuple<a_t, b_t, c_t> actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c)
                        )
                    );
            }

    #if BOOST_SPIRIT_CLOSURE_LIMIT > 3

            template <
                typename A, typename B, typename C, typename D
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d)
                        )
                    );
            }

            template <
                typename A, typename B, typename C, typename D, typename E
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e)
                        )
                    );
            }

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f)
                        )
                    );
            }

    #if BOOST_SPIRIT_CLOSURE_LIMIT > 6

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F, typename G
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type,
                    typename ::phoenix::as_actor<G>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f, G const &g
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef typename ::phoenix::as_actor<G>::type g_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t, g_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f),
                            ::phoenix::as_actor<G>::convert(g)
                        )
                    );
            }

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F, typename G, typename H
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type,
                    typename ::phoenix::as_actor<G>::type,
                    typename ::phoenix::as_actor<H>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f, G const &g, H const &h
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef typename ::phoenix::as_actor<G>::type g_t;
                typedef typename ::phoenix::as_actor<H>::type h_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t, g_t, h_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f),
                            ::phoenix::as_actor<G>::convert(g),
                            ::phoenix::as_actor<H>::convert(h)
                        )
                    );
            }

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F, typename G, typename H, typename I
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type,
                    typename ::phoenix::as_actor<G>::type,
                    typename ::phoenix::as_actor<H>::type,
                    typename ::phoenix::as_actor<I>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f, G const &g, H const &h, I const &i
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef typename ::phoenix::as_actor<G>::type g_t;
                typedef typename ::phoenix::as_actor<H>::type h_t;
                typedef typename ::phoenix::as_actor<I>::type i_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t, g_t, h_t, i_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f),
                            ::phoenix::as_actor<G>::convert(g),
                            ::phoenix::as_actor<H>::convert(h),
                            ::phoenix::as_actor<I>::convert(i)
                        )
                    );
            }

    #if BOOST_SPIRIT_CLOSURE_LIMIT > 9

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F, typename G, typename H, typename I, typename J
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type,
                    typename ::phoenix::as_actor<G>::type,
                    typename ::phoenix::as_actor<H>::type,
                    typename ::phoenix::as_actor<I>::type,
                    typename ::phoenix::as_actor<J>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f, G const &g, H const &h, I const &i, J const &j
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef typename ::phoenix::as_actor<G>::type g_t;
                typedef typename ::phoenix::as_actor<H>::type h_t;
                typedef typename ::phoenix::as_actor<I>::type i_t;
                typedef typename ::phoenix::as_actor<J>::type j_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t, g_t, h_t, i_t, j_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f),
                            ::phoenix::as_actor<G>::convert(g),
                            ::phoenix::as_actor<H>::convert(h),
                            ::phoenix::as_actor<I>::convert(i),
                            ::phoenix::as_actor<J>::convert(j)
                        )
                    );
            }

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F, typename G, typename H, typename I, typename J,
                typename K
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type,
                    typename ::phoenix::as_actor<G>::type,
                    typename ::phoenix::as_actor<H>::type,
                    typename ::phoenix::as_actor<I>::type,
                    typename ::phoenix::as_actor<J>::type,
                    typename ::phoenix::as_actor<K>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f, G const &g, H const &h, I const &i, J const &j,
                K const &k
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef typename ::phoenix::as_actor<G>::type g_t;
                typedef typename ::phoenix::as_actor<H>::type h_t;
                typedef typename ::phoenix::as_actor<I>::type i_t;
                typedef typename ::phoenix::as_actor<J>::type j_t;
                typedef typename ::phoenix::as_actor<K>::type k_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t, g_t, h_t, i_t, j_t,
                            k_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f),
                            ::phoenix::as_actor<G>::convert(g),
                            ::phoenix::as_actor<H>::convert(h),
                            ::phoenix::as_actor<I>::convert(i),
                            ::phoenix::as_actor<J>::convert(j),
                            ::phoenix::as_actor<K>::convert(k)
                        )
                    );
            }

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F, typename G, typename H, typename I, typename J,
                typename K, typename L
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type,
                    typename ::phoenix::as_actor<G>::type,
                    typename ::phoenix::as_actor<H>::type,
                    typename ::phoenix::as_actor<I>::type,
                    typename ::phoenix::as_actor<J>::type,
                    typename ::phoenix::as_actor<K>::type,
                    typename ::phoenix::as_actor<L>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f, G const &g, H const &h, I const &i, J const &j,
                K const &k, L const &l
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef typename ::phoenix::as_actor<G>::type g_t;
                typedef typename ::phoenix::as_actor<H>::type h_t;
                typedef typename ::phoenix::as_actor<I>::type i_t;
                typedef typename ::phoenix::as_actor<J>::type j_t;
                typedef typename ::phoenix::as_actor<K>::type k_t;
                typedef typename ::phoenix::as_actor<L>::type l_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t, g_t, h_t, i_t, j_t,
                            k_t, l_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f),
                            ::phoenix::as_actor<G>::convert(g),
                            ::phoenix::as_actor<H>::convert(h),
                            ::phoenix::as_actor<I>::convert(i),
                            ::phoenix::as_actor<J>::convert(j),
                            ::phoenix::as_actor<K>::convert(k),
                            ::phoenix::as_actor<L>::convert(l)
                        )
                    );
            }

    #if BOOST_SPIRIT_CLOSURE_LIMIT > 12

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F, typename G, typename H, typename I, typename J,
                typename K, typename L, typename M
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type,
                    typename ::phoenix::as_actor<G>::type,
                    typename ::phoenix::as_actor<H>::type,
                    typename ::phoenix::as_actor<I>::type,
                    typename ::phoenix::as_actor<J>::type,
                    typename ::phoenix::as_actor<K>::type,
                    typename ::phoenix::as_actor<L>::type,
                    typename ::phoenix::as_actor<M>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f, G const &g, H const &h, I const &i, J const &j,
                K const &k, L const &l, M const &m
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef typename ::phoenix::as_actor<G>::type g_t;
                typedef typename ::phoenix::as_actor<H>::type h_t;
                typedef typename ::phoenix::as_actor<I>::type i_t;
                typedef typename ::phoenix::as_actor<J>::type j_t;
                typedef typename ::phoenix::as_actor<K>::type k_t;
                typedef typename ::phoenix::as_actor<L>::type l_t;
                typedef typename ::phoenix::as_actor<M>::type m_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t, g_t, h_t, i_t, j_t,
                            k_t, l_t, m_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f),
                            ::phoenix::as_actor<G>::convert(g),
                            ::phoenix::as_actor<H>::convert(h),
                            ::phoenix::as_actor<I>::convert(i),
                            ::phoenix::as_actor<J>::convert(j),
                            ::phoenix::as_actor<K>::convert(k),
                            ::phoenix::as_actor<L>::convert(l),
                            ::phoenix::as_actor<M>::convert(m)
                        )
                    );
            }

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F, typename G, typename H, typename I, typename J,
                typename K, typename L, typename M, typename N
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type,
                    typename ::phoenix::as_actor<G>::type,
                    typename ::phoenix::as_actor<H>::type,
                    typename ::phoenix::as_actor<I>::type,
                    typename ::phoenix::as_actor<J>::type,
                    typename ::phoenix::as_actor<K>::type,
                    typename ::phoenix::as_actor<L>::type,
                    typename ::phoenix::as_actor<M>::type,
                    typename ::phoenix::as_actor<N>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f, G const &g, H const &h, I const &i, J const &j,
                K const &k, L const &l, M const &m, N const &n
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef typename ::phoenix::as_actor<G>::type g_t;
                typedef typename ::phoenix::as_actor<H>::type h_t;
                typedef typename ::phoenix::as_actor<I>::type i_t;
                typedef typename ::phoenix::as_actor<J>::type j_t;
                typedef typename ::phoenix::as_actor<K>::type k_t;
                typedef typename ::phoenix::as_actor<L>::type l_t;
                typedef typename ::phoenix::as_actor<M>::type m_t;
                typedef typename ::phoenix::as_actor<N>::type n_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t, g_t, h_t, i_t, j_t,
                            k_t, l_t, m_t, n_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f),
                            ::phoenix::as_actor<G>::convert(g),
                            ::phoenix::as_actor<H>::convert(h),
                            ::phoenix::as_actor<I>::convert(i),
                            ::phoenix::as_actor<J>::convert(j),
                            ::phoenix::as_actor<K>::convert(k),
                            ::phoenix::as_actor<L>::convert(l),
                            ::phoenix::as_actor<M>::convert(m),
                            ::phoenix::as_actor<N>::convert(n)
                        )
                    );
            }

            template <
                typename A, typename B, typename C, typename D, typename E,
                typename F, typename G, typename H, typename I, typename J,
                typename K, typename L, typename M, typename N, typename O
            >
            init_closure_parser<
                DerivedT2,
                ::phoenix::tuple<
                    typename ::phoenix::as_actor<A>::type,
                    typename ::phoenix::as_actor<B>::type,
                    typename ::phoenix::as_actor<C>::type,
                    typename ::phoenix::as_actor<D>::type,
                    typename ::phoenix::as_actor<E>::type,
                    typename ::phoenix::as_actor<F>::type,
                    typename ::phoenix::as_actor<G>::type,
                    typename ::phoenix::as_actor<H>::type,
                    typename ::phoenix::as_actor<I>::type,
                    typename ::phoenix::as_actor<J>::type,
                    typename ::phoenix::as_actor<K>::type,
                    typename ::phoenix::as_actor<L>::type,
                    typename ::phoenix::as_actor<M>::type,
                    typename ::phoenix::as_actor<N>::type,
                    typename ::phoenix::as_actor<O>::type
                >
            >
            operator()(
                A const &a, B const &b, C const &c, D const &d, E const &e,
                F const &f, G const &g, H const &h, I const &i, J const &j,
                K const &k, L const &l, M const &m, N const &n, O const &o
            ) const
            {
                typedef typename ::phoenix::as_actor<A>::type a_t;
                typedef typename ::phoenix::as_actor<B>::type b_t;
                typedef typename ::phoenix::as_actor<C>::type c_t;
                typedef typename ::phoenix::as_actor<D>::type d_t;
                typedef typename ::phoenix::as_actor<E>::type e_t;
                typedef typename ::phoenix::as_actor<F>::type f_t;
                typedef typename ::phoenix::as_actor<G>::type g_t;
                typedef typename ::phoenix::as_actor<H>::type h_t;
                typedef typename ::phoenix::as_actor<I>::type i_t;
                typedef typename ::phoenix::as_actor<J>::type j_t;
                typedef typename ::phoenix::as_actor<K>::type k_t;
                typedef typename ::phoenix::as_actor<L>::type l_t;
                typedef typename ::phoenix::as_actor<M>::type m_t;
                typedef typename ::phoenix::as_actor<N>::type n_t;
                typedef typename ::phoenix::as_actor<O>::type o_t;
                typedef ::phoenix::tuple<
                            a_t, b_t, c_t, d_t, e_t, f_t, g_t, h_t, i_t, j_t,
                            k_t, l_t, m_t, n_t, o_t
                        > actor_tuple_t;

                return init_closure_parser<DerivedT2, actor_tuple_t>(
                        aux_derived(),
                        actor_tuple_t(
                            ::phoenix::as_actor<A>::convert(a),
                            ::phoenix::as_actor<B>::convert(b),
                            ::phoenix::as_actor<C>::convert(c),
                            ::phoenix::as_actor<D>::convert(d),
                            ::phoenix::as_actor<E>::convert(e),
                            ::phoenix::as_actor<F>::convert(f),
                            ::phoenix::as_actor<G>::convert(g),
                            ::phoenix::as_actor<H>::convert(h),
                            ::phoenix::as_actor<I>::convert(i),
                            ::phoenix::as_actor<J>::convert(j),
                            ::phoenix::as_actor<K>::convert(k),
                            ::phoenix::as_actor<L>::convert(l),
                            ::phoenix::as_actor<M>::convert(m),
                            ::phoenix::as_actor<N>::convert(n),
                            ::phoenix::as_actor<O>::convert(o)
                        )
                    );
            }

    #endif
    #endif
    #endif
    #endif
        };

        ~closure() {}
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  overloads for chseq_p and str_p taking in phoenix actors
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ActorT>
    struct container_begin
    {
        typedef container_begin<ActorT> self_t;

        template <typename TupleT>
        struct result
        {
            typedef typename ::phoenix::actor_result<ActorT, TupleT>
                ::plain_type::iterator type;
        };

        container_begin(ActorT actor_)
        : actor(actor_) {}

        template <typename TupleT>
        typename ::phoenix::actor_result<self_t, TupleT>::type
        eval(TupleT const& /*args*/) const
        { return actor().begin(); }

        ActorT actor;
    };

    template <typename ActorT>
    struct container_end
    {
        typedef container_begin<ActorT> self_t;

        template <typename TupleT>
        struct result
        {
            typedef typename ::phoenix::actor_result<ActorT, TupleT>
                ::plain_type::iterator type;
        };

        container_end(ActorT actor_)
        : actor(actor_) {}

        template <typename TupleT>
        typename ::phoenix::actor_result<self_t, TupleT>::type
        eval(TupleT const& /*args*/) const
        { return actor().end(); }

        ActorT actor;
    };

    template <typename BaseT>
    inline f_chseq<
        ::phoenix::actor<container_begin< ::phoenix::actor<BaseT> > >,
        ::phoenix::actor<container_end< ::phoenix::actor<BaseT> > >
    >
    f_chseq_p(::phoenix::actor<BaseT> const& a)
    {
        typedef ::phoenix::actor<container_begin< ::phoenix::actor<BaseT> > >
            container_begin_t;
        typedef ::phoenix::actor<container_end< ::phoenix::actor<BaseT> > >
            container_end_t;
        typedef f_chseq<container_begin_t, container_end_t> result_t;

        return result_t(container_begin_t(a), container_end_t(a));
    }

    template <typename BaseT>
    inline f_strlit<
        ::phoenix::actor<container_begin< ::phoenix::actor<BaseT> > >,
        ::phoenix::actor<container_end< ::phoenix::actor<BaseT> > >
    >
    f_str_p(::phoenix::actor<BaseT> const& a)
    {
        typedef ::phoenix::actor<container_begin< ::phoenix::actor<BaseT> > >
            container_begin_t;
        typedef ::phoenix::actor<container_end< ::phoenix::actor<BaseT> > >
            container_end_t;
        typedef f_strlit<container_begin_t, container_end_t> result_t;

        return result_t(container_begin_t(a), container_end_t(a));
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
