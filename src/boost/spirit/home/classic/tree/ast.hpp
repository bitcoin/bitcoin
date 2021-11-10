/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2001-2007 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_TREE_AST_HPP
#define BOOST_SPIRIT_TREE_AST_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/tree/common.hpp>
#include <boost/spirit/home/classic/core/scanner/scanner.hpp>

#include <boost/spirit/home/classic/tree/ast_fwd.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

//////////////////////////////////
//  ast_match_policy is simply an id so the correct specialization of
//  tree_policy can be found.
template <
    typename IteratorT,
    typename NodeFactoryT,
    typename T
>
struct ast_match_policy :
    public common_tree_match_policy<
        ast_match_policy<IteratorT, NodeFactoryT, T>,
        IteratorT,
        NodeFactoryT,
        ast_tree_policy<
            ast_match_policy<IteratorT, NodeFactoryT, T>,
            NodeFactoryT,
            T
        >,
        T
    >
{
    typedef
        common_tree_match_policy<
            ast_match_policy<IteratorT, NodeFactoryT, T>,
            IteratorT,
            NodeFactoryT,
            ast_tree_policy<
                ast_match_policy<IteratorT, NodeFactoryT, T>,
                NodeFactoryT,
                T
            >,
            T
        >
    common_tree_match_policy_;
    
    ast_match_policy()
    {
    }

    template <typename PolicyT>
    ast_match_policy(PolicyT const & policies)
        : common_tree_match_policy_(policies)
    {
    }
};

//////////////////////////////////
template <typename MatchPolicyT, typename NodeFactoryT, typename T>
struct ast_tree_policy :
    public common_tree_tree_policy<MatchPolicyT, NodeFactoryT>
{
    typedef typename MatchPolicyT::match_t match_t;
    typedef typename MatchPolicyT::iterator_t iterator_t;

    template<typename MatchAT, typename MatchBT>
    static void concat(MatchAT& a, MatchBT const& b)
    {
        BOOST_SPIRIT_ASSERT(a && b);

#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES)
        BOOST_SPIRIT_DEBUG_OUT << "\n>>>AST concat. a = " << a <<
            "\n\tb = " << b << "<<<\n";
#endif
        typedef typename tree_match<iterator_t, NodeFactoryT, T>::container_t
            container_t;

        // test for size() is necessary, because no_tree_gen_node leaves a.trees
        // and/or b.trees empty
        if (0 != b.trees.size() && b.trees.begin()->value.is_root())
        {
            BOOST_SPIRIT_ASSERT(b.trees.size() == 1);

            container_t tmp;
            std::swap(a.trees, tmp); // save a into tmp
            std::swap(b.trees, a.trees); // make b.trees[0] be new root (a.trees[0])
            container_t* pnon_root_trees = &a.trees;
            while (pnon_root_trees->size() > 0 &&
                    pnon_root_trees->begin()->value.is_root())
            {
                pnon_root_trees = & pnon_root_trees->begin()->children;
            }
            pnon_root_trees->insert(pnon_root_trees->begin(),
                    tmp.begin(), tmp.end());
        }
        else if (0 != a.trees.size() && a.trees.begin()->value.is_root())
        {
            BOOST_SPIRIT_ASSERT(a.trees.size() == 1);

#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
            a.trees.begin()->children.reserve(a.trees.begin()->children.size() + b.trees.size());
#endif
            std::copy(b.trees.begin(),
                 b.trees.end(),
                 std::back_insert_iterator<container_t>(
                     a.trees.begin()->children));
        }
        else
        {
#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
            a.trees.reserve(a.trees.size() + b.trees.size());
#endif
            std::copy(b.trees.begin(),
                 b.trees.end(),
                 std::back_insert_iterator<container_t>(a.trees));
        }

#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES)
        BOOST_SPIRIT_DEBUG_OUT << ">>>after AST concat. a = " << a << "<<<\n\n";
#endif

        return;
    }

    template <typename MatchT, typename Iterator1T, typename Iterator2T>
    static void group_match(MatchT& m, parser_id const& id,
            Iterator1T const& first, Iterator2T const& last)
    {
        if (!m)
            return;

        typedef typename tree_match<iterator_t, NodeFactoryT, T>::container_t
            container_t;
        typedef typename container_t::iterator cont_iterator_t;
        typedef typename NodeFactoryT::template factory<iterator_t> factory_t;

        if (m.trees.size() == 1
#ifdef BOOST_SPIRIT_NO_TREE_NODE_COLLAPSING
            && !(id.to_long() && m.trees.begin()->value.id().to_long())
#endif
            )
        {
            // set rule_id's.  There may have been multiple nodes created.
            // Because of root_node[] they may be left-most children of the top
            // node.
            container_t* punset_id = &m.trees;
            while (punset_id->size() > 0 &&
                    punset_id->begin()->value.id() == 0)
            {
                punset_id->begin()->value.id(id);
                punset_id = &punset_id->begin()->children;
            }

            m.trees.begin()->value.is_root(false);
        }
        else
        {
            match_t newmatch(m.length(),
                m.trees.empty() ? 
                    factory_t::empty_node() : 
                    factory_t::create_node(first, last, false));

            std::swap(newmatch.trees.begin()->children, m.trees);
            // set this node and all it's unset children's rule_id
            newmatch.trees.begin()->value.id(id);
            for (cont_iterator_t i = newmatch.trees.begin();
                 i != newmatch.trees.end();
                 ++i)
            {
                if (i->value.id() == 0)
                    i->value.id(id);
            }
            m = newmatch;
        }
    }

    template <typename FunctorT, typename MatchT>
    static void apply_op_to_match(FunctorT const& op, MatchT& m)
    {
        op(m);
    }
};

namespace impl {

    template <typename IteratorT, typename NodeFactoryT, typename T>
    struct tree_policy_selector<ast_match_policy<IteratorT, NodeFactoryT, T> >
    {
        typedef ast_tree_policy<
            ast_match_policy<IteratorT, NodeFactoryT, T>, 
            NodeFactoryT, 
            T
        > type;
    };

} // namespace impl


//////////////////////////////////
struct gen_ast_node_parser_gen;

template <typename T>
struct gen_ast_node_parser
:   public unary<T, parser<gen_ast_node_parser<T> > >
{
    typedef gen_ast_node_parser<T> self_t;
    typedef gen_ast_node_parser_gen parser_generator_t;
    typedef unary_parser_category parser_category_t;

    gen_ast_node_parser(T const& a)
    : unary<T, parser<gen_ast_node_parser<T> > >(a) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename ScannerT::iteration_policy_t iteration_policy_t;
        typedef typename ScannerT::match_policy_t::iterator_t iterator_t;
        typedef typename ScannerT::match_policy_t::factory_t factory_t;
        typedef ast_match_policy<iterator_t, factory_t> match_policy_t;
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
struct gen_ast_node_parser_gen
{
    template <typename T>
    struct result {

        typedef gen_ast_node_parser<T> type;
    };

    template <typename T>
    static gen_ast_node_parser<T>
    generate(parser<T> const& s)
    {
        return gen_ast_node_parser<T>(s.derived());
    }

    template <typename T>
    gen_ast_node_parser<T>
    operator[](parser<T> const& s) const
    {
        return gen_ast_node_parser<T>(s.derived());
    }
};

//////////////////////////////////
const gen_ast_node_parser_gen gen_ast_node_d = gen_ast_node_parser_gen();


//////////////////////////////////
struct root_node_op
{
    template <typename MatchT>
    void operator()(MatchT& m) const
    {
        BOOST_SPIRIT_ASSERT(m.trees.size() > 0);
        m.trees.begin()->value.is_root(true);
    }
};

const node_parser_gen<root_node_op> root_node_d =
    node_parser_gen<root_node_op>();

///////////////////////////////////////////////////////////////////////////////
//
//  Parse functions for ASTs
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename AstFactoryT, typename IteratorT, typename ParserT, 
    typename SkipT
>
inline tree_parse_info<IteratorT, AstFactoryT>
ast_parse(
    IteratorT const&        first_,
    IteratorT const&        last_,
    parser<ParserT> const&  parser,
    SkipT const&            skip_,
    AstFactoryT const &   /*dummy_*/ = AstFactoryT())
{
    typedef skip_parser_iteration_policy<SkipT> it_policy_t;
    typedef ast_match_policy<IteratorT, AstFactoryT> ast_match_policy_t;
    typedef
        scanner_policies<it_policy_t, ast_match_policy_t>
        scan_policies_t;
    typedef scanner<IteratorT, scan_policies_t> scanner_t;

    it_policy_t iter_policy(skip_);
    scan_policies_t policies(iter_policy);
    IteratorT first = first_;
    scanner_t scan(first, last_, policies);
    tree_match<IteratorT, AstFactoryT> hit = parser.derived().parse(scan);
    return tree_parse_info<IteratorT, AstFactoryT>(
        first, hit, hit && (first == last_), hit.length(), hit.trees);
}

//////////////////////////////////
template <typename IteratorT, typename ParserT, typename SkipT>
inline tree_parse_info<IteratorT>
ast_parse(
    IteratorT const&        first_,
    IteratorT const&        last_,
    parser<ParserT> const&  parser,
    SkipT const&            skip_)
{
    typedef node_val_data_factory<nil_t> default_factory_t;
    return ast_parse(first_, last_, parser, skip_, default_factory_t());
}
  
//////////////////////////////////
template <typename IteratorT, typename ParserT>
inline tree_parse_info<IteratorT>
ast_parse(
    IteratorT const&        first_,
    IteratorT const&        last,
    parser<ParserT> const&  parser)
{
    typedef ast_match_policy<IteratorT> ast_match_policy_t;
    IteratorT first = first_;
    scanner<
        IteratorT,
        scanner_policies<iteration_policy, ast_match_policy_t>
    > scan(first, last);
    tree_match<IteratorT> hit = parser.derived().parse(scan);
    return tree_parse_info<IteratorT>(
        first, hit, hit && (first == last), hit.length(), hit.trees);
}

//////////////////////////////////
template <typename CharT, typename ParserT, typename SkipT>
inline tree_parse_info<CharT const*>
ast_parse(
    CharT const*            str,
    parser<ParserT> const&  parser,
    SkipT const&            skip)
{
    CharT const* last = str;
    while (*last)
        last++;
    return ast_parse(str, last, parser, skip);
}

//////////////////////////////////
template <typename CharT, typename ParserT>
inline tree_parse_info<CharT const*>
ast_parse(
    CharT const*            str,
    parser<ParserT> const&  parser)
{
    CharT const* last = str;
    while (*last)
    {
        last++;
    }
    return ast_parse(str, last, parser);
}

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

