/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_TST_JUNE_03_2007_1031AM)
#define BOOST_SPIRIT_TST_JUNE_03_2007_1031AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/string/detail/tst.hpp>

namespace boost { namespace spirit { namespace qi
{
    struct tst_pass_through
    {
        template <typename Char>
        Char operator()(Char ch) const
        {
            return ch;
        }
    };

    template <typename Char, typename T>
    struct tst
    {
        typedef Char char_type; // the character type
        typedef T value_type; // the value associated with each entry
        typedef detail::tst_node<Char, T> node;

        tst()
          : root(0)
        {
        }

        ~tst()
        {
            clear();
        }

        tst(tst const& rhs)
          : root(0)
        {
            copy(rhs);
        }

        tst& operator=(tst const& rhs)
        {
            return assign(rhs);
        }

        template <typename Iterator, typename Filter>
        T* find(Iterator& first, Iterator last, Filter filter) const
        {
            return node::find(root, first, last, filter);
        }

        template <typename Iterator>
        T* find(Iterator& first, Iterator last) const
        {
            return find(first, last, tst_pass_through());
        }

        template <typename Iterator>
        T* add(
            Iterator first
          , Iterator last
          , typename boost::call_traits<T>::param_type val)
        {
            return node::add(root, first, last, val, this);
        }

        template <typename Iterator>
        void remove(Iterator first, Iterator last)
        {
            node::remove(root, first, last, this);
        }

        void clear()
        {
            node::destruct_node(root, this);
            root = 0;
        }

        template <typename F>
        void for_each(F f) const
        {
            node::for_each(root, std::basic_string<Char>(), f);
        }

    private:

        friend struct detail::tst_node<Char, T>;

        void copy(tst const& rhs)
        {
            root = node::clone_node(rhs.root, this);
        }

        tst& assign(tst const& rhs)
        {
            if (this != &rhs)
            {
                clear();
                copy(rhs);
            }
            return *this;
        }

        node* root;

        node* new_node(Char id)
        {
            return new node(id);
        }

        T* new_data(typename boost::call_traits<T>::param_type val)
        {
            return new T(val);
        }

        void delete_node(node* p)
        {
            delete p;
        }

        void delete_data(T* p)
        {
            delete p;
        }
    };
}}}

#endif
