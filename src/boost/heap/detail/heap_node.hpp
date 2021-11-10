// boost heap: heap node helper classes
//
// Copyright (C) 2010 Tim Blechmann
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HEAP_DETAIL_HEAP_NODE_HPP
#define BOOST_HEAP_DETAIL_HEAP_NODE_HPP

#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/core/allocator_access.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/type_traits/conditional.hpp>

#ifdef BOOST_HEAP_SANITYCHECKS
#define BOOST_HEAP_ASSERT BOOST_ASSERT
#else
#define BOOST_HEAP_ASSERT(expression)
#endif


namespace boost  {
namespace heap   {
namespace detail {

namespace bi = boost::intrusive;

template <bool auto_unlink = false>
struct heap_node_base:
    bi::list_base_hook<typename boost::conditional<auto_unlink,
                                          bi::link_mode<bi::auto_unlink>,
                                          bi::link_mode<bi::safe_link>
                                         >::type
                      >
{};

typedef bi::list<heap_node_base<false> > heap_node_list;

struct nop_disposer
{
    template <typename T>
    void operator()(T * n)
    {
        BOOST_HEAP_ASSERT(false);
    }
};

template <typename Node, typename HeapBase>
bool is_heap(const Node * n, typename HeapBase::value_compare const & cmp)
{
    for (typename Node::const_child_iterator it = n->children.begin(); it != n->children.end(); ++it) {
        Node const & this_node = static_cast<Node const &>(*it);
        const Node * child = static_cast<const Node*>(&this_node);

        if (cmp(HeapBase::get_value(n->value), HeapBase::get_value(child->value)) ||
            !is_heap<Node, HeapBase>(child, cmp))
            return false;
    }
    return true;
}

template <typename Node>
std::size_t count_nodes(const Node * n);

template <typename Node, typename List>
std::size_t count_list_nodes(List const & node_list)
{
    std::size_t ret = 0;

    for (typename List::const_iterator it = node_list.begin(); it != node_list.end(); ++it) {
        const Node * child = static_cast<const Node*>(&*it);
        ret += count_nodes<Node>(child);
    }
    return ret;
}

template <typename Node>
std::size_t count_nodes(const Node * n)
{
    return 1 + count_list_nodes<Node, typename Node::child_list>(n->children);
}

template<class Node>
void destroy_node(Node& node)
{
    node.~Node();
}


/* node cloner
 *
 * Requires `Clone Constructor':
 * template <typename Alloc>
 * Node::Node(Node const &, Alloc &)
 *
 * template <typename Alloc>
 * Node::Node(Node const &, Alloc &, Node * parent)
 *
 * */
template <typename Node,
          typename NodeBase,
          typename Alloc>
struct node_cloner
{
    node_cloner(Alloc & allocator):
        allocator(allocator)
    {}

    Node * operator() (NodeBase const & node)
    {
        Node * ret = allocator.allocate(1);
        new (ret) Node(static_cast<Node const &>(node), allocator);
        return ret;
    }

    Node * operator() (NodeBase const & node, Node * parent)
    {
        Node * ret = allocator.allocate(1);
        new (ret) Node(static_cast<Node const &>(node), allocator, parent);
        return ret;
    }

private:
    Alloc & allocator;
};

/* node disposer
 *
 * Requirements:
 * Node::clear_subtree(Alloc &) clears the subtree via allocator
 *
 * */
template <typename Node,
          typename NodeBase,
          typename Alloc>
struct node_disposer
{
    typedef typename boost::allocator_pointer<Alloc>::type node_pointer;

    node_disposer(Alloc & alloc):
        alloc_(alloc)
    {}

    void operator()(NodeBase * base)
    {
        node_pointer n = static_cast<node_pointer>(base);
        n->clear_subtree(alloc_);
        boost::heap::detail::destroy_node(*n);
        alloc_.deallocate(n, 1);
    }

    Alloc & alloc_;
};


template <typename ValueType,
          bool constant_time_child_size = true
         >
struct heap_node:
    heap_node_base<!constant_time_child_size>
{
    typedef heap_node_base<!constant_time_child_size> node_base;

public:
    typedef ValueType value_type;

    typedef bi::list<node_base,
                     bi::constant_time_size<constant_time_child_size> > child_list;

    typedef typename child_list::iterator child_iterator;
    typedef typename child_list::const_iterator const_child_iterator;
    typedef typename child_list::size_type size_type;

    heap_node(ValueType const & v):
        value(v)
    {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <class... Args>
    heap_node(Args&&... args):
        value(std::forward<Args>(args)...)
    {}
#endif

/* protected:                      */
    heap_node(heap_node const & rhs):
        value(rhs.value)
    {
        /* we don't copy the child list, but clone it later */
    }

public:

    template <typename Alloc>
    heap_node (heap_node const & rhs, Alloc & allocator):
        value(rhs.value)
    {
        children.clone_from(rhs.children, node_cloner<heap_node, node_base, Alloc>(allocator), nop_disposer());
    }

    size_type child_count(void) const
    {
        BOOST_STATIC_ASSERT(constant_time_child_size);
        return children.size();
    }

    void add_child(heap_node * n)
    {
        children.push_back(*n);
    }

    template <typename Alloc>
    void clear_subtree(Alloc & alloc)
    {
        children.clear_and_dispose(node_disposer<heap_node, node_base, Alloc>(alloc));
    }

    void swap_children(heap_node * rhs)
    {
        children.swap(rhs->children);
    }

    ValueType value;
    child_list children;
};

template <typename value_type>
struct parent_pointing_heap_node:
    heap_node<value_type>
{
    typedef heap_node<value_type> super_t;

    parent_pointing_heap_node(value_type const & v):
        super_t(v), parent(NULL)
    {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <class... Args>
    parent_pointing_heap_node(Args&&... args):
        super_t(std::forward<Args>(args)...), parent(NULL)
    {}
#endif

    template <typename Alloc>
    struct node_cloner
    {
        node_cloner(Alloc & allocator, parent_pointing_heap_node * parent):
            allocator(allocator), parent_(parent)
        {}

        parent_pointing_heap_node * operator() (typename super_t::node_base const & node)
        {
            parent_pointing_heap_node * ret = allocator.allocate(1);
            new (ret) parent_pointing_heap_node(static_cast<parent_pointing_heap_node const &>(node), allocator, parent_);
            return ret;
        }

    private:
        Alloc & allocator;
        parent_pointing_heap_node * parent_;
    };

    template <typename Alloc>
    parent_pointing_heap_node (parent_pointing_heap_node const & rhs, Alloc & allocator, parent_pointing_heap_node * parent):
        super_t(static_cast<super_t const &>(rhs)), parent(parent)
    {
        super_t::children.clone_from(rhs.children, node_cloner<Alloc>(allocator, this), nop_disposer());
    }

    void update_children(void)
    {
        typedef heap_node_list::iterator node_list_iterator;
        for (node_list_iterator it = super_t::children.begin(); it != super_t::children.end(); ++it) {
            parent_pointing_heap_node * child = static_cast<parent_pointing_heap_node*>(&*it);
            child->parent = this;
        }
    }

    void remove_from_parent(void)
    {
        BOOST_HEAP_ASSERT(parent);
        parent->children.erase(heap_node_list::s_iterator_to(*this));
        parent = NULL;
    }

    void add_child(parent_pointing_heap_node * n)
    {
        BOOST_HEAP_ASSERT(n->parent == NULL);
        n->parent = this;
        super_t::add_child(n);
    }

    parent_pointing_heap_node * get_parent(void)
    {
        return parent;
    }

    const parent_pointing_heap_node * get_parent(void) const
    {
        return parent;
    }

    parent_pointing_heap_node * parent;
};


template <typename value_type>
struct marked_heap_node:
    parent_pointing_heap_node<value_type>
{
    typedef parent_pointing_heap_node<value_type> super_t;

    marked_heap_node(value_type const & v):
        super_t(v), mark(false)
    {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <class... Args>
    marked_heap_node(Args&&... args):
        super_t(std::forward<Args>(args)...), mark(false)
    {}
#endif

    marked_heap_node * get_parent(void)
    {
        return static_cast<marked_heap_node*>(super_t::parent);
    }

    const marked_heap_node * get_parent(void) const
    {
        return static_cast<marked_heap_node*>(super_t::parent);
    }

    bool mark;
};


template <typename Node>
struct cmp_by_degree
{
    template <typename NodeBase>
    bool operator()(NodeBase const & left,
                    NodeBase const & right)
    {
        return static_cast<const Node*>(&left)->child_count() < static_cast<const Node*>(&right)->child_count();
    }
};

template <typename List, typename Node, typename Cmp>
Node * find_max_child(List const & list, Cmp const & cmp)
{
    BOOST_HEAP_ASSERT(!list.empty());

    const Node * ret = static_cast<const Node *> (&list.front());
    for (typename List::const_iterator it = list.begin(); it != list.end(); ++it) {
        const Node * current = static_cast<const Node *> (&*it);

        if (cmp(ret->value, current->value))
            ret = current;
    }

    return const_cast<Node*>(ret);
}

} /* namespace detail */
} /* namespace heap */
} /* namespace boost */

#undef BOOST_HEAP_ASSERT
#endif /* BOOST_HEAP_DETAIL_HEAP_NODE_HPP */
