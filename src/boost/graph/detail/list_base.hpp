//=======================================================================
// Copyright 2002 Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_LIST_BASE_HPP
#define BOOST_LIST_BASE_HPP

#include <boost/iterator_adaptors.hpp>

// Perhaps this should go through formal review, and move to <boost/>.

/*
  An alternate interface idea:
    Extend the std::list functionality by creating remove/insert
    functions that do not require the container object!
 */

namespace boost
{
namespace detail
{

    //=========================================================================
    // Linked-List Generic Implementation Functions

    template < class Node, class Next >
    inline Node slist_insert_after(Node pos, Node x, Next next)
    {
        next(x) = next(pos);
        next(pos) = x;
        return x;
    }

    // return next(pos) or next(next(pos)) ?
    template < class Node, class Next >
    inline Node slist_remove_after(Node pos, Next next)
    {
        Node n = next(pos);
        next(pos) = next(n);
        return n;
    }

    template < class Node, class Next >
    inline Node slist_remove_range(Node before_first, Node last, Next next)
    {
        next(before_first) = last;
        return last;
    }

    template < class Node, class Next >
    inline Node slist_previous(Node head, Node x, Node empty, Next next)
    {
        while (head != empty && next(head) != x)
            head = next(head);
        return head;
    }

    template < class Node, class Next >
    inline void slist_splice_after(
        Node pos, Node before_first, Node before_last, Next next)
    {
        if (pos != before_first && pos != before_last)
        {
            Node first = next(before_first);
            Node after = next(pos);
            next(before_first) = next(before_last);
            next(pos) = first;
            next(before_last) = after;
        }
    }

    template < class Node, class Next >
    inline Node slist_reverse(Node node, Node empty, Next next)
    {
        Node result = node;
        node = next(node);
        next(result) = empty;
        while (node)
        {
            Node next = next(node);
            next(node) = result;
            result = node;
            node = next;
        }
        return result;
    }

    template < class Node, class Next >
    inline std::size_t slist_size(Node head, Node empty, Next next)
    {
        std::size_t s = 0;
        for (; head != empty; head = next(head))
            ++s;
        return s;
    }

    template < class Next, class Data > class slist_iterator_policies
    {
    public:
        explicit slist_iterator_policies(const Next& n, const Data& d)
        : m_next(n), m_data(d)
        {
        }

        template < class Reference, class Node >
        Reference dereference(type< Reference >, const Node& x) const
        {
            return m_data(x);
        }

        template < class Node > void increment(Node& x) const { x = m_next(x); }

        template < class Node > bool equal(Node& x, Node& y) const
        {
            return x == y;
        }

    protected:
        Next m_next;
        Data m_data;
    };

    //===========================================================================
    // Doubly-Linked List Generic Implementation Functions

    template < class Node, class Next, class Prev >
    inline void dlist_insert_before(Node pos, Node x, Next next, Prev prev)
    {
        next(x) = pos;
        prev(x) = prev(pos);
        next(prev(pos)) = x;
        prev(pos) = x;
    }

    template < class Node, class Next, class Prev >
    void dlist_remove(Node pos, Next next, Prev prev)
    {
        Node next_node = next(pos);
        Node prev_node = prev(pos);
        next(prev_node) = next_node;
        prev(next_node) = prev_node;
    }

    // This deletes every node in the list except the
    // sentinel node.
    template < class Node, class Delete >
    inline void dlist_clear(Node sentinel, Delete del)
    {
        Node i, tmp;
        i = next(sentinel);
        while (i != sentinel)
        {
            tmp = i;
            i = next(i);
            del(tmp);
        }
    }

    template < class Node > inline bool dlist_empty(Node dummy)
    {
        return next(dummy) == dummy;
    }

    template < class Node, class Next, class Prev >
    void dlist_transfer(Node pos, Node first, Node last, Next next, Prev prev)
    {
        if (pos != last)
        {
            // Remove [first,last) from its old position
            next(prev(last)) = pos;
            next(prev(first)) = last;
            next(prev(pos)) = first;

            // Splice [first,last) into its new position
            Node tmp = prev(pos);
            prev(pos) = prev(last);
            prev(last) = prev(first);
            prev(first) = tmp;
        }
    }

    template < class Next, class Prev, class Data >
    class dlist_iterator_policies : public slist_iterator_policies< Next, Data >
    {
        typedef slist_iterator_policies< Next, Data > Base;

    public:
        template < class Node > void decrement(Node& x) const { x = m_prev(x); }

        dlist_iterator_policies(Next n, Prev p, Data d) : Base(n, d), m_prev(p)
        {
        }

    protected:
        Prev m_prev;
    };

} // namespace detail
} // namespace boost

#endif // BOOST_LIST_BASE_HPP
