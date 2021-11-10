/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_TST_MARCH_09_2007_0905AM)
#define BOOST_SPIRIT_TST_MARCH_09_2007_0905AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/call_traits.hpp>
#include <iterator> // for std::iterator_traits

namespace boost { namespace spirit { namespace qi { namespace detail
{
    // This file contains low level TST routines, not for
    // public consumption.

    template <typename Char, typename T>
    struct tst_node
    {
        tst_node(Char id_)
          : id(id_), data(0), lt(0), eq(0), gt(0)
        {
        }

        template <typename Alloc>
        static void
        destruct_node(tst_node* p, Alloc* alloc)
        {
            if (p)
            {
                if (p->data)
                    alloc->delete_data(p->data);
                destruct_node(p->lt, alloc);
                destruct_node(p->eq, alloc);
                destruct_node(p->gt, alloc);
                alloc->delete_node(p);
            }
        }

        template <typename Alloc>
        static tst_node*
        clone_node(tst_node* p, Alloc* alloc)
        {
            if (p)
            {
                tst_node* clone = alloc->new_node(p->id);
                if (p->data)
                    clone->data = alloc->new_data(*p->data);
                clone->lt = clone_node(p->lt, alloc);
                clone->eq = clone_node(p->eq, alloc);
                clone->gt = clone_node(p->gt, alloc);
                return clone;
            }
            return 0;
        }

        template <typename Iterator, typename Filter>
        static T*
        find(tst_node* start, Iterator& first, Iterator last, Filter filter)
        {
            if (first == last)
                return 0;

            Iterator i = first;
            Iterator latest = first;
            tst_node* p = start;
            T* found = 0;

            while (p && i != last)
            {
                typename
                    std::iterator_traits<Iterator>::value_type
                c = filter(*i); // filter only the input

                if (c == p->id)
                {
                    if (p->data)
                    {
                        found = p->data;
                        latest = i;
                    }
                    p = p->eq;
                    i++;
                }
                else if (c < p->id)
                {
                    p = p->lt;
                }
                else
                {
                    p = p->gt;
                }
            }

            if (found)
                first = ++latest; // one past the last matching char
            return found;
        }

        template <typename Iterator, typename Alloc>
        static T*
        add(
            tst_node*& start
          , Iterator first
          , Iterator last
          , typename boost::call_traits<T>::param_type val
          , Alloc* alloc)
        {
            if (first == last)
                return 0;

            tst_node** pp = &start;
            for(;;)
            {
                typename
                    std::iterator_traits<Iterator>::value_type
                c = *first;

                if (*pp == 0)
                    *pp = alloc->new_node(c);
                tst_node* p = *pp;

                if (c == p->id)
                {
                    if (++first == last)
                    {
                        if (p->data == 0)
                            p->data = alloc->new_data(val);
                        return p->data;
                    }
                    pp = &p->eq;
                }
                else if (c < p->id)
                {
                    pp = &p->lt;
                }
                else
                {
                    pp = &p->gt;
                }
            }
        }

        template <typename Iterator, typename Alloc>
        static void
        remove(tst_node*& p, Iterator first, Iterator last, Alloc* alloc)
        {
            if (p == 0 || first == last)
                return;

            typename
                std::iterator_traits<Iterator>::value_type
            c = *first;

            if (c == p->id)
            {
                if (++first == last)
                {
                    if (p->data)
                    {
                        alloc->delete_data(p->data);
                        p->data = 0;
                    }
                }
                remove(p->eq, first, last, alloc);
            }
            else if (c < p->id)
            {
                remove(p->lt, first, last, alloc);
            }
            else
            {
                remove(p->gt, first, last, alloc);
            }

            if (p->data == 0 && p->lt == 0 && p->eq == 0 && p->gt == 0)
            {
                alloc->delete_node(p);
                p = 0;
            }
        }

        template <typename F>
        static void
        for_each(tst_node* p, std::basic_string<Char> prefix, F f)
        {
            if (p)
            {
                for_each(p->lt, prefix, f);
                std::basic_string<Char> s = prefix + p->id;
                for_each(p->eq, s, f);
                if (p->data)
                    f(s, *p->data);
                for_each(p->gt, prefix, f);
            }
        }

        Char id;        // the node's identity character
        T* data;        // optional data
        tst_node* lt;   // left pointer
        tst_node* eq;   // middle pointer
        tst_node* gt;   // right pointer
    };
}}}}

#endif
