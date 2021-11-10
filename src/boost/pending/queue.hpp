//  (C) Copyright Jeremy Siek 2004
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_QUEUE_HPP
#define BOOST_QUEUE_HPP

#include <deque>
#include <algorithm>

namespace boost
{

template < class _Tp, class _Sequence = std::deque< _Tp > > class queue;

template < class _Tp, class _Seq >
inline bool operator==(const queue< _Tp, _Seq >&, const queue< _Tp, _Seq >&);

template < class _Tp, class _Seq >
inline bool operator<(const queue< _Tp, _Seq >&, const queue< _Tp, _Seq >&);

template < class _Tp, class _Sequence > class queue
{

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
    template < class _Tp1, class _Seq1 >
    friend bool operator==(
        const queue< _Tp1, _Seq1 >&, const queue< _Tp1, _Seq1 >&);
    template < class _Tp1, class _Seq1 >
    friend bool operator<(
        const queue< _Tp1, _Seq1 >&, const queue< _Tp1, _Seq1 >&);
#endif
public:
    typedef typename _Sequence::value_type value_type;
    typedef typename _Sequence::size_type size_type;
    typedef _Sequence container_type;

    typedef typename _Sequence::reference reference;
    typedef typename _Sequence::const_reference const_reference;
#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
protected:
#endif
    _Sequence c;

public:
    queue() : c() {}
    explicit queue(const _Sequence& __c) : c(__c) {}

    bool empty() const { return c.empty(); }
    size_type size() const { return c.size(); }
    reference front() { return c.front(); }
    const_reference front() const { return c.front(); }
    reference top() { return c.front(); }
    const_reference top() const { return c.front(); }
    reference back() { return c.back(); }
    const_reference back() const { return c.back(); }
    void push(const value_type& __x) { c.push_back(__x); }
    void pop() { c.pop_front(); }

    void swap(queue& other)
    {
        using std::swap;
        swap(c, other.c);
    }
};

template < class _Tp, class _Sequence >
bool operator==(
    const queue< _Tp, _Sequence >& __x, const queue< _Tp, _Sequence >& __y)
{
    return __x.c == __y.c;
}

template < class _Tp, class _Sequence >
bool operator<(
    const queue< _Tp, _Sequence >& __x, const queue< _Tp, _Sequence >& __y)
{
    return __x.c < __y.c;
}

template < class _Tp, class _Sequence >
bool operator!=(
    const queue< _Tp, _Sequence >& __x, const queue< _Tp, _Sequence >& __y)
{
    return !(__x == __y);
}

template < class _Tp, class _Sequence >
bool operator>(
    const queue< _Tp, _Sequence >& __x, const queue< _Tp, _Sequence >& __y)
{
    return __y < __x;
}

template < class _Tp, class _Sequence >
bool operator<=(
    const queue< _Tp, _Sequence >& __x, const queue< _Tp, _Sequence >& __y)
{
    return !(__y < __x);
}

template < class _Tp, class _Sequence >
bool operator>=(
    const queue< _Tp, _Sequence >& __x, const queue< _Tp, _Sequence >& __y)
{
    return !(__x < __y);
}

template < class _Tp, class _Sequence >
inline void swap(queue< _Tp, _Sequence >& __x, queue< _Tp, _Sequence >& __y)
{
    __x.swap(__y);
}

} /* namespace boost */

#endif /* BOOST_QUEUE_HPP */
