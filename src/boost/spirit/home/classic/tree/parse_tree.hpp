/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2001-2007 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_TREE_PARSE_TREE_HPP
#define BOOST_SPIRIT_TREE_PARSE_TREE_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/tree/common.hpp>
#include <boost/spirit/home/classic/core/scanner/scanner.hpp>

#include <boost/spirit/home/classic/tree/parse_tree_fwd.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

//////////////////////////////////
// pt_match_policy is simply an id so the correct specialization of tree_policy can be found.
template <
    typename IteratorT,
    typename NodeFactoryT,
    typename T 
>
struct pt_match_policy :
    public common_tree_match_policy<
        pt_match_policy<IteratorT, NodeFactoryT, T>,
        IteratorT,
        NodeFactoryT,
        pt_tree_policy<
            pt_match_policy<IteratorT, NodeFactoryT, T>,
            NodeFactoryT,
            T
        >,
        T
    >
{
    typedef
        common_tree_match_policy<
            pt_match_policy<IteratorT, NodeFactoryT, T>,
            IteratorT,
            NodeFactoryT,
            pt_tree_policy<
                pt_match_policy<IteratorT, NodeFactoryT, T>,
                NodeFactoryT,
                T
            >,
            T
        >
    common_tree_match_policy_;

    pt_match_policy()
    {
    }

    template <typename PolicyT>
    pt_match_policy(PolicyT const & policies)
        : common_tree_match_policy_(policies)
    {
    }
};

//////////////////////////////////
template <typename MatchPolicyT, typename NodeFactoryT, typename T>
struct pt_tree_policy :
    public common_tree_tree_policy<MatchPolicyT, NodeFactoryT>
{
    typedef typename MatchPolicyT::match_t match_t;
    typedef typename MatchPolicyT::iterator_t iterator_t;

    template<typename MatchAT, typename MatchBT>
    static void concat(MatchAT& a, MatchBT const& b)
    {
        BOOST_SPIRIT_ASSERT(a && b);

        std::copy(b.trees.begin(), b.trees.end(),
            std::back_insert_iterator<typename match_t::container_t>(a.trees));
    }

    template <typename MatchT, typename Iterator1T, typename Iterator2T>
    static void group_match(MatchT& m, parser_id const& id,
            Iterator1T const& first, Iterator2T const& last)
    {
        if (!m)
            return;

        typedef typename NodeFactoryT::template factory<iterator_t> factory_t;
        typedef typename tree_match<iterator_t, NodeFactoryT, T>::container_t
            container_t;
        typedef typename container_t::iterator cont_iterator_t;

        match_t newmatch(m.length(),
                factory_t::create_node(first, last, false));

        std::swap(newmatch.trees.begin()->children, m.trees);
        // set this node and all it's unset children's rule_id
        newmatch.trees.begin()->value.id(id);
        for (cont_iterator_t i = newmatch.trees.begin()->children.begin();
                i != newmatch.trees.begin()->children.end();
                ++i)
        {
            if (i->value.id() == 0)
                i->value.id(id);
        }
        m = newmatch;
    }

    template <typename FunctorT, typename MatchT>
    static void apply_op_to_match(FunctorT const& op, MatchT& m)
    {
        op(m);
    }
};

namespace impl {

    template <typename IteratorT, typename NodeFactoryT, typename T>
    struct tree_policy_selector<pt_match_policy<IteratorT, NodeFactoryT, T> >
    {
        typedef pt_tree_policy<
            pt_match_policy<IteratorT, NodeFactoryT, T>, 
            NodeFactoryT, 
            T
        > type;
    };

} // namespace impl


//////////////////////////////////
struct gen_pt_node_parser_gen;

template <typename T>
struct gen_pt_node_parser
:   public unary<T, parser<gen_pt_node_parser<T> > >
{
    typedef gen_pt_node_parser<T> self_t;
    typedef gen_pt_node_parser_gen parser_generator_t;
    typedef unary_parser_category parser_category_t;

    gen_pt_node_parser(T const& a)
    : unary<T, parser<gen_pt_node_parser<T> > >(a) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename ScannerT::iteration_policy_t iteration_policy_t;
        typedef typename ScannerT::match_policy_t::iterator_t iterator_t;
        typedef typename ScannerT::match_policy_t::factory_t factory_t;
        typedef pt_match_policy<iterator_t, factory_t> match_policy_t;
        typedef typename ScannerT::action_policy_t action_policy_t;
        typedef scanner_policies<
            iteration_policy_t,
            match_policy_t,
            action_policy_t
        > policies_t;

        return this->subject().parse(scan.change_policies(policies_t(scan)));
    }
};

//////////////////////////////////
struct gen_pt_node_parser_gen
{
    template <typename T>
    struct result {

        typedef gen_pt_node_parser<T> type;
    };

    template <typename T>
    static gen_pt_node_parser<T>
    generate(parser<T> const& s)
    {
        return gen_pt_node_parser<T>(s.derived());
    }

    template <typename T>
    gen_pt_node_parser<T>
    operator[](parser<T> const& s) const
    {
        return gen_pt_node_parser<T>(s.derived());
    }
};

//////////////////////////////////
const gen_pt_node_parser_gen gen_pt_node_d = gen_pt_node_parser_gen();


///////////////////////////////////////////////////////////////////////////////
//
//  Parse functions for parse trees
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename NodeFactoryT, typename IteratorT, typename ParserT, 
    typename SkipT
>
inline tree_parse_info<IteratorT, NodeFactoryT>
pt_parse(
    IteratorT const&        first_,
    IteratorT const&        last,
    parser<ParserT> const&  p,
    SkipT const&            skip,
    NodeFactoryT const&   /*dummy_*/ = NodeFactoryT())
{
    typedef skip_parser_iteration_policy<SkipT> it_policy_t;
    typedef pt_match_policy<IteratorT, NodeFactoryT> pt_match_policy_t;
    typedef
        scanner_policies<it_policy_t, pt_match_policy_t>
        scan_policies_t;
    typedef scanner<IteratorT, scan_policies_t> scanner_t;

    it_policy_t iter_policy(skip);
    scan_policies_t policies(iter_policy);
    IteratorT first = first_;
    scanner_t scan(first, last, policies);
    tree_match<IteratorT, NodeFactoryT> hit = p.derived().parse(scan);
    return tree_parse_info<IteratorT, NodeFactoryT>(
        first, hit, hit && (first == last), hit.length(), hit.trees);
}

template <typename IteratorT, typename ParserT, typename SkipT>
inline tree_parse_info<IteratorT>
pt_parse(
    IteratorT const&        first,
    IteratorT const&        last,
    parser<ParserT> const&  p,
    SkipT const&            skip)
{
    typedef node_val_data_factory<nil_t> default_node_factory_t;
    return pt_parse(first, last, p, skip, default_node_factory_t());
}

//////////////////////////////////
template <typename IteratorT, typename ParserT>
inline tree_parse_info<IteratorT>
pt_parse(
    IteratorT const&        first_,
    IteratorT const&        last,
    parser<ParserT> const&  parser)
{
    typedef pt_match_policy<IteratorT> pt_match_policy_t;
    IteratorT first = first_;
    scanner<
        IteratorT,
        scanner_policies<iteration_policy, pt_match_policy_t>
    > scan(first, last);
    tree_match<IteratorT> hit = parser.derived().parse(scan);
    return tree_parse_info<IteratorT>(
        first, hit, hit && (first == last), hit.length(), hit.trees);
}

//////////////////////////////////
template <typename CharT, typename ParserT, typename SkipT>
inline tree_parse_info<CharT const*>
pt_parse(
    CharT const*            str,
    parser<ParserT> const&  p,
    SkipT const&            skip)
{
    CharT const* last = str;
    while (*last)
        last++;
    return pt_parse(str, last, p, skip);
}

//////////////////////////////////
template <typename CharT, typename ParserT>
inline tree_parse_info<CharT const*>
pt_parse(
    CharT const*            str,
    parser<ParserT> const&  parser)
{
    CharT const* last = str;
    while (*last)
    {
        last++;
    }
    return pt_parse(str, last, parser);
}

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

