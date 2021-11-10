// boost heap: node tree iterator helper classes
//
// Copyright (C) 2010 Tim Blechmann
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HEAP_DETAIL_TREE_ITERATOR_HPP
#define BOOST_HEAP_DETAIL_TREE_ITERATOR_HPP

#include <functional>
#include <vector>

#include <boost/core/allocator_access.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/type_traits/conditional.hpp>
#include <queue>

namespace boost  {
namespace heap   {
namespace detail {


template<typename type>
struct identity
{
    type& operator()(type& x) const BOOST_NOEXCEPT
    { return x; }

    const type& operator()(const type& x) const BOOST_NOEXCEPT
    { return x; }
};

template<typename Node>
struct dereferencer
{
    template <typename Iterator>
    Node * operator()(Iterator const & it)
    {
        return static_cast<Node *>(*it);
    }
};

template<typename Node>
struct pointer_to_reference
{
    template <typename Iterator>
    const Node * operator()(Iterator const & it)
    {
        return static_cast<const Node *>(&*it);
    }
};


template <typename HandleType,
          typename Alloc,
          typename ValueCompare
         >
struct unordered_tree_iterator_storage
{
    unordered_tree_iterator_storage(ValueCompare const & cmp)
    {}

    void push(HandleType h)
    {
        data_.push_back(h);
    }

    HandleType const & top(void)
    {
        return data_.back();
    }

    void pop(void)
    {
        data_.pop_back();
    }

    bool empty(void) const
    {
        return data_.empty();
    }

    std::vector<HandleType, typename boost::allocator_rebind<Alloc, HandleType>::type> data_;
};

template <typename ValueType,
          typename HandleType,
          typename Alloc,
          typename ValueCompare,
          typename ValueExtractor
         >
struct ordered_tree_iterator_storage:
    ValueExtractor
{
    struct compare_values_by_handle:
        ValueExtractor,
        ValueCompare
    {
        compare_values_by_handle(ValueCompare const & cmp):
            ValueCompare(cmp)
        {}

        bool operator()(HandleType const & lhs, HandleType const & rhs) const
        {
            ValueType const & lhs_value = ValueExtractor::operator()(lhs->value);
            ValueType const & rhs_value = ValueExtractor::operator()(rhs->value);
            return ValueCompare::operator()(lhs_value, rhs_value);
        }
    };

    ordered_tree_iterator_storage(ValueCompare const & cmp):
        data_(compare_values_by_handle(cmp))
    {}

    void push(HandleType h)
    {
        data_.push(h);
    }

    void pop(void)
    {
        data_.pop();
    }

    HandleType const & top(void)
    {
        return data_.top();
    }

    bool empty(void) const BOOST_NOEXCEPT
    {
        return data_.empty();
    }

    std::priority_queue<HandleType,
                        std::vector<HandleType, typename boost::allocator_rebind<Alloc, HandleType>::type>,
                        compare_values_by_handle> data_;
};


/* tree iterator helper class
 *
 * Requirements:
 * Node provides child_iterator
 * ValueExtractor can convert Node->value to ValueType
 *
 * */
template <typename Node,
          typename ValueType,
          typename Alloc = std::allocator<Node>,
          typename ValueExtractor = identity<typename Node::value_type>,
          typename PointerExtractor = dereferencer<Node>,
          bool check_null_pointer = false,
          bool ordered_iterator = false,
          typename ValueCompare = std::less<ValueType>
         >
class tree_iterator:
    public boost::iterator_adaptor<tree_iterator<Node,
                                                 ValueType,
                                                 Alloc,
                                                 ValueExtractor,
                                                 PointerExtractor,
                                                 check_null_pointer,
                                                 ordered_iterator,
                                                 ValueCompare
                                                >,
                                   const Node *,
                                   ValueType,
                                   boost::forward_traversal_tag
                                  >,
    ValueExtractor,
    PointerExtractor
{
    typedef boost::iterator_adaptor<tree_iterator<Node,
                                                  ValueType,
                                                  Alloc,
                                                  ValueExtractor,
                                                  PointerExtractor,
                                                  check_null_pointer,
                                                  ordered_iterator,
                                                  ValueCompare
                                                 >,
                                    const Node *,
                                    ValueType,
                                    boost::forward_traversal_tag
                                   > adaptor_type;

    friend class boost::iterator_core_access;

    typedef typename boost::conditional< ordered_iterator,
                                       ordered_tree_iterator_storage<ValueType, const Node*, Alloc, ValueCompare, ValueExtractor>,
                                       unordered_tree_iterator_storage<const Node*, Alloc, ValueCompare>
                                     >::type
        unvisited_node_container;

public:
    tree_iterator(void):
        adaptor_type(0), unvisited_nodes(ValueCompare())
    {}

    tree_iterator(ValueCompare const & cmp):
        adaptor_type(0), unvisited_nodes(cmp)
    {}

    tree_iterator(const Node * it, ValueCompare const & cmp):
        adaptor_type(it), unvisited_nodes(cmp)
    {
        if (it)
            discover_nodes(it);
    }

    /* fills the iterator from a list of possible top nodes */
    template <typename NodePointerIterator>
    tree_iterator(NodePointerIterator begin, NodePointerIterator end, const Node * top_node, ValueCompare const & cmp):
        adaptor_type(0), unvisited_nodes(cmp)
    {
        BOOST_STATIC_ASSERT(ordered_iterator);
        if (begin == end)
            return;

        adaptor_type::base_reference() = top_node;
        discover_nodes(top_node);

        for (NodePointerIterator it = begin; it != end; ++it) {
            const Node * current_node = static_cast<const Node*>(&*it);
            if (current_node != top_node)
                unvisited_nodes.push(current_node);
        }
    }

    bool operator!=(tree_iterator const & rhs) const
    {
        return adaptor_type::base() != rhs.base();
    }

    bool operator==(tree_iterator const & rhs) const
    {
        return !operator!=(rhs);
    }

    const Node * get_node() const
    {
        return adaptor_type::base_reference();
    }

private:
    void increment(void)
    {
        if (unvisited_nodes.empty())
            adaptor_type::base_reference() = 0;
        else {
            const Node * next = unvisited_nodes.top();
            unvisited_nodes.pop();
            discover_nodes(next);
            adaptor_type::base_reference() = next;
        }
    }

    ValueType const & dereference() const
    {
        return ValueExtractor::operator()(adaptor_type::base_reference()->value);
    }

    void discover_nodes(const Node * n)
    {
        for (typename Node::const_child_iterator it = n->children.begin(); it != n->children.end(); ++it) {
            const Node * n = PointerExtractor::operator()(it);
            if (check_null_pointer && n == NULL)
                continue;
            unvisited_nodes.push(n);
        }
    }

    unvisited_node_container unvisited_nodes;
};

template <typename Node, typename NodeList>
struct list_iterator_converter
{
    typename NodeList::const_iterator operator()(const Node * node)
    {
        return NodeList::s_iterator_to(*node);
    }

    Node * operator()(typename NodeList::const_iterator it)
    {
        return const_cast<Node*>(static_cast<const Node*>(&*it));
    }
};

template <typename Node,
          typename NodeIterator,
          typename ValueType,
          typename ValueExtractor = identity<typename Node::value_type>,
          typename IteratorCoverter = identity<NodeIterator>
         >
class recursive_tree_iterator:
    public boost::iterator_adaptor<recursive_tree_iterator<Node,
                                                           NodeIterator,
                                                           ValueType,
                                                           ValueExtractor,
                                                           IteratorCoverter
                                                          >,
                                    NodeIterator,
                                    ValueType const,
                                    boost::bidirectional_traversal_tag>,
    ValueExtractor, IteratorCoverter
{
    typedef boost::iterator_adaptor<recursive_tree_iterator<Node,
                                                            NodeIterator,
                                                            ValueType,
                                                            ValueExtractor,
                                                            IteratorCoverter
                                                           >,
                                    NodeIterator,
                                    ValueType const,
                                    boost::bidirectional_traversal_tag> adaptor_type;

    friend class boost::iterator_core_access;


public:
    recursive_tree_iterator(void):
        adaptor_type(0)
    {}

    explicit recursive_tree_iterator(NodeIterator const & it):
        adaptor_type(it)
    {}

    void increment(void)
    {
        NodeIterator next = adaptor_type::base_reference();

        const Node * n = get_node(next);
        if (n->children.empty()) {
            const Node * parent = get_node(next)->get_parent();

            ++next;

            while (true) {
                if (parent == NULL || next != parent->children.end())
                    break;

                next = IteratorCoverter::operator()(parent);
                parent = get_node(next)->get_parent();
                ++next;
            }
        } else
            next = n->children.begin();

        adaptor_type::base_reference() = next;
        return;
    }

    ValueType const & dereference() const
    {
        return ValueExtractor::operator()(get_node(adaptor_type::base_reference())->value);
    }

    static const Node * get_node(NodeIterator const & it)
    {
        return static_cast<const Node *>(&*it);
    }

    const Node * get_node() const
    {
        return get_node(adaptor_type::base_reference());
    }
};


} /* namespace detail */
} /* namespace heap */
} /* namespace boost */

#endif /* BOOST_HEAP_DETAIL_TREE_ITERATOR_HPP */
