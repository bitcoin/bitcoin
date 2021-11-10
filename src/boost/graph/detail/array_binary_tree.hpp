//
//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
#ifndef BOOST_ARRAY_BINARY_TREE_HPP
#define BOOST_ARRAY_BINARY_TREE_HPP

#include <iterator>
#include <functional>
#include <boost/config.hpp>

namespace boost
{

/*
 * Note: array_binary_tree is a completey balanced binary tree.
 */
#if !defined BOOST_NO_STD_ITERATOR_TRAITS
template < class RandomAccessIterator, class ID >
#else
template < class RandomAccessIterator, class ValueType, class ID >
#endif
class array_binary_tree_node
{
public:
    typedef array_binary_tree_node ArrayBinaryTreeNode;
    typedef RandomAccessIterator rep_iterator;
#if !defined BOOST_NO_STD_ITERATOR_TRAITS
    typedef
        typename std::iterator_traits< RandomAccessIterator >::difference_type
            difference_type;
    typedef typename std::iterator_traits< RandomAccessIterator >::value_type
        value_type;
#else
    typedef int difference_type;
    typedef ValueType value_type;
#endif
    typedef difference_type size_type;

    struct children_type
    {
        struct iterator
        { // replace with iterator_adaptor implementation -JGS
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef ArrayBinaryTreeNode value_type;
            typedef size_type difference_type;
            typedef array_binary_tree_node* pointer;
            typedef ArrayBinaryTreeNode& reference;

            inline iterator() : i(0), n(0) {}
            inline iterator(const iterator& x)
            : r(x.r), i(x.i), n(x.n), id(x.id)
            {
            }
            inline iterator& operator=(const iterator& x)
            {
                r = x.r;
                i = x.i;
                n = x.n;
                /*egcs generate a warning*/
                id = x.id;
                return *this;
            }
            inline iterator(
                rep_iterator rr, size_type ii, size_type nn, const ID& _id)
            : r(rr), i(ii), n(nn), id(_id)
            {
            }
            inline array_binary_tree_node operator*()
            {
                return ArrayBinaryTreeNode(r, i, n, id);
            }
            inline iterator& operator++()
            {
                ++i;
                return *this;
            }
            inline iterator operator++(int)
            {
                iterator t = *this;
                ++(*this);
                return t;
            }
            inline iterator& operator--()
            {
                --i;
                return *this;
            }
            inline iterator operator--(int)
            {
                iterator t = *this;
                --(*this);
                return t;
            }
            inline bool operator==(const iterator& x) const { return i == x.i; }
            inline bool operator!=(const iterator& x) const
            {
                return !(*this == x);
            }
            rep_iterator r;
            size_type i;
            size_type n;
            ID id;
        };
        inline children_type() : i(0), n(0) {}
        inline children_type(const children_type& x)
        : r(x.r), i(x.i), n(x.n), id(x.id)
        {
        }
        inline children_type& operator=(const children_type& x)
        {
            r = x.r;
            i = x.i;
            n = x.n;
            /*egcs generate a warning*/
            id = x.id;
            return *this;
        }
        inline children_type(
            rep_iterator rr, size_type ii, size_type nn, const ID& _id)
        : r(rr), i(ii), n(nn), id(_id)
        {
        }
        inline iterator begin() { return iterator(r, 2 * i + 1, n, id); }
        inline iterator end() { return iterator(r, 2 * i + 1 + size(), n, id); }
        inline size_type size() const
        {
            size_type c = 2 * i + 1;
            size_type s;
            if (c + 1 < n)
                s = 2;
            else if (c < n)
                s = 1;
            else
                s = 0;
            return s;
        }
        rep_iterator r;
        size_type i;
        size_type n;
        ID id;
    };
    inline array_binary_tree_node() : i(0), n(0) {}
    inline array_binary_tree_node(const array_binary_tree_node& x)
    : r(x.r), i(x.i), n(x.n), id(x.id)
    {
    }
    inline ArrayBinaryTreeNode& operator=(const ArrayBinaryTreeNode& x)
    {
        r = x.r;
        i = x.i;
        n = x.n;
        /*egcs generate a warning*/
        id = x.id;
        return *this;
    }
    inline array_binary_tree_node(
        rep_iterator start, rep_iterator end, rep_iterator pos, const ID& _id)
    : r(start), i(pos - start), n(end - start), id(_id)
    {
    }
    inline array_binary_tree_node(
        rep_iterator rr, size_type ii, size_type nn, const ID& _id)
    : r(rr), i(ii), n(nn), id(_id)
    {
    }
    inline value_type& value() { return *(r + i); }
    inline const value_type& value() const { return *(r + i); }
    inline ArrayBinaryTreeNode parent() const
    {
        return ArrayBinaryTreeNode(r, (i - 1) / 2, n, id);
    }
    inline bool has_parent() const { return i != 0; }
    inline children_type children() { return children_type(r, i, n, id); }
    /*
    inline void swap(array_binary_tree_node x) {
      value_type tmp = x.value();
      x.value() = value();
      value() = tmp;
      i = x.i;
    }
    */
    template < class ExternalData >
    inline void swap(ArrayBinaryTreeNode x, ExternalData& edata)
    {
        using boost::get;

        value_type tmp = x.value();

        /*swap external data*/
        edata[get(id, tmp)] = i;
        edata[get(id, value())] = x.i;

        x.value() = value();
        value() = tmp;
        i = x.i;
    }
    inline const children_type children() const
    {
        return children_type(r, i, n);
    }
    inline size_type index() const { return i; }
    rep_iterator r;
    size_type i;
    size_type n;
    ID id;
};

template < class RandomAccessContainer,
    class Compare = std::less< typename RandomAccessContainer::value_type > >
struct compare_array_node
{
    typedef typename RandomAccessContainer::value_type value_type;
    compare_array_node(const Compare& x) : comp(x) {}
    compare_array_node(const compare_array_node& x) : comp(x.comp) {}

    template < class node_type >
    inline bool operator()(const node_type& x, const node_type& y)
    {
        return comp(x.value(), y.value());
    }

    template < class node_type >
    inline bool operator()(const node_type& x, const node_type& y) const
    {
        return comp(x.value(), y.value());
    }
    Compare comp;
};

} // namespace boost

#endif /* BOOST_ARRAY_BINARY_TREE_HPP */
