/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2001-2007 Hartmut Kaiser
    Revised 2007, Copyright (c) Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_TREE_COMMON_HPP
#define BOOST_SPIRIT_TREE_COMMON_HPP

#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
#include <vector>
#else
#include <list>
#endif

#if defined(BOOST_SPIRIT_USE_BOOST_ALLOCATOR_FOR_TREES)
#include <boost/pool/pool_alloc.hpp>
#endif

#include <algorithm>

#include <boost/ref.hpp>
#include <boost/call_traits.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core.hpp>
#include <boost/assert.hpp>

#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES)
#include <iostream>
#include <boost/spirit/home/classic/debug/debug_node.hpp>
#endif

#include <boost/spirit/home/classic/tree/common_fwd.hpp>

#include <iterator> // for std::iterator_traits, std::distance

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

template <typename T>
void swap(tree_node<T>& a, tree_node<T>& b);

template <typename T, typename V>
void swap(node_iter_data<T, V>& a, node_iter_data<T, V>& b);

namespace impl {
    template <typename T>
    inline void cp_swap(T& t1, T& t2);
}

template <typename T>
struct tree_node
{
    typedef T parse_node_t;
    
#if !defined(BOOST_SPIRIT_USE_BOOST_ALLOCATOR_FOR_TREES)
    typedef std::allocator<tree_node<T> > allocator_type;
#elif !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
    typedef boost::pool_allocator<tree_node<T> > allocator_type;
#else
    typedef boost::fast_pool_allocator<tree_node<T> > allocator_type;
#endif

#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
    typedef std::vector<tree_node<T>, allocator_type> children_t;
#else
    typedef std::list<tree_node<T>, allocator_type> children_t;
#endif  // BOOST_SPIRIT_USE_LIST_FOR_TREES

    typedef typename children_t::iterator tree_iterator;
    typedef typename children_t::const_iterator const_tree_iterator;

    T value;
    children_t children;

    tree_node()
        : value()
        , children()
    {}

    explicit tree_node(T const& v)
        : value(v)
        , children()
    {}

    tree_node(T const& v, children_t const& c)
        : value(v)
        , children(c)
    {}

    void swap(tree_node<T>& x)
    {
        impl::cp_swap(value, x.value);
        impl::cp_swap(children, x.children);
    }
};

#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES)
template <typename T>
inline std::ostream&
operator<<(std::ostream& o, tree_node<T> const& n)
{
    static int depth = 0;
    o << "\n";
    for (int i = 0; i <= depth; ++i)
    {
        o << "\t";
    }
    o << "(depth = " << depth++ << " value = " << n.value;
    int c = 0;
    for (typename tree_node<T>::children_t::const_iterator it = n.children.begin();
         it != n.children.end(); ++it)
    {
        o << " children[" << c++ << "] = " << *it;
    }
    o << ")";
    --depth;
    return o;
}
#endif

//////////////////////////////////
template <typename IteratorT, typename ValueT>
struct node_iter_data
{
    typedef IteratorT iterator_t;
    typedef IteratorT /*const*/ const_iterator_t;

    node_iter_data()
        : first(), last(), is_root_(false), parser_id_(), value_()
        {}

    node_iter_data(IteratorT const& _first, IteratorT const& _last)
        : first(_first), last(_last), is_root_(false), parser_id_(), value_()
        {}

    void swap(node_iter_data& x)
    {
        impl::cp_swap(first, x.first);
        impl::cp_swap(last, x.last);
        impl::cp_swap(parser_id_, x.parser_id_);
        impl::cp_swap(is_root_, x.is_root_);
        impl::cp_swap(value_, x.value_);
    }

    IteratorT begin()
    {
        return first;
    }

    IteratorT const& begin() const
    {
        return first;
    }

    IteratorT end()
    {
        return last;
    }

    IteratorT const& end() const
    {
        return last;
    }

    bool is_root() const
    {
        return is_root_;
    }

    void is_root(bool b)
    {
        is_root_ = b;
    }

    parser_id id() const
    {
        return parser_id_;
    }

    void id(parser_id r)
    {
        parser_id_ = r;
    }

    ValueT const& value() const
    {
        return value_;
    }

    void value(ValueT const& v)
    {
        value_ = v;
    }
private:
    IteratorT first, last;
    bool is_root_;
    parser_id parser_id_;
    ValueT value_;

public:
};

#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES)
// value is default nil_t, so provide an operator<< for nil_t
inline std::ostream&
operator<<(std::ostream& o, nil_t const&)
{
    return o;
}

template <typename IteratorT, typename ValueT>
inline std::ostream&
operator<<(std::ostream& o, node_iter_data<IteratorT, ValueT> const& n)
{
    o << "(id = " << n.id() << " text = \"";
    typedef typename node_iter_data<IteratorT, ValueT>::const_iterator_t
        iterator_t;
    for (iterator_t it = n.begin(); it != n.end(); ++it)
        impl::token_printer(o, *it);
    o << "\" is_root = " << n.is_root()
        << /*" value = " << n.value() << */")";
    return o;
}
#endif

//////////////////////////////////
template <typename IteratorT = char const*, typename ValueT = nil_t>
struct node_val_data
{
    typedef
        typename std::iterator_traits<IteratorT>::value_type
        value_type;

#if !defined(BOOST_SPIRIT_USE_BOOST_ALLOCATOR_FOR_TREES)
    typedef std::allocator<value_type> allocator_type;
#elif !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
    typedef boost::pool_allocator<value_type> allocator_type;
#else
    typedef boost::fast_pool_allocator<value_type> allocator_type;
#endif

#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
    typedef std::vector<value_type, allocator_type> container_t;
#else
    typedef std::list<value_type, allocator_type> container_t;
#endif

    typedef typename container_t::iterator iterator_t;
    typedef typename container_t::const_iterator const_iterator_t;

    node_val_data()
        : text(), is_root_(false), parser_id_(), value_()
        {}

#if defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)
    node_val_data(IteratorT const& _first, IteratorT const& _last)
        : text(), is_root_(false), parser_id_(), value_()
        {
            std::copy(_first, _last, std::inserter(text, text.end()));
        }

    // This constructor is for building text out of iterators
    template <typename IteratorT2>
    node_val_data(IteratorT2 const& _first, IteratorT2 const& _last)
        : text(), is_root_(false), parser_id_(), value_()
        {
            std::copy(_first, _last, std::inserter(text, text.end()));
        }
#else
    node_val_data(IteratorT const& _first, IteratorT const& _last)
        : text(_first, _last), is_root_(false), parser_id_(), value_()
        {}

    // This constructor is for building text out of iterators
    template <typename IteratorT2>
    node_val_data(IteratorT2 const& _first, IteratorT2 const& _last)
        : text(_first, _last), is_root_(false), parser_id_(), value_()
        {}
#endif

    void swap(node_val_data& x)
    {
        impl::cp_swap(text, x.text);
        impl::cp_swap(is_root_, x.is_root_);
        impl::cp_swap(parser_id_, x.parser_id_);
        impl::cp_swap(value_, x.value_);
    }

    typename container_t::iterator begin()
    {
        return text.begin();
    }

    typename container_t::const_iterator begin() const
    {
        return text.begin();
    }

    typename container_t::iterator end()
    {
        return text.end();
    }

    typename container_t::const_iterator end() const
    {
        return text.end();
    }

    bool is_root() const
    {
        return is_root_;
    }

    void is_root(bool b)
    {
        is_root_ = b;
    }

    parser_id id() const
    {
        return parser_id_;
    }

    void id(parser_id r)
    {
        parser_id_ = r;
    }

    ValueT const& value() const
    {
        return value_;
    }

    void value(ValueT const& v)
    {
        value_ = v;
    }

private:
    container_t text;
    bool is_root_;
    parser_id parser_id_;
    ValueT value_;
};

#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES)
template <typename IteratorT, typename ValueT>
inline std::ostream&
operator<<(std::ostream& o, node_val_data<IteratorT, ValueT> const& n)
{
    o << "(id = " << n.id() << " text = \"";
    typedef typename node_val_data<IteratorT, ValueT>::const_iterator_t
        iterator_t;
    for (iterator_t it = n.begin(); it != n.end(); ++it)
        impl::token_printer(o, *it);
    o << "\" is_root = " << n.is_root()
        << " value = " << n.value() << ")";
    return o;
}
#endif

template <typename T>
inline void
swap(tree_node<T>& a, tree_node<T>& b)
{
    a.swap(b);
}

template <typename T, typename V>
inline void
swap(node_iter_data<T, V>& a, node_iter_data<T, V>& b)
{
    a.swap(b);
}

//////////////////////////////////
template <typename ValueT>
class node_iter_data_factory
{
public:
    // This inner class is so that node_iter_data_factory can simulate
    // a template template parameter
    template <typename IteratorT>
    class factory
    {
    public:
        typedef IteratorT iterator_t;
        typedef node_iter_data<iterator_t, ValueT> node_t;

        static node_t create_node(iterator_t const& first, iterator_t const& last,
                bool /*is_leaf_node*/)
        {
            return node_t(first, last);
        }

        static node_t empty_node()
        {
            return node_t();
        }

        // precondition: ContainerT contains a tree_node<node_t>.  And all
        // iterators in the container point to the same sequence.
        template <typename ContainerT>
        static node_t group_nodes(ContainerT const& nodes)
        {
            return node_t(nodes.begin()->value.begin(),
                    nodes.back().value.end());
        }
    };
};

//////////////////////////////////
template <typename ValueT>
class node_val_data_factory 
{
public:
    // This inner class is so that node_val_data_factory can simulate
    // a template template parameter
    template <typename IteratorT>
    class factory
    {
    public:
        typedef IteratorT iterator_t;
        typedef node_val_data<iterator_t, ValueT> node_t;

        static node_t create_node(iterator_t const& first, iterator_t const& last,
                bool is_leaf_node)
        {
            if (is_leaf_node)
                return node_t(first, last);
            else
                return node_t();
        }

        static node_t empty_node()
        {
            return node_t();
        }

        template <typename ContainerT>
        static node_t group_nodes(ContainerT const& nodes)
        {
            typename node_t::container_t c;
            typename ContainerT::const_iterator i_end = nodes.end();
            // copy all the nodes text into a new one
            for (typename ContainerT::const_iterator i = nodes.begin();
                 i != i_end; ++i)
            {
                // See docs: reduced_node_d cannot be used with a
                // rule inside the [].
                BOOST_ASSERT(i->children.size() == 0);
                c.insert(c.end(), i->value.begin(), i->value.end());
            }
            return node_t(c.begin(), c.end());
        }
    };
};

//////////////////////////////////
template <typename ValueT>
class node_all_val_data_factory
{
public:
    // This inner class is so that node_all_val_data_factory can simulate
    // a template template parameter
    template <typename IteratorT>
    class factory
    {
    public:
        typedef IteratorT iterator_t;
        typedef node_val_data<iterator_t, ValueT> node_t;

        static node_t create_node(iterator_t const& first, iterator_t const& last,
                bool /*is_leaf_node*/)
        {
            return node_t(first, last);
        }

        static node_t empty_node()
        {
            return node_t();
        }

        template <typename ContainerT>
        static node_t group_nodes(ContainerT const& nodes)
        {
            typename node_t::container_t c;
            typename ContainerT::const_iterator i_end = nodes.end();
            // copy all the nodes text into a new one
            for (typename ContainerT::const_iterator i = nodes.begin();
                    i != i_end; ++i)
            {
                BOOST_ASSERT(i->children.size() == 0);
                c.insert(c.end(), i->value.begin(), i->value.end());
            }
            return node_t(c.begin(), c.end());
        }
    };
};

namespace impl {

    ///////////////////////////////////////////////////////////////////////////
    // can't call unqualified swap from within classname::swap
    // as Koenig lookup rules will find only the classname::swap
    // member function not the global declaration, so use cp_swap
    // as a forwarding function (JM):
    template <typename T>
    inline void cp_swap(T& t1, T& t2)
    {
        using std::swap;
        using BOOST_SPIRIT_CLASSIC_NS::swap;
        using boost::swap;
        swap(t1, t2);
    }
}

//////////////////////////////////
template <typename IteratorT, typename NodeFactoryT, typename T>
class tree_match : public match<T>
{
public:

    typedef typename NodeFactoryT::template factory<IteratorT> node_factory_t;
    typedef typename node_factory_t::node_t parse_node_t;
    typedef tree_node<parse_node_t> node_t;
    typedef typename node_t::children_t container_t;
    typedef typename container_t::iterator tree_iterator;
    typedef typename container_t::const_iterator const_tree_iterator;

    typedef T attr_t;
    typedef typename boost::call_traits<T>::param_type      param_type;
    typedef typename boost::call_traits<T>::reference       reference;
    typedef typename boost::call_traits<T>::const_reference const_reference;

    tree_match()
    : match<T>(), trees()
    {}

    explicit
    tree_match(std::size_t length_)
    : match<T>(length_), trees()
    {}

    tree_match(std::size_t length_, parse_node_t const& n)
    : match<T>(length_), trees()
    { 
        trees.push_back(node_t(n)); 
    }

    tree_match(std::size_t length_, param_type val, parse_node_t const& n)
    : match<T>(length_, val), trees()
    {
#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
        trees.reserve(10); // this is more or less an arbitrary number...
#endif
        trees.push_back(node_t(n));
    }

    // attention, these constructors will change the second parameter!
    tree_match(std::size_t length_, container_t& c)
    : match<T>(length_), trees()
    { 
        impl::cp_swap(trees, c);
    }

    tree_match(std::size_t length_, param_type val, container_t& c)
    : match<T>(length_, val), trees()
    {
        impl::cp_swap(trees, c);
    }

    template <typename T2>
    tree_match(match<T2> const& other)
    : match<T>(other), trees()
    {}

    template <typename T2, typename T3, typename T4>
    tree_match(tree_match<T2, T3, T4> const& other)
    : match<T>(other), trees()
    { impl::cp_swap(trees, other.trees); }

    template <typename T2>
    tree_match&
    operator=(match<T2> const& other)
    {
        match<T>::operator=(other);
        return *this;
    }

    template <typename T2, typename T3, typename T4>
    tree_match&
    operator=(tree_match<T2, T3, T4> const& other)
    {
        match<T>::operator=(other);
        impl::cp_swap(trees, other.trees);
        return *this;
    }

    tree_match(tree_match const& x)
    : match<T>(x), trees()
    {
        // use auto_ptr like ownership for the trees data member
        impl::cp_swap(trees, x.trees);
    }

    tree_match& operator=(tree_match const& x)
    {
        tree_match tmp(x);
        this->swap(tmp);
        return *this;
    }

    void swap(tree_match& x)
    {
        match<T>::swap(x);
        impl::cp_swap(trees, x.trees);
    }

    mutable container_t trees;
};

#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES)
template <typename IteratorT, typename NodeFactoryT, typename T>
inline std::ostream&
operator<<(std::ostream& o, tree_match<IteratorT, NodeFactoryT, T> const& m)
{
    typedef
        typename tree_match<IteratorT, NodeFactoryT, T>::container_t::iterator
        iterator;

    o << "(length = " << (int)m.length();
    int c = 0;
    for (iterator i = m.trees.begin(); i != m.trees.end(); ++i)
    {
        o << " trees[" << c++ << "] = " << *i;
    }
    o << "\n)";
    return o;
}
#endif

//////////////////////////////////
struct tree_policy
{
    template <typename FunctorT, typename MatchT>
    static void apply_op_to_match(FunctorT const& /*op*/, MatchT& /*m*/)
    {}

    template <typename MatchT, typename Iterator1T, typename Iterator2T>
    static void group_match(MatchT& /*m*/, parser_id const& /*id*/,
            Iterator1T const& /*first*/, Iterator2T const& /*last*/)
    {}

    template <typename MatchT>
    static void concat(MatchT& /*a*/, MatchT const& /*b*/)
    {}
};

//////////////////////////////////
template <
    typename MatchPolicyT,
    typename IteratorT,
    typename NodeFactoryT,
    typename TreePolicyT, 
    typename T
>
struct common_tree_match_policy : public match_policy
{
    common_tree_match_policy()
    {
    }

    template <typename PolicyT>
    common_tree_match_policy(PolicyT const & policies)
        : match_policy((match_policy const &)policies)
    {
    }

    template <typename U>
    struct result { typedef tree_match<IteratorT, NodeFactoryT, U> type; };

    typedef tree_match<IteratorT, NodeFactoryT, T> match_t;
    typedef IteratorT iterator_t;
    typedef TreePolicyT tree_policy_t;
    typedef NodeFactoryT factory_t;

    static const match_t no_match() { return match_t(); }
    static const match_t empty_match()
    { return match_t(0, tree_policy_t::empty_node()); }

    template <typename AttrT, typename Iterator1T, typename Iterator2T>
    static tree_match<IteratorT, NodeFactoryT, AttrT> create_match(
        std::size_t length,
        AttrT const& val,
        Iterator1T const& first,
        Iterator2T const& last)
    {
#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES)

        BOOST_SPIRIT_DEBUG_OUT << "\n>>> create_node(begin) <<<\n" 
            "creating node text: \"";
        for (Iterator1T it = first; it != last; ++it)
            impl::token_printer(BOOST_SPIRIT_DEBUG_OUT, *it);
        BOOST_SPIRIT_DEBUG_OUT << "\"\n";
        BOOST_SPIRIT_DEBUG_OUT << ">>> create_node(end) <<<\n\n"; 
#endif
        return tree_match<IteratorT, NodeFactoryT, AttrT>(length, val,
            tree_policy_t::create_node(length, first, last, true));
    }

    template <typename Match1T, typename Match2T>
    static void concat_match(Match1T& a, Match2T const& b)
    {
#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES)

        BOOST_SPIRIT_DEBUG_OUT << "\n>>> concat_match(begin) <<<\n";
        BOOST_SPIRIT_DEBUG_OUT << "tree a:\n" << a << "\n";
        BOOST_SPIRIT_DEBUG_OUT << "tree b:\n" << b << "\n";
        BOOST_SPIRIT_DEBUG_OUT << ">>> concat_match(end) <<<\n\n";
#endif
        BOOST_SPIRIT_ASSERT(a && b);
        if (a.length() == 0)
        {
            a = b;
            return;
        }
        else if (b.length() == 0
#ifdef BOOST_SPIRIT_NO_TREE_NODE_COLLAPSING
            && !b.trees.begin()->value.id().to_long()
#endif
            )
        {
            return;
        }
        a.concat(b);
        tree_policy_t::concat(a, b);
    }

    template <typename MatchT, typename IteratorT2>
    void
    group_match(
        MatchT&             m,
        parser_id const&    id,
        IteratorT2 const&   first,
        IteratorT2 const&   last) const
    {
        if (!m) return;
        
#if defined(BOOST_SPIRIT_DEBUG) && \
    (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_TREES)

        BOOST_SPIRIT_DEBUG_OUT << "\n>>> group_match(begin) <<<\n"
            "new node(" << id << ") \"";
        for (IteratorT2 it = first; it != last; ++it)
            impl::token_printer(BOOST_SPIRIT_DEBUG_OUT, *it);
        BOOST_SPIRIT_DEBUG_OUT << "\"\n";
        BOOST_SPIRIT_DEBUG_OUT << "new child tree (before grouping):\n" << m << "\n";

        tree_policy_t::group_match(m, id, first, last);

        BOOST_SPIRIT_DEBUG_OUT << "new child tree (after grouping):\n" << m << "\n";
        BOOST_SPIRIT_DEBUG_OUT << ">>> group_match(end) <<<\n\n";
#else
        tree_policy_t::group_match(m, id, first, last);
#endif
    }
};

//////////////////////////////////
template <typename MatchPolicyT, typename NodeFactoryT>
struct common_tree_tree_policy
{
    typedef typename MatchPolicyT::iterator_t iterator_t;
    typedef typename MatchPolicyT::match_t match_t;
    typedef typename NodeFactoryT::template factory<iterator_t> factory_t;
    typedef typename factory_t::node_t node_t;

    template <typename Iterator1T, typename Iterator2T>
        static node_t
        create_node(std::size_t /*length*/, Iterator1T const& first,
            Iterator2T const& last, bool leaf_node)
    {
        return factory_t::create_node(first, last, leaf_node);
    }

    static node_t
        empty_node()
    {
        return factory_t::empty_node();
    }

    template <typename FunctorT>
        static void apply_op_to_match(FunctorT const& op, match_t& m)
    {
        op(m);
    }
};

//////////////////////////////////
// directives to modify how the parse tree is generated

struct no_tree_gen_node_parser_gen;

template <typename T>
struct no_tree_gen_node_parser
:   public unary<T, parser<no_tree_gen_node_parser<T> > >
{
    typedef no_tree_gen_node_parser<T> self_t;
    typedef no_tree_gen_node_parser_gen parser_generator_t;
    typedef unary_parser_category parser_category_t;

    no_tree_gen_node_parser(T const& a)
    : unary<T, parser<no_tree_gen_node_parser<T> > >(a) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scanner) const
    {
        typedef typename ScannerT::iteration_policy_t iteration_policy_t;
        typedef match_policy match_policy_t;
        typedef typename ScannerT::action_policy_t action_policy_t;
        typedef scanner_policies<
            iteration_policy_t,
            match_policy_t,
            action_policy_t
        > policies_t;

        return this->subject().parse(scanner.change_policies(policies_t(scanner)));
    }
};

struct no_tree_gen_node_parser_gen
{
    template <typename T>
    struct result {

        typedef no_tree_gen_node_parser<T> type;
    };

    template <typename T>
    static no_tree_gen_node_parser<T>
    generate(parser<T> const& s)
    {
        return no_tree_gen_node_parser<T>(s.derived());
    }

    template <typename T>
    no_tree_gen_node_parser<T>
    operator[](parser<T> const& s) const
    {
        return no_tree_gen_node_parser<T>(s.derived());
    }
};

const no_tree_gen_node_parser_gen no_node_d = no_tree_gen_node_parser_gen();

//////////////////////////////////

struct leaf_node_parser_gen;

template<typename T>
struct leaf_node_parser
:   public unary<T, parser<leaf_node_parser<T> > >
{
    typedef leaf_node_parser<T> self_t;
    typedef leaf_node_parser_gen parser_generator_t;
    typedef unary_parser_category parser_category_t;

    leaf_node_parser(T const& a)
    : unary<T, parser<leaf_node_parser<T> > >(a) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scanner) const
    {
        typedef scanner_policies< typename ScannerT::iteration_policy_t,
            match_policy, typename ScannerT::action_policy_t > policies_t;

        typedef typename ScannerT::iterator_t iterator_t;
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename result_t::node_factory_t factory_t;

        iterator_t from = scanner.first;
        result_t hit = impl::contiguous_parser_parse<result_t>(this->subject(),
            scanner.change_policies(policies_t(scanner,match_policy(),scanner)),
            scanner);

        if (hit)
            return result_t(hit.length(), 
                factory_t::create_node(from, scanner.first, true));
        else
            return result_t(hit.length());
    }
};

struct leaf_node_parser_gen
{
    template <typename T>
    struct result {

        typedef leaf_node_parser<T> type;
    };

    template <typename T>
    static leaf_node_parser<T>
    generate(parser<T> const& s)
    {
        return leaf_node_parser<T>(s.derived());
    }

    template <typename T>
    leaf_node_parser<T>
    operator[](parser<T> const& s) const
    {
        return leaf_node_parser<T>(s.derived());
    }
};

const leaf_node_parser_gen leaf_node_d = leaf_node_parser_gen();
const leaf_node_parser_gen token_node_d = leaf_node_parser_gen();

//////////////////////////////////
namespace impl {

    template <typename MatchPolicyT>
    struct tree_policy_selector
    {
        typedef tree_policy type;
    };

} // namespace impl

//////////////////////////////////
template <typename NodeParserT>
struct node_parser_gen;

template <typename T, typename NodeParserT>
struct node_parser
:   public unary<T, parser<node_parser<T, NodeParserT> > >
{
    typedef node_parser<T, NodeParserT> self_t;
    typedef node_parser_gen<NodeParserT> parser_generator_t;
    typedef unary_parser_category parser_category_t;

    node_parser(T const& a)
    : unary<T, parser<node_parser<T, NodeParserT> > >(a) {}

    template <typename ScannerT>
    struct result
    {
        typedef typename parser_result<T, ScannerT>::type type;
    };

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scanner) const
    {
        typename parser_result<self_t, ScannerT>::type hit = this->subject().parse(scanner);
        if (hit)
        {
            impl::tree_policy_selector<typename ScannerT::match_policy_t>::type::apply_op_to_match(NodeParserT(), hit);
        }
        return hit;
    }
};

template <typename NodeParserT>
struct node_parser_gen
{
    template <typename T>
    struct result {

        typedef node_parser<T, NodeParserT> type;
    };

    template <typename T>
    static node_parser<T, NodeParserT>
    generate(parser<T> const& s)
    {
        return node_parser<T, NodeParserT>(s.derived());
    }

    template <typename T>
    node_parser<T, NodeParserT>
    operator[](parser<T> const& s) const
    {
        return node_parser<T, NodeParserT>(s.derived());
    }
};
//////////////////////////////////
struct reduced_node_op
{
    template <typename MatchT>
    void operator()(MatchT& m) const
    {
        if (m.trees.size() == 1)
        {
            m.trees.begin()->children.clear();
        }
        else if (m.trees.size() > 1)
        {
            typedef typename MatchT::node_factory_t node_factory_t;
            m = MatchT(m.length(), node_factory_t::group_nodes(m.trees));
        }
    }
};

const node_parser_gen<reduced_node_op> reduced_node_d =
    node_parser_gen<reduced_node_op>();


struct discard_node_op
{
    template <typename MatchT>
    void operator()(MatchT& m) const
    {
        m.trees.clear();
    }
};

const node_parser_gen<discard_node_op> discard_node_d =
    node_parser_gen<discard_node_op>();

struct infix_node_op
{
    template <typename MatchT>
    void operator()(MatchT& m) const
    {
        typedef typename MatchT::container_t container_t;
        typedef typename MatchT::container_t::iterator iter_t;
        typedef typename MatchT::container_t::value_type value_t;

        using std::swap;
        using boost::swap;
        using BOOST_SPIRIT_CLASSIC_NS::swap;

        // copying the tree nodes is expensive, since it may copy a whole
        // tree.  swapping them is cheap, so swap the nodes we want into
        // a new container of children.
        container_t new_children;
        std::size_t length = 0;
        std::size_t tree_size = m.trees.size();

        // the infix_node_d[] make no sense for nodes with no subnodes
        BOOST_SPIRIT_ASSERT(tree_size >= 1);

        bool keep = true;
#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
        new_children.reserve((tree_size+1)/2);
#endif
        iter_t i_end = m.trees.end();
        for (iter_t i = m.trees.begin(); i != i_end; ++i)
        {
            if (keep) {
                // adjust the length
                length += std::distance((*i).value.begin(), (*i).value.end());

                // move the child node
                new_children.push_back(value_t());
                swap(new_children.back(), *i);
                keep = false;
            }
            else {
                // ignore this child node
                keep = true;
            }
        }

        m = MatchT(length, new_children);
    }
};

const node_parser_gen<infix_node_op> infix_node_d =
    node_parser_gen<infix_node_op>();

struct discard_first_node_op
{
    template <typename MatchT>
    void operator()(MatchT& m) const
    {
        typedef typename MatchT::container_t container_t;
        typedef typename MatchT::container_t::iterator iter_t;
        typedef typename MatchT::container_t::value_type value_t;

        using std::swap;
        using boost::swap;
        using BOOST_SPIRIT_CLASSIC_NS::swap;

        // copying the tree nodes is expensive, since it may copy a whole
        // tree.  swapping them is cheap, so swap the nodes we want into
        // a new container of children, instead of saying
        // m.trees.erase(m.trees.begin()) because, on a container_t that will 
        // cause all the nodes afterwards to be copied into the previous 
        // position.
        container_t new_children;
        std::size_t length = 0;
        std::size_t tree_size = m.trees.size();

        // the discard_first_node_d[] make no sense for nodes with no subnodes
        BOOST_SPIRIT_ASSERT(tree_size >= 1);

        if (tree_size > 1) {
#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
            new_children.reserve(tree_size - 1);
#endif
            iter_t i = m.trees.begin(), i_end = m.trees.end();
            for (++i; i != i_end; ++i)
            {
                // adjust the length
                length += std::distance((*i).value.begin(), (*i).value.end());

                // move the child node
                new_children.push_back(value_t());
                swap(new_children.back(), *i);
            }
        }
        else {
        // if there was a tree and now there isn't any, insert an empty node
            iter_t i = m.trees.begin(); 

        // This isn't entirely correct, since the empty node will reference
        // the end of the discarded node, but I currently don't see any way to 
        // get at the begin of the node following this subnode.
        // This should be safe anyway because the it shouldn't get dereferenced
        // under any circumstances.
            typedef typename value_t::parse_node_t::iterator_t iterator_type;
            iterator_type it = (*i).value.end();
            
            new_children.push_back(
                value_t(typename value_t::parse_node_t(it, it)));
        }
        
        m = MatchT(length, new_children);
    }
};

const node_parser_gen<discard_first_node_op> discard_first_node_d =
    node_parser_gen<discard_first_node_op>();

struct discard_last_node_op
{
    template <typename MatchT>
    void operator()(MatchT& m) const
    {
        typedef typename MatchT::container_t container_t;
        typedef typename MatchT::container_t::iterator iter_t;
        typedef typename MatchT::container_t::value_type value_t;

        using std::swap;
        using boost::swap;
        using BOOST_SPIRIT_CLASSIC_NS::swap;

        // copying the tree nodes is expensive, since it may copy a whole
        // tree.  swapping them is cheap, so swap the nodes we want into
        // a new container of children, instead of saying
        // m.trees.erase(m.trees.begin()) because, on a container_t that will 
        // cause all the nodes afterwards to be copied into the previous 
        // position.
        container_t new_children;
        std::size_t length = 0;
        std::size_t tree_size = m.trees.size();

        // the discard_last_node_d[] make no sense for nodes with no subnodes
        BOOST_SPIRIT_ASSERT(tree_size >= 1);

        if (tree_size > 1) {
            m.trees.pop_back();
#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
            new_children.reserve(tree_size - 1);
#endif            
            iter_t i_end = m.trees.end();
            for (iter_t i = m.trees.begin(); i != i_end; ++i)
            {
                // adjust the length
                length += std::distance((*i).value.begin(), (*i).value.end());

                // move the child node
                new_children.push_back(value_t());
                swap(new_children.back(), *i);
            }
        }
        else {
        // if there was a tree and now there isn't any, insert an empty node
            iter_t i = m.trees.begin(); 

            typedef typename value_t::parse_node_t::iterator_t iterator_type;
            iterator_type it = (*i).value.begin();
            
            new_children.push_back(
                value_t(typename value_t::parse_node_t(it, it)));
        }
        
        m = MatchT(length, new_children);
    }
};

const node_parser_gen<discard_last_node_op> discard_last_node_d =
    node_parser_gen<discard_last_node_op>();

struct inner_node_op
{
    template <typename MatchT>
    void operator()(MatchT& m) const
    {
        typedef typename MatchT::container_t container_t;
        typedef typename MatchT::container_t::iterator iter_t;
        typedef typename MatchT::container_t::value_type value_t;

        using std::swap;
        using boost::swap;
        using BOOST_SPIRIT_CLASSIC_NS::swap;

        // copying the tree nodes is expensive, since it may copy a whole
        // tree.  swapping them is cheap, so swap the nodes we want into
        // a new container of children, instead of saying
        // m.trees.erase(m.trees.begin()) because, on a container_t that will 
        // cause all the nodes afterwards to be copied into the previous 
        // position.
        container_t new_children;
        std::size_t length = 0;
        std::size_t tree_size = m.trees.size();
        
        // the inner_node_d[] make no sense for nodes with less then 2 subnodes
        BOOST_SPIRIT_ASSERT(tree_size >= 2);

        if (tree_size > 2) {
            m.trees.pop_back(); // erase the last element
#if !defined(BOOST_SPIRIT_USE_LIST_FOR_TREES)
            new_children.reserve(tree_size - 1);
#endif
            iter_t i = m.trees.begin(); // skip over the first element
            iter_t i_end = m.trees.end();
            for (++i; i != i_end; ++i)
            {
                // adjust the length
                length += std::distance((*i).value.begin(), (*i).value.end());
                
                // move the child node
                new_children.push_back(value_t());
                swap(new_children.back(), *i);
            }
        }
        else {
        // if there was a tree and now there isn't any, insert an empty node
            iter_t i = m.trees.begin(); // skip over the first element

            typedef typename value_t::parse_node_t::iterator_t iterator_type;
            iterator_type it = (*++i).value.begin();
            
            new_children.push_back(
                value_t(typename value_t::parse_node_t(it, it)));
        }
        
        m = MatchT(length, new_children);
    }
};

const node_parser_gen<inner_node_op> inner_node_d =
    node_parser_gen<inner_node_op>();


//////////////////////////////////
// action_directive_parser and action_directive_parser_gen
// are meant to be used as a template to create directives that
// generate action classes.  For example access_match and
// access_node.  The ActionParserT template parameter must be
// a class that has an innter class called action that is templated
// on the parser type and the action type.
template <typename ActionParserT>
struct action_directive_parser_gen;

template <typename T, typename ActionParserT>
struct action_directive_parser
:   public unary<T, parser<action_directive_parser<T, ActionParserT> > >
{
    typedef action_directive_parser<T, ActionParserT> self_t;
    typedef action_directive_parser_gen<ActionParserT> parser_generator_t;
    typedef unary_parser_category parser_category_t;

    action_directive_parser(T const& a)
        : unary<T, parser<action_directive_parser<T, ActionParserT> > >(a) {}

    template <typename ScannerT>
    struct result
    {
        typedef typename parser_result<T, ScannerT>::type type;
    };

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scanner) const
    {
        return this->subject().parse(scanner);
    }

    template <typename ActionT>
    typename ActionParserT::template action<action_directive_parser<T, ActionParserT>, ActionT>
    operator[](ActionT const& actor) const
    {
        typedef typename
            ActionParserT::template action<action_directive_parser, ActionT>
            action_t;
        return action_t(*this, actor);
    }
};

//////////////////////////////////
template <typename ActionParserT>
struct action_directive_parser_gen
{
    template <typename T>
    struct result {

        typedef action_directive_parser<T, ActionParserT> type;
    };

    template <typename T>
    static action_directive_parser<T, ActionParserT>
    generate(parser<T> const& s)
    {
        return action_directive_parser<T, ActionParserT>(s.derived());
    }

    template <typename T>
    action_directive_parser<T, ActionParserT>
    operator[](parser<T> const& s) const
    {
        return action_directive_parser<T, ActionParserT>(s.derived());
    }
};

//////////////////////////////////
// Calls the attached action passing it the match from the parser
// and the first and last iterators.
// The inner template class is used to simulate template-template parameters
// (declared in common_fwd.hpp).
template <typename ParserT, typename ActionT>
struct access_match_action::action
:   public unary<ParserT, parser<access_match_action::action<ParserT, ActionT> > >
{
    typedef action_parser_category parser_category;
    typedef action<ParserT, ActionT> self_t;
    
    template <typename ScannerT>
    struct result
    {
        typedef typename parser_result<ParserT, ScannerT>::type type;
    };

    action( ParserT const& subject,
            ActionT const& actor_);

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scanner) const;

    ActionT const &predicate() const;

    private:
    ActionT actor;
};

//////////////////////////////////
template <typename ParserT, typename ActionT>
access_match_action::action<ParserT, ActionT>::action(
    ParserT const& subject,
    ActionT const& actor_)
: unary<ParserT, parser<access_match_action::action<ParserT, ActionT> > >(subject)
, actor(actor_)
{}

//////////////////////////////////
template <typename ParserT, typename ActionT>
template <typename ScannerT>
typename parser_result<access_match_action::action<ParserT, ActionT>, ScannerT>::type
access_match_action::action<ParserT, ActionT>::
parse(ScannerT const& scan) const
{
    typedef typename ScannerT::iterator_t iterator_t;
    typedef typename parser_result<self_t, ScannerT>::type result_t;
    if (!scan.at_end())
    {
        iterator_t save = scan.first;
        result_t hit = this->subject().parse(scan);
        actor(hit, save, scan.first);
        return hit;
    }
    return scan.no_match();
}

//////////////////////////////////
template <typename ParserT, typename ActionT>
ActionT const &access_match_action::action<ParserT, ActionT>::predicate() const
{
    return actor;
}

//////////////////////////////////
const action_directive_parser_gen<access_match_action> access_match_d
    = action_directive_parser_gen<access_match_action>();



//////////////////////////////////
// Calls the attached action passing it the node from the parser
// and the first and last iterators
// The inner template class is used to simulate template-template parameters
// (declared in common_fwd.hpp).
template <typename ParserT, typename ActionT>
struct access_node_action::action
:   public unary<ParserT, parser<access_node_action::action<ParserT, ActionT> > >
{
    typedef action_parser_category parser_category;
    typedef action<ParserT, ActionT> self_t;
    
    template <typename ScannerT>
    struct result
    {
        typedef typename parser_result<ParserT, ScannerT>::type type;
    };

    action( ParserT const& subject,
            ActionT const& actor_);

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scanner) const;

    ActionT const &predicate() const;

    private:
    ActionT actor;
};

//////////////////////////////////
template <typename ParserT, typename ActionT>
access_node_action::action<ParserT, ActionT>::action(
    ParserT const& subject,
    ActionT const& actor_)
: unary<ParserT, parser<access_node_action::action<ParserT, ActionT> > >(subject)
, actor(actor_)
{}

//////////////////////////////////
template <typename ParserT, typename ActionT>
template <typename ScannerT>
typename parser_result<access_node_action::action<ParserT, ActionT>, ScannerT>::type
access_node_action::action<ParserT, ActionT>::
parse(ScannerT const& scan) const
{
    typedef typename ScannerT::iterator_t iterator_t;
    typedef typename parser_result<self_t, ScannerT>::type result_t;
    if (!scan.at_end())
    {
        iterator_t save = scan.first;
        result_t hit = this->subject().parse(scan);
        if (hit && hit.trees.size() > 0)
            actor(*hit.trees.begin(), save, scan.first);
        return hit;
    }
    return scan.no_match();
}

//////////////////////////////////
template <typename ParserT, typename ActionT>
ActionT const &access_node_action::action<ParserT, ActionT>::predicate() const
{
    return actor;
}

//////////////////////////////////
const action_directive_parser_gen<access_node_action> access_node_d
    = action_directive_parser_gen<access_node_action>();



//////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  tree_parse_info
//
//      Results returned by the tree parse functions:
//
//      stop:   points to the final parse position (i.e parsing
//              processed the input up to this point).
//
//      match:  true if parsing is successful. This may be full:
//              the parser consumed all the input, or partial:
//              the parser consumed only a portion of the input.
//
//      full:   true when we have a full match (i.e the parser
//              consumed all the input.
//
//      length: The number of characters consumed by the parser.
//              This is valid only if we have a successful match
//              (either partial or full). A negative value means
//              that the match is unsuccessful.
//
//     trees:   Contains the root node(s) of the tree.
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename IteratorT,
    typename NodeFactoryT,
    typename T
>
struct tree_parse_info 
{
    IteratorT   stop;
    bool        match;
    bool        full;
    std::size_t length;
    typename tree_match<IteratorT, NodeFactoryT, T>::container_t trees;

    tree_parse_info()
        : stop()
        , match(false)
        , full(false)
        , length(0)
        , trees()
    {}

    template <typename IteratorT2>
    tree_parse_info(tree_parse_info<IteratorT2> const& pi)
        : stop(pi.stop)
        , match(pi.match)
        , full(pi.full)
        , length(pi.length)
        , trees()
    {
        using std::swap;
        using boost::swap;
        using BOOST_SPIRIT_CLASSIC_NS::swap;

        // use auto_ptr like ownership for the trees data member
        swap(trees, pi.trees);
    }

    tree_parse_info(
        IteratorT   stop_,
        bool        match_,
        bool        full_,
        std::size_t length_,
        typename tree_match<IteratorT, NodeFactoryT, T>::container_t trees_)
    :   stop(stop_)
        , match(match_)
        , full(full_)
        , length(length_)
        , trees()
    {
        using std::swap;
        using boost::swap;
        using BOOST_SPIRIT_CLASSIC_NS::swap;

        // use auto_ptr like ownership for the trees data member
        swap(trees, trees_);
    }
};

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

