// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MRUSET_H
#define BITCOIN_MRUSET_H

#include <set>
#include <vector>
#include <utility>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

/** STL-like set container that only keeps the most recent N elements. */
template <typename T>
class mruset
{
public:
    typedef T key_type;
    typedef T value_type;
    typedef typename std::set<T>::iterator iterator;
    typedef typename std::set<T>::const_iterator const_iterator;
    typedef typename std::set<T>::size_type size_type;

protected:
    std::set<T> set;
    std::vector<iterator> order;
    size_type first_used;
    size_type first_unused;
    const size_type nMaxSize;

public:
    mruset(size_type nMaxSizeIn = 1) : nMaxSize(nMaxSizeIn) { clear(); }
    iterator begin() const { return set.begin(); }
    iterator end() const { return set.end(); }
    size_type size() const { return set.size(); }
    bool empty() const { return set.empty(); }
    iterator find(const key_type& k) const { return set.find(k); }
    size_type count(const key_type& k) const { return set.count(k); }
    void clear()
    {
        set.clear();
        order.assign(nMaxSize, set.end());
        first_used = 0;
        first_unused = 0;
    }
    bool inline friend operator==(const mruset<T>& a, const mruset<T>& b) { return a.set == b.set; }
    bool inline friend operator==(const mruset<T>& a, const std::set<T>& b) { return a.set == b; }
    bool inline friend operator<(const mruset<T>& a, const mruset<T>& b) { return a.set < b.set; }
    std::pair<iterator, bool> insert(const key_type& x)
    {
        std::pair<iterator, bool> ret = set.insert(x);
        if (ret.second) {
            if (set.size() == nMaxSize + 1) {
                set.erase(order[first_used]);
                order[first_used] = set.end();
                if (++first_used == nMaxSize) first_used = 0;
            }
            order[first_unused] = ret.first;
            if (++first_unused == nMaxSize) first_unused = 0;
        }
        return ret;
    }
    size_type max_size() const { return nMaxSize; }
};

/** STL-like map container that only keeps the most recent N elements. */
template <typename K, typename V>
class mrumap
{
private:
    struct key_extractor {
        typedef K result_type;
        const result_type& operator()(const std::pair<K, V>& e) const { return e.first; }
        result_type& operator()(std::pair<K, V>* e) const { return e->first; }
    };

    typedef boost::multi_index_container<
        std::pair<K, V>,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<key_extractor>
        >
    > map_type;

public:
    typedef K key_type;
    typedef std::pair<K, V> value_type;
    typedef typename map_type::iterator iterator;
    typedef typename map_type::const_iterator const_iterator;
    typedef typename map_type::size_type size_type;

protected:
    map_type m_;
    size_type max_size_;

public:
    mrumap(size_type max_size_in = 1) { clear(max_size_in); }
    iterator begin() { return m_.begin(); }
    iterator end() { return m_.end(); }
    const_iterator begin() const { return m_.begin(); }
    const_iterator end() const { return m_.end(); }
    size_type size() const { return m_.size(); }
    bool empty() const { return m_.empty(); }
    iterator find(const key_type& key) { return m_.template project<0>(boost::get<1>(m_).find(key)); }
    const_iterator find(const key_type& key) const { return m_.template project<0>(boost::get<1>(m_).find(key)); }
    size_type count(const key_type& key) const { return boost::get<1>(m_).count(key); }
    void clear(size_type max_size_in) { m_.clear(); max_size_ = max_size_in; }
    std::pair<iterator, bool> insert(const K& key, const V& value) 
    {
        std::pair<K, V> elem(key, value);
        std::pair<iterator, bool> p = m_.push_front(elem);
        if (p.second && m_.size() > max_size_) {
            m_.pop_back();
        }
        return p;
    }
    void erase(iterator it) { m_.erase(it); }
    void erase(const key_type& k) { boost::get<1>(m_).erase(k); }
    size_type max_size() const { return max_size_; }
};

#endif // BITCOIN_MRUSET_H
