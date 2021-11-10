/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_TST_MAP_JUNE_03_2007_1143AM)
#define BOOST_SPIRIT_TST_MAP_JUNE_03_2007_1143AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/string/tst.hpp>
#include <boost/spirit/home/qi/string/detail/tst.hpp>
#include <boost/unordered_map.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/foreach.hpp>

namespace boost { namespace spirit { namespace qi
{
    template <typename Char, typename T>
    struct tst_map
    {
        typedef Char char_type; // the character type
        typedef T value_type; // the value associated with each entry
        typedef detail::tst_node<Char, T> node;

        tst_map()
        {
        }

        ~tst_map()
        {
            // Nothing to do here.
            // The pools do the right thing for us
        }

        tst_map(tst_map const& rhs)
        {
            copy(rhs);
        }

        tst_map& operator=(tst_map const& rhs)
        {
            return assign(rhs);
        }

        template <typename Iterator, typename Filter>
        T* find(Iterator& first, Iterator last, Filter filter) const
        {
            if (first != last)
            {
                Iterator save = first;
                typename map_type::const_iterator
                    i = map.find(filter(*first++));

                if (i != map.end())
                {
                    if (T* p = node::find(i->second.root, first, last, filter))
                    {
                        return p;
                    }
                   
                    if (i->second.data)
                    {
                        return i->second.data;
                    }
                }
                first = save;
            }
            return 0;
        }

        template <typename Iterator>
        T* find(Iterator& first, Iterator last) const
        {
            return find(first, last, tst_pass_through());
        }

        template <typename Iterator>
        bool add(
            Iterator first
          , Iterator last
          , typename boost::call_traits<T>::param_type val)
        {
            if (first != last)
            {
                map_data x = {0, 0};
                std::pair<typename map_type::iterator, bool>
                    r = map.insert(std::pair<Char, map_data>(*first++, x));

                if (first != last)
                {
                    return node::add(r.first->second.root
                      , first, last, val, this) ? true : false;
                }
                else
                {
                    if (r.first->second.data)
                        return false;
                    r.first->second.data = this->new_data(val);
                }
                return true;
            }
            return false;
        }

        template <typename Iterator>
        void remove(Iterator first, Iterator last)
        {
            if (first != last)
            {
                typename map_type::iterator i = map.find(*first++);
                if (i != map.end())
                {
                    if (first != last)
                    {
                        node::remove(i->second.root, first, last, this);
                    }
                    else if (i->second.data)
                    {
                        this->delete_data(i->second.data);
                        i->second.data = 0;
                    }
                    if (i->second.data == 0 && i->second.root == 0)
                    {
                        map.erase(i);
                    }
                }
            }
        }

        void clear()
        {
            typedef typename map_type::iterator iter_t;
            for (iter_t it = map.begin(), end = map.end(); it != end; ++it)
            {
                node::destruct_node(it->second.root, this);
                if (it->second.data)
                    this->delete_data(it->second.data);
            }
            map.clear();
        }

        template <typename F>
        void for_each(F f) const
        {
            typedef typename map_type::const_iterator iter_t;
            for (iter_t it = map.begin(), end = map.end(); it != end; ++it)
            {
                std::basic_string<Char> s(1, it->first);
                node::for_each(it->second.root, s, f);
                if (it->second.data)
                    f(s, *it->second.data);
            }
        }

    private:

        friend struct detail::tst_node<Char, T>;

        struct map_data
        {
            node* root;
            T* data;
        };

        typedef unordered_map<Char, map_data> map_type;

        void copy(tst_map const& rhs)
        {
            typedef typename map_type::const_iterator iter_t;
            for (iter_t it = rhs.map.begin(), end = rhs.map.end(); it != end; ++it)
            {
                map_data xx = {node::clone_node(it->second.root, this), 0};
                if (it->second.data)
                    xx.data = data_pool.construct(*it->second.data);
                map[it->first] = xx;
            }
        }

        tst_map& assign(tst_map const& rhs)
        {
            if (this != &rhs)
            {
                typedef typename map_type::const_iterator iter_t;
                for (iter_t it = map.begin(), end = map.end(); it != end; ++it)
                {
                    node::destruct_node(it->second.root, this);
                }
                map.clear();
                copy(rhs);
            }
            return *this;
        }

        node* new_node(Char id)
        {
            return node_pool.construct(id);
        }

        T* new_data(typename boost::call_traits<T>::param_type val)
        {
            return data_pool.construct(val);
        }

        void delete_node(node* p)
        {
            node_pool.destroy(p);
        }

        void delete_data(T* p)
        {
            data_pool.destroy(p);
        }

        map_type map;
        object_pool<node> node_pool;
        object_pool<T> data_pool;
    };
}}}

#endif
