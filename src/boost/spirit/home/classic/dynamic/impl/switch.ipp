/*=============================================================================
    Copyright (c) 2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_SWITCH_IPP
#define BOOST_SPIRIT_SWITCH_IPP

#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <boost/core/ignore_unused.hpp>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/inc.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/repeat_from_to.hpp>

#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/primitives/primitives.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/meta/as_parser.hpp>

#include <boost/spirit/home/classic/phoenix/actor.hpp>
#include <boost/spirit/home/classic/phoenix/tuples.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

// forward declaration
template <int N, typename ParserT, bool IsDefault> struct case_parser;

///////////////////////////////////////////////////////////////////////////////
namespace impl {

///////////////////////////////////////////////////////////////////////////////
//  parse helper functions
template <typename ParserT, typename ScannerT>
inline typename parser_result<ParserT, ScannerT>::type
delegate_parse(ParserT const &p, ScannerT const &scan,
    typename ScannerT::iterator_t const save)
{
    typedef typename parser_result<ParserT, ScannerT>::type result_t;

    result_t result (p.subject().parse(scan));
    if (!result)
        scan.first = save;
    return result;
}

///////////////////////////////////////////////////////////////////////////////
//  General default case handling (no default_p case branch given).
//  First try to match the current parser node (if the condition value is
//  matched) and, if that fails, return a no_match
template <int N, bool IsDefault, bool HasDefault>
struct default_delegate_parse {

    template <
        typename ParserT, typename DefaultT,
        typename ValueT, typename ScannerT
    >
    static typename parser_result<ParserT, ScannerT>::type
    parse (ValueT const &value, ParserT const &p, DefaultT const &,
        ScannerT const &scan, typename ScannerT::iterator_t const save)
    {
        if (value == N)
            return delegate_parse(p, scan, save);
        return scan.no_match();
    }
};

//  The current case parser node is the default parser.
//  Ignore the given case value and try to match the given default parser.
template <int N, bool HasDefault>
struct default_delegate_parse<N, true, HasDefault> {

    template <
        typename ParserT, typename DefaultT,
        typename ValueT, typename ScannerT
    >
    static typename parser_result<ParserT, ScannerT>::type
    parse (ValueT const& /*value*/, ParserT const &, DefaultT const &d,
        ScannerT const &scan, typename ScannerT::iterator_t const save)
    {
        //  Since there is a default_p case branch defined, the corresponding
        //  parser shouldn't be the nothing_parser
        BOOST_STATIC_ASSERT((!boost::is_same<DefaultT, nothing_parser>::value));
        return delegate_parse(d, scan, save);
    }
};

//  The current case parser node is not the default parser, but there is a
//  default_p branch given inside the switch_p parser.
//  First try to match the current parser node (if the condition value is
//  matched) and, if that fails, match the given default_p parser.
template <int N>
struct default_delegate_parse<N, false, true> {

    template <
        typename ParserT, typename DefaultT,
        typename ValueT, typename ScannerT
    >
    static typename parser_result<ParserT, ScannerT>::type
    parse (ValueT const &value, ParserT const &p, DefaultT const &d,
        ScannerT const &scan, typename ScannerT::iterator_t const save)
    {
        //  Since there is a default_p case branch defined, the corresponding
        //  parser shouldn't be the nothing_parser
        BOOST_STATIC_ASSERT((!boost::is_same<DefaultT, nothing_parser>::value));
        if (value == N)
            return delegate_parse(p, scan, save);

        return delegate_parse(d, scan, save);
    }
};

///////////////////////////////////////////////////////////////////////////////
//  Look through the case parser chain to test, if there is a default case
//  branch defined (returned by 'value').
template <typename CaseT, bool IsSimple = CaseT::is_simple>
struct default_case;

////////////////////////////////////////
template <typename ResultT, bool IsDefault>
struct get_default_parser {

    template <typename ParserT>
    static ResultT
    get(parser<ParserT> const &p)
    {
        return default_case<typename ParserT::derived_t::left_t>::
            get(p.derived().left());
    }
};

template <typename ResultT>
struct get_default_parser<ResultT, true> {

    template <typename ParserT>
    static ResultT
    get(parser<ParserT> const &p) { return p.derived().right(); }
};

////////////////////////////////////////
template <typename CaseT, bool IsSimple>
struct default_case {

    //  The 'value' constant is true, if the current case_parser or one of its
    //  left siblings is a default_p generated case_parser.
    BOOST_STATIC_CONSTANT(bool, value =
        (CaseT::is_default || default_case<typename CaseT::left_t>::value));

    //  The 'is_epsilon' constant is true, if the current case_parser or one of
    //  its left siblings is a default_p generated parser with an attached
    //  epsilon_p (this is generated by the plain default_p).
    BOOST_STATIC_CONSTANT(bool, is_epsilon = (
        (CaseT::is_default && CaseT::is_epsilon) ||
            default_case<typename CaseT::left_t>::is_epsilon
    ));

    //  The computed 'type' represents the type of the default case branch
    //  parser (if there is one) or nothing_parser (if there isn't any default
    //  case branch).
    typedef typename boost::mpl::if_c<
            CaseT::is_default, typename CaseT::right_embed_t,
            typename default_case<typename CaseT::left_t>::type
        >::type type;

    //  The get function returns the parser attached to the default case branch
    //  (if there is one) or an instance of a nothing_parser (if there isn't
    //  any default case branch).
    template <typename ParserT>
    static type
    get(parser<ParserT> const &p)
    { return get_default_parser<type, CaseT::is_default>::get(p); }
};

////////////////////////////////////////
template <typename ResultT, bool IsDefault>
struct get_default_parser_simple {

    template <typename ParserT>
    static ResultT
    get(parser<ParserT> const &p) { return p.derived(); }
};

template <typename ResultT>
struct get_default_parser_simple<ResultT, false> {

    template <typename ParserT>
    static nothing_parser
    get(parser<ParserT> const &) { return nothing_p; }
};

////////////////////////////////////////
//  Specialization of the default_case template for the last (leftmost) element
//  of the case parser chain.
template <typename CaseT>
struct default_case<CaseT, true> {

    //  The 'value' and 'is_epsilon' constant, the 'type' type and the function
    //  'get' are described above.

    BOOST_STATIC_CONSTANT(bool, value = CaseT::is_default);
    BOOST_STATIC_CONSTANT(bool, is_epsilon = (
        CaseT::is_default && CaseT::is_epsilon
    ));

    typedef typename boost::mpl::if_c<
            CaseT::is_default, CaseT, nothing_parser
        >::type type;

    template <typename ParserT>
    static type
    get(parser<ParserT> const &p)
    { return get_default_parser_simple<type, value>::get(p); }
};

///////////////////////////////////////////////////////////////////////////////
//  The case_chain template calculates recursively the depth of the left
//  subchain of the given case branch node.
template <typename CaseT, bool IsSimple = CaseT::is_simple>
struct case_chain {

    BOOST_STATIC_CONSTANT(int, depth = (
        case_chain<typename CaseT::left_t>::depth + 1
    ));
};

template <typename CaseT>
struct case_chain<CaseT, true> {

    BOOST_STATIC_CONSTANT(int, depth = 0);
};

///////////////////////////////////////////////////////////////////////////////
//  The chain_parser template is used to extract the type and the instance of
//  a left or a right parser, buried arbitrary deep inside the case parser
//  chain.
template <int Depth, typename CaseT>
struct chain_parser {

    typedef typename CaseT::left_t our_left_t;

    typedef typename chain_parser<Depth-1, our_left_t>::left_t  left_t;
    typedef typename chain_parser<Depth-1, our_left_t>::right_t right_t;

    static left_t
    left(CaseT const &p)
    { return chain_parser<Depth-1, our_left_t>::left(p.left()); }

    static right_t
    right(CaseT const &p)
    { return chain_parser<Depth-1, our_left_t>::right(p.left()); }
};

template <typename CaseT>
struct chain_parser<1, CaseT> {

    typedef typename CaseT::left_t  left_t;
    typedef typename CaseT::right_t right_t;

    static left_t left(CaseT const &p) { return p.left(); }
    static right_t right(CaseT const &p) { return p.right(); }
};

template <typename CaseT>
struct chain_parser<0, CaseT>;      // shouldn't be instantiated

///////////////////////////////////////////////////////////////////////////////
//  Type computing meta function for calculating the type of the return value
//  of the used conditional switch expression
template <typename TargetT, typename ScannerT>
struct condition_result {

    typedef typename TargetT::template result<ScannerT>::type type;
};

///////////////////////////////////////////////////////////////////////////////
template <typename LeftT, typename RightT, bool IsDefault>
struct compound_case_parser
:   public binary<LeftT, RightT,
        parser<compound_case_parser<LeftT, RightT, IsDefault> > >
{
    typedef compound_case_parser<LeftT, RightT, IsDefault>    self_t;
    typedef binary_parser_category                  parser_category_t;
    typedef binary<LeftT, RightT, parser<self_t> >  base_t;

    BOOST_STATIC_CONSTANT(int, value = RightT::value);
    BOOST_STATIC_CONSTANT(bool, is_default = IsDefault);
    BOOST_STATIC_CONSTANT(bool, is_simple = false);
    BOOST_STATIC_CONSTANT(bool, is_epsilon = (
        is_default &&
            boost::is_same<typename RightT::subject_t, epsilon_parser>::value
    ));

    compound_case_parser(parser<LeftT> const &lhs, parser<RightT> const &rhs)
    :   base_t(lhs.derived(), rhs.derived())
    {}

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, nil_t>::type type;
    };

    template <typename ScannerT, typename CondT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan, CondT const &cond) const;

    template <int N1, typename ParserT1, bool IsDefault1>
    compound_case_parser<
        self_t, case_parser<N1, ParserT1, IsDefault1>, IsDefault1
    >
    operator, (case_parser<N1, ParserT1, IsDefault1> const &p) const
    {
        //  If the following compile time assertion fires, you've probably used
        //  more than one default_p case inside the switch_p parser construct.
        BOOST_STATIC_ASSERT(!default_case<self_t>::value || !IsDefault1);

        //  If this compile time assertion fires, you've probably want to use
        //  more case_p/default_p case branches, than possible.
        BOOST_STATIC_ASSERT(
            case_chain<self_t>::depth < BOOST_SPIRIT_SWITCH_CASE_LIMIT
        );

        typedef case_parser<N1, ParserT1, IsDefault1> rhs_t;
        return compound_case_parser<self_t, rhs_t, IsDefault1>(*this, p);
    }
};

///////////////////////////////////////////////////////////////////////////////
//  The parse_switch::do_ functions dispatch to the correct parser, which is
//  selected through the given conditional switch value.
template <int Value, int Depth, bool IsDefault>
struct parse_switch;

///////////////////////////////////////////////////////////////////////////////
//
//  The following generates a couple of parse_switch template specializations
//  with an increasing number of handled case branches (for 1..N).
//
//      template <int Value, bool IsDefault>
//      struct parse_switch<Value, N, IsDefault> {
//
//          template <typename ParserT, typename ScannerT>
//          static typename parser_result<ParserT, ScannerT>::type
//          do_(ParserT const &p, ScannerT const &scan, long cond_value,
//              typename ScannerT::iterator_t const &save)
//          {
//              typedef ParserT left_t0;
//              typedef typename left_t0::left left_t1;
//              ...
//
//              switch (cond_value) {
//              case left_tN::value:
//                  return delegate_parse(chain_parser<
//                          case_chain<ParserT>::depth, ParserT
//                      >::left(p), scan, save);
//              ...
//              case left_t1::value:
//                  return delegate_parse(chain_parser<
//                          1, left_t1
//                      >::right(p.left()), scan, save);
//
//              case left_t0::value:
//              default:
//                  typedef default_case<ParserT> default_t;
//                  typedef default_delegate_parse<
//                              Value, IsDefault, default_t::value>
//                      default_parse_t;
//
//                  return default_parse_t::parse(cond_value, p.right(),
//                      default_t::get(p), scan, save);
//              }
//          }
//      };
//
///////////////////////////////////////////////////////////////////////////////
#define BOOST_SPIRIT_PARSE_SWITCH_TYPEDEFS(z, N, _)                         \
    typedef typename BOOST_PP_CAT(left_t, N)::left_t                        \
        BOOST_PP_CAT(left_t, BOOST_PP_INC(N));                              \
    /**/

#define BOOST_SPIRIT_PARSE_SWITCH_CASES(z, N, _)                            \
    case (long)(BOOST_PP_CAT(left_t, N)::value):                            \
        return delegate_parse(chain_parser<N, left_t1>::right(p.left()),    \
            scan, save);                                                    \
    /**/

#define BOOST_SPIRIT_PARSE_SWITCHES(z, N, _)                                \
    template <int Value, bool IsDefault>                                    \
    struct parse_switch<Value, BOOST_PP_INC(N), IsDefault> {                \
                                                                            \
        template <typename ParserT, typename ScannerT>                      \
        static typename parser_result<ParserT, ScannerT>::type              \
        do_(ParserT const &p, ScannerT const &scan, long cond_value,        \
            typename ScannerT::iterator_t const &save)                      \
        {                                                                   \
            typedef ParserT left_t0;                                        \
            BOOST_PP_REPEAT_FROM_TO_ ## z(0, BOOST_PP_INC(N),               \
                BOOST_SPIRIT_PARSE_SWITCH_TYPEDEFS, _)                      \
                                                                            \
            switch (cond_value) {                                           \
            case (long)(BOOST_PP_CAT(left_t, BOOST_PP_INC(N))::value):      \
                return delegate_parse(                                      \
                    chain_parser<                                           \
                        case_chain<ParserT>::depth, ParserT                 \
                    >::left(p), scan, save);                                \
                                                                            \
            BOOST_PP_REPEAT_FROM_TO_ ## z(1, BOOST_PP_INC(N),               \
                BOOST_SPIRIT_PARSE_SWITCH_CASES, _)                         \
                                                                            \
            case (long)(left_t0::value):                                    \
            default:                                                        \
                typedef default_case<ParserT> default_t;                    \
                typedef                                                     \
                    default_delegate_parse<Value, IsDefault, default_t::value> \
                    default_parse_t;                                        \
                                                                            \
                return default_parse_t::parse(cond_value, p.right(),        \
                    default_t::get(p), scan, save);                         \
            }                                                               \
        }                                                                   \
    };                                                                      \
    /**/

BOOST_PP_REPEAT(BOOST_PP_DEC(BOOST_SPIRIT_SWITCH_CASE_LIMIT),
    BOOST_SPIRIT_PARSE_SWITCHES, _)

#undef BOOST_SPIRIT_PARSE_SWITCH_TYPEDEFS
#undef BOOST_SPIRIT_PARSE_SWITCH_CASES
#undef BOOST_SPIRIT_PARSE_SWITCHES
///////////////////////////////////////////////////////////////////////////////

template <typename LeftT, typename RightT, bool IsDefault>
template <typename ScannerT, typename CondT>
inline typename parser_result<
    compound_case_parser<LeftT, RightT, IsDefault>, ScannerT
>::type
compound_case_parser<LeftT, RightT, IsDefault>::
    parse(ScannerT const& scan, CondT const &cond) const
{
    ignore_unused(scan.at_end());    // allow skipper to take effect
    return parse_switch<value, case_chain<self_t>::depth, is_default>::
        do_(*this, scan, cond(scan), scan.first);
}

///////////////////////////////////////////////////////////////////////////////
//  The switch condition is to be evaluated from a parser result value.
template <typename ParserT>
struct cond_functor {

    typedef cond_functor<ParserT> self_t;

    cond_functor(ParserT const &p_)
    :   p(p_)
    {}

    template <typename ScannerT>
    struct result
    {
        typedef typename parser_result<ParserT, ScannerT>::type::attr_t type;
    };

    template <typename ScannerT>
    typename condition_result<self_t, ScannerT>::type
    operator()(ScannerT const &scan) const
    {
        typedef typename parser_result<ParserT, ScannerT>::type result_t;
        typedef typename result_t::attr_t attr_t;

        result_t result(p.parse(scan));
        return !result ? attr_t() : result.value();
    }

    typename ParserT::embed_t p;
};

template <typename ParserT>
struct make_cond_functor {

    typedef as_parser<ParserT> as_parser_t;

    static cond_functor<typename as_parser_t::type>
    do_(ParserT const &cond)
    {
        return cond_functor<typename as_parser_t::type>(
            as_parser_t::convert(cond));
    }
};

///////////////////////////////////////////////////////////////////////////////
//  The switch condition is to be evaluated from a phoenix actor
template <typename ActorT>
struct cond_actor {

    typedef cond_actor<ActorT> self_t;

    cond_actor(ActorT const &actor_)
    :   actor(actor_)
    {}

    template <typename ScannerT>
    struct result
    {
        typedef typename ::phoenix::actor_result<ActorT, ::phoenix::tuple<> >::type
            type;
    };

    template <typename ScannerT>
    typename condition_result<self_t, ScannerT>::type
    operator()(ScannerT const& /*scan*/) const
    {
        return actor();
    }

    ActorT const &actor;
};

template <typename ActorT>
struct make_cond_functor< ::phoenix::actor<ActorT> > {

    static cond_actor< ::phoenix::actor<ActorT> >
    do_(::phoenix::actor<ActorT> const &actor)
    {
        return cond_actor< ::phoenix::actor<ActorT> >(actor);
    }
};

///////////////////////////////////////////////////////////////////////////////
//  The switch condition is to be taken directly from the input stream
struct get_next_token_cond {

    typedef get_next_token_cond self_t;

    template <typename ScannerT>
    struct result
    {
        typedef typename ScannerT::value_t type;
    };

    template <typename ScannerT>
    typename condition_result<self_t, ScannerT>::type
    operator()(ScannerT const &scan) const
    {
        typename ScannerT::value_t val(*scan);
        ++scan.first;
        return val;
    }
};

template <>
struct make_cond_functor<get_next_token_cond> {

    static get_next_token_cond
    do_(get_next_token_cond const &cond)
    {
        return cond;
    }
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace impl

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}  // namespace boost::spirit

#endif  // BOOST_SPIRIT_SWITCH_IPP
