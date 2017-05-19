#ifndef BITCOIN_OPEN_HASH_SET_H
#define BITCOIN_OPEN_HASH_SET_H

#include <functional>
#include <utility>
#include <vector>

/** Implements an open hash set.
 */
template<class Key, class IsKeyNull, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class open_hash_set
{
public:
    typedef Key key_type;
    typedef Key value_type;
    typedef size_t size_type;
    typedef Hash hasher;
    typedef KeyEqual key_equal;

    class iterator
    {
        value_type* ptr;
    public:
        iterator(value_type* ptr_) : ptr(ptr_) {}
        value_type& operator*() const { return *ptr; }
        value_type* operator->() const { return ptr; }
        bool operator==(iterator x) const { return ptr == x.ptr; }
        bool operator!=(iterator x) const { return ptr != x.ptr; }
        bool operator>=(iterator x) const { return ptr >= x.ptr; }
        bool operator<=(iterator x) const { return ptr <= x.ptr; }
        bool operator>(iterator x) const { return ptr > x.ptr; }
        bool operator<(iterator x) const { return ptr < x.ptr; }
    };

private:
    hasher m_hash_instance;
    key_equal m_equal_instance;
    IsKeyNull m_null_instance;
    std::vector<value_type> m_table;
    size_type m_count = 0, m_scan_max;

    inline size_t hash_pos(uint64_t hash, size_t i) {
        uint64_t input = hash * (i + 1);
        uint32_t value = (input & 0xffffffffffLLU) ^ ((input & 0xffff00000000LLU) >> 16) ^ ((input & 0xffff000000000000LLU) >> 32);
        return (value * uint64_t(m_table.size())) >> 32;
    }

public:
    open_hash_set(size_type entry_count=1000) :
        m_table(std::max(128*1024/sizeof(value_type), entry_count*3)), // max(1/2 of L2, 3*entries)
        m_scan_max(m_table.size() / 2)
    {}

    std::pair<iterator, bool> insert(const value_type& value) {
        size_t pos;
        size_t i = 0;
        while (i < m_scan_max) {
            pos = hash_pos(m_hash_instance(value), i);
            if (m_null_instance(m_table[pos])) break;
            if (m_equal_instance(m_table[pos], value)) break;
            i++;
        }

        if (i == m_scan_max) {
            return std::make_pair(end(), false);
        }

        if (m_equal_instance(m_table[pos], value)) {
            return std::make_pair(iterator(&m_table[pos]), false);
        }

        m_table[pos] = value;
        m_count++;
        return std::make_pair(iterator(&m_table[pos]), true);
    }

    iterator find(const value_type& value) {
        size_t pos;
        size_t i = 0;
        while (i < m_scan_max) {
            pos = hash_pos(m_hash_instance(value), i);
            if (m_null_instance(m_table[pos])) break;
            if (m_equal_instance(m_table[pos], value)) break;
            i++;
        }

        if (i == m_scan_max || m_null_instance(m_table[pos]) || !m_equal_instance(m_table[pos], value)) {
            return end();
        }
        return iterator(&m_table[pos]);
    }

    iterator end() {
        value_type* ptr = &m_table[m_table.size() - 1];
        return iterator(ptr + 1);
    }

    size_type size() const { return m_count; }
};

#endif // BITCOIN_OPEN_HASH_SET_H
