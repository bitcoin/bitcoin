// (C) Copyright Jeremy Siek    2004.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_FIBONACCI_HEAP_HPP
#define BOOST_FIBONACCI_HEAP_HPP

#if defined(__sgi) && !defined(__GNUC__)
#include <math.h>
#else
#include <boost/config/no_tr1/cmath.hpp>
#endif
#include <iosfwd>
#include <vector>
#include <functional>
#include <boost/config.hpp>
#include <boost/property_map/property_map.hpp>

//
// An adaptation of Knuth's Fibonacci heap implementation
// in "The Stanford Graph Base", pages 475-482.
//

namespace boost
{

template < class T, class Compare = std::less< T >,
    class ID = identity_property_map >
class fibonacci_heap
{
    typedef typename boost::property_traits< ID >::value_type size_type;
    typedef T value_type;

protected:
    typedef fibonacci_heap self;
    typedef std::vector< size_type > LinkVec;
    typedef typename LinkVec::iterator LinkIter;

public:
    fibonacci_heap(
        size_type n, const Compare& cmp, const ID& id = identity_property_map())
    : _key(n)
    , _left(n)
    , _right(n)
    , _p(n)
    , _mark(n)
    , _degree(n)
    , _n(0)
    , _root(n)
    , _id(id)
    , _compare(cmp)
    , _child(n)
    ,
#if defined(BOOST_MSVC) || defined(__ICL) // need a new macro?
        new_roots(size_type(log(float(n))) + 5)
    {
    }
#else
        new_roots(size_type(std::log(float(n))) + 5)
    {
    }
#endif

    // 33
    void push(const T& d)
    {
        ++_n;
        size_type v = get(_id, d);
        _key[v] = d;
        _p[v] = nil();
        _degree[v] = 0;
        _mark[v] = false;
        _child[v] = nil();
        if (_root == nil())
        {
            _root = _left[v] = _right[v] = v;
            // std::cout << "root added" << std::endl;
        }
        else
        {
            size_type u = _left[_root];
            _left[v] = u;
            _right[v] = _root;
            _left[_root] = _right[u] = v;
            if (_compare(d, _key[_root]))
                _root = v;
            // std::cout << "non-root node added" << std::endl;
        }
    }
    T& top() { return _key[_root]; }
    const T& top() const { return _key[_root]; }

    // 38
    void pop()
    {
        --_n;
        int h = -1;
        size_type v, w;
        if (_root != nil())
        {
            if (_degree[_root] == 0)
            {
                v = _right[_root];
            }
            else
            {
                w = _child[_root];
                v = _right[w];
                _right[w] = _right[_root];
                for (w = v; w != _right[_root]; w = _right[w])
                    _p[w] = nil();
            }
            while (v != _root)
            {
                w = _right[v];
                add_tree_to_new_roots(v, new_roots.begin(), h);
                v = w;
            }
            rebuild_root_list(new_roots.begin(), h);
        }
    }
    // 39
    inline void add_tree_to_new_roots(size_type v, LinkIter new_roots, int& h)
    {
        int r;
        size_type u;
        r = _degree[v];
        while (1)
        {
            if (h < r)
            {
                do
                {
                    ++h;
                    new_roots[h] = (h == r ? v : nil());
                } while (h < r);
                break;
            }
            if (new_roots[r] == nil())
            {
                new_roots[r] = v;
                break;
            }
            u = new_roots[r];
            new_roots[r] = nil();
            if (_compare(_key[u], _key[v]))
            {
                _degree[v] = r;
                _mark[v] = false;
                std::swap(u, v);
            }
            make_child(u, v, r);
            ++r;
        }
        _degree[v] = r;
        _mark[v] = false;
    }
    // 40
    void make_child(size_type u, size_type v, size_type r)
    {
        if (r == 0)
        {
            _child[v] = u;
            _left[u] = u;
            _right[u] = u;
        }
        else
        {
            size_type t = _child[v];
            _right[u] = _right[t];
            _left[u] = t;
            _right[t] = u;
            _left[_right[u]] = u;
        }
        _p[u] = v;
    }
    // 41
    inline void rebuild_root_list(LinkIter new_roots, int& h)
    {
        size_type u, v, w;
        if (h < 0)
            _root = nil();
        else
        {
            T d;
            u = v = new_roots[h];
            d = _key[u];
            _root = u;
            for (h--; h >= 0; --h)
                if (new_roots[h] != nil())
                {
                    w = new_roots[h];
                    _left[w] = v;
                    _right[v] = w;
                    if (_compare(_key[w], d))
                    {
                        _root = w;
                        d = _key[w];
                    }
                    v = w;
                }
            _right[v] = u;
            _left[u] = v;
        }
    }

    // 34
    void update(const T& d)
    {
        size_type v = get(_id, d);
        assert(!_compare(_key[v], d));
        _key[v] = d;
        size_type p = _p[v];
        if (p == nil())
        {
            if (_compare(d, _key[_root]))
                _root = v;
        }
        else if (_compare(d, _key[p]))
            while (1)
            {
                size_type r = _degree[p];
                if (r >= 2)
                    remove_from_family(v, p);
                insert_into_forest(v, d);
                size_type pp = _p[p];
                if (pp == nil())
                {
                    --_degree[p];
                    break;
                }
                if (_mark[p] == false)
                {
                    _mark[p] = true;
                    --_degree[p];
                    break;
                }
                else
                    --_degree[p];
                v = p;
                p = pp;
            }
    }

    inline size_type size() const { return _n; }
    inline bool empty() const { return _n == 0; }

    void print(std::ostream& os)
    {
        if (_root != nil())
        {
            size_type i = _root;
            do
            {
                print_recur(i, os);
                os << std::endl;
                i = _right[i];
            } while (i != _root);
        }
    }

protected:
    // 35
    inline void remove_from_family(size_type v, size_type p)
    {
        size_type u = _left[v];
        size_type w = _right[v];
        _right[u] = w;
        _left[w] = u;
        if (_child[p] == v)
            _child[p] = w;
    }
    // 36
    inline void insert_into_forest(size_type v, const T& d)
    {
        _p[v] = nil();
        size_type u = _left[_root];
        _left[v] = u;
        _right[v] = _root;
        _left[_root] = _right[u] = v;
        if (_compare(d, _key[_root]))
            _root = v;
    }

    void print_recur(size_type x, std::ostream& os)
    {
        if (x != nil())
        {
            os << x;
            if (_degree[x] > 0)
            {
                os << "(";
                size_type i = _child[x];
                do
                {
                    print_recur(i, os);
                    os << " ";
                    i = _right[i];
                } while (i != _child[x]);
                os << ")";
            }
        }
    }

    size_type nil() const { return _left.size(); }

    std::vector< T > _key;
    LinkVec _left, _right, _p;
    std::vector< bool > _mark;
    LinkVec _degree;
    size_type _n, _root;
    ID _id;
    Compare _compare;
    LinkVec _child;
    LinkVec new_roots;
};

} // namespace boost

#endif // BOOST_FIBONACCI_HEAP_HPP
