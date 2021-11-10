/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_TST_MAP_JUNE_03_2007_1143AM)
#define BOOST_SPIRIT_X3_TST_MAP_JUNE_03_2007_1143AM

#include <boost/spirit/home/x3/string/detail/tst.hpp>
#include <unordered_map>
#include <boost/pool/object_pool.hpp>

namespace boost { namespace spirit { namespace x3
{
    struct tst_pass_through; // declared in tst.hpp

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
                if (i == map.end())
                {
                    first = save;
                    return 0;
                }
                if (T* p = node::find(i->second.root, first, last, filter))
                {
                    return p;
                }
                return i->second.data;
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
            for (typename map_type::value_type& x : map)
            {
                node::destruct_node(x.second.root, this);
                if (x.second.data)
                    this->delete_data(x.second.data);
            }
            map.clear();
        }

        template <typename F>
        void for_each(F f) const
        {
            for (typename map_type::value_type const& x : map)
            {
                std::basic_string<Char> s(1, x.first);
                node::for_each(x.second.root, s, f);
                if (x.second.data)
                    f(s, *x.second.data);
            }
        }

    private:

        friend struct detail::tst_node<Char, T>;

        struct map_data
        {
            node* root;
            T* data;
        };

        typedef std::unordered_map<Char, map_data> map_type;

        void copy(tst_map const& rhs)
        {
            for (typename map_type::value_type const& x : rhs.map)
            {
                map_data xx = {node::clone_node(x.second.root, this), 0};
                if (x.second.data)
                    xx.data = data_pool.construct(*x.second.data);
                map[x.first] = xx;
            }
        }

        tst_map& assign(tst_map const& rhs)
        {
            if (this != &rhs)
            {
                for (typename map_type::value_type& x : map)
                {
                    node::destruct_node(x.second.root, this);
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
