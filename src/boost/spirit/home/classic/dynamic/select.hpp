/*=============================================================================
    Copyright (c) 2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_SELECT_HPP
#define BOOST_SPIRIT_SELECT_HPP

#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/enum.hpp>
#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/enum_params_with_defaults.hpp>
#include <boost/preprocessor/inc.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>

#include <boost/spirit/home/classic/phoenix/tuples.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit predefined maximum number of possible embedded select_p parsers.
//  It should NOT be greater than PHOENIX_LIMIT!
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_SELECT_LIMIT)
#define BOOST_SPIRIT_SELECT_LIMIT PHOENIX_LIMIT
#endif // !defined(BOOST_SPIRIT_SELECT_LIMIT)

///////////////////////////////////////////////////////////////////////////////
//
// ensure   BOOST_SPIRIT_SELECT_LIMIT <= PHOENIX_LIMIT and 
//          BOOST_SPIRIT_SELECT_LIMIT > 0
//          BOOST_SPIRIT_SELECT_LIMIT <= 15
//
//  [Pushed this down a little to make CW happy with BOOST_STATIC_ASSERT]
//  [Otherwise, it complains: 'boost_static_assert_test_42' redefined]
//
///////////////////////////////////////////////////////////////////////////////
BOOST_STATIC_ASSERT(BOOST_SPIRIT_SELECT_LIMIT <= PHOENIX_LIMIT);
BOOST_STATIC_ASSERT(BOOST_SPIRIT_SELECT_LIMIT > 0);
BOOST_STATIC_ASSERT(BOOST_SPIRIT_SELECT_LIMIT <= 15);

///////////////////////////////////////////////////////////////////////////////
//
//  Calculate the required amount of tuple members rounded up to the nearest 
//  integer dividable by 3
//
///////////////////////////////////////////////////////////////////////////////
#if BOOST_SPIRIT_SELECT_LIMIT > 12
#define BOOST_SPIRIT_SELECT_LIMIT_A     15
#elif BOOST_SPIRIT_SELECT_LIMIT > 9
#define BOOST_SPIRIT_SELECT_LIMIT_A     12
#elif BOOST_SPIRIT_SELECT_LIMIT > 6
#define BOOST_SPIRIT_SELECT_LIMIT_A     9
#elif BOOST_SPIRIT_SELECT_LIMIT > 3
#define BOOST_SPIRIT_SELECT_LIMIT_A     6
#else
#define BOOST_SPIRIT_SELECT_LIMIT_A     3
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  The select_default_no_fail and select_default_fail structs are used to 
//  distinguish two different behaviours for the select_parser in case that not
//  any of the given sub-parsers match.
//
//  If the select_parser is used with the select_default_no_fail behaviour,
//  then in case of no matching sub-parser the whole select_parser returns an
//  empty match and the value -1.
//
//  If the select_parser is used with the select_default_fail behaviour, then
//  in case of no matching sub-parser the whole select_parser fails to match at 
//  all.
//
///////////////////////////////////////////////////////////////////////////////
struct select_default_no_fail {};
struct select_default_fail {};

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}  // namespace BOOST_SPIRIT_CLASSIC_NS

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/dynamic/impl/select.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
template <typename TupleT, typename BehaviourT, typename T>
struct select_parser
:   public parser<select_parser<TupleT, BehaviourT, T> >
{
    typedef select_parser<TupleT, BehaviourT, T> self_t;

    select_parser(TupleT const &t_)
    :   t(t_)
    {}
    
    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, T>::type type;
    };

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        
        if (!scan.at_end()) {
            return impl::parse_tuple_element<
                TupleT::length, result_t, TupleT, BehaviourT>::do_(t, scan);
        }
        return impl::select_match_gen<result_t, BehaviourT>::do_(scan);
    }
        
    TupleT const t;
};

///////////////////////////////////////////////////////////////////////////////
template <typename BehaviourT, typename T = int>
struct select_parser_gen {

    ///////////////////////////////////////////////////////////////////////////
    //
    //  This generates different select_parser_gen::operator()() functions with 
    //  an increasing number of parser parameters:
    //
    //      template <typename ParserT0, ...>
    //      select_parser<
    //          ::phoenix::tuple<
    //              typename impl::as_embedded_parser<ParserT0>::type,
    //              ...
    //          >,
    //          BehaviourT,
    //          T
    //      >
    //      operator()(ParserT0 const &p0, ...) const
    //      {
    //          typedef impl::as_embedded_parser<ParserT0> parser_t0;
    //          ...
    //
    //          typedef ::phoenix::tuple< 
    //                  parser_t0::type,
    //                  ...
    //              > tuple_t; 
    //          typedef select_parser<tuple_t, BehaviourT, T> result_t;
    //
    //          return result_t(tuple_t(
    //                  parser_t0::convert(p0),
    //                  ...
    //              ));
    //      }
    //
    //  The number of generated functions depends on the maximum tuple member 
    //  limit defined by the PHOENIX_LIMIT pp constant. 
    //
    ///////////////////////////////////////////////////////////////////////////
    #define BOOST_SPIRIT_SELECT_EMBEDDED(z, N, _)                           \
        typename impl::as_embedded_parser<BOOST_PP_CAT(ParserT, N)>::type   \
        /**/
    #define BOOST_SPIRIT_SELECT_EMBEDDED_TYPEDEF(z, N, _)                   \
        typedef impl::as_embedded_parser<BOOST_PP_CAT(ParserT, N)>          \
            BOOST_PP_CAT(parser_t, N);                                      \
        /**/
    #define BOOST_SPIRIT_SELECT_CONVERT(z, N, _)                            \
        BOOST_PP_CAT(parser_t, N)::convert(BOOST_PP_CAT(p, N))              \
        /**/
        
    #define BOOST_SPIRIT_SELECT_PARSER(z, N, _)                             \
        template <                                                          \
            BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), typename ParserT)    \
        >                                                                   \
        select_parser<                                                      \
            ::phoenix::tuple<                                                 \
                BOOST_PP_ENUM_ ## z(BOOST_PP_INC(N),                        \
                    BOOST_SPIRIT_SELECT_EMBEDDED, _)                        \
            >,                                                              \
            BehaviourT,                                                     \
            T                                                               \
        >                                                                   \
        operator()(                                                         \
            BOOST_PP_ENUM_BINARY_PARAMS_Z(z, BOOST_PP_INC(N),               \
                ParserT, const &p)                                          \
        ) const                                                             \
        {                                                                   \
            BOOST_PP_REPEAT_ ## z(BOOST_PP_INC(N),                          \
                BOOST_SPIRIT_SELECT_EMBEDDED_TYPEDEF, _)                    \
                                                                            \
            typedef ::phoenix::tuple<                                         \
                    BOOST_PP_ENUM_BINARY_PARAMS_Z(z, BOOST_PP_INC(N),       \
                        typename parser_t, ::type BOOST_PP_INTERCEPT)       \
                > tuple_t;                                                  \
            typedef select_parser<tuple_t, BehaviourT, T> result_t;         \
                                                                            \
            return result_t(tuple_t(                                        \
                    BOOST_PP_ENUM_ ## z(BOOST_PP_INC(N),                    \
                        BOOST_SPIRIT_SELECT_CONVERT, _)                     \
                ));                                                         \
        }                                                                   \
        /**/
        
    BOOST_PP_REPEAT(BOOST_SPIRIT_SELECT_LIMIT_A, 
        BOOST_SPIRIT_SELECT_PARSER, _)
        
    #undef BOOST_SPIRIT_SELECT_PARSER
    #undef BOOST_SPIRIT_SELECT_CONVERT
    #undef BOOST_SPIRIT_SELECT_EMBEDDED_TYPEDEF
    #undef BOOST_SPIRIT_SELECT_EMBEDDED
    ///////////////////////////////////////////////////////////////////////////
};

///////////////////////////////////////////////////////////////////////////////
//
//  Predefined parser generator helper objects
//
///////////////////////////////////////////////////////////////////////////////
select_parser_gen<select_default_no_fail> const select_p = 
    select_parser_gen<select_default_no_fail>();

select_parser_gen<select_default_fail> const select_fail_p = 
    select_parser_gen<select_default_fail>();

#undef BOOST_SPIRIT_SELECT_LIMIT_A

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}  // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // BOOST_SPIRIT_SELECT_HPP
