///////////////////////////////////////////////////////////////////////////////
// list.hpp
//    A simple implementation of std::list that allows incomplete
//    types, does no dynamic allocation in the default constructor,
//    and has a guarnteed O(1) splice.
//
//  Copyright 2009 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_LIST_HPP_EAN_10_26_2009
#define BOOST_XPRESSIVE_DETAIL_CORE_LIST_HPP_EAN_10_26_2009

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <cstddef>
#include <iterator>
#include <algorithm>
#include <boost/assert.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace boost { namespace xpressive { namespace detail
{

    ///////////////////////////////////////////////////////////////////////////////
    // list
    //
    template<typename T>
    struct list
    {
    private:
        struct node_base
        {
            node_base *_prev;
            node_base *_next;
        };

        struct node : node_base
        {
            explicit node(T const &value)
              : _value(value)
            {}

            T _value;
        };

        node_base _sentry;

        template<typename Ref = T &>
        struct list_iterator
          : boost::iterator_facade<list_iterator<Ref>, T, std::bidirectional_iterator_tag, Ref>
        {
            list_iterator(list_iterator<> const &it) : _node(it._node) {}
            explicit list_iterator(node_base *n = 0) : _node(n) {}
        private:
            friend struct list<T>;
            friend class boost::iterator_core_access;
            Ref dereference() const { return static_cast<node *>(_node)->_value; }
            void increment() { _node = _node->_next; }
            void decrement() { _node = _node->_prev; }
            bool equal(list_iterator const &it) const { return _node == it._node; }
            node_base *_node;
        };

    public:
        typedef T *pointer;
        typedef T const *const_pointer;
        typedef T &reference;
        typedef T const &const_reference;
        typedef list_iterator<> iterator;
        typedef list_iterator<T const &> const_iterator;
        typedef std::size_t size_type;

        list()
        {
            _sentry._next = _sentry._prev = &_sentry;
        }

        list(list const &that)
        {
            _sentry._next = _sentry._prev = &_sentry;
            const_iterator it = that.begin(), e = that.end();
            for( ; it != e; ++it)
                push_back(*it);
        }

        list &operator =(list const &that)
        {
            list(that).swap(*this);
            return *this;
        }

        ~list()
        {
            clear();
        }

        void clear()
        {
            while(!empty())
                pop_front();
        }

        void swap(list &that) // throw()
        {
            list temp;
            temp.splice(temp.begin(), that);  // move that to temp
            that.splice(that.begin(), *this); // move this to that
            splice(begin(), temp);            // move temp to this
        }

        void push_front(T const &t)
        {
            node *new_node = new node(t);

            new_node->_next = _sentry._next;
            new_node->_prev = &_sentry;

            _sentry._next->_prev = new_node;
            _sentry._next = new_node;
        }

        void push_back(T const &t)
        {
            node *new_node = new node(t);

            new_node->_next = &_sentry;
            new_node->_prev = _sentry._prev;

            _sentry._prev->_next = new_node;
            _sentry._prev = new_node;
        }

        void pop_front()
        {
            BOOST_ASSERT(!empty());
            node *old_node = static_cast<node *>(_sentry._next);
            _sentry._next = old_node->_next;
            _sentry._next->_prev = &_sentry;
            delete old_node;
        }

        void pop_back()
        {
            BOOST_ASSERT(!empty());
            node *old_node = static_cast<node *>(_sentry._prev);
            _sentry._prev = old_node->_prev;
            _sentry._prev->_next = &_sentry;
            delete old_node;
        }

        bool empty() const
        {
            return _sentry._next == &_sentry;
        }

        void splice(iterator it, list &x)
        {
            if(x.empty())
                return;

            x._sentry._prev->_next = it._node;
            x._sentry._next->_prev = it._node->_prev;

            it._node->_prev->_next = x._sentry._next;
            it._node->_prev = x._sentry._prev;

            x._sentry._prev = x._sentry._next = &x._sentry;
        }

        void splice(iterator it, list &, iterator xit)
        {
            xit._node->_prev->_next = xit._node->_next;
            xit._node->_next->_prev = xit._node->_prev;

            xit._node->_next = it._node;
            xit._node->_prev = it._node->_prev;

            it._node->_prev = it._node->_prev->_next = xit._node;
        }

        reference front()
        {
            BOOST_ASSERT(!empty());
            return static_cast<node *>(_sentry._next)->_value;
        }

        const_reference front() const
        {
            BOOST_ASSERT(!empty());
            return static_cast<node *>(_sentry._next)->_value;
        }

        reference back()
        {
            BOOST_ASSERT(!empty());
            return static_cast<node *>(_sentry._prev)->_value;
        }

        const_reference back() const
        {
            BOOST_ASSERT(!empty());
            return static_cast<node *>(_sentry._prev)->_value;
        }

        iterator begin()
        {
            return iterator(_sentry._next);
        }

        const_iterator begin() const
        {
            return const_iterator(_sentry._next);
        }

        iterator end()
        {
            return iterator(&_sentry);
        }

        const_iterator end() const
        {
            return const_iterator(const_cast<node_base *>(&_sentry));
        }

        size_type size() const
        {
            return static_cast<size_type>(std::distance(begin(), end()));
        }
    };

    template<typename T>
    void swap(list<T> &lhs, list<T> &rhs)
    {
        lhs.swap(rhs);
    }

}}} // namespace boost::xpressive::detail

#endif
