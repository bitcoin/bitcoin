/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2003 Vaclav Vesely
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_NO_ACTIONS_HPP)
#define BOOST_SPIRIT_NO_ACTIONS_HPP

#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/core/non_terminal/rule.hpp>

namespace boost {
namespace spirit {
BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------
// no_actions_action_policy

template<typename BaseT = action_policy>
struct no_actions_action_policy:
    public BaseT
{
    typedef BaseT base_t;

    no_actions_action_policy():
        BaseT()
    {}

    template<typename PolicyT>
    no_actions_action_policy(PolicyT const& other):
        BaseT(other)
    {}

    template<typename ActorT, typename AttrT, typename IteratorT>
    void
    do_action(
        ActorT const&       /*actor*/,
        AttrT&              /*val*/,
        IteratorT const&    /*first*/,
        IteratorT const&    /*last*/) const
    {}
};

//-----------------------------------------------------------------------------
// no_actions_scanner


namespace detail
{
    template <typename ActionPolicy>
    struct compute_no_actions_action_policy
    {
        typedef no_actions_action_policy<ActionPolicy> type;
    };

    template <typename ActionPolicy>
    struct compute_no_actions_action_policy<no_actions_action_policy<ActionPolicy> >
    {
        typedef no_actions_action_policy<ActionPolicy> type;
    };
}

template<typename ScannerT = scanner<> >
struct no_actions_scanner
{
    typedef scanner_policies<
        typename ScannerT::iteration_policy_t,
        typename ScannerT::match_policy_t,
        typename detail::compute_no_actions_action_policy<typename ScannerT::action_policy_t>::type
    > policies_t;

    typedef typename
        rebind_scanner_policies<ScannerT, policies_t>::type type;
};

#if BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 1

template<typename ScannerT = scanner<> >
struct no_actions_scanner_list
{
    typedef
        scanner_list<
            ScannerT,
            typename no_actions_scanner<ScannerT>::type
        >
            type;
};

#endif // BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 1

//-----------------------------------------------------------------------------
// no_actions_parser

struct no_actions_parser_gen;

template<typename ParserT>
struct no_actions_parser:
    public unary<ParserT, parser<no_actions_parser<ParserT> > >
{
    typedef no_actions_parser<ParserT>      self_t;
    typedef unary_parser_category           parser_category_t;
    typedef no_actions_parser_gen           parser_generator_t;
    typedef unary<ParserT, parser<self_t> > base_t;

    template<typename ScannerT>
    struct result
    {
        typedef typename parser_result<ParserT, ScannerT>::type type;
    };

    no_actions_parser(ParserT const& p)
    :   base_t(p)
    {}

    template<typename ScannerT>
    typename result<ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename no_actions_scanner<ScannerT>::policies_t policies_t;

        return this->subject().parse(scan.change_policies(policies_t(scan)));
    }
};

//-----------------------------------------------------------------------------
// no_actions_parser_gen

struct no_actions_parser_gen
{
    template<typename ParserT>
    struct result
    {
        typedef no_actions_parser<ParserT> type;
    };

    template<typename ParserT>
    static no_actions_parser<ParserT>
    generate(parser<ParserT> const& subject)
    {
        return no_actions_parser<ParserT>(subject.derived());
    }

    template<typename ParserT>
    no_actions_parser<ParserT>
    operator[](parser<ParserT> const& subject) const
    {
        return no_actions_parser<ParserT>(subject.derived());
    }
};

//-----------------------------------------------------------------------------
// no_actions_d

const no_actions_parser_gen no_actions_d = no_actions_parser_gen();

//-----------------------------------------------------------------------------
BOOST_SPIRIT_CLASSIC_NAMESPACE_END
} // namespace spirit
} // namespace boost

#endif // !defined(BOOST_SPIRIT_NO_ACTIONS_HPP)
